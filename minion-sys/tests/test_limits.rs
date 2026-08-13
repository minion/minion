//! `RunOptions::node_limit` must actually reach minion's search options —
//! it is the only way a library caller can bound a runaway search, and a
//! silently-ignored limit looks exactly like a search that happened to
//! finish.
//!
//! The `time_limit` half of the same plumbing is tested in
//! `test_time_limit_wall.rs` and `test_time_limit_cpu.rs`, one test per
//! binary: minion's timeout uses `alarm()`/`setrlimit(RLIMIT_CPU)` and a
//! process-static trigger pointer (system/trigger_timer.cpp), so two
//! concurrent solves in one process clobber each other's timers. The node
//! limit is per-solve state and has no such problem.

mod common;

use common::{build_free_model, solve};
use minion_sys::RunOptions;

#[test]
fn node_limit_stops_search() {
    let (all_sols, all_nodes, _) = solve(build_free_model(5, 5), RunOptions::default());
    assert_eq!(all_sols, 5u64.pow(5), "unlimited run must be complete");

    let (capped_sols, capped_nodes, _) = solve(
        build_free_model(5, 5),
        RunOptions {
            node_limit: 100,
            ..Default::default()
        },
    );
    assert!(
        capped_nodes < all_nodes,
        "node limit ignored: {capped_nodes} nodes vs {all_nodes} unlimited"
    );
    assert!(
        capped_sols < all_sols,
        "node limit ignored: {capped_sols} solutions vs {all_sols} unlimited"
    );
    // Minion checks the limit between nodes, so a small overshoot is
    // allowed; anything near the unlimited count is not.
    assert!(
        capped_nodes <= 200,
        "node limit overshot badly: {capped_nodes} nodes for a limit of 100"
    );
}

#[test]
fn node_limit_zero_means_unlimited() {
    let (sols, _, _) = solve(
        build_free_model(4, 4),
        RunOptions {
            node_limit: 0,
            ..Default::default()
        },
    );
    assert_eq!(sols, 4u64.pow(4), "node_limit 0 must not cap the search");
}

#[test]
fn generous_node_limit_does_not_disturb_a_complete_search() {
    let (baseline, _, _) = solve(build_free_model(4, 4), RunOptions::default());
    let (limited, _, _) = solve(
        build_free_model(4, 4),
        RunOptions {
            node_limit: 1_000_000,
            ..Default::default()
        },
    );
    assert_eq!(baseline, limited);
}
