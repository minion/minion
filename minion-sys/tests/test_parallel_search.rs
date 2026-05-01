//! Integration tests for [`run_minion_parallel`] and
//! [`run_minion_parallel_with_options`].
//!
//! These tests must run serially because `run_minion_parallel` is documented
//! as not safe to call from multiple host threads concurrently (the Minion
//! alarm/ctrl-C handler state is process-global). Run with
//! `cargo test --test test_parallel_search -- --test-threads=1`. We also
//! gate concurrent execution by acquiring a process-wide mutex inside each
//! test so the suite is robust to default cargo invocations.

use std::collections::HashMap;
use std::sync::{Arc, Mutex, OnceLock};

use minion_sys::ast::*;
use minion_sys::{
    RunOptions, run_minion_parallel, run_minion_parallel_with_options, run_minion_with_options,
};

/// Process-wide mutex held for the duration of every parallel test, so two
/// tests in this file never run their `run_minion_parallel` calls
/// concurrently. See module docs.
fn serialise() -> std::sync::MutexGuard<'static, ()> {
    static LOCK: OnceLock<Mutex<()>> = OnceLock::new();
    LOCK.get_or_init(|| Mutex::new(()))
        .lock()
        .unwrap_or_else(|e| e.into_inner())
}

/// Build a small CSP with K solutions: x in 1..K, y in 1..K, no constraints
/// linking them beyond x = i implies y = K-i+1, encoded as a table.
///
/// Solution count = K. Used as a fixture for sequential vs parallel checks.
fn build_pair_model(k: i32) -> Model {
    let mut model = Model::new();
    model
        .named_variables
        .add_var("x".into(), VarDomain::Bound(1, k));
    model
        .named_variables
        .add_var("y".into(), VarDomain::Bound(1, k));
    // x + y == k + 1
    let mut tuples: Vec<Vec<Constant>> = vec![];
    for i in 1..=k {
        tuples.push(vec![Constant::Integer(i), Constant::Integer(k + 1 - i)]);
    }
    model.add_tuple_table("pairs".into(), tuples);
    model.constraints.push(Constraint::Str2Plus(
        vec![Var::NameRef("x".into()), Var::NameRef("y".into())],
        Var::NameRef("pairs".into()),
    ));
    model
}

#[test]
fn n1_finds_same_count_as_sequential() {
    let _g = serialise();
    let model_seq = build_pair_model(7);
    let seq_count = Arc::new(Mutex::new(0u32));
    {
        let c = seq_count.clone();
        let cb = Box::new(move |_: HashMap<VarName, Constant>| {
            let mut g = c.lock().unwrap();
            *g += 1;
            true
        });
        let opts = RunOptions {
            seed: Some(1),
            ..Default::default()
        };
        run_minion_with_options(model_seq, opts, cb).expect("sequential run failed");
    }
    let seq = *seq_count.lock().unwrap();

    let model_par = build_pair_model(7);
    let par_count = Arc::new(Mutex::new(0u32));
    {
        let c = par_count.clone();
        let cb = Box::new(move |_: HashMap<VarName, Constant>| {
            let mut g = c.lock().unwrap();
            *g += 1;
            true
        });
        let opts = RunOptions {
            seed: Some(1),
            ..Default::default()
        };
        run_minion_parallel_with_options(1, model_par, opts, cb)
            .expect("parallel N=1 run failed");
    }
    let par = *par_count.lock().unwrap();

    assert_eq!(seq, 7, "sequential should find all 7 solutions");
    assert_eq!(
        par, seq,
        "N=1 parallel must match sequential exactly (got {par} vs {seq})"
    );
}

#[test]
fn n4_finds_at_least_one_solution_per_thread() {
    let _g = serialise();
    // With 4 threads each independently enumerating, a model with K
    // solutions yields up to 4K callback invocations. We just check the
    // basic plumbing: callback runs, count is positive, run returns OK.
    let model = build_pair_model(5);
    let count = Arc::new(Mutex::new(0u32));
    {
        let c = count.clone();
        let cb = Box::new(move |_: HashMap<VarName, Constant>| {
            let mut g = c.lock().unwrap();
            *g += 1;
            true
        });
        run_minion_parallel(4, model, cb).expect("N=4 run failed");
    }
    let n = *count.lock().unwrap();
    assert!(n > 0, "expected at least one solution from N=4 portfolio");
    assert!(n <= 4 * 5, "expected at most 4*K solutions from portfolio");
}

#[test]
fn n4_first_solution_stops_others_when_callback_returns_false() {
    let _g = serialise();
    // Returning false from the callback signals the controller to stop all
    // threads. We expect to see exactly one invocation (or possibly a
    // small number if multiple threads found solutions before the stop
    // flag propagated).
    let model = build_pair_model(20);
    let count = Arc::new(Mutex::new(0u32));
    {
        let c = count.clone();
        let cb = Box::new(move |_: HashMap<VarName, Constant>| {
            let mut g = c.lock().unwrap();
            *g += 1;
            false // stop the portfolio
        });
        run_minion_parallel(4, model, cb).expect("N=4 stop run failed");
    }
    let n = *count.lock().unwrap();
    // The first thread to call the callback returns false and gets stopped.
    // Other threads may have already passed the callback for their own
    // first solution before the stop flag propagated, so we allow up to N.
    assert!(n >= 1 && n <= 4, "expected 1..=4 callback calls, got {n}");
}

#[test]
fn zero_threads_is_an_error() {
    let _g = serialise();
    let model = build_pair_model(3);
    let cb = Box::new(|_: HashMap<VarName, Constant>| true);
    let r = run_minion_parallel(0, model, cb);
    assert!(r.is_err(), "expected error for num_threads=0");
}
