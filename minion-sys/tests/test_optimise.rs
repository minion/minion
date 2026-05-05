//! Single-objective optimisation via the FFI.
//!
//! Sets [`Model::optimise`] to MINIMISING / MAXIMISING on a chosen
//! variable, runs minion, and reads `OptimumValue` back from
//! `TableOut`. A reference all-solutions run computes the expected
//! best directly from the solution set and the directions must agree.

use std::collections::HashMap;

use minion_sys::ast::{Constant, Constraint, Model, Optimise, Var, VarDomain, VarName};
use minion_sys::{run_minion_with_options, RunOptions};

/// Three integer vars in 1..=3 with a sum cap. Leaves enough slack
/// that minimising or maximising the sum is non-trivial.
fn build_model() -> Model {
    let mut m = Model::new();
    m.named_variables.add_var("x".into(), VarDomain::Discrete(1, 3));
    m.named_variables.add_var("y".into(), VarDomain::Discrete(1, 3));
    m.named_variables.add_var("z".into(), VarDomain::Discrete(1, 3));
    m.named_variables.add_var("s".into(), VarDomain::Bound(3, 9));

    let xyz = vec![
        Var::NameRef("x".into()),
        Var::NameRef("y".into()),
        Var::NameRef("z".into()),
    ];
    // s = sum(x,y,z), enforced by a sumleq + sumgeq pair.
    m.constraints
        .push(Constraint::SumLeq(xyz.clone(), Var::NameRef("s".into())));
    m.constraints
        .push(Constraint::SumGeq(xyz, Var::NameRef("s".into())));
    m
}

fn baseline_solutions() -> Vec<i32> {
    let mut model = build_model();
    model.optimise = None;
    let order = vec!["s".to_string()];
    let mut s_values: Vec<i32> = Vec::new();
    let callback: minion_sys::Callback<'_> = {
        let s_values = &mut s_values;
        let order = &order;
        Box::new(move |sol: HashMap<VarName, Constant>| -> bool {
            for name in order.iter() {
                if let Some(Constant::Integer(v)) = sol.get(name) {
                    s_values.push(*v);
                }
            }
            true
        })
    };
    let _ = run_minion_with_options(model, RunOptions::default(), callback)
        .expect("baseline solve");
    s_values
}

fn run_opt(minimise: bool) -> Option<i64> {
    let mut model = build_model();
    model.optimise = Some(Optimise {
        minimise,
        var: Var::NameRef("s".into()),
    });
    let callback: minion_sys::Callback<'_> = Box::new(|_: HashMap<VarName, Constant>| true);
    let ctx = run_minion_with_options(model, RunOptions::default(), callback)
        .expect("optimisation solve");
    ctx.get_from_table("OptimumValue".to_string())
        .and_then(|s| s.parse::<i64>().ok())
}

#[test]
fn minimising_finds_smallest_objective() {
    let sols = baseline_solutions();
    let expected = *sols.iter().min().expect("instance has solutions");
    let got = run_opt(/*minimise=*/ true).expect("OptimumValue missing");
    assert_eq!(got as i32, expected, "MINIMISING reported wrong optimum");
}

#[test]
fn maximising_finds_largest_objective() {
    let sols = baseline_solutions();
    let expected = *sols.iter().max().expect("instance has solutions");
    let got = run_opt(/*minimise=*/ false).expect("OptimumValue missing");
    assert_eq!(got as i32, expected, "MAXIMISING reported wrong optimum");
}

#[test]
fn unsat_optimisation_reports_no_optimum() {
    let mut model = build_model();
    // Force UNSAT by demanding s outside its feasible 3..=9 range.
    let xyz = vec![
        Var::NameRef("x".into()),
        Var::NameRef("y".into()),
        Var::NameRef("z".into()),
    ];
    // sum >= 100 contradicts sum<=9 from the existing sumleq.
    model
        .constraints
        .push(Constraint::SumGeq(xyz, Var::ConstantAsVar(100)));
    model.optimise = Some(Optimise {
        minimise: false,
        var: Var::NameRef("s".into()),
    });
    let callback: minion_sys::Callback<'_> = Box::new(|_: HashMap<VarName, Constant>| true);
    let ctx = run_minion_with_options(model, RunOptions::default(), callback)
        .expect("UNSAT solve runs cleanly");
    assert!(
        ctx.get_from_table("OptimumValue".to_string()).is_none(),
        "OptimumValue must be absent when there are no solutions"
    );
    let sols = ctx
        .get_from_table("SolutionsFound".to_string())
        .expect("SolutionsFound key");
    assert_eq!(sols, "0", "SolutionsFound must be 0 on UNSAT");
}
