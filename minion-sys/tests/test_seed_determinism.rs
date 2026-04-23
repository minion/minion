//! Same seed must produce bit-identical solution sequences (including order).
//!
//! Without this, the mid-search injection tests (which diff two solves
//! sharing a prefix) can't distinguish "search diverged because we
//! injected" from "search diverged because the RNG initialised differently".

use std::collections::HashMap;

use minion_sys::ast::{Constant, Constraint, Model, Var, VarDomain, VarName};
use minion_sys::{RunOptions, run_minion_with_options};

fn build_small_model() -> Model {
    // Three sum-to-4 vars with a tiny extra constraint so the search tree
    // has a non-trivial number of leaves.
    let mut m = Model::new();
    m.named_variables
        .add_var("x".into(), VarDomain::Discrete(1, 3));
    m.named_variables
        .add_var("y".into(), VarDomain::Discrete(1, 3));
    m.named_variables
        .add_var("z".into(), VarDomain::Discrete(1, 3));

    let vars = vec![
        Var::NameRef("x".into()),
        Var::NameRef("y".into()),
        Var::NameRef("z".into()),
    ];

    m.constraints
        .push(Constraint::SumLeq(vars.clone(), Var::ConstantAsVar(6)));
    m.constraints
        .push(Constraint::SumGeq(vars, Var::ConstantAsVar(5)));
    m
}

fn collect_solutions(seed: u32) -> Vec<Vec<i32>> {
    let order = vec!["x".to_string(), "y".to_string(), "z".to_string()];
    let mut solutions: Vec<Vec<i32>> = Vec::new();
    let callback: minion_sys::Callback<'_> = {
        let order = &order;
        let solutions = &mut solutions;
        Box::new(move |sol: HashMap<VarName, Constant>| -> bool {
            let mut row = Vec::with_capacity(order.len());
            for name in order.iter() {
                match sol.get(name) {
                    Some(Constant::Integer(v)) => row.push(*v),
                    Some(Constant::Bool(b)) => row.push(if *b { 1 } else { 0 }),
                    _ => return false,
                }
            }
            solutions.push(row);
            true
        })
    };
    let _ctx = run_minion_with_options(
        build_small_model(),
        RunOptions {
            seed: Some(seed),
            ..Default::default()
        },
        callback,
    )
    .expect("solve");
    solutions
}

#[test]
fn same_seed_same_solution_sequence() {
    let a = collect_solutions(0xC0FFEE);
    let b = collect_solutions(0xC0FFEE);
    assert_eq!(a, b, "same seed must give identical solution order");
    assert!(!a.is_empty(), "expected at least one solution");
}

#[test]
fn different_seed_ok_with_static_order() {
    // Static var order + ascend val order is deterministic regardless
    // of seed, so these should still be equal — but if minion ever
    // starts reaching for the RNG here, this test will catch it.
    let a = collect_solutions(1);
    let b = collect_solutions(999_999);
    assert_eq!(
        a, b,
        "static/ascend default should not consult the seed"
    );
}
