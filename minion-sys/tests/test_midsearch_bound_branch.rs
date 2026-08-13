//! Regression: backtracking past a mid-search-injected constraint used
//! to kill the process outright when the branch variable was a BOUND
//! (or SPARSEBOUND) variable.
//!
//! `branch_right` turns the left branch `var = val` into `var != val`.
//! It special-cases the bounds so bound variables work — `setMin(val+1)`
//! if `val` is the min, `setMax(val-1)` if it's the max — and otherwise
//! calls `removeFromDomain`. A constraint injected mid-search and
//! re-propagated by the worldPop can prune `val` before we get here, so
//! `val` is neither bound and control reached `removeFromDomain`, which
//! on a bound variable is not a no-op but a hard `USER_ERROR` — and
//! `USER_ERROR` calls `exit(1)`, taking the host process with it.
//!
//! Own test binary because the old failure was a process exit, not a
//! panic: it would have taken any test sharing the process with it.

use std::collections::HashMap;

use minion_sys::ast::{Constant, Constraint, Model, Var, VarDomain, VarName};
use minion_sys::{RunOptions, run_minion_midsearch_with_options};

#[test]
fn injected_prune_of_the_branch_value_on_a_bound_var() {
    // `a` is branched first and is a BOUND variable, so it cannot
    // represent a hole; `b` gives the search something to backtrack
    // over. Ascending value order tries a = 0 first, which is exactly
    // the value the injected constraint forbids.
    let mut model = Model::new();
    model
        .named_variables
        .add_var("a".to_string(), VarDomain::Bound(0, 3));
    model
        .named_variables
        .add_var("b".to_string(), VarDomain::Discrete(0, 3));
    let order = ["a".to_string(), "b".to_string()];

    let mut rows: Vec<Vec<i32>> = Vec::new();
    let mut injected_after: Option<usize> = None;

    {
        let order = &order;
        let rows = &mut rows;
        let injected_after = &mut injected_after;
        let callback: minion_sys::MidSearchCallback<'_> =
            Box::new(move |ctx, sol: HashMap<VarName, Constant>| -> bool {
                let mut row = Vec::with_capacity(order.len());
                for name in order.iter() {
                    match sol.get(name) {
                        Some(Constant::Integer(v)) => row.push(*v),
                        Some(Constant::Bool(bo)) => row.push(if *bo { 1 } else { 0 }),
                        _ => return false,
                    }
                }
                rows.push(row);

                if injected_after.is_none() {
                    ctx.add_constraint(Constraint::WNotLiteral(
                        Var::NameRef("a".to_string()),
                        Constant::Integer(0),
                    ))
                    .expect("inject w-notliteral mid-search");
                    *injected_after = Some(rows.len());
                }
                true
            });

        run_minion_midsearch_with_options(model, RunOptions::default(), callback).expect("solve");
    }

    let injected_after = injected_after.expect("expected at least one solution before injecting");
    assert_eq!(rows[0][0], 0, "expected the first branch to assign a = 0");
    assert!(
        rows.len() > injected_after,
        "no solutions after the injection: search never backtracked past it"
    );
    for row in &rows[injected_after..] {
        assert_ne!(
            row[0], 0,
            "solution {row:?} found after forbidding a = 0 still has a = 0"
        );
    }
    // a ∈ {1,2,3} × b ∈ {0..3} = 12, plus the solutions with a = 0 that
    // were reported before the constraint existed.
    assert!(
        rows.len() >= 13,
        "expected the search to complete after the injection, got {} solutions",
        rows.len()
    );
}
