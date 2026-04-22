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
        "forwardchecking" => MCon::ForwardChecking(child(0)?),
        "check[gsa]" => MCon::CheckGsa(child(0)?),
        "check[assign]" => MCon::CheckAssign(child(0)?),

        // Tableised instance. The tester names this "str2plus" in the text
        // encoding (minion's CT_STR, which takes a tuple-table symbol by
        // name). The library Model AST has no way to register named tuple
        // tables yet, and CT_WATCHED_TABLE / CT_GACSCHEMA / CT_LIGHTTABLE all
        // reject BOUND / SPARSEBOUND variables. CT_MDDC accepts any domain
        // type, takes inline tuples, and is GAC — so node counts still match
        // the original-constraint run for GAC propagators.
        "str2plus" => {
            let tups = instance
                .tuples
                .as_ref()
                .ok_or_else(|| anyhow!("str2plus instance has no tuple table"))?;
            // MDDC chokes on both degenerate corners of the tuple table:
            //   - empty tuple list (no tuples): no tt nodes are ever made,
            //     the top node's type=0, propagation segfaults later.
            //   - zero-arity tuples (vars list empty): the inner build loop
            //     does nothing, assertion `curnode->type == -1` fails.
            // The semantics are trivial in both cases, so short-circuit.
            if tups.tupledata.is_empty() {
                MCon::False
            } else if v[0].is_empty() {
                MCon::True
            } else {
                let tuple_vecs: Vec<Vec<MC>> = tups
                    .tupledata
                    .iter()
                    .map(|row| row.iter().map(|n| MC::Integer(*n as i32)).collect())
                    .collect();
                MCon::Mddc(list_of_vars(&v[0]), tuple_vecs)
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
