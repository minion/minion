//! Regression: a constraint injected mid-search keeps internal structures
//! that are only valid at the depth it was built at.
//!
//! gcc maintains an incremental flow graph. `adjlistlength` lives in
//! backtrackable memory, so the first backtrack above the depth the
//! constraint was created at zeroes it, leaving `fullPropagate`'s
//! narrowing loop nothing to walk while `adjlist`/`adjlistpos` (plain
//! vectors) still describe the full graph. The two halves then disagree
//! and `check_adjlists` fires:
//!
//!   Assert Failure: in == varArray[i].inDomain(j) at check_adjlists
//!
//! (minion-sys builds libminion with -DDOM_ASSERT, so that assert is
//! live here and aborts the test process.)
//!
//! Fixed by `AbstractConstraint::init_constraint`, called from worldPop
//! for every constraint being re-established at a shallower level, which
//! rebuilds gcc's structures from scratch before it is propagated again.

use std::collections::HashMap;

use minion_sys::ast::{Constant, Constraint, Model, Var, VarDomain, VarName};
use minion_sys::{RunOptions, run_minion_midsearch_with_options};

#[test]
fn gcc_injected_midsearch_survives_backtracking() {
    // x0, x1 free over 0..2 and a capacity variable c. Injected part-way
    // through, gcc([x0,x1], [0], [c]) says c counts the 0s among x0, x1.
    // Three search variables means the search backtracks repeatedly after
    // the injection point, which is what exercises the reinitialisation.
    let mut model = Model::new();
    for name in ["x0", "x1", "c"] {
        model
            .named_variables
            .add_var(name.to_string(), VarDomain::Discrete(0, 2));
    }
    let order = ["x0".to_string(), "x1".to_string(), "c".to_string()];

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
                    ctx.add_constraint(Constraint::Gcc(
                        vec![Var::NameRef("x0".into()), Var::NameRef("x1".into())],
                        vec![Constant::Integer(0)],
                        vec![Var::NameRef("c".into())],
                    ))
                    .expect("inject gcc mid-search");
                    *injected_after = Some(rows.len());
                }
                true
            });

        run_minion_midsearch_with_options(model, RunOptions::default(), callback).expect("solve");
    }

    let injected_after = injected_after.expect("expected a solution before injecting");
    assert!(
        rows.len() > injected_after,
        "no solutions after the injection: search never backtracked past it"
    );
    for row in &rows[injected_after..] {
        let zeros = row[..2].iter().filter(|v| **v == 0).count() as i32;
        assert_eq!(
            zeros, row[2],
            "solution {row:?} found after injecting gcc does not satisfy it \
             ({zeros} zeros among x0,x1 but c = {})",
            row[2]
        );
    }
}
