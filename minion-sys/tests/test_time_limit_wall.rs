//! `RunOptions::time_limit` with `is_cpu_time = false` maps to minion's
//! `-timelimit`: a wall-clock `alarm()`. A run that hits it comes back as
//! `RuntimeError::Timeout` (libwrapper turns the TableOut "TimeOut" entry
//! into MINION_TIMEOUT), so no context and no partial statistics —
//! unlike `node_limit`, which just truncates the search silently.
//!
//! One test per binary on purpose — see the header of `test_limits.rs`.

mod common;

use std::time::Instant;

use common::{build_free_model, try_solve};
use minion_sys::error::{MinionError, RuntimeError};
use minion_sys::{RunOptions, TimeLimit};

#[test]
fn wall_time_limit_stops_search() {
    // 10^10 solutions: hours of search, so a run that stops in seconds
    // stopped because of the limit. The node limit is only a backstop so
    // that a broken timeout fails the test instead of hanging the suite.
    let start = Instant::now();
    let res = try_solve(
        build_free_model(10, 10),
        RunOptions {
            node_limit: 20_000_000,
            time_limit: Some(TimeLimit {
                seconds: 2,
                is_cpu_time: false,
            }),
            ..Default::default()
        },
    );
    let elapsed = start.elapsed();

    match res {
        Err(MinionError::RuntimeError(RuntimeError::Timeout)) => {}
        Err(e) => panic!("expected a timeout after {elapsed:?}, got {e}"),
        Ok((sols, nodes, _)) => panic!(
            "wall-clock limit did not fire: search ended after {elapsed:?}, \
             {nodes} nodes, {sols} solutions"
        ),
    }
    assert!(
        elapsed.as_secs() < 30,
        "timeout fired far too late: {elapsed:?} for a 2-second limit"
    );
}
