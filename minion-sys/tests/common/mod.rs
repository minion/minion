//! Shared helpers for the search-limit tests.

use std::collections::HashMap;

use minion_sys::ast::{Constant, Model, VarDomain, VarName};
use minion_sys::error::MinionError;
use minion_sys::{RunOptions, SolverContext, run_minion_with_options};

/// `n` unconstrained discrete variables over `1..=dom`: dom^n solutions,
/// and a node for each. No constraints, so nothing prunes.
#[allow(dead_code)]
pub fn build_free_model(n: usize, dom: i32) -> Model {
    let mut m = Model::new();
    for i in 0..n {
        m.named_variables
            .add_var(format!("x{i}"), VarDomain::Discrete(1, dom));
    }
    m
}

/// Returns (solutions found, nodes reported, the solver context), or
/// whatever error minion reported. A timed-out run reports
/// `RuntimeError::Timeout` and yields no context.
#[allow(dead_code)]
pub fn try_solve(
    model: Model,
    options: RunOptions,
) -> Result<(u64, i64, SolverContext), MinionError> {
    let mut count: u64 = 0;
    let callback: minion_sys::Callback<'_> = {
        let count = &mut count;
        Box::new(move |_sol: HashMap<VarName, Constant>| -> bool {
            *count += 1;
            true
        })
    };
    let ctx = run_minion_with_options(model, options, callback)?;
    let nodes: i64 = ctx
        .get_from_table("Nodes".to_string())
        .expect("Nodes missing")
        .parse()
        .expect("Nodes not an integer");
    Ok((count, nodes, ctx))
}

/// Returns (solutions found, nodes reported, the solver context).
#[allow(dead_code)]
pub fn solve(model: Model, options: RunOptions) -> (u64, i64, SolverContext) {
    try_solve(model, options).expect("solve")
}
