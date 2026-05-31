//! In-process runner: builds a `minion_sys::Model` from a `ConstraintInstance`
//! and calls into libminion directly instead of shelling out to a binary.
//!
//! Each call to `get_minion_solutions_in_process` creates an independent
//! `MinionContext`. The rayon threadpool in `main` will drive many such calls
//! in parallel, which stress-tests libminion's thread-isolation between
//! independent solver contexts.

use std::collections::HashMap;
use std::sync::Arc;

use anyhow::{anyhow, bail, Context, Result};

use minion_sys::ast::{
    Constant as MC, Constraint as MCon, Model, Optimise, ShortTuple as MShort, Var, VarDomain,
    VarName,
};

use crate::constraint_def::{ConstraintInstance, MinionVariable, VarType};
use crate::minion_instance::OptimisationWrapper;
use crate::run_minion::{CleanupFiles, MinionOutput};
use crate::solution_digest::SolutionDigest;

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

/// Convert tester-side tuple data (i64 rows) to minion-sys [`minion_sys::ast::Constant`] tuples.
fn convert_tuples(tuples: &crate::constraint_def::Tuples) -> Vec<Vec<MC>> {
    tuples
        .tupledata
        .iter()
        .map(|row| row.iter().map(|&n| MC::Integer(n as i32)).collect())
        .collect()
}

/// Convert tester-side short-tuple data (`Vec<Vec<(usize, i64)>>`) to
/// minion-sys [`MShort`] (`Vec<(usize, Constant)>`).
fn convert_short_tuples(st: &crate::constraint_def::ShortTuples) -> Vec<MShort> {
    st.data
        .iter()
        .map(|short| {
            short
                .iter()
                .map(|&(idx, val)| (idx, MC::Integer(val as i32)))
                .collect()
        })
        .collect()
}

/// Build a minion-sys `Constraint` tree from a tester `ConstraintInstance`,
/// dispatched on the constraint's registered `name`.
fn build_constraint(instance: &ConstraintInstance) -> Result<MCon> {
    // Negated bool slots have no minion-sys representation today
    // (`Var` is `NameRef | ConstantAsVar`, no `NegatedBool` variant).
    // The random generator is supposed to keep NEGATION_PERMILLE at 0
    // under `--in-process`; if a `!`-flagged instance still reaches
    // here something has bypassed that gate and silently dropping the
    // `!` would give wrong answers.
    assert!(
        !instance.has_any_negated(),
        "in-process backend received a ConstraintInstance with negated bool slots; \
         the generator must keep NEGATION_PERMILLE at 0 in --in-process mode"
    );
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
        "alldiffmatrix" => MCon::AllDiffMatrix(list_of_vars(&v[0]), const_of(&v[1][0])),
        "gacalldiff" => MCon::GacAllDiff(list_of_vars(&v[0])),
        "difference" => MCon::Difference((var_of(&v[0][0]), var_of(&v[1][0])), var_of(&v[2][0])),
        "diseq" => MCon::DisEq(var_of(&v[0][0]), var_of(&v[1][0])),
        "div" => MCon::Div((var_of(&v[0][0]), var_of(&v[1][0])), var_of(&v[2][0])),
        "div_undefzero" => {
            MCon::DivUndefZero((var_of(&v[0][0]), var_of(&v[1][0])), var_of(&v[2][0]))
        }
        "element" => MCon::Element(list_of_vars(&v[0]), var_of(&v[1][0]), var_of(&v[2][0])),
        "element_one" => MCon::ElementOne(list_of_vars(&v[0]), var_of(&v[1][0]), var_of(&v[2][0])),
        "element_undefzero" => {
            MCon::ElementUndefZero(list_of_vars(&v[0]), var_of(&v[1][0]), var_of(&v[2][0]))
        }
        "eq" => MCon::Eq(var_of(&v[0][0]), var_of(&v[1][0])),
        "gaceq" => MCon::GacEq(var_of(&v[0][0]), var_of(&v[1][0])),
        "gcc" => MCon::Gcc(
            list_of_vars(&v[0]),
            list_of_consts(&v[1]),
            list_of_vars(&v[2]),
        ),
        "gccweak" => MCon::GccWeak(
            list_of_vars(&v[0]),
            list_of_consts(&v[1]),
            list_of_vars(&v[2]),
        ),
        "false" => MCon::False,
        "frameupdate" => MCon::FrameUpdate(
            list_of_vars(&v[0]),
            list_of_vars(&v[1]),
            list_of_vars(&v[2]),
            list_of_vars(&v[3]),
            const_of(&v[4][0]),
        ),
        "true" => MCon::True,
        "ineq" => MCon::Ineq(var_of(&v[0][0]), var_of(&v[1][0]), const_of(&v[2][0])),
        "lexleq" => MCon::LexLeq(list_of_vars(&v[0]), list_of_vars(&v[1])),
        "lexleq[rv]" => MCon::LexLeqRv(list_of_vars(&v[0]), list_of_vars(&v[1])),
        "lexleq[quick]" => MCon::LexLeqQuick(list_of_vars(&v[0]), list_of_vars(&v[1])),
        "lexless" => MCon::LexLess(list_of_vars(&v[0]), list_of_vars(&v[1])),
        "lexless[quick]" => MCon::LexLessQuick(list_of_vars(&v[0]), list_of_vars(&v[1])),
        "max" => MCon::Max(list_of_vars(&v[0]), var_of(&v[1][0])),
        "min" => MCon::Min(list_of_vars(&v[0]), var_of(&v[1][0])),
        "minuseq" => MCon::MinusEq(var_of(&v[0][0]), var_of(&v[1][0])),
        "modulo" => MCon::Modulo((var_of(&v[0][0]), var_of(&v[1][0])), var_of(&v[2][0])),
        "modulo_undefzero" => {
            MCon::ModuloUndefZero((var_of(&v[0][0]), var_of(&v[1][0])), var_of(&v[2][0]))
        }
        "nvaluegeq" => MCon::NvalueGeq(list_of_vars(&v[0]), var_of(&v[1][0])),
        "nvalueleq" => MCon::NvalueLeq(list_of_vars(&v[0]), var_of(&v[1][0])),
        "occurrence" => MCon::Occurrence(list_of_vars(&v[0]), const_of(&v[1][0]), var_of(&v[2][0])),
        "occurrencegeq" => {
            MCon::OccurrenceGeq(list_of_vars(&v[0]), const_of(&v[1][0]), const_of(&v[2][0]))
        }
        "occurrenceleq" => {
            MCon::OccurrenceLeq(list_of_vars(&v[0]), const_of(&v[1][0]), const_of(&v[2][0]))
        }
        "pow" => MCon::Pow((var_of(&v[0][0]), var_of(&v[1][0])), var_of(&v[2][0])),
        "product" => MCon::Product((var_of(&v[0][0]), var_of(&v[1][0])), var_of(&v[2][0])),
        "sumgeq" => MCon::SumGeq(list_of_vars(&v[0]), var_of(&v[1][0])),
        "sumleq" => MCon::SumLeq(list_of_vars(&v[0]), var_of(&v[1][0])),
        "watchelement" => {
            MCon::WatchElement(list_of_vars(&v[0]), var_of(&v[1][0]), var_of(&v[2][0]))
        }
        "watchelement_one" => {
            MCon::WatchElementOne(list_of_vars(&v[0]), var_of(&v[1][0]), var_of(&v[2][0]))
        }
        "watchelement_one_undefzero" => {
            MCon::WatchElementOneUndefZero(list_of_vars(&v[0]), var_of(&v[1][0]), var_of(&v[2][0]))
        }
        "watchelement_undefzero" => {
            MCon::WatchElementUndefZero(list_of_vars(&v[0]), var_of(&v[1][0]), var_of(&v[2][0]))
        }
        "watchsumgeq" => MCon::WatchSumGeq(list_of_vars(&v[0]), const_of(&v[1][0])),
        "watchsumleq" => MCon::WatchSumLeq(list_of_vars(&v[0]), const_of(&v[1][0])),
        "watchless" => MCon::WatchLess(var_of(&v[0][0]), var_of(&v[1][0])),
        "watchneq" => MCon::WatchNeq(var_of(&v[0][0]), var_of(&v[1][0])),
        "hamming" => MCon::Hamming(list_of_vars(&v[0]), list_of_vars(&v[1]), const_of(&v[2][0])),
        "not-hamming" => {
            MCon::NotHamming(list_of_vars(&v[0]), list_of_vars(&v[1]), const_of(&v[2][0]))
        }
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
        "watchvecexists_less" => MCon::WatchVecExistsLess(list_of_vars(&v[0]), list_of_vars(&v[1])),
        "weightedsumgeq" => {
            MCon::WeightedSumGeq(list_of_consts(&v[0]), list_of_vars(&v[1]), var_of(&v[2][0]))
        }
        "weightedsumleq" => {
            MCon::WeightedSumLeq(list_of_consts(&v[0]), list_of_vars(&v[1]), var_of(&v[2][0]))
        }

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

        // Inline-tuple table constraints.
        "table" => {
            let tups = instance
                .tuples
                .as_ref()
                .ok_or_else(|| anyhow!("table instance has no tuple data"))?;
            MCon::Table(list_of_vars(&v[0]), convert_tuples(tups))
        }
        "negativetable" => {
            let tups = instance
                .tuples
                .as_ref()
                .ok_or_else(|| anyhow!("negativetable instance has no tuple data"))?;
            MCon::NegativeTable(list_of_vars(&v[0]), convert_tuples(tups))
        }
        "gacschema" => {
            let tups = instance
                .tuples
                .as_ref()
                .ok_or_else(|| anyhow!("gacschema instance has no tuple data"))?;
            MCon::GacSchema(list_of_vars(&v[0]), convert_tuples(tups))
        }
        "lighttable" => {
            let tups = instance
                .tuples
                .as_ref()
                .ok_or_else(|| anyhow!("lighttable instance has no tuple data"))?;
            MCon::LightTable(list_of_vars(&v[0]), convert_tuples(tups))
        }
        "mddc" => {
            let tups = instance
                .tuples
                .as_ref()
                .ok_or_else(|| anyhow!("mddc instance has no tuple data"))?;
            MCon::Mddc(list_of_vars(&v[0]), convert_tuples(tups))
        }
        "negativemddc" => {
            let tups = instance
                .tuples
                .as_ref()
                .ok_or_else(|| anyhow!("negativemddc instance has no tuple data"))?;
            MCon::NegativeMddc(list_of_vars(&v[0]), convert_tuples(tups))
        }

        // Short-tuple constraints. All four read `**SHORTTUPLELIST**`
        // data and have the same declarative semantics; the tester
        // generates the same data shape for all of them.
        "shortstr2" => {
            let st = instance
                .short_tuples
                .as_ref()
                .ok_or_else(|| anyhow!("shortstr2 instance has no short-tuple data"))?;
            MCon::ShortStr2(list_of_vars(&v[0]), convert_short_tuples(st))
        }
        "haggisgac" => {
            let st = instance
                .short_tuples
                .as_ref()
                .ok_or_else(|| anyhow!("haggisgac instance has no short-tuple data"))?;
            MCon::HaggisGac(list_of_vars(&v[0]), convert_short_tuples(st))
        }
        "haggisgac-stable" => {
            let st = instance
                .short_tuples
                .as_ref()
                .ok_or_else(|| anyhow!("haggisgac-stable instance has no short-tuple data"))?;
            MCon::HaggisGacStable(list_of_vars(&v[0]), convert_short_tuples(st))
        }
        "shortctuplestr2" => {
            let st = instance
                .short_tuples
                .as_ref()
                .ok_or_else(|| anyhow!("shortctuplestr2 instance has no short-tuple data"))?;
            MCon::ShortCTupleStr2(list_of_vars(&v[0]), convert_short_tuples(st))
        }

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
            // A reused variable appears in several slots; declare it
            // (and its domain holes) only once. Reuse always shares the
            // same Arc, so a name already present carries an identical
            // domain — no conflict to worry about.
            if model
                .named_variables
                .get_vartype(v.name.clone())
                .is_some()
            {
                continue;
            }
            let tableised = instance.constraint.name == "str2plus";
            let domain = match v.var_type {
                VarType::Bool => VarDomain::Bool,
                VarType::Bound => VarDomain::Bound(
                    *v.domain.first().unwrap() as i32,
                    *v.domain.last().unwrap() as i32,
                ),
                VarType::SparseBound => {
                    if tableised {
                        VarDomain::Discrete(
                            *v.domain.first().unwrap() as i32,
                            *v.domain.last().unwrap() as i32,
                        )
                    } else {
                        VarDomain::SparseBound(v.domain.iter().map(|&n| n as i32).collect())
                    }
                }
                VarType::Discrete => VarDomain::Discrete(
                    *v.domain.first().unwrap() as i32,
                    *v.domain.last().unwrap() as i32,
                ),
                VarType::Constant => unreachable!(),
            };
            if model
                .named_variables
                .add_var(v.name.clone(), domain)
                .is_none()
            {
                bail!("duplicate variable name in instance: {}", v.name);
            }

            if v.var_type == VarType::SparseBound && tableised {
                model.constraints.push(MCon::WInset(
                    Var::NameRef(v.name.clone()),
                    v.domain.iter().map(|&n| MC::Integer(n as i32)).collect(),
                ));
            } else if v.var_type != VarType::Bound && v.var_type != VarType::SparseBound {
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
    options: minion_sys::RunOptions,
    max_solutions: Option<usize>,
    testname: &str,
) -> Result<CappedOutput> {
    let model = build_multi_model(with_constraints, vars_only)?;
    let variable_order = model.named_variables.get_variable_order();

    if std::env::var("TESTER_DEBUG").is_ok() {
        eprintln!(
            "--- multi-model for {testname} (seed={:?}) ---",
            options.seed
        );
        eprintln!("variable order (col: name = domain):");
        for (i, name) in variable_order.iter().enumerate() {
            eprintln!(
                "  col{i}: {name} = {:?}",
                model.named_variables.get_vartype(name.clone())
            );
        }
        eprintln!("constraints:");
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

    let ctx = minion_sys::run_minion_with_options(model, options, callback)
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
    options: minion_sys::RunOptions,
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
        eprintln!(
            "--- multi-inject model for {testname} (seed={:?}) ---",
            options.seed
        );
        eprintln!("variable order (col: name = domain):");
        for (i, name) in initial_variable_order.iter().enumerate() {
            eprintln!(
                "  col{i}: {name} = {:?}",
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
                InjectionPacket::AddVarsAndConstraint {
                    new_vars,
                    constraint,
                } => {
                    let ns: Vec<&str> = new_vars.iter().map(|(n, _)| n.as_str()).collect();
                    eprintln!("  inject at {cnt}: add_vars={ns:?} + {constraint:?}");
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
            InjectionPacket::AddVarsAndConstraint { constraint, .. } => Ok(constraint.clone()),
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
                            *cb_err_ref =
                                Some(format!("callback: var {name} has no value ({other:?})"));
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
                        InjectionPacket::AddVarsAndConstraint { new_vars, .. } => {
                            for (name, domain) in new_vars {
                                if let Err(e) = midctx.add_var(name, domain.clone()) {
                                    *cb_err_ref = Some(format!("add_var({name}): {e}"));
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

    let ctx = minion_sys::run_minion_midsearch_with_options(model, options, callback)
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
    options: minion_sys::RunOptions,
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
        Box::new(
            move |midctx: &mut minion_sys::MidSearchContext<'_>,
                  sol: HashMap<VarName, MC>|
                  -> bool {
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
                            if let Err(e) = midctx.add_var(name, domain.clone()) {
                                *cb_err_ref = Some(format!("add_var({name}): {e}"));
                                return false;
                            }
                        }
                        *injected_ref = true;
                    }
                }
                true
            },
        )
    };

    let ctx = minion_sys::run_minion_midsearch_with_options(model, options, callback)
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
/// In-process work-stealing variant: builds the same model as
/// [`get_minion_solutions_in_process`] but routes through
/// `minion_sys::run_minion_work_steal_with_options`. The work-steal
/// entry doesn't return a `SolverContext`, so we don't have Nodes from
/// the FFI side — we report 0, which is fine for the tester because
/// `test_constraint_workstal` doesn't compare node counts (work-steal
/// changes traversal so node-count equality isn't a sound invariant).
pub fn get_minion_solutions_in_process_work_steal(
    instance: &ConstraintInstance,
    options: minion_sys::RunOptions,
    num_threads: usize,
    testname: &str,
    max_solutions: i64,
    keep_full_solutions: bool,
) -> Result<MinionOutput> {
    let mut model = Model::new();
    register_tuple_tables_for_instance(&mut model, instance)?;
    add_variables_and_holes(&mut model, instance)?;
    let top = build_constraint(instance)
        .with_context(|| format!("building constraint for {testname}"))?;
    model.constraints.push(top);

    let variable_order = model.named_variables.get_variable_order();

    // The work-steal callback is invoked from any worker thread (the
    // C-side controller mutex serialises calls so no two run
    // concurrently, but the worker that calls in varies). The shared
    // state is a (digest, optional Vec) pair under one mutex. WE ARE
    // HASHING SOLUTIONS, NOT STORING THEM by default — see
    // solution_digest.rs for the rationale and collision analysis.
    let shared: std::sync::Mutex<(SolutionDigest, Option<Vec<Vec<i64>>>)> =
        std::sync::Mutex::new((
            SolutionDigest::new(),
            if keep_full_solutions { Some(Vec::new()) } else { None },
        ));

    let cb: minion_sys::ParallelCallback<'_> = {
        let variable_order = &variable_order;
        let shared = &shared;
        Box::new(move |sol_map: HashMap<VarName, MC>| -> bool {
            let mut row = Vec::with_capacity(variable_order.len());
            for name in variable_order.iter() {
                match sol_map.get(name).copied() {
                    Some(MC::Integer(n)) => row.push(n as i64),
                    Some(MC::Bool(b)) => row.push(if b { 1 } else { 0 }),
                    Some(_) | None => return false,
                }
            }
            #[allow(clippy::unwrap_used)]
            let mut s = shared.lock().unwrap();
            if let Some(v) = s.1.as_mut() {
                v.push(row.clone());
            }
            s.0.add(row);
            // Stop when the cap is reached. Returning false signals
            // the controller to broadcast stop to the other workers.
            if max_solutions > 0 && s.0.count as i64 >= max_solutions {
                return false;
            }
            true
        })
    };

    let stats = minion_sys::run_minion_work_steal_with_options(num_threads, model, options, cb)
        .map_err(|e| anyhow!("minion in-process work-steal error ({testname}): {e}"))?;

    #[allow(clippy::unwrap_used)]
    let (digest, raw_solutions) = shared.into_inner().unwrap();

    let hit_solution_cap = max_solutions > 0 && digest.count as i64 >= max_solutions;

    Ok(MinionOutput {
        solutions: digest,
        raw_solutions,
        nodes: stats.total_nodes,
        filename: format!("<in-process-work-steal:{testname}>"),
        cleanup: CleanupFiles::empty(),
        work_steal_donations: Some(stats.donations),
        // In-process backend doesn't currently surface the parallel-
        // preprocess counters (they're only emitted in the exec
        // backend's TableOut). Work-steal is mutex with -X-parallel
        // Preprocess so this combination never fires anyway.
        parallel_preprocess_rounds: None,
        parallel_preprocess_prunings: None,
        hit_solution_cap,
        // In-process backend doesn't yet expose optimisation through
        // the FFI — the optimisation sweep is exec-only for now.
        optimum_value: None,
        rejected: false,
    })
}

pub fn get_minion_solutions_in_process(
    instance: &ConstraintInstance,
    find_all_sols: bool,
    options: minion_sys::RunOptions,
    testname: &str,
    max_solutions: i64,
    keep_full_solutions: bool,
) -> Result<MinionOutput> {
    let mut model = Model::new();
    register_tuple_tables_for_instance(&mut model, instance)?;
    add_variables_and_holes(&mut model, instance)?;
    let top = build_constraint(instance)
        .with_context(|| format!("building constraint for {testname}"))?;
    model.constraints.push(top);

    let variable_order = model.named_variables.get_variable_order();

    // WE ARE HASHING SOLUTIONS, NOT STORING THEM by default —
    // `keep_full_solutions` opts callers (mid-search tests) into Vec
    // storage when they need indexed access. See solution_digest.rs.
    let mut digest = SolutionDigest::new();
    let mut raw_solutions: Option<Vec<Vec<i64>>> =
        if keep_full_solutions { Some(Vec::new()) } else { None };

    let callback: minion_sys::Callback<'_> = {
        let variable_order = &variable_order;
        let digest = &mut digest;
        let raw_solutions = &mut raw_solutions;
        Box::new(move |sol_map: HashMap<VarName, MC>| -> bool {
            let mut row = Vec::with_capacity(variable_order.len());
            for name in variable_order.iter() {
                match sol_map.get(name).copied() {
                    Some(MC::Integer(n)) => row.push(n as i64),
                    Some(MC::Bool(b)) => row.push(if b { 1 } else { 0 }),
                    Some(_) | None => return false,
                }
            }
            if let Some(v) = raw_solutions.as_mut() {
                v.push(row.clone());
            }
            digest.add(row);
            // Stop when the per-trial cap is reached (mirrors exec
            // mode's -sollimit). Caller checks `hit_solution_cap` and
            // skips comparison when the cap was hit.
            if max_solutions > 0 && digest.count as i64 >= max_solutions {
                return false;
            }
            find_all_sols
        })
    };

    if std::env::var("TESTER_DEBUG").is_ok() {
        eprintln!("--- in-process model for {testname} ---");
        eprintln!("variable order (col: name = domain):");
        for (i, name) in model
            .named_variables
            .get_variable_order()
            .iter()
            .enumerate()
        {
            eprintln!(
                "  col{i}: {name} = {:?}",
                model.named_variables.get_vartype(name.clone())
            );
        }
        eprintln!("constraints:");
        for c in &model.constraints {
            eprintln!("  {c:?}");
        }
        eprintln!("--- end model ---");
    }

    let ctx = minion_sys::run_minion_with_options(model, options, callback)
        .map_err(|e| anyhow!("minion in-process error ({testname}): {e}"))?;

    let nodes_str = ctx
        .get_from_table("Nodes".to_string())
        .ok_or_else(|| anyhow!("Nodes key missing from minion stats"))?;
    let nodes: i64 = nodes_str
        .parse()
        .with_context(|| format!("parsing Nodes={nodes_str:?}"))?;

    let hit_solution_cap = max_solutions > 0 && digest.count as i64 >= max_solutions;

    Ok(MinionOutput {
        solutions: digest,
        raw_solutions,
        nodes,
        filename: format!("<in-process:{testname}>"),
        cleanup: CleanupFiles::empty(),
        work_steal_donations: None,
        // In-process backend doesn't currently surface the parallel-
        // preprocess counters; the parallel-preprocess sweep is
        // exec-only.
        parallel_preprocess_rounds: None,
        parallel_preprocess_prunings: None,
        hit_solution_cap,
        // The non-optimisation entry point never sets an optimisation
        // directive, so the FFI never reports OptimumValue.
        optimum_value: None,
        rejected: false,
    })
}

/// In-process optimisation runner. Mirrors
/// [`get_minion_solutions_in_process`] but augments the model with an
/// auxiliary objective variable equal to the sum of the named real
/// vars, plus a `MINIMISING`/`MAXIMISING` directive on it. Reads
/// `OptimumValue` out of TableOut after solve.
///
/// Exec mode does the equivalent textually via
/// [`crate::minion_instance::print_minion_file_pair_optimisation`].
/// This function lets `--optimisation-sweep` participate under
/// `--in-process` once the FFI plumbing for optimisation lands.
pub fn get_minion_solutions_in_process_optimisation(
    instance: &ConstraintInstance,
    options: minion_sys::RunOptions,
    testname: &str,
    optimisation: &OptimisationWrapper,
) -> Result<MinionOutput> {
    let mut model = Model::new();
    register_tuple_tables_for_instance(&mut model, instance)?;
    add_variables_and_holes(&mut model, instance)?;

    // Aux objective variable as a Bound var spanning the sum range.
    model
        .named_variables
        .add_var(
            optimisation.aux_name.to_string(),
            VarDomain::Bound(optimisation.aux_min as i32, optimisation.aux_max as i32),
        )
        .ok_or_else(|| anyhow!("aux variable name {:?} already in use", optimisation.aux_name))?;

    let top = build_constraint(instance)
        .with_context(|| format!("building constraint for {testname}"))?;
    model.constraints.push(top);

    // aux = sum(real_vars) — encoded as a sumleq + sumgeq pair (minion
    // doesn't have a native sum-equality constraint, and tying through
    // a tabulated relation would be overkill at the sizes the tester
    // generates).
    let sum_vars: Vec<Var> = optimisation
        .sum_var_names
        .iter()
        .map(|n| Var::NameRef(n.clone()))
        .collect();
    let aux_var = Var::NameRef(optimisation.aux_name.to_string());
    model
        .constraints
        .push(MCon::SumLeq(sum_vars.clone(), aux_var.clone()));
    model
        .constraints
        .push(MCon::SumGeq(sum_vars, aux_var.clone()));

    model.optimise = Some(Optimise {
        minimise: optimisation.minimise,
        var: aux_var,
    });

    let variable_order = model.named_variables.get_variable_order();

    // Optimisation runs don't need solution storage — the metamorphic
    // sweep only compares OptimumValue. Still install a callback to
    // count nodes and let minion's findAllSolutions logic drive
    // search through the bound-tightening loop.
    let mut digest = SolutionDigest::new();

    let callback: minion_sys::Callback<'_> = {
        let variable_order = &variable_order;
        let digest = &mut digest;
        Box::new(move |sol_map: HashMap<VarName, MC>| -> bool {
            // Touch each var to keep parity with the satisfaction
            // path's invariant ("every search var appears in the
            // print matrix on a solution").
            let mut row = Vec::with_capacity(variable_order.len());
            for name in variable_order.iter() {
                match sol_map.get(name).copied() {
                    Some(MC::Integer(n)) => row.push(n as i64),
                    Some(MC::Bool(b)) => row.push(if b { 1 } else { 0 }),
                    Some(_) | None => return false,
                }
            }
            digest.add(row);
            true
        })
    };

    let ctx = minion_sys::run_minion_with_options(model, options, callback)
        .map_err(|e| anyhow!("minion in-process error ({testname}): {e}"))?;

    let nodes_str = ctx
        .get_from_table("Nodes".to_string())
        .ok_or_else(|| anyhow!("Nodes key missing from minion stats"))?;
    let nodes: i64 = nodes_str
        .parse()
        .with_context(|| format!("parsing Nodes={nodes_str:?}"))?;

    // Parse OptimumValue if present (absent under UNSAT).
    let optimum_value = ctx
        .get_from_table("OptimumValue".to_string())
        .and_then(|s| s.parse::<i64>().ok());

    Ok(MinionOutput {
        solutions: digest,
        raw_solutions: None,
        nodes,
        filename: format!("<in-process-opt:{testname}>"),
        cleanup: CleanupFiles::empty(),
        work_steal_donations: None,
        parallel_preprocess_rounds: None,
        parallel_preprocess_prunings: None,
        hit_solution_cap: false,
        optimum_value,
        rejected: false,
    })
}
