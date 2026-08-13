//! Regression: a propagator with backtrackable internal state, injected
//! mid-search, used to corrupt memory on the first backtrack.
//!
//! haggisgac (and haggisgac-stable, gacschema) registers itself with the
//! GenericBacktracker in its *constructor* and keeps its own
//! `backtrack_stack`: `mark()` pushes a marker on every worldPush, and
//! `pop()` unwinds records back to the last marker on every worldPop.
//! A constraint built mid-search has missed every worldPush so far, but
//! worldPop still calls `pop()` on it once per open world level, so it
//! ran off the bottom of its own stack — a heap-buffer-overflow read
//! (`backtrack_stack.back()` on an empty vector), which ASan caught and
//! which segfaulted often enough in practice to kill the tester's whole
//! `--midsearch-constraints` sweep.

use std::collections::HashMap;

use minion_sys::ast::{Constant, Constraint, Model, ShortTuple, Var, VarDomain, VarName};
use minion_sys::{RunOptions, run_minion_midsearch_with_options};

/// Satisfied iff x=0∧z=0, or x=1∧y=1, or x=y=z=2.
fn short_tuples() -> Vec<ShortTuple> {
    vec![
        vec![(0, Constant::Integer(0)), (2, Constant::Integer(0))],
        vec![(0, Constant::Integer(1)), (1, Constant::Integer(1))],
        vec![
            (0, Constant::Integer(2)),
            (1, Constant::Integer(2)),
            (2, Constant::Integer(2)),
        ],
    ]
}

fn satisfies(row: &[i32]) -> bool {
    (row[0] == 0 && row[2] == 0)
        || (row[0] == 1 && row[1] == 1)
        || (row[0] == 2 && row[1] == 2 && row[2] == 2)
}

#[test]
fn haggisgac_injected_midsearch_survives_backtracking() {
    // Three free variables: 27 solutions, and enough depth that the
    // search backtracks repeatedly after the injection point.
    let mut model = Model::new();
    for name in ["x", "y", "z"] {
        model
            .named_variables
            .add_var(name.to_string(), VarDomain::Discrete(0, 2));
    }
    let order = ["x".to_string(), "y".to_string(), "z".to_string()];

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
                        Some(Constant::Bool(b)) => row.push(if *b { 1 } else { 0 }),
                        _ => return false,
                    }
                }
                rows.push(row);

                if injected_after.is_none() {
                    let vars = order.iter().map(|n| Var::NameRef(n.clone())).collect();
                    ctx.add_constraint(Constraint::HaggisGac(vars, short_tuples()))
                        .expect("inject haggisgac mid-search");
                    *injected_after = Some(rows.len());
                }
                true
            });

        run_minion_midsearch_with_options(model, RunOptions::default(), callback).expect("solve");
    }

    let injected_after = injected_after.expect("expected at least one solution before injecting");
    assert!(
        rows.len() > injected_after,
        "search stopped at the injection point: no solutions after it, so \
         nothing backtracked past the injected constraint"
    );
    for row in &rows[injected_after..] {
        assert!(
            satisfies(row),
            "solution {row:?} found after injecting haggisgac does not satisfy it"
        );
    }
}
