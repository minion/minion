//! `RunOptions::time_limit` with `is_cpu_time = true` maps to minion's
//! `-cpulimit`: `setrlimit(RLIMIT_CPU)` plus a SIGXCPU handler. As with
//! the wall-clock limit, a run that hits it comes back as
//! `RuntimeError::Timeout`.
//!
//! One test per binary on purpose — see the header of `test_limits.rs`.
//! This one especially: the soft limit stays set for the rest of the
//! process's life, and minion sets the hard limit to `seconds + 5`, so any
//! later work in the same process would be killed outright.

mod common;

use std::time::Instant;

use common::{build_free_model, try_solve};
use minion_sys::error::{MinionError, RuntimeError};
use minion_sys::{RunOptions, TimeLimit};

#[test]
fn cpu_time_limit_stops_search() {
    let start = Instant::now();
    let res = try_solve(
        build_free_model(10, 10),
        RunOptions {
            node_limit: 20_000_000,
            time_limit: Some(TimeLimit {
                seconds: 2,
                is_cpu_time: true,
            }),
            ..Default::default()
        },
    );
    let elapsed = start.elapsed();

    match res {
        Err(MinionError::RuntimeError(RuntimeError::Timeout)) => {}
        Err(e) => panic!("expected a timeout after {elapsed:?}, got {e}"),
        Ok((sols, nodes, _)) => panic!(
            "CPU-time limit did not fire: search ended after {elapsed:?}, \
             {nodes} nodes, {sols} solutions"
        ),
    }
    assert!(
        elapsed.as_secs() < 30,
        "timeout fired far too late: {elapsed:?} for a 2-second limit"
    );
}
