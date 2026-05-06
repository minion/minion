// End-to-end FFI coverage for the four short-tuple constraints
// (`shortstr2`, `haggisgac`, `haggisgac-stable`, `shortctuplestr2`).
// All four read the same `**SHORTTUPLELIST**` data and have the
// same declarative semantics (an assignment satisfies the
// constraint iff some short tuple's literals match), so we run
// each over the same instance and check they all agree on the
// solution count.

use minion_sys::ast::{Constant, Constraint, Model, ShortTuple, Var, VarDomain};
use minion_sys::error::MinionError;

fn build_model_with(constraint_kind: &str) -> Model {
    let mut model = Model::new();
    model
        .named_variables
        .add_var("x".to_string(), VarDomain::Discrete(0, 2));
    model
        .named_variables
        .add_var("y".to_string(), VarDomain::Discrete(0, 2));
    model
        .named_variables
        .add_var("z".to_string(), VarDomain::Discrete(0, 2));

    // Three short tuples:
    //   [(0,0),(2,0)]  — x=0 ∧ z=0   (y is free, 3 assignments)
    //   [(0,1),(1,1)]  — x=1 ∧ y=1   (z is free, 3 assignments)
    //   [(0,2),(1,2),(2,2)] — x=y=z=2 (1 assignment)
    let short_tuples: Vec<ShortTuple> = vec![
        vec![(0, Constant::Integer(0)), (2, Constant::Integer(0))],
        vec![(0, Constant::Integer(1)), (1, Constant::Integer(1))],
        vec![
            (0, Constant::Integer(2)),
            (1, Constant::Integer(2)),
            (2, Constant::Integer(2)),
        ],
    ];

    let vars = vec![
        Var::NameRef("x".to_string()),
        Var::NameRef("y".to_string()),
        Var::NameRef("z".to_string()),
    ];

    let c = match constraint_kind {
        "shortstr2" => Constraint::ShortStr2(vars, short_tuples),
        "haggisgac" => Constraint::HaggisGac(vars, short_tuples),
        "haggisgac-stable" => Constraint::HaggisGacStable(vars, short_tuples),
        "shortctuplestr2" => Constraint::ShortCTupleStr2(vars, short_tuples),
        other => panic!("unknown constraint kind: {other}"),
    };
    model.constraints.push(c);
    model
}

fn count_solutions(model: Model) -> Result<u32, MinionError> {
    let mut count = 0u32;
    minion_sys::run_minion(
        model,
        Box::new(|_| {
            count += 1;
            true
        }),
    )?;
    Ok(count)
}

#[test]
#[allow(clippy::panic_in_result_fn)]
fn shortstr2_solution_count() -> Result<(), MinionError> {
    // 3 (y free) + 3 (z free) - 0 (no overlap, since the first
    // short tuple needs x=0 but the second needs x=1) + 1 = 7.
    let n = count_solutions(build_model_with("shortstr2"))?;
    assert_eq!(n, 7, "shortstr2 expected 7 solutions, got {n}");
    Ok(())
}

#[test]
#[allow(clippy::panic_in_result_fn)]
fn haggisgac_solution_count() -> Result<(), MinionError> {
    let n = count_solutions(build_model_with("haggisgac"))?;
    assert_eq!(n, 7, "haggisgac expected 7 solutions, got {n}");
    Ok(())
}

#[test]
#[allow(clippy::panic_in_result_fn)]
fn haggisgac_stable_solution_count() -> Result<(), MinionError> {
    let n = count_solutions(build_model_with("haggisgac-stable"))?;
    assert_eq!(n, 7, "haggisgac-stable expected 7 solutions, got {n}");
    Ok(())
}

#[test]
#[allow(clippy::panic_in_result_fn)]
fn shortctuplestr2_solution_count() -> Result<(), MinionError> {
    let n = count_solutions(build_model_with("shortctuplestr2"))?;
    assert_eq!(n, 7, "shortctuplestr2 expected 7 solutions, got {n}");
    Ok(())
}

#[test]
#[allow(clippy::panic_in_result_fn)]
fn empty_short_tuple_list_is_unsat() -> Result<(), MinionError> {
    // An empty short-tuple list = constraint false everywhere.
    let mut model = Model::new();
    model
        .named_variables
        .add_var("x".to_string(), VarDomain::Discrete(0, 2));
    model.constraints.push(Constraint::ShortStr2(
        vec![Var::NameRef("x".to_string())],
        vec![],
    ));
    let n = count_solutions(model)?;
    assert_eq!(n, 0);
    Ok(())
}
