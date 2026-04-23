//! In-process runner: builds a `minion_sys::Model` from a `ConstraintInstance`
//! and calls into libminion directly instead of shelling out to a binary.
//!
//! Each call to `get_minion_solutions_in_process` creates an independent
//! `MinionContext`. The rayon threadpool in `main` will drive many such calls
//! in parallel, which stress-tests libminion's thread-isolation between
//! independent solver contexts.

use std::collections::HashMap;
use std::sync::Arc;

use anyhow::{Context, Result, anyhow, bail};

use minion_sys::ast::{Constant as MC, Constraint as MCon, Model, Var, VarDomain, VarName};

use crate::constraint_def::{ConstraintInstance, MinionVariable, VarType};
use crate::run_minion::{CleanupFiles, MinionOutput};

/// Convert a tester `MinionVariable` to a minion-sys `Var`.
/// Constants become anonymous `ConstantAsVar`; everything else is a named ref.
fn var_of(v: &MinionVariable) -> Var {
    match v.var_type {
        VarType::Constant => Var::ConstantAsVar(v.domain[0] as i32),
        _ => Var::NameRef(v.name.clone()),
    }
}

fn list_of_vars(vars: &[Arc<MinionVariable>]) -> Vec<Var> {
    vars.iter().map(|v| var_of(v)).collect()
}

/// Extract the singleton value of a constant variable.
fn const_of(v: &MinionVariable) -> MC {
    assert_eq!(
        v.var_type,
        VarType::Constant,
        "expected Constant var, got {:?}",
        v.var_type
    );
    MC::Integer(v.domain[0] as i32)
}

fn list_of_consts(vars: &[Arc<MinionVariable>]) -> Vec<MC> {
    vars.iter().map(|v| const_of(v)).collect()
}

/// Build a minion-sys `Constraint` tree from a tester `ConstraintInstance`,
/// dispatched on the constraint's registered `name`.
fn build_constraint(instance: &ConstraintInstance) -> Result<MCon> {
    let name = instance.constraint.name.as_str();
    let v = instance.top_level_vars();
    let children = &instance.child_constraints;

    let child = |idx: usize| -> Result<Box<MCon>> {
        let c = children
            .get(idx)
            .ok_or_else(|| anyhow!("{name}: missing child constraint at index {idx}"))?;
        Ok(Box::new(build_constraint(c)?))
    };

    let cons = match name {
        "abs" => MCon::Abs(var_of(&v[0][0]), var_of(&v[1][0])),
        "alldiff" => MCon::AllDiff(list_of_vars(&v[0])),
        "gacalldiff" => MCon::GacAllDiff(list_of_vars(&v[0])),
        "difference" => MCon::Difference(
            (var_of(&v[0][0]), var_of(&v[1][0])),
            var_of(&v[2][0]),
        ),
        "diseq" => MCon::DisEq(var_of(&v[0][0]), var_of(&v[1][0])),
        "div" => MCon::Div(
            (var_of(&v[0][0]), var_of(&v[1][0])),
            var_of(&v[2][0]),
        ),
        "div_undefzero" => MCon::DivUndefZero(
            (var_of(&v[0][0]), var_of(&v[1][0])),
            var_of(&v[2][0]),
        ),
        "element" => MCon::Element(
            list_of_vars(&v[0]),
            var_of(&v[1][0]),
            var_of(&v[2][0]),
        ),
        "element_one" => MCon::ElementOne(
            list_of_vars(&v[0]),
            var_of(&v[1][0]),
            var_of(&v[2][0]),
        ),
        "element_undefzero" => MCon::ElementUndefZero(
            list_of_vars(&v[0]),
            var_of(&v[1][0]),
            var_of(&v[2][0]),
        ),
        "eq" => MCon::Eq(var_of(&v[0][0]), var_of(&v[1][0])),
        "gaceq" => MCon::GacEq(var_of(&v[0][0]), var_of(&v[1][0])),
        "false" => MCon::False,
        "true" => MCon::True,
        "ineq" => MCon::Ineq(
            var_of(&v[0][0]),
            var_of(&v[1][0]),
            const_of(&v[2][0]),
        ),
        "lexleq" => MCon::LexLeq(list_of_vars(&v[0]), list_of_vars(&v[1])),
        "lexleq[rv]" => MCon::LexLeqRv(list_of_vars(&v[0]), list_of_vars(&v[1])),
        "lexleq[quick]" => MCon::LexLeqQuick(list_of_vars(&v[0]), list_of_vars(&v[1])),
        "lexless" => MCon::LexLess(list_of_vars(&v[0]), list_of_vars(&v[1])),
        "lexless[quick]" => MCon::LexLessQuick(list_of_vars(&v[0]), list_of_vars(&v[1])),
        "max" => MCon::Max(list_of_vars(&v[0]), var_of(&v[1][0])),
        "min" => MCon::Min(list_of_vars(&v[0]), var_of(&v[1][0])),
        "minuseq" => MCon::MinusEq(var_of(&v[0][0]), var_of(&v[1][0])),
        "modulo" => MCon::Modulo(
            (var_of(&v[0][0]), var_of(&v[1][0])),
            var_of(&v[2][0]),
        ),
        "modulo_undefzero" => MCon::ModuloUndefZero(
            (var_of(&v[0][0]), var_of(&v[1][0])),
            var_of(&v[2][0]),
        ),
        "nvaluegeq" => MCon::NvalueGeq(list_of_vars(&v[0]), var_of(&v[1][0])),
        "nvalueleq" => MCon::NvalueLeq(list_of_vars(&v[0]), var_of(&v[1][0])),
        "occurrence" => MCon::Occurrence(
            list_of_vars(&v[0]),
            const_of(&v[1][0]),
            var_of(&v[2][0]),
        ),
        "occurrencegeq" => MCon::OccurrenceGeq(
            list_of_vars(&v[0]),
            const_of(&v[1][0]),
            const_of(&v[2][0]),
        ),
        "occurrenceleq" => MCon::OccurrenceLeq(
            list_of_vars(&v[0]),
            const_of(&v[1][0]),
            const_of(&v[2][0]),
        ),
        "pow" => MCon::Pow(
            (var_of(&v[0][0]), var_of(&v[1][0])),
            var_of(&v[2][0]),
        ),
        "product" => MCon::Product(
            (var_of(&v[0][0]), var_of(&v[1][0])),
            var_of(&v[2][0]),
        ),
        "sumgeq" => MCon::SumGeq(list_of_vars(&v[0]), var_of(&v[1][0])),
        "sumleq" => MCon::SumLeq(list_of_vars(&v[0]), var_of(&v[1][0])),
        "watchelement" => MCon::WatchElement(
            list_of_vars(&v[0]),
            var_of(&v[1][0]),
            var_of(&v[2][0]),
        ),
        "watchelement_one" => MCon::WatchElementOne(
            list_of_vars(&v[0]),
            var_of(&v[1][0]),
            var_of(&v[2][0]),
        ),
        "watchelement_one_undefzero" => MCon::WatchElementOneUndefZero(
            list_of_vars(&v[0]),
            var_of(&v[1][0]),
            var_of(&v[2][0]),
        ),
        "watchelement_undefzero" => MCon::WatchElementUndefZero(
            list_of_vars(&v[0]),
            var_of(&v[1][0]),
            var_of(&v[2][0]),
        ),
        "watchsumgeq" => MCon::WatchSumGeq(list_of_vars(&v[0]), const_of(&v[1][0])),
        "watchsumleq" => MCon::WatchSumLeq(list_of_vars(&v[0]), const_of(&v[1][0])),
        "watchless" => MCon::WatchLess(var_of(&v[0][0]), var_of(&v[1][0])),
        "watchneq" => MCon::WatchNeq(var_of(&v[0][0]), var_of(&v[1][0])),
        "hamming" => MCon::Hamming(
            list_of_vars(&v[0]),
            list_of_vars(&v[1]),
            const_of(&v[2][0]),
        ),
        "not-hamming" => MCon::NotHamming(
            list_of_vars(&v[0]),
            list_of_vars(&v[1]),
            const_of(&v[2][0]),
        ),
        "w-literal" => MCon::WLiteral(var_of(&v[0][0]), const_of(&v[1][0])),
        "w-notliteral" => MCon::WNotLiteral(var_of(&v[0][0]), const_of(&v[1][0])),
        "w-inintervalset" => MCon::WInIntervalSet(var_of(&v[0][0]), list_of_consts(&v[1])),
        "w-inrange" => MCon::WInRange(var_of(&v[0][0]), list_of_consts(&v[1])),
        "w-inset" => MCon::WInset(var_of(&v[0][0]), list_of_consts(&v[1])),
        "w-notinrange" => MCon::WNotInRange(var_of(&v[0][0]), list_of_consts(&v[1])),
        "w-notinset" => MCon::WNotInset(var_of(&v[0][0]), list_of_consts(&v[1])),
        "litsumgeq" => MCon::LitSumGeq(
            list_of_vars(&v[0]),
            list_of_consts(&v[1]),
            const_of(&v[2][0]),
        ),
        "watchvecneq" => MCon::WatchVecNeq(list_of_vars(&v[0]), list_of_vars(&v[1])),
        "watchvecexists_less" => MCon::WatchVecExistsLess(
            list_of_vars(&v[0]),
            list_of_vars(&v[1]),
        ),
        "weightedsumgeq" => MCon::WeightedSumGeq(
            list_of_consts(&v[0]),
            list_of_vars(&v[1]),
            var_of(&v[2][0]),
        ),
        "weightedsumleq" => MCon::WeightedSumLeq(
            list_of_consts(&v[0]),
            list_of_vars(&v[1]),
            var_of(&v[2][0]),
        ),

        // Nested parent constraints.
        "reify" => MCon::Reify(child(0)?, var_of(&v[1][0])),
        "reifyimply" => MCon::ReifyImply(child(0)?, var_of(&v[1][0])),
        "reifyimply-quick" => MCon::ReifyImplyQuick(child(0)?, var_of(&v[1][0])),
        "watched-and" => {
            let cs: Vec<MCon> = (0..children.len())
                .map(|i| build_constraint(&children[i]))
                .collect::<Result<Vec<_>>>()?;
            MCon::WatchedAnd(cs)
        }
        "watched-or" => {
            let cs: Vec<MCon> = (0..children.len())
                .map(|i| build_constraint(&children[i]))
                .collect::<Result<Vec<_>>>()?;
            MCon::WatchedOr(cs)
        }
        "forwardchecking" => MCon::ForwardChecking(child(0)?),
        "check[gsa]" => MCon::CheckGsa(child(0)?),
        "check[assign]" => MCon::CheckAssign(child(0)?),

        // Tableised instance. Uses minion's CT_STR (str2plus), the same
        // universal-tuple-constraint the exec tester uses as its
        // workhorse. Its tuple table is registered on the Model by
        // `register_tuple_tables_for_instance` before any constraint
        // is built; here we just reference the table by name.
        //
        // Exec-mode minion's text parser short-circuits the two
        // degenerate cases at parse time (see MinionThreeInputReader
        // `tuples->size() == 0` → CT_FALSE); when we go through the
        // library API we skip the parser, so do the equivalent here.
        "str2plus" => {
            let tups = instance
                .tuples
                .as_ref()
                .ok_or_else(|| anyhow!("str2plus instance has no tuple table"))?;
            if tups.tupledata.is_empty() {
                MCon::False
            } else if v[0].is_empty() {
                MCon::True
            } else {
                MCon::Str2Plus(
                    list_of_vars(&v[0]),
                    minion_sys::ast::Var::NameRef(tups.name.clone()),
                )
            }
        }

        other => bail!("in-process mode: constraint {:?} not yet supported", other),
    };
    Ok(cons)
}

/// Add every named (non-constant) variable in the instance to the model,
/// and emit a `DisEq(x, val)` constraint for every value in a hole of x's
/// domain — mirroring what `print_variable_def` does in text mode.
fn add_variables_and_holes(model: &mut Model, instance: &ConstraintInstance) -> Result<()> {
    for varlist in instance.vars().iter() {
        for v in varlist.iter() {
            if v.var_type == VarType::Constant {
                continue;
            }
            // We always use Discrete in-process. minion's BOUND variables
            // don't support interior-value removal, and MDDC (our chosen
            // tableise target, since str2plus with inline tuples isn't yet
            // exposed in the Model API) propagates aggressively enough to
            // need it. Promoting Bound and SparseBound to Discrete yields
            // the same solution set; node counts may differ from exec mode
            // but still agree between original and tableised within a run.
            let domain = match v.var_type {
                VarType::Bool => VarDomain::Bool,
                VarType::Bound | VarType::Discrete | VarType::SparseBound => {
                    let lo = *v.domain.first().unwrap() as i32;
                    let hi = *v.domain.last().unwrap() as i32;
                    VarDomain::Discrete(lo, hi)
                }
                VarType::Constant => unreachable!(),
            };
            if model
                .named_variables
                .add_var(v.name.clone(), domain)
                .is_none()
            {
                bail!("duplicate variable name in instance: {}", v.name);
            }

            // Holes (bool has no holes; its domain is always {0,1} after random_sublist,
            // and the text encoder matches {0..1} too — emit diseqs if needed).
            let lo = *v.domain.first().unwrap();
            let hi = *v.domain.last().unwrap();
            let range: Vec<i64> = if v.var_type == VarType::Bool {
                (0..2).collect()
            } else {
                (lo..hi).collect()
            };
            for val in range {
                if !v.domain.contains(&val) {
                    model.constraints.push(MCon::DisEq(
                        Var::NameRef(v.name.clone()),
                        Var::ConstantAsVar(val as i32),
                    ));
                }
            }
        }
    }
    Ok(())
}

/// Output of a constraint-injection run.
pub struct InjectOutput {
    pub solutions: Vec<Vec<i64>>,
    /// Number of scheduled injections that actually fired (i.e. search
    /// produced ≥ their threshold number of solutions before ending).
    pub injections_fired: usize,
    pub nodes: i64,
    /// True if the solution-count cap passed to the runner was reached
    /// and the callback returned false to stop search. When this is
    /// true, `solutions` is truncated and should not be trusted as a
    /// complete enumeration.
    pub stopped_at_limit: bool,
    /// Variable declaration order at the END of the run: the original
    /// declared vars followed by every mid-search-added var in the
    /// order they were declared. Solutions produced AFTER a new var
    /// was added include its value in the corresponding column;
    /// solutions produced before it exist with fewer columns and are
    /// left-padded as needed. Use this to map a column index back to
    /// a variable name.
    pub final_variable_order: Vec<String>,
}

/// One mid-search injection step. Each step fires when the solve has
/// reported its `count`th solution.
pub enum InjectionPacket<'a> {
    /// Inject a constraint that uses only variables already declared
    /// in the model (the existing-vars case).
    ExistingVarsConstraint(&'a ConstraintInstance),
    /// Declare `new_vars` mid-search via [`MidSearchContext::add_var`]
    /// (they land in the aux block), then add `constraint` via
    /// [`MidSearchContext::add_constraint`]. `constraint` may reference
    /// either the newly-added vars or any already-declared variable.
    AddVarsAndConstraint {
        new_vars: Vec<(String, minion_sys::ast::VarDomain)>,
        constraint: minion_sys::ast::Constraint,
    },
}

/// Walk a `ConstraintInstance` (recursively through children) and
/// register any tuple table it carries onto `model`. Must run before
/// `build_constraint` for any instance whose constraint references a
/// tuple table by name (e.g. `str2plus`).
fn register_tuple_tables_for_instance(
    model: &mut Model,
    instance: &ConstraintInstance,
) -> Result<()> {
    if let Some(ref tups) = instance.tuples {
        // Empty tuple tables would be short-circuited to MCon::False at
        // constraint-build time, so there's no downstream consumer —
        // and registering an empty table on the CSPInstance triggers a
        // crash inside TupleList's constructor.
        if !tups.tupledata.is_empty() {
            let tuple_vecs: Vec<Vec<minion_sys::ast::Constant>> = tups
                .tupledata
                .iter()
                .map(|row| {
                    row.iter()
                        .map(|n| minion_sys::ast::Constant::Integer(*n as i32))
                        .collect()
                })
                .collect();
            if model
                .add_tuple_table(tups.name.clone(), tuple_vecs)
                .is_none()
            {
                // get_unique_name should make collisions impossible in
                // practice, so flag loudly if we ever see one.
                bail!("duplicate tuple-table name registered: {}", tups.name);
            }
        }
    }
    for child in &instance.child_constraints {
        register_tuple_tables_for_instance(model, child)?;
    }
    Ok(())
}

/// Build a model from a set of [`ConstraintInstance`]s: every instance
/// in `with_constraints` contributes both its variables (with diseq
/// constraints for domain holes) AND its top-level constraint; every
/// instance in `vars_only` contributes only its variables.
///
/// Variable declaration order is `with_constraints` first (in the given
/// order), then `vars_only`. Callers that need solution outputs from
/// several runs to align must pass the instances in the same sequence.
fn build_multi_model(
    with_constraints: &[&ConstraintInstance],
    vars_only: &[&ConstraintInstance],
) -> Result<Model> {
    let mut model = Model::new();
    for inst in with_constraints {
        register_tuple_tables_for_instance(&mut model, inst)?;
    }
    for inst in vars_only {
        register_tuple_tables_for_instance(&mut model, inst)?;
    }
    for inst in with_constraints {
        add_variables_and_holes(&mut model, inst)?;
    }
    for inst in vars_only {
        add_variables_and_holes(&mut model, inst)?;
    }
    for inst in with_constraints {
        model
            .constraints
            .push(build_constraint(inst).context("building constraint")?);
    }
    Ok(model)
}

/// Run several [`ConstraintInstance`]s together as a single model, with
/// a pinned seed. `vars_only` instances contribute declared variables
/// but no top-level constraint.
/// Output of a capped run.
///
/// Callers that need a complete enumeration must check
/// `stopped_at_limit`; when it's true the solution list was truncated
/// by the callback returning false, and any invariant that compares
/// against the full set is unverifiable.
pub struct CappedOutput {
    pub solutions: Vec<Vec<i64>>,
    pub stopped_at_limit: bool,
    pub nodes: i64,
}

pub fn run_multi(
    with_constraints: &[&ConstraintInstance],
    vars_only: &[&ConstraintInstance],
    seed: u32,
    max_solutions: Option<usize>,
    testname: &str,
) -> Result<CappedOutput> {
    let model = build_multi_model(with_constraints, vars_only)?;
    let variable_order = model.named_variables.get_variable_order();

    if std::env::var("TESTER_DEBUG").is_ok() {
        eprintln!("--- multi-model for {testname} (seed={seed:#x}) ---");
        for name in &variable_order {
            eprintln!(
                "  {name}: {:?}",
                model.named_variables.get_vartype(name.clone())
            );
        }
        for c in &model.constraints {
            eprintln!("  {c:?}");
        }
        eprintln!("--- end {testname} ---");
    }

    let mut solutions: Vec<Vec<i64>> = Vec::new();
    let mut stopped_at_limit = false;
    let callback: minion_sys::Callback<'_> = {
        let variable_order = &variable_order;
        let solutions = &mut solutions;
        let stopped_at_limit = &mut stopped_at_limit;
        Box::new(move |sol: HashMap<VarName, MC>| -> bool {
            if let Some(cap) = max_solutions {
                if solutions.len() >= cap {
                    *stopped_at_limit = true;
                    return false;
                }
            }
            let mut row = Vec::with_capacity(variable_order.len());
            for name in variable_order.iter() {
                match sol.get(name).copied() {
                    Some(MC::Integer(n)) => row.push(n as i64),
                    Some(MC::Bool(b)) => row.push(if b { 1 } else { 0 }),
                    _ => return false,
                }
            }
            solutions.push(row);
            true
        })
    };

    let ctx = minion_sys::run_minion_with_options(
        model,
        minion_sys::RunOptions {
            seed: Some(seed),
            ..Default::default()
        },
        callback,
    )
    .map_err(|e| anyhow!("minion ({testname}): {e}"))?;

    let nodes = ctx
        .get_from_table("Nodes".to_string())
        .ok_or_else(|| anyhow!("Nodes missing"))?
        .parse::<i64>()
        .context("parsing Nodes")?;

    Ok(CappedOutput {
        solutions,
        stopped_at_limit,
        nodes,
    })
}

/// Run the model built from `base` (vars + constraints) plus variables
/// declared in `vars_only`, and apply each [`InjectionPacket`] at its
/// scheduled callback count.
///
/// `injections` must be sorted by callback count ascending. For
/// [`InjectionPacket::ExistingVarsConstraint`] the constraint can
/// refer to any variable declared before the solve started; for
/// [`InjectionPacket::AddVarsAndConstraint`] the added vars land in
/// the aux block (one aux assignment per decision leaf — see the
/// Phase 2 midsearch-add-vars test for the surrounding semantics).
pub fn run_multi_injected(
    base: &[&ConstraintInstance],
    vars_only: &[&ConstraintInstance],
    injections: &[(usize, InjectionPacket<'_>)],
    seed: u32,
    max_solutions: Option<usize>,
    testname: &str,
) -> Result<InjectOutput> {
    for w in injections.windows(2) {
        if w[0].0 > w[1].0 {
            bail!("run_multi_injected: injections must be sorted by count");
        }
    }

    let mut model = build_multi_model(base, vars_only)?;
    // Register tuple tables on any injection instance too, so a
    // mid-search-injected `str2plus` can reference the right table.
    for (_, pkt) in injections {
        if let InjectionPacket::ExistingVarsConstraint(inst) = pkt {
            register_tuple_tables_for_instance(&mut model, inst)?;
        }
    }
    let initial_variable_order = model.named_variables.get_variable_order();

    if std::env::var("TESTER_DEBUG").is_ok() {
        eprintln!("--- multi-inject model for {testname} (seed={seed:#x}) ---");
        for name in &initial_variable_order {
            eprintln!(
                "  {name}: {:?}",
                model.named_variables.get_vartype(name.clone())
            );
        }
        eprintln!("  base constraints:");
        for c in &model.constraints {
            eprintln!("    {c:?}");
        }
        for (cnt, pkt) in injections {
            match pkt {
                InjectionPacket::ExistingVarsConstraint(inst) => {
                    eprintln!("  inject at {cnt}: {}", inst.constraint.name);
                }
                InjectionPacket::AddVarsAndConstraint { new_vars, constraint } => {
                    let ns: Vec<&str> =
                        new_vars.iter().map(|(n, _)| n.as_str()).collect();
                    eprintln!(
                        "  inject at {cnt}: add_vars={ns:?} + {constraint:?}"
                    );
                }
            }
        }
        eprintln!("--- end {testname} ---");
    }

    // Pre-build each injection's constraint so the callback only has
    // to clone an already-valid AST. For existing-vars packets this
    // comes from the tester-side ConstraintInstance; for
    // add-vars-and-constraint packets we use the caller-supplied AST
    // directly.
    let prebuilt: Vec<minion_sys::ast::Constraint> = injections
        .iter()
        .map(|(_, pkt)| match pkt {
            InjectionPacket::ExistingVarsConstraint(inst) => {
                build_constraint(inst).with_context(|| {
                    format!(
                        "building injection constraint {:?} for {testname}",
                        inst.constraint.name
                    )
                })
            }
            InjectionPacket::AddVarsAndConstraint { constraint, .. } => {
                Ok(constraint.clone())
            }
        })
        .collect::<Result<Vec<_>>>()?;

    let mut solutions: Vec<Vec<i64>> = Vec::new();
    let mut count: usize = 0;
    let mut next_injection: usize = 0;
    let mut fired: usize = 0;
    let mut cb_err: Option<String> = None;
    let mut stopped_at_limit = false;
    // Track the growing set of variables actually live in the model
    // so each row we emit has the right column set. Start with the
    // initial declaration order; as each add-vars packet fires, we
    // append its new vars (in the order they're declared).
    let mut live_var_order: Vec<String> = initial_variable_order.clone();

    let callback: minion_sys::MidSearchCallback<'_> = {
        let live_var_order_ref = &mut live_var_order;
        let solutions = &mut solutions;
        let count_ref = &mut count;
        let next_injection_ref = &mut next_injection;
        let fired_ref = &mut fired;
        let cb_err_ref = &mut cb_err;
        let stopped_ref = &mut stopped_at_limit;
        let prebuilt = &prebuilt;
        let injections = injections;
        Box::new(
            move |midctx: &mut minion_sys::MidSearchContext<'_>,
                  sol: HashMap<VarName, MC>|
                  -> bool {
                if let Some(cap) = max_solutions {
                    if solutions.len() >= cap {
                        *stopped_ref = true;
                        return false;
                    }
                }
                *count_ref += 1;
                let mut row = Vec::with_capacity(live_var_order_ref.len());
                for name in live_var_order_ref.iter() {
                    match sol.get(name).copied() {
                        Some(MC::Integer(n)) => row.push(n as i64),
                        Some(MC::Bool(b)) => row.push(if b { 1 } else { 0 }),
                        other => {
                            *cb_err_ref = Some(format!(
                                "callback: var {name} has no value ({other:?})"
                            ));
                            return false;
                        }
                    }
                }
                solutions.push(row);
                while *next_injection_ref < injections.len()
                    && injections[*next_injection_ref].0 == *count_ref
                {
                    match &injections[*next_injection_ref].1 {
                        InjectionPacket::ExistingVarsConstraint(_) => {
                            let constr = prebuilt[*next_injection_ref].clone();
                            if let Err(e) = midctx.add_constraint(constr) {
                                *cb_err_ref = Some(format!("add_constraint: {e}"));
                                return false;
                            }
                        }
                        InjectionPacket::AddVarsAndConstraint {
                            new_vars,
                            ..
                        } => {
                            for (name, domain) in new_vars {
                                if let Err(e) = midctx.add_var(name, *domain) {
                                    *cb_err_ref = Some(format!(
                                        "add_var({name}): {e}"
                                    ));
                                    return false;
                                }
                                live_var_order_ref.push(name.clone());
                            }
                            let constr = prebuilt[*next_injection_ref].clone();
                            if let Err(e) = midctx.add_constraint(constr) {
                                *cb_err_ref =
                                    Some(format!("add_constraint (new-vars packet): {e}"));
                                return false;
                            }
                        }
                    }
                    *next_injection_ref += 1;
                    *fired_ref += 1;
                }
                true
            },
        )
    };

    let ctx = minion_sys::run_minion_midsearch_with_options(
        model,
        minion_sys::RunOptions {
            seed: Some(seed),
            ..Default::default()
        },
        callback,
    )
    .map_err(|e| anyhow!("minion midsearch ({testname}): {e}"))?;

    if let Some(err) = cb_err {
        bail!("callback error ({testname}): {err}");
    }

    let nodes = ctx
        .get_from_table("Nodes".to_string())
        .ok_or_else(|| anyhow!("Nodes missing"))?
        .parse::<i64>()
        .context("parsing Nodes")?;

    Ok(InjectOutput {
        solutions,
        injections_fired: fired,
        nodes,
        stopped_at_limit,
        final_variable_order: live_var_order,
    })
}

/// Result of a midsearch variable-injection run.
#[derive(Debug)]
pub struct MidsearchOutput {
    /// Solutions reported before the injection callback fired. Each row
    /// holds values for the original (pre-injection) named variables in
    /// declaration order.
    pub pre_injection: Vec<Vec<i64>>,
    /// Solutions reported after the injection. Each row holds values
    /// for the original vars first, then the newly-added vars in the
    /// order they were added.
    pub post_injection: Vec<Vec<i64>>,
    /// Names of the variables that were added mid-search.
    pub new_var_names: Vec<String>,
    /// True if the injection callback actually ran. False if baseline
    /// had fewer than `inject_after` solutions.
    pub injected: bool,
    pub nodes: i64,
}

/// Run `instance`, and after `inject_after` solutions have been reported
/// add the variables in `new_vars` via `MidSearchContext::add_var`.
///
/// The new variables are aux search variables with no constraints on
/// them, so every "remaining" decision-var assignment should fan out
/// into the cartesian product of the new vars' domains.
pub fn run_inject_vars_after(
    instance: &ConstraintInstance,
    inject_after: usize,
    new_vars: &[(String, minion_sys::ast::VarDomain)],
    testname: &str,
) -> Result<MidsearchOutput> {
    let mut model = Model::new();
    add_variables_and_holes(&mut model, instance)?;
    let top = build_constraint(instance)
        .with_context(|| format!("building constraint for {testname}"))?;
    model.constraints.push(top);

    let variable_order = model.named_variables.get_variable_order();
    let new_var_names: Vec<String> = new_vars.iter().map(|(n, _)| n.clone()).collect();

    let mut pre_injection: Vec<Vec<i64>> = Vec::new();
    let mut post_injection: Vec<Vec<i64>> = Vec::new();
    let mut injected = false;
    let mut count: usize = 0;
    let mut cb_err: Option<String> = None;

    let callback: minion_sys::MidSearchCallback<'_> = {
        let variable_order = &variable_order;
        let new_var_names = &new_var_names;
        let pre = &mut pre_injection;
        let post = &mut post_injection;
        let injected_ref = &mut injected;
        let count_ref = &mut count;
        let cb_err_ref = &mut cb_err;
        Box::new(move |midctx: &mut minion_sys::MidSearchContext<'_>, sol: HashMap<VarName, MC>| -> bool {
            *count_ref += 1;
            // Read original vars.
            let mut row = Vec::with_capacity(variable_order.len() + new_var_names.len());
            for name in variable_order.iter() {
                match sol.get(name).copied() {
                    Some(MC::Integer(n)) => row.push(n as i64),
                    Some(MC::Bool(b)) => row.push(if b { 1 } else { 0 }),
                    other => {
                        *cb_err_ref = Some(format!(
                            "callback: original var {name} has no value ({other:?})"
                        ));
                        return false;
                    }
                }
            }
            if *injected_ref {
                // Append new vars.
                for name in new_var_names.iter() {
                    match sol.get(name).copied() {
                        Some(MC::Integer(n)) => row.push(n as i64),
                        Some(MC::Bool(b)) => row.push(if b { 1 } else { 0 }),
                        other => {
                            *cb_err_ref = Some(format!(
                                "callback: mid-search var {name} has no value ({other:?})"
                            ));
                            return false;
                        }
                    }
                }
                post.push(row);
            } else {
                pre.push(row);
                if *count_ref == inject_after {
                    for (name, domain) in new_vars {
                        if let Err(e) = midctx.add_var(name, *domain) {
                            *cb_err_ref = Some(format!("add_var({name}): {e}"));
                            return false;
                        }
                    }
                    *injected_ref = true;
                }
            }
            true
        })
    };

    let ctx = minion_sys::run_minion_midsearch(model, callback)
        .map_err(|e| anyhow!("minion midsearch error ({testname}): {e}"))?;

    if let Some(err) = cb_err {
        bail!("callback error ({testname}): {err}");
    }

    let nodes: i64 = ctx
        .get_from_table("Nodes".to_string())
        .ok_or_else(|| anyhow!("Nodes missing from minion stats"))?
        .parse()
        .context("parsing Nodes")?;

    Ok(MidsearchOutput {
        pre_injection,
        post_injection,
        new_var_names,
        injected,
        nodes,
    })
}

/// In-process equivalent of `run_minion::get_minion_solutions`.
///
/// `find_all_sols` mirrors the `-findallsols` command-line flag — when true,
/// the callback returns `true` to keep enumerating; when false, it returns
/// `false` after the first solution to stop.
pub fn get_minion_solutions_in_process(
    instance: &ConstraintInstance,
    find_all_sols: bool,
    testname: &str,
) -> Result<MinionOutput> {
    let mut model = Model::new();
    register_tuple_tables_for_instance(&mut model, instance)?;
    add_variables_and_holes(&mut model, instance)?;
    let top = build_constraint(instance)
        .with_context(|| format!("building constraint for {testname}"))?;
    model.constraints.push(top);

    let variable_order = model.named_variables.get_variable_order();

    let mut solutions: Vec<Vec<i64>> = Vec::new();

    let callback: minion_sys::Callback<'_> = {
        let variable_order = &variable_order;
        let solutions = &mut solutions;
        Box::new(move |sol_map: HashMap<VarName, MC>| -> bool {
            let mut row = Vec::with_capacity(variable_order.len());
            for name in variable_order.iter() {
                match sol_map.get(name).copied() {
                    Some(MC::Integer(n)) => row.push(n as i64),
                    Some(MC::Bool(b)) => row.push(if b { 1 } else { 0 }),
                    Some(_) | None => return false,
                }
            }
            solutions.push(row);
            find_all_sols
        })
    };

    if std::env::var("TESTER_DEBUG").is_ok() {
        eprintln!("--- in-process model for {testname} ---");
        eprintln!("variables:");
        for name in model.named_variables.get_variable_order() {
            eprintln!("  {name}: {:?}", model.named_variables.get_vartype(name.clone()));
        }
        eprintln!("constraints:");
        for c in &model.constraints {
            eprintln!("  {c:?}");
        }
        eprintln!("--- end model ---");
    }

    let ctx = minion_sys::run_minion(model, callback)
        .map_err(|e| anyhow!("minion in-process error ({testname}): {e}"))?;

    let nodes_str = ctx
        .get_from_table("Nodes".to_string())
        .ok_or_else(|| anyhow!("Nodes key missing from minion stats"))?;
    let nodes: i64 = nodes_str
        .parse()
        .with_context(|| format!("parsing Nodes={nodes_str:?}"))?;

    Ok(MinionOutput {
        solutions,
        nodes,
        filename: format!("<in-process:{testname}>"),
        cleanup: CleanupFiles::empty(),
    })
}
