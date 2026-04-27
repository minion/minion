//! Error-path tests for minion-sys.

use minion_sys::ast::{Constant, Constraint, Model, Var, VarDomain};
use minion_sys::error::{MinionError, RuntimeError};

#[test]
fn duplicate_variable_name_rejected() {
    let mut m = Model::new();
    assert!(m.named_variables.add_var("x".into(), VarDomain::Bool).is_some());
    // Second add with same name should return None.
    assert!(m.named_variables.add_var("x".into(), VarDomain::Bool).is_none());
}

#[test]
fn empty_model_finds_one_empty_solution() {
    let m = Model::new();
    let mut count = 0u32;
    let ctx = minion_sys::run_minion(
        m,
        Box::new(|_| {
            count += 1;
            true
        }),
    );
    assert!(ctx.is_ok(), "empty model should solve");
    assert_eq!(count, 1); // Minion reports one empty assignment.
}

#[test]
fn constraint_with_missing_var_fails() {
    let mut m = Model::new();
    m.named_variables.add_var("x".into(), VarDomain::Bool);
    // Reference "y" which was never added.
    m.constraints.push(Constraint::Eq(
        Var::NameRef("x".into()),
        Var::NameRef("y".into()),
    ));
    let result = minion_sys::run_minion(m, Box::new(|_| true));
    match result {
        Err(e) => assert!(
            matches!(e, MinionError::RuntimeError(RuntimeError::ParseError(_))
                  | MinionError::RuntimeError(RuntimeError::InvalidInstance(_))),
            "expected ParseError or InvalidInstance, got {e:?}"
        ),
        Ok(_) => panic!("expected error for missing var, got success"),
    }
}

#[test]
fn constraint_with_all_constants_solves() {
    let mut m = Model::new();
    m.named_variables.add_var("x".into(), VarDomain::Bool);
    // Eq with one var and one constant should work.
    m.constraints.push(Constraint::Eq(
        Var::NameRef("x".into()),
        Var::ConstantAsVar(1),
    ));
    let mut count = 0u32;
    let ctx = minion_sys::run_minion(
        m,
        Box::new(|_| {
            count += 1;
            true
        }),
    );
    assert!(ctx.is_ok(), "solve failed: {:?}", ctx.is_err());
    assert_eq!(count, 1); // x=1 only
}

#[test]
fn unsat_model_finds_zero_solutions() {
    let mut m = Model::new();
    m.named_variables.add_var("x".into(), VarDomain::Bool);
    m.constraints.push(Constraint::False);
    let mut count = 0u32;
    let ctx = minion_sys::run_minion(
        m,
        Box::new(|_| {
            count += 1;
            true
        }),
    );
    assert!(ctx.is_ok(), "unsat model should solve: {:?}", ctx.is_err());
    assert_eq!(count, 0);
}

#[test]
fn true_constraint_enumerates_all() {
    let mut m = Model::new();
    m.named_variables
        .add_var("x".into(), VarDomain::Discrete(0, 2));
    m.named_variables
        .add_var("y".into(), VarDomain::Discrete(5, 6));
    m.constraints.push(Constraint::True);
    let mut count = 0u32;
    let ctx = minion_sys::run_minion(
        m,
        Box::new(|_| {
            count += 1;
            true
        }),
    );
    assert!(ctx.is_ok(), "solve failed: {:?}", ctx.is_err());
    assert_eq!(count, 6); // 3 × 2 = 6
}

#[test]
fn running_two_models_in_sequence_works() {
    for _ in 0..5 {
        let mut m = Model::new();
        m.named_variables.add_var("a".into(), VarDomain::Bool);
        m.constraints.push(Constraint::True);
        let ctx = minion_sys::run_minion(m, Box::new(|_| true));
        assert!(ctx.is_ok());
    }
}

#[test]
fn seed_reproducibility_across_models() {
    use minion_sys::{RunOptions, run_minion_with_options};
    use std::collections::HashMap;
    let opts = RunOptions {
        seed: Some(42),
        ..Default::default()
    };
    let build = || {
        let mut m = Model::new();
        m.named_variables
            .add_var("x".into(), VarDomain::Discrete(1, 3));
        m.named_variables
            .add_var("y".into(), VarDomain::Discrete(1, 3));
        m.constraints
            .push(Constraint::SumGeq(vec![Var::NameRef("x".into()), Var::NameRef("y".into())], Var::ConstantAsVar(4)));
        m
    };
    let run = |m: Model| {
        let sols = std::cell::RefCell::new(Vec::new());
        let _ = run_minion_with_options(
            m,
            opts,
            Box::new(|sol: HashMap<String, Constant>| {
                sols.borrow_mut().push(vec![
                    match sol["x"] { Constant::Integer(n) => n, _ => 0 },
                    match sol["y"] { Constant::Integer(n) => n, _ => 0 },
                ]);
                true
            }),
        );
        sols.into_inner()
    };
    let a = run(build());
    let b = run(build());
    assert_eq!(a, b, "same seed across models should give same solutions");
}

#[test]
fn w_inset_with_empty_set_is_false() {
    let mut m = Model::new();
    m.named_variables.add_var("x".into(), VarDomain::Bound(0, 5));
    m.constraints
        .push(Constraint::WInset(Var::NameRef("x".into()), vec![]));
    let mut count = 0u32;
    let ctx = minion_sys::run_minion(
        m,
        Box::new(|_| {
            count += 1;
            true
        }),
    );
    assert!(ctx.is_ok(), "solve failed: {:?}", ctx.is_err());
    assert_eq!(count, 0);
}
