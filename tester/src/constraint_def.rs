extern crate itertools;
extern crate rand;

use num_integer::Integer;

use crate::counter::{get_unique_name, get_unique_value};
use std::collections::HashSet;
use std::fmt;
use std::iter::FromIterator;

use self::rand::seq::SliceRandom;
use self::rand::Rng;

use std::sync::atomic::{AtomicU32, Ordering};
use std::sync::Arc;

use arrayvec::ArrayVec;

/// Scales the size of randomly-generated instances. `1` is the
/// historical default (lists up to 4, integer domain ±10, sublist
/// target up to 8). `main` sets this from `--size-factor`. The
/// adaptive work-steal pass overrides it per-trial via
/// [`build_random_instance_sized`].
///
/// Integer domain bounds, sublist target sizes, and list lengths all
/// scale linearly with this factor; the search-space size scales
/// roughly polynomially because all three multiply together.
pub static SIZE_FACTOR: AtomicU32 = AtomicU32::new(1);

fn current_size_factor() -> u32 {
    SIZE_FACTOR.load(Ordering::Relaxed).max(1)
}

/// Probability, in per-mille (0..=1000), that a non-constant variable
/// slot reuses a variable already generated elsewhere in the *same*
/// constraint tree -- meaning either an earlier slot of the same
/// constraint, an ancestor's slot, or an earlier sibling subtree.
/// `0` reproduces the historical all-fresh behaviour; `main` sets it
/// from `--var-reuse`.
///
/// Sharing is what makes meta-constraints actually meta. With every
/// child given its own disjoint variable universe (the pre-`pool`
/// behaviour), `watched-and({eq(a, 1), eq(b, 1)})` is solver-
/// equivalent to the two conjuncts run independently -- no
/// propagation can flow between them, and the test is blind to bugs
/// in the parent's child-state handling. Likewise within one
/// constraint: aliasing-unsoundness bugs (occurrence's count vs
/// counted, alldiff-on-the-same-var, etc.) only surface when the
/// same `Arc<MinionVariable>` actually appears in multiple slots.
pub static VAR_REUSE_PERMILLE: AtomicU32 = AtomicU32::new(0);

fn reuse_roll() -> bool {
    let p = VAR_REUSE_PERMILLE.load(Ordering::Relaxed).min(1000);
    p != 0 && rand::thread_rng().gen_range(0..1000) < p
}

/// True if a freshly-minted variable of actual `var_type = actual`
/// would have been an acceptable choice to fill a slot whose declared
/// kind is `slot_kind`. Mirrors the per-kind acceptance map embedded
/// in `MinionVariable::random_sized`: e.g. a `Bound` slot is allowed
/// to be filled by Bound, SparseBound, Discrete, Bool, or Constant.
/// Used when picking pool reuse candidates so we never substitute an
/// incompatible type that the constraint's invariants don't expect.
fn compatible_with_slot(slot_kind: VarType, actual: VarType) -> bool {
    match slot_kind {
        Constant => actual == Constant,
        Bool => matches!(actual, Bool | Constant),
        Discrete => matches!(actual, Discrete | Bool | Constant),
        Bound | SparseBound => matches!(
            actual,
            Bound | SparseBound | Discrete | Bool | Constant
        ),
    }
}

/// Fill one variable slot of kind `d`: with probability
/// [`VAR_REUSE_PERMILLE`] reuse a previously-generated non-constant
/// variable from `pool` whose actual type satisfies
/// [`compatible_with_slot`], otherwise mint a fresh one and (if it's
/// non-constant) push it into `pool` so later slots can reuse it.
///
/// Constants are excluded from reuse: they're literal values, not
/// shared state, so the only meaningful flavour of "reusing" a
/// constant is using its value -- which a fresh Constant slot is
/// equally happy to do, at the cost of nothing.
fn pool_or_fresh(
    pool: &mut Vec<Arc<MinionVariable>>,
    d: VarType,
    size_factor: u32,
) -> Arc<MinionVariable> {
    if d != Constant && reuse_roll() {
        let mut rng = rand::thread_rng();
        let compatible: Vec<usize> = pool
            .iter()
            .enumerate()
            .filter(|(_, v)| v.var_type != Constant && compatible_with_slot(d, v.var_type))
            .map(|(i, _)| i)
            .collect();
        if let Some(&idx) = compatible.choose(&mut rng) {
            return pool[idx].clone();
        }
    }
    let v = MinionVariable::random_sized(d, size_factor);
    if v.var_type != Constant {
        pool.push(v.clone());
    }
    v
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum VarType {
    Constant,
    Bool,
    Bound,
    SparseBound,
    Discrete,
}

use self::VarType::*;

#[derive(Clone, Copy, Debug)]
pub enum Arg {
    Var(VarType),
    List(VarType),
    // TwoVars(VarType),
    //    ConstantList,
    Tuples,
    /// Short-tuple list (`**SHORTTUPLELIST**`). Triggers
    /// generation of a [`ShortTuples`] from the variables seen so
    /// far. Used by `shortstr2`, `haggisgac`, `haggisgac-stable`,
    /// and `shortctuplestr2`.
    ShortTuples,
    Constraint,
    //    Constraint_List,
}

use self::Arg::*;

#[derive(Debug, Clone)]
pub struct MinionVariable {
    pub var_type: VarType,
    pub name: String,
    pub domain: Vec<i64>,
}

#[derive(Clone)]
pub struct Tuples {
    pub tupledata: Vec<Vec<i64>>,
    pub name: String,
}

impl Tuples {
    fn new(tupledata: Vec<Vec<i64>>) -> Tuples {
        Tuples {
            tupledata,
            name: get_unique_name("tuples", ""),
        }
    }
}

/// A `**SHORTTUPLELIST**` value. Each inner `Vec<(usize, i64)>` is
/// one short tuple — a list of `(variable_index, value)` literals.
/// Variable indexes refer to the position within the constraint's
/// variable list. Multiple literals for the same `usize` in one
/// short tuple are only meaningful for `shortctuplestr2` (OR
/// semantics); the other propagators reject such input. This
/// generator never produces them.
#[derive(Clone)]
pub struct ShortTuples {
    pub data: Vec<Vec<(usize, i64)>>,
    pub name: String,
}

impl ShortTuples {
    fn new(data: Vec<Vec<(usize, i64)>>) -> ShortTuples {
        ShortTuples {
            data,
            name: get_unique_name("shorttuples", ""),
        }
    }
}

fn random_in_range(low: i64, high: i64) -> i64 {
    let mut rng = rand::thread_rng();
    rng.gen_range(low..=high)
}

pub fn random_sublist(list: &[i64]) -> Vec<i64> {
    let mut vec = vec![];
    while vec.is_empty() {
        for l in list {
            if rand::random() {
                vec.push(*l)
            }
        }
    }
    vec
}

pub fn random_sublist_of_size(list: &[i64], target: i64) -> Vec<i64> {
    let mut rng = rand::thread_rng();
    let mut vec = list.to_vec();
    vec.shuffle(&mut rng);
    while vec.len() as i64 > target {
        vec.pop();
    }
    vec.sort();
    vec
}

impl MinionVariable {
    fn random(in_d: VarType) -> Arc<MinionVariable> {
        Self::random_sized(in_d, current_size_factor())
    }

    fn random_sized(in_d: VarType, size_factor: u32) -> Arc<MinionVariable> {
        let f = size_factor.max(1) as i64;
        // Bool domain stays {0,1} regardless of size_factor — it's
        // structurally fixed by the constraint signature. Integer
        // domains scale ±10 → ±10*f.
        let dom_min = match in_d {
            Bool => 0,
            _ => -10 * f,
        };

        let dom_max = match in_d {
            Bool => 1,
            _ => 10 * f,
        };

        // Sublist target / bound length: 1..=8 unscaled. Scaling makes
        // domains denser, which is what increases search-tree size.
        let sublist_max: i64 = 8 * f;

        let d = *match in_d {
            Constant => vec![Constant],
            Bool => vec![Bool, Bool, Constant],
            Discrete => vec![Discrete, Discrete, Discrete, Bool, Constant],
            Bound => vec![
                Bound,
                Bound,
                SparseBound,
                Discrete,
                Discrete,
                Bool,
                Constant,
            ],
            SparseBound => vec![
                Bound,
                Bound,
                SparseBound,
                Discrete,
                Discrete,
                Bool,
                Constant,
            ],
        }
        .choose(&mut rand::thread_rng())
        .unwrap();

        let domain = match d {
            Constant => vec![random_in_range(dom_min, dom_max)],
            Bool => random_sublist(&[0, 1]),
            Discrete => {
                let v: Vec<i64> = (dom_min..dom_max).collect();
                random_sublist_of_size(&v, random_in_range(1, sublist_max))
            }
            SparseBound => {
                let v: Vec<i64> = (dom_min..dom_max).collect();
                random_sublist_of_size(&v, random_in_range(1, sublist_max))
            }
            Bound => {
                let low = random_in_range(dom_min, dom_max);
                let len = random_in_range(1, sublist_max);
                (low..(low + len)).collect()
            }
        };

        let name = match d {
            Constant => domain[0].to_string(),
            _ => "x".to_owned() + &get_unique_value().to_string(),
        };

        Arc::new(MinionVariable {
            var_type: d,
            name,
            domain,
        })
    }

    fn get_value(&self) -> i64 {
        if self.domain.len() != 1 {
            panic!("Invalid var!");
        }
        self.domain[0]
    }
}

#[derive(Clone)]
pub struct ConstraintDef {
    pub name: String,
    pub arg: Vec<Arg>,
    pub checker: Arc<dyn Fn(&ConstraintInstance, &[&[i64]]) -> bool + Sync + Send>,
    pub gac: bool,
    pub reifygac: bool,
    pub valid_instance: fn(&ConstraintInstance) -> bool,
}

impl ConstraintDef {
    fn new(
        name: &str,
        arg: Vec<Arg>,
        checker: fn(&[&[i64]]) -> bool,
        gac: bool,
        reifygac: bool,
    ) -> ConstraintDef {
        ConstraintDef {
            name: name.to_string(),
            arg,
            checker: Arc::new(move |_c: &ConstraintInstance, v: &[&[i64]]| -> bool { checker(v) }),
            gac,
            reifygac,
            valid_instance: |_| true,
        }
    }

    fn new_parent(
        name: &str,
        arg: Vec<Arg>,
        checker: fn(&ConstraintInstance, &[&[i64]]) -> bool,
        gac: bool,
        reifygac: bool,
    ) -> ConstraintDef {
        ConstraintDef {
            name: name.to_string(),
            arg,
            checker: Arc::new(checker),
            gac,
            reifygac,
            valid_instance: |_| true,
        }
    }

    fn new_with_validator(
        name: &str,
        arg: Vec<Arg>,
        checker: fn(&[&[i64]]) -> bool,
        gac: bool,
        reifygac: bool,
        valid_instance: fn(&ConstraintInstance) -> bool,
    ) -> ConstraintDef {
        ConstraintDef {
            name: name.to_string(),
            arg,
            checker: Arc::new(move |_c: &ConstraintInstance, v: &[&[i64]]| -> bool { checker(v) }),
            gac,
            reifygac,
            valid_instance,
        }
    }

    fn new_full(
        name: &str,
        arg: Vec<Arg>,
        checker: fn(&ConstraintInstance, &[&[i64]]) -> bool,
        gac: bool,
        reifygac: bool,
        valid_instance: fn(&ConstraintInstance) -> bool,
    ) -> ConstraintDef {
        ConstraintDef {
            name: name.to_string(),
            arg,
            checker: Arc::new(checker),
            gac,
            reifygac,
            valid_instance,
        }
    }
}

impl fmt::Debug for ConstraintDef {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(
            f,
            "ConstraintDef {{{:?},{:?},{:?},{:?}}}",
            self.name, self.arg, self.gac, self.reifygac
        )
    }
}

#[derive(Clone)]
pub struct ConstraintInstance {
    pub constraint: ConstraintDef,
    varlist: Arc<Vec<Vec<Arc<MinionVariable>>>>,
    pub tuples: Option<Tuples>,
    pub short_tuples: Option<ShortTuples>,
    pub child_constraints: Vec<ConstraintInstance>,
}

impl ConstraintInstance {
    pub fn vars(&self) -> Arc<Vec<Vec<Arc<MinionVariable>>>> {
        let mut varlist = (*self.varlist).clone();
        for child in &self.child_constraints {
            varlist.extend(child.vars().iter().cloned());
        }
        Arc::new(varlist)
    }

    /// True if the same variable (by `Arc` identity) fills more than
    /// one slot anywhere in this constraint tree. Constants don't
    /// count: they're literal values, not shared state. Used to skip
    /// the GAC node-count assertion — propagators aren't expected to
    /// achieve GAC when a variable is repeated, only to stay sound.
    pub fn has_repeated_vars(&self) -> bool {
        let vars = self.vars();
        let mut seen: Vec<*const MinionVariable> = Vec::new();
        for v in vars.iter().flatten() {
            if v.var_type == Constant {
                continue;
            }
            let p = Arc::as_ptr(v);
            if seen.contains(&p) {
                return true;
            }
            seen.push(p);
        }
        false
    }

    fn total_var_slots(&self) -> usize {
        self.varlist.len()
            + self
                .child_constraints
                .iter()
                .map(|c| c.total_var_slots())
                .sum::<usize>()
    }

    /// Variables belonging directly to this constraint, one entry per argument slot.
    /// For argument slots whose kind is `Constraint`, the slot is empty — the child
    /// constraint is stored in `child_constraints` instead.
    pub fn top_level_vars(&self) -> &Vec<Vec<Arc<MinionVariable>>> {
        &self.varlist
    }

    fn check_tuple(&self, tup: &[i64]) -> bool {
        let mut slices: Vec<&[i64]> = Vec::with_capacity(self.total_var_slots());
        let mut place: usize = 0;
        for var in self.vars().iter() {
            let i = var.len();
            slices.push(&tup[place..place + i]);
            place += i;
        }
        (self.constraint.checker)(self, &slices)
    }

    /// Enumerate the truth table of this constraint up to `limit`
    /// satisfying tuples, returning `None` if either:
    ///
    ///   - more than `limit` satisfying tuples are found (the
    ///     constraint is too wide to tabulate at the configured
    ///     `--maxtuples`); or
    ///   - the unfiltered Cartesian product exceeds an iteration
    ///     budget proportional to `limit` (a sparse predicate over
    ///     huge domains would walk billions of candidates and
    ///     never collect enough — bail before time/memory blows up).
    ///
    /// `--size-factor 4` and beyond can produce wide-list random
    /// instances whose Cartesian product is 10^20+; without these
    /// caps the iteration alone OOMs the process well before any
    /// `tuples.len() > limit` post-check.
    fn build_tuples_capped(&self, limit: usize) -> Option<Vec<Vec<i64>>> {
        use self::itertools::Itertools;

        // Special case 0 variables.
        if self.vars().concat().is_empty() {
            if self.check_tuple(&[]) {
                return Some(vec![vec![]]);
            } else {
                return Some(vec![]);
            }
        }
        let domains: Vec<Vec<i64>> = (*self.vars())
            .concat()
            .iter()
            .map(|x| x.domain.clone())
            .collect();

        // Pre-check the Cartesian product size. u128 saturates at a
        // huge ceiling; if the product overflows or exceeds the
        // iteration budget, bail without iterating.
        let mut cart: u128 = 1;
        for d in &domains {
            cart = cart.saturating_mul(d.len() as u128);
        }
        // Iteration budget: 1000× the satisfying-tuple cap, with a
        // floor at 100k. Lets sparse-predicate small-domain cases
        // succeed (most real tester instances) while bailing fast on
        // wide-list scaled-up instances.
        let iter_budget: u128 = (limit as u128).saturating_mul(1000).max(100_000);
        if cart > iter_budget {
            return None;
        }

        let mut out: Vec<Vec<i64>> = Vec::new();
        for t in domains.iter().cloned().multi_cartesian_product() {
            if self.check_tuple(&t) {
                out.push(t);
                if out.len() > limit {
                    return None;
                }
            }
        }
        Some(out)
    }

    pub fn tableise(&self, limit: usize) -> Option<ConstraintInstance> {
        let tuples = match self.build_tuples_capped(limit) {
            Some(t) => t,
            None => return None,
        };
        if tuples.len() > limit {
            return None;
        }

        Some(ConstraintInstance {
            constraint: STANDARD_TABLE_CONSTRAINT.clone(),
            varlist: {
                let vars: Vec<Arc<MinionVariable>> =
                    self.vars().iter().flatten().cloned().collect();
                Arc::new(vec![vars])
            },
            tuples: Some(Tuples::new(tuples)),
            short_tuples: None,
            child_constraints: vec![],
        })
    }
    /*
    pub fn variables(&self) -> usize {
        self.lengths.iter().clone().sum()
    }
    */
}

/// Generate a random non-empty subset of the cartesian product of all
/// variable domains collected so far. Used by tuple-bearing constraints
/// (table, negativetable, mddc, gacschema, lighttable, etc.).
fn generate_random_tuples_from_vars(variables: &[Vec<Arc<MinionVariable>>]) -> Option<Tuples> {
    let all_domains: Vec<Vec<i64>> = variables
        .iter()
        .flatten()
        .map(|v| v.domain.clone())
        .collect();

    if all_domains.is_empty() {
        return Some(Tuples::new(vec![vec![]]));
    }

    // Cap on the Cartesian product size we're willing to materialise.
    // Below this we keep the original enumerate-shuffle-truncate path
    // (preserves the "leave at least one out" property exactly). Above
    // it we sample tuples directly from the per-variable domains —
    // statistically sound for the tester (random non-empty subset)
    // and avoids OOM on `--size-factor 4`-style wide-list instances
    // where the product is 10^20+.
    const ENUM_CAP: u128 = 1_000_000;

    let mut cart: u128 = 1;
    for d in &all_domains {
        cart = cart.saturating_mul(d.len() as u128);
    }

    let mut rng = rand::thread_rng();

    if cart <= ENUM_CAP {
        use self::itertools::Itertools;
        let mut all_assignments: Vec<Vec<i64>> = all_domains
            .iter()
            .cloned()
            .multi_cartesian_product()
            .collect();
        all_assignments.shuffle(&mut rng);
        let take = if all_assignments.len() > 1 {
            rng.gen_range(1..all_assignments.len())
        } else {
            1
        };
        all_assignments.truncate(take);
        all_assignments.sort();
        Some(Tuples::new(all_assignments))
    } else {
        // Sample path: pick K random points directly. For huge
        // products K ≪ cart so leaving "at least one out" is
        // automatic with overwhelming probability.
        const TUPLE_SAMPLE_CAP: usize = 10_000;
        let take = rng.gen_range(1..=TUPLE_SAMPLE_CAP);
        let mut sampled: Vec<Vec<i64>> = (0..take)
            .map(|_| {
                all_domains
                    .iter()
                    .map(|d| *d.choose(&mut rand::thread_rng()).unwrap())
                    .collect()
            })
            .collect();
        sampled.sort();
        sampled.dedup();
        Some(Tuples::new(sampled))
    }
}

/// Generate a random short-tuple list over the variables collected
/// so far. Each short tuple selects a random non-empty subset of
/// the variable positions and assigns each chosen position a value
/// drawn uniformly from that variable's domain. Positions are
/// distinct within a short tuple (so the result satisfies
/// `validateShortTuples`'s "no two literals for the same variable"
/// rule used by shortstr2/haggisgac/haggisgac-stable; shortctuplestr2
/// is more permissive and accepts the same input).
fn generate_random_short_tuples_from_vars(
    variables: &[Vec<Arc<MinionVariable>>],
) -> Option<ShortTuples> {
    let mut rng = rand::thread_rng();
    let all_vars: Vec<Arc<MinionVariable>> =
        variables.iter().flatten().cloned().collect();
    let n_vars = all_vars.len();
    if n_vars == 0 {
        // Zero-variable corner case. An empty data list = constraint
        // false everywhere; an empty short tuple = trivially true.
        // The 0-var instance is uninteresting either way; pick "true".
        return Some(ShortTuples::new(vec![vec![]]));
    }

    // Number of short tuples in the list. Cap by the number of
    // assignments any short tuple could rule in (≤ 2^n_vars-ish), so
    // we don't generate redundant short tuples that all map to the
    // same dense truth table.
    let upper = (2_usize).saturating_pow(n_vars as u32).min(20).max(2);
    let num_short = rng.gen_range(1..=upper);

    let mut tuples: Vec<Vec<(usize, i64)>> = Vec::with_capacity(num_short);
    for _ in 0..num_short {
        // Pick 1..=n_vars distinct positions for this short tuple.
        let len = rng.gen_range(1..=n_vars);
        let mut positions: Vec<usize> = (0..n_vars).collect();
        positions.shuffle(&mut rng);
        positions.truncate(len);
        positions.sort();
        let short: Vec<(usize, i64)> = positions
            .iter()
            .map(|&p| {
                let val = *all_vars[p].domain.choose(&mut rng).unwrap();
                (p, val)
            })
            .collect();
        tuples.push(short);
    }
    Some(ShortTuples::new(tuples))
}

pub fn build_random_instance(constraint: &ConstraintDef) -> ConstraintInstance {
    build_random_instance_with_children(constraint, &[])
}

pub fn build_random_instance_with_children(
    constraint: &ConstraintDef,
    children: &[&ConstraintDef],
) -> ConstraintInstance {
    build_random_instance_with_children_sized(constraint, children, current_size_factor())
}

/// Like [`build_random_instance`] but with an explicit `size_factor`
/// override. The adaptive work-steal pass uses this to grow instances
/// per-trial without touching the global SIZE_FACTOR.
pub fn build_random_instance_sized(
    constraint: &ConstraintDef,
    size_factor: u32,
) -> ConstraintInstance {
    build_random_instance_with_children_sized(constraint, &[], size_factor)
}

pub fn build_random_instance_with_children_sized(
    constraint: &ConstraintDef,
    children: &[&ConstraintDef],
    size_factor: u32,
) -> ConstraintInstance {
    // Public entry: each top-level call starts with an empty pool.
    // Recursive child calls thread the same `&mut pool` through so a
    // child can reuse vars the parent (or an earlier sibling subtree)
    // already minted.
    let mut pool: Vec<Arc<MinionVariable>> = Vec::new();
    build_with_pool(constraint, children, size_factor, &mut pool)
}

/// Internal kernel for [`build_random_instance_with_children_sized`].
/// Threads a single mutable variable pool through the parent's own
/// slots and into all recursive child builds so that the resulting
/// tree can have cross-slot and cross-child variable sharing.
fn build_with_pool(
    constraint: &ConstraintDef,
    children: &[&ConstraintDef],
    size_factor: u32,
    pool: &mut Vec<Arc<MinionVariable>>,
) -> ConstraintInstance {
    let f = size_factor.max(1) as usize;
    loop {
        // Snapshot pool length so a `valid_instance`-rejected attempt
        // doesn't leak its partial vars into the next attempt's pool.
        let pool_checkpoint = pool.len();
        let mut variables: Vec<Vec<Arc<MinionVariable>>> = vec![];
        let mut constraints: Vec<ConstraintInstance> = vec![];
        let mut constraint_child = 0;
        let mut generated_tuples: Option<Tuples> = None;
        let mut generated_short_tuples: Option<ShortTuples> = None;

        for i in 0..constraint.arg.len() {
            match constraint.arg[i] {
                Var(d) => {
                    variables.push(vec![pool_or_fresh(pool, d, size_factor)]);
                }
                List(d) => {
                    let len = rand::random::<usize>() % (5 * f);
                    variables.push(
                        (0..len)
                            .map(|_x| pool_or_fresh(pool, d, size_factor))
                            .collect(),
                    );
                }
                Tuples => {
                    generated_tuples = generate_random_tuples_from_vars(&variables);
                }
                ShortTuples => {
                    generated_short_tuples = generate_random_short_tuples_from_vars(&variables);
                }
                Constraint => {
                    // Leave dummy empty vec in variables
                    variables.push(vec![]);
                    let owned_child: ConstraintDef;
                    let child_def: &ConstraintDef = if constraint_child < children.len() {
                        children[constraint_child]
                    } else {
                        // Pick a random leaf constraint among types the
                        // Rust solver supports as children of meta-constraints.
                        const SUPPORTED: &[&str] = &[
                            "w-literal", "w-notliteral", "eq", "diseq",
                        ];
                        let leafs: Vec<&ConstraintDef> = CONSTRAINT_LIST.iter()
                            .filter(|d| SUPPORTED.contains(&d.name.as_str())
                                && !d.arg.iter().any(|a| matches!(a, Arg::Constraint)))
                            .collect();
                        let idx = rand::random::<usize>() % leafs.len();
                        owned_child = leafs[idx].clone();
                        &owned_child
                    };
                    // Recurse with the SAME pool: the child can reuse
                    // any variable the parent already minted, and any
                    // earlier sibling's vars are visible too.
                    constraints.push(build_with_pool(child_def, &[], size_factor, pool));
                    constraint_child += 1;
                }
            }
        }
        let c = ConstraintInstance {
            constraint: constraint.clone(),
            varlist: Arc::new(variables),
            tuples: generated_tuples,
            short_tuples: generated_short_tuples,
            child_constraints: constraints,
        };
        if (constraint.valid_instance)(&c) {
            return c;
        }
        pool.truncate(pool_checkpoint);
    }
}

/// Build a random depth-`depth` parent tree with `leaf_def` at the
/// bottom of one branch and random leaves at the bottoms of any
/// sibling branches.
///
/// - `depth == 0` returns `build_random_instance(leaf_def)` — the
///   bare leaf.
/// - `depth == 1` picks one parent from the nesting shortlist
///   (reify / reifyimply / reifyimply-quick / watched-and /
///   watched-or), puts `leaf_def` in its first child slot and
///   random leaves in any remaining slots. Matches the existing
///   1-level `test_constraint_nested`.
/// - `depth >= 2` does the same, but each child subtree is itself a
///   depth-(`depth-1`) random tree. Exercises cross-type parent
///   interactions: e.g. `reify(watched-or(reifyimply(leaf),
///   watched-and(leaf, leaf)))`. The iterated leaf always sits at
///   the bottom of the first-child chain; sibling branches pick
///   random leaves of their own.
///
/// The `Constraint`-arg count of each parent decides child fan-out
/// (1 for reify/reifyimply, 2 for watched-and/watched-or).
///
/// The shortlist excludes `forwardchecking` / `check[gsa]` /
/// `check[assign]` — those are thin wrappers whose interactions
/// with each other aren't structurally interesting for this test.
///
/// A single variable pool threads through the entire tree (parent
/// slots and all descendant subtrees), so [`VAR_REUSE_PERMILLE`]
/// controls how often a child slot draws a variable that an ancestor
/// or earlier sibling has already minted. Without that, every child
/// gets a disjoint variable universe and meta-constraints test
/// nothing about interaction across their children.
pub fn build_random_nested_tree(leaf_def: &ConstraintDef, depth: usize) -> ConstraintInstance {
    let mut pool: Vec<Arc<MinionVariable>> = Vec::new();
    nested_with_pool(leaf_def, depth, &mut pool)
}

/// Internal kernel for [`build_random_nested_tree`]. Threads `pool`
/// through depth-N recursion so the entire tree shares one variable
/// universe.
fn nested_with_pool(
    leaf_def: &ConstraintDef,
    depth: usize,
    pool: &mut Vec<Arc<MinionVariable>>,
) -> ConstraintInstance {
    if depth == 0 {
        return build_with_pool(leaf_def, &[], current_size_factor(), pool);
    }
    let mut rng = rand::thread_rng();

    let shortlist = [
        "reify",
        "reifyimply",
        "reifyimply-quick",
        "watched-and",
        "watched-or",
    ];
    let name = shortlist.choose(&mut rng).expect("shortlist is non-empty");
    let parent_def = NESTED_CONSTRAINT_LIST
        .iter()
        .find(|d| d.name == *name)
        .expect("shortlist name not in NESTED_CONSTRAINT_LIST");

    let size_factor = current_size_factor();
    let f = size_factor as usize;
    loop {
        let pool_checkpoint = pool.len();
        let mut variables: Vec<Vec<Arc<MinionVariable>>> = vec![];
        let mut constraints: Vec<ConstraintInstance> = vec![];
        let mut constraint_child = 0;
        let mut generated_tuples: Option<Tuples> = None;
        let mut generated_short_tuples: Option<ShortTuples> = None;

        for i in 0..parent_def.arg.len() {
            match parent_def.arg[i] {
                Var(d) => {
                    variables.push(vec![pool_or_fresh(pool, d, size_factor)]);
                }
                List(d) => {
                    let len = rand::random::<usize>() % (5 * f);
                    variables.push(
                        (0..len)
                            .map(|_x| pool_or_fresh(pool, d, size_factor))
                            .collect(),
                    );
                }
                Tuples => {
                    generated_tuples = generate_random_tuples_from_vars(&variables);
                }
                ShortTuples => {
                    generated_short_tuples = generate_random_short_tuples_from_vars(&variables);
                }
                Constraint => {
                    variables.push(vec![]);
                    // First child carries the iterated leaf (recursive).
                    // Sibling slots pick a random leaf from the full
                    // list, wrapped to the same depth so we get
                    // symmetric cross-type nesting at every level.
                    let inner = if constraint_child == 0 {
                        leaf_def
                    } else {
                        CONSTRAINT_LIST
                            .choose(&mut rng)
                            .expect("CONSTRAINT_LIST empty")
                    };
                    constraints.push(nested_with_pool(inner, depth - 1, pool));
                    constraint_child += 1;
                }
            }
        }
        let c = ConstraintInstance {
            constraint: parent_def.clone(),
            varlist: Arc::new(variables),
            tuples: generated_tuples,
            short_tuples: generated_short_tuples,
            child_constraints: constraints,
        };
        if (parent_def.valid_instance)(&c) {
            return c;
        }
        pool.truncate(pool_checkpoint);
    }
}

fn check_alldiff(v: &[&[i64]]) -> bool {
    assert!(v.len() == 1);
    for i in 0..v[0].len() {
        for j in (i + 1)..v[0].len() {
            if v[0][i] == v[0][j] {
                return false;
            }
        }
    }
    true
}

fn check_alldiffmatrix(_c: &ConstraintInstance, v: &[&[i64]]) -> bool {
    let vars: &[i64] = v[0];
    let var_count = vars.len();
    let n = (var_count as f64).sqrt() as usize;
    if n * n != var_count {
        return false;
    }
    let value = v[1][0];

    // Match minion's checkAssignment (constraint_alldiffmatrix.h:220):
    // every row AND every column must contain exactly one cell equal
    // to `value`. The bipartite-matching reading is strictly weaker —
    // it admits assignments where extra cells equal the value, which
    // minion's propagator (correctly) rejects.
    for i in 0..n {
        let row_count = (0..n).filter(|&j| vars[i * n + j] == value).count();
        if row_count != 1 {
            return false;
        }
        let col_count = (0..n).filter(|&j| vars[j * n + i] == value).count();
        if col_count != 1 {
            return false;
        }
    }
    true
}

fn valid_alldiffmatrix(c: &ConstraintInstance) -> bool {
    let var_count = c.vars()[0].len();
    let n = (var_count as f64).sqrt() as usize;
    n * n == var_count && var_count > 0
}

fn check_gcc(_c: &ConstraintInstance, v: &[&[i64]]) -> bool {
    let primary = &v[0];
    let values = &v[1];
    let capacities = &v[2];
    for j in 0..values.len() {
        let val = values[j];
        let count = primary.iter().filter(|&&x| x == val).count();
        if count as i64 != capacities[j] {
            return false;
        }
    }
    true
}

fn valid_gcc(c: &ConstraintInstance) -> bool {
    // Need at least one primary var, at least one value-of-interest,
    // matching capacities, and values-of-interest must be distinct.
    use std::collections::HashSet;
    if c.vars()[0].is_empty() || c.vars()[1].is_empty() {
        return false;
    }
    if c.vars()[1].len() != c.vars()[2].len() {
        return false;
    }
    let vals: Vec<i64> = c.vars()[1].iter().map(|v| v.get_value()).collect();
    let unique: HashSet<i64> = vals.iter().cloned().collect();
    if unique.len() != vals.len() {
        return false;
    }
    // gcc/gccweak refuse the same variable in both the counted vector
    // (slot 0) and the cardinality vector (slot 2) -- the incremental
    // matching cannot stay consistent with that aliasing, so minion
    // rejects the instance at construction. Repeats within either
    // vector alone are fine. Compare by name; constants don't count
    // (they're not shared state).
    let vars = c.vars();
    let counted_names: HashSet<String> = vars[0]
        .iter()
        .filter(|v| v.var_type != VarType::Constant)
        .map(|v| v.name.clone())
        .collect();
    !vars[2]
        .iter()
        .filter(|v| v.var_type != VarType::Constant)
        .any(|v| counted_names.contains(&v.name))
}

fn check_table(c: &ConstraintInstance, v: &[&[i64]]) -> bool {
    let tup: Vec<i64> = v[0].to_vec();
    c.tuples.as_ref().unwrap().tupledata.contains(&tup)
}

fn check_negative_table(c: &ConstraintInstance, v: &[&[i64]]) -> bool {
    let tup: Vec<i64> = v[0].to_vec();
    !c.tuples.as_ref().unwrap().tupledata.contains(&tup)
}

fn valid_positive_table(c: &ConstraintInstance) -> bool {
    !c.tuples.as_ref().unwrap().tupledata.is_empty()
}

fn valid_negative_table(c: &ConstraintInstance) -> bool {
    let tuples = c.tuples.as_ref().unwrap();
    if tuples.tupledata.is_empty() {
        return false;
    }
    let all_domains: Vec<Vec<i64>> = c.vars()[0].iter().map(|v| v.domain.clone()).collect();
    let total: usize = all_domains.iter().map(|d| d.len()).product();
    tuples.tupledata.len() < total
}

fn check_short_tuples(c: &ConstraintInstance, v: &[&[i64]]) -> bool {
    let assignment = v[0];
    let st = c.short_tuples.as_ref().unwrap();
    // An empty short-tuple list = constraint false everywhere.
    // An empty individual short tuple = trivially satisfied.
    for short in &st.data {
        let mut all_match = true;
        for &(idx, val) in short {
            if assignment[idx] != val {
                all_match = false;
                break;
            }
        }
        if all_match {
            return true;
        }
    }
    false
}

/// Reject degenerate short-tuple instances:
///   - an empty short-tuple list (constraint false everywhere — solvers
///     differ in how they short-circuit, and the trial is uninteresting);
///   - any short tuple that is empty (collapses the constraint to true).
/// Both are uninteresting for metamorphic testing and the second
/// hides bugs in the literal-iteration logic.
fn valid_short_tuples_instance(c: &ConstraintInstance) -> bool {
    let st = match c.short_tuples.as_ref() {
        Some(s) => s,
        None => return false,
    };
    if st.data.is_empty() {
        return false;
    }
    if st.data.iter().any(|t| t.is_empty()) {
        return false;
    }
    true
}

fn check_frameupdate(_c: &ConstraintInstance, v: &[&[i64]]) -> bool {
    // Argument order in the minion input file
    // (frameupdate(source, target, idx_source, idx_target, blocksize)),
    // confirmed against test_frameupdate.minion and BuildCT_FRAMEUPDATE.
    let src = v[0];
    let tgt = v[1];
    let idx_src = v[2];
    let idx_tgt = v[3];
    let blocksize = v[4][0];
    if blocksize <= 0 {
        return false;
    }
    let bs = blocksize as usize;
    if src.len() != tgt.len() || src.len() % bs != 0 {
        return false;
    }
    let numblocks = (src.len() / bs) as i64;

    // Per minion (constraint_frameupdate.h:checkAssignment), idx
    // values must be distinct, positive, and ≤ source.size() (NOT
    // ≤ numblocks — values above numblocks are allowed but never
    // get matched in the block-walking loop).
    use std::collections::HashSet;
    let src_len = src.len() as i64;
    let tgt_len = tgt.len() as i64;
    let mut idx_src_set: HashSet<i64> = HashSet::new();
    for &val in idx_src {
        if val <= 0 || val > src_len || !idx_src_set.insert(val) {
            return false;
        }
    }
    let mut idx_tgt_set: HashSet<i64> = HashSet::new();
    for &val in idx_tgt {
        if val <= 0 || val > tgt_len || !idx_tgt_set.insert(val) {
            return false;
        }
    }

    // walk surviving blocks in lockstep, comparing block contents
    let mut idxsource: i64 = 1;
    let mut idxtarget: i64 = 1;
    while idxsource <= numblocks && idxtarget <= numblocks {
        while idx_src_set.contains(&idxsource) {
            idxsource += 1;
        }
        while idx_tgt_set.contains(&idxtarget) {
            idxtarget += 1;
        }
        if idxsource <= numblocks && idxtarget <= numblocks {
            for i in 0..bs {
                let s_pos = (idxsource as usize - 1) * bs + i;
                let t_pos = (idxtarget as usize - 1) * bs + i;
                if src[s_pos] != tgt[t_pos] {
                    return false;
                }
            }
            idxsource += 1;
            idxtarget += 1;
        }
    }
    true
}

/// Frameupdate has structural relations between argument sizes
/// (`source.len() == target.len()`, divisible by `blocksize`,
/// `blocksize >= 1`). Random rolls satisfy these only sometimes;
/// the generator retries until they do. We also cap source.len()
/// here because the truth-table enumeration walks the Cartesian
/// product of *all* variables — long lists blow past --maxtuples
/// trivially.
fn valid_frameupdate(c: &ConstraintInstance) -> bool {
    let blocksize = c.vars()[4][0].get_value();
    // Tight bounds (independent of --size-factor) so retry-based
    // generation lands on a valid config quickly AND the search
    // space is small enough to enumerate fully via tableise.
    if !(1..=2).contains(&blocksize) {
        return false;
    }
    let source_len = c.vars()[0].len();
    let target_len = c.vars()[1].len();
    if source_len == 0 || source_len != target_len {
        return false;
    }
    if source_len > 4 {
        return false;
    }
    if source_len as i64 % blocksize != 0 {
        return false;
    }
    // idx variables have huge default Bound domains (±10) but only
    // values 1..=source.size() are feasible; the propagator only
    // detects the failure once all idx vars are assigned, so a
    // long idx list with wide domains explodes the search tree.
    // Cap the idx lists at length 1 to keep cost manageable —
    // still exercises the skip path (one block on each side may
    // be skipped) without the combinatorial blow-up.
    if c.vars()[2].len() > 1 {
        return false;
    }
    if c.vars()[3].len() > 1 {
        return false;
    }
    true
}

fn check_element(v: &[&[i64]], offset: i64, undefzero: bool) -> bool {
    let index = v[1][0] - offset;
    if index < 0 || index >= v[0].len() as i64 {
        if undefzero {
            return v[2][0] == 0;
        } else {
            return false;
        }
    }
    v[0][index as usize] == v[2][0]
}

lazy_static! {
    pub static ref STANDARD_TABLE_CONSTRAINT: ConstraintDef = {
        ConstraintDef {
            name: "str2plus".to_string(),
            arg: vec![List(Bound), Tuples],
            checker: Arc::new(|_, __| unimplemented!()),
            gac: true,
            reifygac: true,
            valid_instance: |_| true,
        }
    };
}

lazy_static! {
    pub static ref NESTED_CONSTRAINT_LIST: Vec<ConstraintDef> = nested_constraint_list();
}

fn nested_constraint_list() -> Vec<ConstraintDef> {
    vec![
        ConstraintDef::new_parent(
            "reify", // CT_REIFY
            vec![Constraint, Var(Bool)],
            |c, assign| {
                let child = &c.child_constraints[0];
                let ischildtrue = (child.constraint.checker)(child, &assign[2..]);
                (assign[1][0] == 1) == ischildtrue
            },
            false,
            false,
        ),
        ConstraintDef::new_parent(
            "reifyimply-quick", // CT_REIFYIMPLY_QUICK
            vec![Constraint, Var(Bool)],
            |c, assign| {
                let child = &c.child_constraints[0];
                let ischildtrue = (child.constraint.checker)(child, &assign[2..]);
                (assign[1][0] == 1) <= ischildtrue
            },
            false,
            false,
        ),
        ConstraintDef::new_parent(
            "reifyimply", // CT_REIFYIMPLY
            vec![Constraint, Var(Bool)],
            |c, assign| {
                let child = &c.child_constraints[0];
                let ischildtrue = (child.constraint.checker)(child, &assign[2..]);
                (assign[1][0] == 1) <= ischildtrue
            },
            false,
            false,
        ),
        ConstraintDef::new_parent(
            "forwardchecking", // CT_FORWARD_CHECKING
            vec![Constraint],
            |c, assign| {
                (c.child_constraints[0].constraint.checker)(&c.child_constraints[0], &assign[1..])
            },
            false,
            false,
        ),
        ConstraintDef::new_parent(
            "check[gsa]", // CT_CHECK_GSA
            vec![Constraint],
            |c, assign| {
                (c.child_constraints[0].constraint.checker)(&c.child_constraints[0], &assign[1..])
            },
            false,
            false,
        ),
        ConstraintDef::new_parent(
            "check[assign]", // CT_CHECK_ASSIGN
            vec![Constraint],
            |c, assign| {
                (c.child_constraints[0].constraint.checker)(&c.child_constraints[0], &assign[1..])
            },
            false,
            false,
        ),
        ConstraintDef::new_parent(
            "watched-and", // CT_WATCHED_NEW_AND
            vec![Constraint, Constraint],
            |c, assign| {
                let mut offset = c.constraint.arg.len();
                for child in &c.child_constraints {
                    let nslots = child.total_var_slots();
                    let child_assign = &assign[offset..offset + nslots];
                    if !(child.constraint.checker)(child, child_assign) {
                        return false;
                    }
                    offset += nslots;
                }
                true
            },
            false,
            false,
        ),
        ConstraintDef::new_parent(
            "watched-or", // CT_WATCHED_NEW_OR
            vec![Constraint, Constraint],
            |c, assign| {
                let mut offset = c.constraint.arg.len();
                for child in &c.child_constraints {
                    let nslots = child.total_var_slots();
                    let child_assign = &assign[offset..offset + nslots];
                    if (child.constraint.checker)(child, child_assign) {
                        return true;
                    }
                    offset += nslots;
                }
                false
            },
            false,
            false,
        ),
    ]
}

lazy_static! {
    pub static ref CONSTRAINT_LIST: Vec<ConstraintDef> = constraint_list();
}

/// Groups of constraint variants that share the same declarative
/// semantic — every member of a group, run on the same instance,
/// must produce the same solution set. Different propagators within
/// a group differ in *strength* (node count, propagation completeness)
/// but not in *correctness*.
///
/// The first name in each group is the representative whose
/// `ConstraintDef` is used to generate the random instance: its
/// argument types must be strict enough that every other member
/// accepts them. For example, `gacalldiff` requires `Discrete` vars
/// while `alldiff` accepts any var type, so `gacalldiff` is the
/// representative — generating with Discrete satisfies both.
pub static EQUIVALENCE_GROUPS: &[&[&str]] = &[
    &["gacalldiff", "alldiff"],
    &["gaceq", "eq"],
    &["lexleq[rv]", "lexleq", "lexleq[quick]"],
    &["lexless", "lexless[quick]"],
    &["table", "gacschema", "lighttable", "mddc"],
    &["negativetable", "negativemddc"],
    &["shortstr2", "haggisgac", "haggisgac-stable", "shortctuplestr2"],
];

/// Look up a registered constraint definition by exact name.
pub fn find_constraint_def(name: &str) -> Option<&'static ConstraintDef> {
    CONSTRAINT_LIST.iter().find(|c| c.name == name)
}

fn constraint_list() -> Vec<ConstraintDef> {
    vec![
        ConstraintDef::new(
            "abs", // ABS
            vec![Var(Bound), Var(Bound)],
            |v| v[0][0] == v[1][0].abs(),
            false,
            false,
        ),
        ConstraintDef::new(
            "alldiff", // ALLDIFF
            vec![List(Bound)],
            check_alldiff,
            false,
            false,
        ),
        ConstraintDef::new(
            "difference", // CT_DIFFERENCE
            vec![Var(Bound), Var(Bound), Var(Bound)],
            |v| (v[0][0] - v[1][0]).abs() == v[2][0],
            false,
            false,
        ),
        // CT_DISEQ_REIFY handled by DISEQ
        ConstraintDef::new(
            "diseq", // CT_DISEQ
            vec![Var(Bound), Var(Bound)],
            |v| (v[0][0] != v[1][0]),
            false,
            false,
        ),
        ConstraintDef::new(
            "div_undefzero", // CT_DIV_UNDEFZERO
            vec![Var(Bound), Var(Bound), Var(Bound)],
            |v| {
                v[2][0]
                    == (if v[1][0] == 0 {
                        0
                    } else {
                        num_integer::Integer::div_floor(&v[0][0], &v[1][0])
                    })
            },
            false,
            false,
        ),
        ConstraintDef::new(
            "div", // CT_DIV_UNDEFZERO
            vec![Var(Bound), Var(Bound), Var(Bound)],
            |v| {
                if v[1][0] == 0 {
                    false
                } else {
                    v[2][0] == num_integer::Integer::div_floor(&v[0][0], &v[1][0])
                }
            },
            false,
            false,
        ),
        ConstraintDef::new(
            "element_one", // CT_ELEMENT_ONE
            vec![List(Bound), Var(Bound), Var(Bound)],
            |v| check_element(v, 1, false),
            false,
            false,
        ),
        ConstraintDef::new(
            "element_undefzero", // CT_ELEMENT_UNDEFZERO
            vec![List(Bound), Var(Bound), Var(Bound)],
            |v| check_element(v, 0, true),
            false,
            false,
        ),
        ConstraintDef::new(
            "element", // CT_ELEMENT
            vec![List(Bound), Var(Bound), Var(Bound)],
            |v| check_element(v, 0, false),
            false,
            false,
        ),
        // CT_EQ_REIFY handled by CT_EQ
        ConstraintDef::new(
            "eq", // CT_EQ
            vec![Var(Bound), Var(Bound)],
            |v| (v[0][0] == v[1][0]),
            false,
            false,
        ),
        ConstraintDef::new(
            "false", // CT_FALSE
            vec![],
            |_| false,
            true,
            true,
        ),
        ConstraintDef::new_full(
            "frameupdate", // CT_FRAMEUPDATE
            // (source, target, idx_source, idx_target, blocksize)
            // All var lists are Bound; blocksize is a constant.
            vec![
                List(Bound),
                List(Bound),
                List(Bound),
                List(Bound),
                Var(Constant),
            ],
            check_frameupdate,
            false,
            false,
            valid_frameupdate,
        ),
        ConstraintDef::new(
            "gacalldiff",
            vec![List(Discrete)],
            check_alldiff,
            true,
            false,
        ),
        ConstraintDef::new(
            "gaceq", // CT_GACEQ
            vec![Var(Discrete), Var(Discrete)],
            |v| (v[0][0] == v[1][0]),
            true,
            true,
        ),
        ConstraintDef::new_with_validator(
            "lexleq[rv]", // CT_GACLEXLEQ
            vec![List(Bound), List(Bound)],
            |v| (v[0] <= v[1]),
            true,
            true,
            |v| v.vars()[0].len() == v.vars()[1].len(),
        ),
        ConstraintDef::new(
            "occurrencegeq", // CT_GEQ_OCCURRENCE
            vec![List(Discrete), Var(Constant), Var(Constant)],
            |v| (v[0].iter().filter(|x| **x == v[1][0]).count() as i64 >= v[2][0]),
            false,
            false,
        ),
        ConstraintDef::new(
            "nvaluegeq", // CT_GEQNVALUE
            vec![List(Bound), Var(Constant)],
            |v| {
                let h: HashSet<i64> = HashSet::from_iter(v[0].iter().cloned());
                h.len() as i64 >= v[1][0]
            },
            false,
            false,
        ),
        ConstraintDef::new(
            "sumgeq", // CT_GEQSUM
            vec![List(Bound), Var(Bound)],
            |v| (v[0].iter().sum::<i64>() >= v[1][0]),
            false,
            false,
        ),
        ConstraintDef::new_full(
            "haggisgac", // CT_HAGGISGAC
            vec![List(Discrete), ShortTuples],
            check_short_tuples,
            true,
            true,
            valid_short_tuples_instance,
        ),
        ConstraintDef::new_full(
            "haggisgac-stable", // CT_HAGGISGAC_STABLE
            vec![List(Discrete), ShortTuples],
            check_short_tuples,
            true,
            true,
            valid_short_tuples_instance,
        ),
        ConstraintDef::new(
            "ineq", // CT_INEQ
            vec![Var(Bound), Var(Bound), Var(Constant)],
            |v| v[0][0] <= v[1][0] + v[2][0],
            false,
            false,
        ),
        ConstraintDef::new(
            "occurrenceleq", // CT_LEQ_OCCURRENCE
            vec![List(Discrete), Var(Constant), Var(Constant)],
            |v| (v[0].iter().filter(|x| **x == v[1][0]).count() as i64 <= v[2][0]),
            false,
            false,
        ),
        ConstraintDef::new(
            "nvalueleq", // CT_LEQNVALUE
            vec![List(Bound), Var(Constant)],
            |v| {
                let h: HashSet<i64> = HashSet::from_iter(v[0].iter().cloned());
                h.len() as i64 <= v[1][0]
            },
            false,
            false,
        ),
        ConstraintDef::new(
            "sumleq", // CT_LEQSUM
            vec![List(Bound), Var(Bound)],
            |v| (v[0].iter().sum::<i64>() <= v[1][0]),
            false,
            false,
        ),
        ConstraintDef::new_with_validator(
            "lexleq", // CT_LEXLEQ
            vec![List(Bound), List(Bound)],
            |v| (v[0] <= v[1]),
            true,
            true,
            |v| v.vars()[0].len() == v.vars()[1].len(),
        ),
        ConstraintDef::new_with_validator(
            "lexless", // CT_LEXLESS
            vec![List(Bound), List(Bound)],
            |v| (v[0] < v[1]),
            true,
            true,
            |v| v.vars()[0].len() == v.vars()[1].len(),
        ),
        ConstraintDef::new(
            "max", // CT_MAX
            vec![List(Bound), Var(Bound)],
            |v| (v[0].iter().cloned().max() == Some(v[1][0])),
            false,
            false,
        ),
        ConstraintDef::new(
            "min", // CT_MIN
            vec![List(Bound), Var(Bound)],
            |v| (v[0].iter().cloned().min() == Some(v[1][0])),
            false,
            false,
        ),
        // MINUSEQ_REIFY handled by CT_MINUSEQ
        ConstraintDef::new(
            "minuseq", // CT_MINUSEQ
            vec![Var(Bound), Var(Bound)],
            |v| (v[0][0] == -v[1][0]),
            false,
            false,
        ),
        ConstraintDef::new(
            "modulo_undefzero", // CT_MODULO_UNDEFZERO
            vec![Var(Bound), Var(Bound), Var(Bound)],
            |v| {
                v[2][0]
                    == (if v[1][0] == 0 {
                        0
                    } else {
                        v[0][0].mod_floor(&v[1][0])
                    })
            },
            false,
            false,
        ),
        ConstraintDef::new(
            "modulo", // CT_MODULO
            vec![Var(Bound), Var(Bound), Var(Bound)],
            |v| (v[1][0] != 0 && (v[0][0].mod_floor(&v[1][0]) == v[2][0])),
            false,
            false,
        ),
        ConstraintDef::new(
            "occurrence", // CT_OCCURRENCE
            vec![List(Discrete), Var(Constant), Var(Discrete)],
            |v| (v[0].iter().filter(|x| **x == v[1][0]).count() as i64 == v[2][0]),
            false,
            false,
        ),
        ConstraintDef::new(
            "pow", // CT_POW
            vec![Var(Bound), Var(Bound), Var(Bound)],
            |v| {
                if v[0][0] == 0 {
                    if v[1][0] < 0 {
                        return false;
                    }
                    return v[2][0] == (if v[1][0] == 0 { 1 } else { 0 });
                }

                if v[0][0] == 1 {
                    return v[2][0] == 1;
                }

                if v[0][0] == -1 {
                    return v[2][0] == (if v[1][0].is_even() { 1 } else { -1 });
                }

                if v[1][0] < 0 {
                    return false;
                }

                v[0][0].pow(v[1][0] as u32) == v[2][0]
            },
            false,
            false,
        ),
        ConstraintDef::new(
            "product", // CT_PRODUCT
            vec![Var(Bound), Var(Bound), Var(Bound)],
            |v| (v[0][0] * v[1][0] == v[2][0]),
            false,
            false,
        ),
        ConstraintDef::new_with_validator(
            "lexleq[quick]", // CT_QUICK_LEXLEQ
            vec![List(Bound), List(Bound)],
            |v| (v[0] <= v[1]),
            false,
            false,
            |v| v.vars()[0].len() == v.vars()[1].len(),
        ),
        ConstraintDef::new_with_validator(
            "lexless[quick]", // CT_QUICK_LEXLESS
            vec![List(Bound), List(Bound)],
            |v| (v[0] < v[1]),
            false,
            false,
            |v| v.vars()[0].len() == v.vars()[1].len(),
        ),
        ConstraintDef::new_full(
            "shortstr2", // CT_SHORTSTR
            // shortstr2 itself accepts any var type, but the
            // equivalence group includes haggisgac/-stable which
            // call removeFromDomain → Discrete only.
            vec![List(Discrete), ShortTuples],
            check_short_tuples,
            true,
            true,
            valid_short_tuples_instance,
        ),
        ConstraintDef::new_full(
            "shortctuplestr2", // CT_SHORTSTR_CTUPLE
            // Accepts the same single-literal short tuples as the
            // others; its multi-literal-per-var feature is not
            // exercised by this generator (would need a separate
            // test entry point).
            vec![List(Discrete), ShortTuples],
            check_short_tuples,
            true,
            true,
            valid_short_tuples_instance,
        ),
        ConstraintDef::new(
            "true", // CT_TRUEArc
            vec![],
            |_| true,
            true,
            true,
        ),
        ConstraintDef::new(
            "watchelement_one_undefzero", // CT_WATCHED_ELEMENT_ONE_UNDEFZERO
            vec![List(Discrete), Var(Discrete), Var(Discrete)],
            |v| check_element(v, 1, true),
            true,
            false,
        ),
        ConstraintDef::new(
            "watchelement_one", // CT_WATCHED_ELEMENT_ONE
            vec![List(Discrete), Var(Discrete), Var(Discrete)],
            |v| check_element(v, 1, false),
            true,
            false,
        ),
        ConstraintDef::new(
            "watchelement_undefzero", // CT_WATCHED_ELEMENT_UNDEFZERO
            vec![List(Discrete), Var(Discrete), Var(Discrete)],
            |v| check_element(v, 0, true),
            true,
            false,
        ),
        ConstraintDef::new(
            "watchelement", // CT_WATCHED_ELEMENT
            vec![List(Discrete), Var(Discrete), Var(Discrete)],
            |v| check_element(v, 0, false),
            true,
            false,
        ),
        ConstraintDef::new(
            "watchsumgeq", // CT_WATCHED_GEQSUM
            vec![List(Bool), Var(Constant)],
            |v| v[0].iter().cloned().sum::<i64>() >= v[1][0],
            true,
            true,
        ),
        ConstraintDef::new_with_validator(
            "hamming", // CT_WATCHED_HAMMING
            vec![List(Bound), List(Bound), Var(Constant)],
            |v| v[0].iter().zip(v[1]).filter(|(a, b)| *a != *b).count() as i64 >= v[2][0],
            true,
            true,
            |c| c.vars()[0].len() == c.vars()[1].len(),
        ),
        ConstraintDef::new_with_validator(
            "w-inintervalset", // CT_WATCHED_ININTERVALSET
            vec![Var(Bound), List(Constant)],
            |v| {
                for i in (0..v[1].len()).step_by(2) {
                    if v[1][i] <= v[0][0] && v[0][0] <= v[1][i + 1] {
                        return true;
                    }
                }
                false
            },
            true,
            true,
            |c| {
                let vals: Vec<i64> = c.vars()[1].iter().map(|x| (*x).get_value()).collect();
                if vals.len().is_odd() || vals.len() < 2 {
                    return false;
                }
                if !vals.is_empty() {
                    for i in (0..vals.len()).step_by(2) {
                        if vals[i] > vals[i + 1] {
                            return false;
                        }
                    }
                    for i in (1..vals.len() - 1).step_by(2) {
                        if vals[i] >= vals[i + 1] - 1 {
                            return false;
                        }
                    }
                }
                true
            },
        ),
        ConstraintDef::new_with_validator(
            "w-inrange", // CT_WATCHED_INRANGE
            vec![Var(Bound), List(Constant)],
            |v| v[1][0] <= v[0][0] && v[0][0] <= v[1][1],
            true,
            true,
            |c| {
                let vals: Vec<i64> = c.vars()[1].iter().map(|x| (*x).get_value()).collect();
                vals.len() == 2 && vals[0] <= vals[1]
            },
        ),
        ConstraintDef::new_with_validator(
            "w-inset", // CT_WATCHED_INSET
            vec![Var(Bound), List(Constant)],
            |v| v[1].contains(&v[0][0]),
            true,
            true,
            |c| {
                let vals: Vec<i64> = c.vars()[1].iter().map(|x| (*x).get_value()).collect();
                vals.windows(2).all(|s| s[0] < s[1])
            },
        ),
        ConstraintDef::new(
            "watchsumleq", // CT_WATCHED_LEQSUM
            vec![List(Bool), Var(Constant)],
            |v| v[0].iter().cloned().sum::<i64>() <= v[1][0],
            true,
            true,
        ),
        ConstraintDef::new(
            "watchless", // CT_WATCHED_LESS
            vec![Var(Bound), Var(Bound)],
            |v| (v[0][0] < v[1][0]),
            true,
            true,
        ),
        ConstraintDef::new(
            "w-literal", // CT_WATCHED_LIT
            vec![Var(Bound), Var(Constant)],
            |v| (v[0][0] == v[1][0]),
            true,
            true,
        ),
        ConstraintDef::new_with_validator(
            "litsumgeq", // CT_WATCHED_LITSUM
            vec![List(Discrete), List(Constant), Var(Constant)],
            |v| v[0].iter().zip(v[1]).filter(|(a, b)| *a == *b).count() as i64 >= v[2][0],
            true,
            true,
            |c| c.vars()[0].len() == c.vars()[1].len(),
        ),
        ConstraintDef::new(
            "watchneq", // CT_WATCHED_NEQ
            vec![Var(Discrete), Var(Discrete)],
            |v| (v[0][0] != v[1][0]),
            true,
            true,
        ),
        // TODO: WATCHED_NEW_AND, WATCHED_NEW_OR
        ConstraintDef::new_with_validator(
            "not-hamming", // CT_WATCHED_NOT_HAMMING
            vec![List(Bound), List(Bound), Var(Constant)],
            |v| (v[0].iter().zip(v[1]).filter(|(a, b)| *a != *b).count() as i64) < v[2][0],
            false,
            false,
            |c| c.vars()[0].len() == c.vars()[1].len(),
        ),
        ConstraintDef::new_with_validator(
            "w-notinrange", // CT_WATCHED_NOT_INRANGE
            vec![Var(Bound), List(Constant)],
            |v| !(v[1][0] <= v[0][0] && v[0][0] <= v[1][1]),
            true,
            true,
            |c| {
                let vals: Vec<i64> = c.vars()[1].iter().map(|x| (*x).get_value()).collect();
                vals.len() == 2 && vals[0] <= vals[1]
            },
        ),
        ConstraintDef::new_with_validator(
            "w-notinset", // CT_WATCHED_NOT_INSET
            vec![Var(Bound), List(Constant)],
            |v| !v[1].contains(&v[0][0]),
            true,
            true,
            |c| {
                let vals: Vec<i64> = c.vars()[1].iter().map(|x| (*x).get_value()).collect();
                vals.windows(2).all(|s| s[0] < s[1])
            },
        ),
        ConstraintDef::new(
            "w-notliteral", // CT_WATCHED_NOTLIT
            vec![Var(Bound), Var(Constant)],
            |v| (v[0][0] != v[1][0]),
            true,
            true,
        ),
        ConstraintDef::new_with_validator(
            "watchvecexists_less", // CT_WATCHED_VEC_OR_LESS
            vec![List(Bound), List(Bound)],
            |v| v[0].iter().zip(v[1]).any(|(a, b)| *a < *b),
            true,
            true,
            |c| c.vars()[0].len() == c.vars()[1].len(),
        ),
        ConstraintDef::new_with_validator(
            "watchvecneq", // CT_WATCHED_VECNEQ
            vec![List(Bound), List(Bound)],
            |v| v[0].iter().zip(v[1]).any(|(a, b)| *a != *b),
            true,
            true,
            |c| c.vars()[0].len() == c.vars()[1].len(),
        ),
        ConstraintDef::new_with_validator(
            "weightedsumgeq", // CT_WEIGHTGEQSUM
            vec![List(Constant), List(Bound), Var(Bound)],
            |v| v[0].iter().zip(v[1]).map(|(a, b)| *a * *b).sum::<i64>() >= v[2][0],
            true,
            true,
            |c| c.vars()[0].len() == c.vars()[1].len(),
        ),
        ConstraintDef::new_with_validator(
            "weightedsumleq", // CT_WEIGHTLEQSUM
            vec![List(Constant), List(Bound), Var(Bound)],
            |v| v[0].iter().zip(v[1]).map(|(a, b)| *a * *b).sum::<i64>() <= v[2][0],
            true,
            true,
            |c| c.vars()[0].len() == c.vars()[1].len(),
        ),
        // --- constraints that need instance-aware checkers ---
        ConstraintDef::new_full(
            "alldiffmatrix",
            vec![List(Discrete), Var(Constant)],
            check_alldiffmatrix,
            false,
            false,
            valid_alldiffmatrix,
        ),
        ConstraintDef::new_full(
            "gcc",
            vec![List(Discrete), List(Constant), List(Discrete)],
            check_gcc,
            false,
            false,
            valid_gcc,
        ),
        ConstraintDef::new_full(
            "gccweak",
            vec![List(Discrete), List(Constant), List(Discrete)],
            check_gcc,
            false,
            false,
            valid_gcc,
        ),
        // Table constraints (inline tuples, positive).
        // All of these reject Bound and SparseBound: table/negativetable/
        // gacschema/lighttable explicitly via CheckNotBound (new_table.h:141,
        // constraint_GACtable_trie.h:90, constraint_lighttable.h:62), and
        // mddc/negativemddc implicitly because their propagator removes
        // individual domain values, which only Discrete supports.
        ConstraintDef::new_full(
            "table",
            vec![List(Discrete), Tuples],
            check_table,
            true,
            true,
            valid_positive_table,
        ),
        ConstraintDef::new_full(
            "gacschema",
            vec![List(Discrete), Tuples],
            check_table,
            true,
            true,
            valid_positive_table,
        ),
        ConstraintDef::new_full(
            "lighttable",
            vec![List(Discrete), Tuples],
            check_table,
            true,
            true,
            valid_positive_table,
        ),
        // mddc / negativemddc reject Bound and SparseBound: their
        // propagator (constraint_mddc.h:581) calls
        // vars[var].removeFromDomain(val) which only Discrete supports.
        // Same restriction as the other table constraints above —
        // earlier comment was wrong to single them out.
        ConstraintDef::new_full(
            "mddc",
            vec![List(Discrete), Tuples],
            check_table,
            true,
            true,
            valid_positive_table,
        ),
        ConstraintDef::new_full(
            "negativemddc",
            vec![List(Discrete), Tuples],
            check_negative_table,
            true,
            true,
            valid_negative_table,
        ),
        ConstraintDef::new_full(
            "negativetable",
            vec![List(Discrete), Tuples],
            check_negative_table,
            true,
            true,
            valid_negative_table,
        ),
    ]
}
