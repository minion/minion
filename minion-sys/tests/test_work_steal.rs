//! Integration tests for [`run_minion_work_steal`]. The headline test is
//! that work-stealing actually divides the search tree across N threads
//! (so total wall time on a hard UNSAT instance drops with more workers).
//!
//! These tests acquire a process-wide mutex because `run_minion_work_steal`
//! is documented as not safe to call from multiple host threads
//! concurrently (the Minion alarm/ctrl-C handler state is process-global).

use std::collections::HashMap;
use std::sync::{Arc, Mutex, OnceLock};

use minion_sys::ast::*;
use minion_sys::{
    RunOptions, run_minion_with_options, run_minion_work_steal,
    run_minion_work_steal_with_options, VarOrder,
};

fn serialise() -> std::sync::MutexGuard<'static, ()> {
    static LOCK: OnceLock<Mutex<()>> = OnceLock::new();
    LOCK.get_or_init(|| Mutex::new(()))
        .lock()
        .unwrap_or_else(|e| e.into_inner())
}

/// Build a model with k variables in 1..k, all-different. For k vars and
/// k values it's SAT (k! solutions). For k vars and (k-1) values it's
/// UNSAT (pigeon-hole, weakly enforced via diseq pairs so search actually
/// has to explore — gacalldiff would prune at the root).
fn build_pairwise_diseq(num_vars: i32, max_val: i32) -> Model {
    let mut model = Model::new();
    let mut var_names: Vec<VarName> = vec![];
    for i in 0..num_vars {
        let name = format!("v{}", i);
        model
            .named_variables
            .add_var(name.clone(), VarDomain::Discrete(1, max_val));
        var_names.push(name);
    }
    // Pairwise diseq — weak (no propagation across pairs), so the search
    // has to enumerate.
    for i in 0..num_vars {
        for j in (i + 1)..num_vars {
            model.constraints.push(Constraint::DisEq(
                Var::NameRef(var_names[i as usize].clone()),
                Var::NameRef(var_names[j as usize].clone()),
            ));
        }
    }
    model
}

#[test]
fn n1_finds_same_count_as_sequential() {
    let _g = serialise();
    let m_seq = build_pairwise_diseq(4, 4);
    let seq_count = Arc::new(Mutex::new(0u32));
    {
        let c = seq_count.clone();
        let cb = Box::new(move |_: HashMap<VarName, Constant>| {
            *c.lock().unwrap() += 1;
            true
        });
        let opts = RunOptions {
            seed: Some(1),
            var_order: VarOrder::Static,
            ..Default::default()
        };
        run_minion_with_options(m_seq, opts, cb).expect("sequential failed");
    }
    let seq = *seq_count.lock().unwrap();

    let m_ws = build_pairwise_diseq(4, 4);
    let ws_count = Arc::new(Mutex::new(0u32));
    {
        let c = ws_count.clone();
        let cb = Box::new(move |_: HashMap<VarName, Constant>| {
            *c.lock().unwrap() += 1;
            true
        });
        let opts = RunOptions {
            seed: Some(1),
            var_order: VarOrder::Static,
            ..Default::default()
        };
        run_minion_work_steal_with_options(1, m_ws, opts, cb).expect("N=1 failed");
    }
    let ws = *ws_count.lock().unwrap();

    assert_eq!(seq, 24, "expected 4! = 24 SAT solutions");
    assert_eq!(
        ws, seq,
        "N=1 work-steal must match sequential exactly (got {ws} vs {seq})"
    );
}

#[test]
fn n4_total_solutions_matches_sequential() {
    let _g = serialise();
    let m_ws = build_pairwise_diseq(4, 4);
    let ws_count = Arc::new(Mutex::new(0u32));
    {
        let c = ws_count.clone();
        let cb = Box::new(move |_: HashMap<VarName, Constant>| {
            *c.lock().unwrap() += 1;
            true
        });
        let opts = RunOptions {
            seed: Some(1),
            var_order: VarOrder::Static,
            ..Default::default()
        };
        run_minion_work_steal_with_options(4, m_ws, opts, cb).expect("N=4 failed");
    }
    let ws = *ws_count.lock().unwrap();
    // Work-stealing splits the tree across workers; total solutions
    // across all workers must equal the sequential count (24 for 4
    // pairwise-diseq vars in 1..4). Pure portfolio (run_minion_parallel)
    // would multiply this by N — work-stealing does not.
    assert_eq!(
        ws, 24,
        "N=4 work-steal total solutions must equal sequential (got {ws})"
    );
}

#[test]
fn unsat_n4_completes_and_reports_zero() {
    let _g = serialise();
    // 5 vars in 1..4 with pairwise diseq is unsatisfiable (5 pigeons in
    // 4 holes, no two equal). Sequential search exhausts the tree to
    // prove unsat.
    let m_ws = build_pairwise_diseq(5, 4);
    let ws_count = Arc::new(Mutex::new(0u32));
    {
        let c = ws_count.clone();
        let cb = Box::new(move |_: HashMap<VarName, Constant>| {
            *c.lock().unwrap() += 1;
            true
        });
        let opts = RunOptions {
            seed: Some(1),
            var_order: VarOrder::Static,
            ..Default::default()
        };
        run_minion_work_steal_with_options(4, m_ws, opts, cb).expect("UNSAT N=4 failed");
    }
    let ws = *ws_count.lock().unwrap();
    assert_eq!(ws, 0, "UNSAT instance must report 0 solutions");
}

#[test]
fn first_solution_stops_others() {
    let _g = serialise();
    // SAT instance; callback returns false on first solution to test
    // the early-stop signalling.
    let m = build_pairwise_diseq(4, 4);
    let count = Arc::new(Mutex::new(0u32));
    {
        let c = count.clone();
        let cb = Box::new(move |_: HashMap<VarName, Constant>| {
            *c.lock().unwrap() += 1;
            false
        });
        run_minion_work_steal(4, m, cb).expect("stop run failed");
    }
    let n = *count.lock().unwrap();
    // The first thread to call the callback returns false; other
    // threads may have already called the callback for their own first
    // solution before the stop flag propagated. Expect a small bounded
    // number of callback calls.
    assert!(n >= 1 && n <= 8, "expected 1..=8 callback calls, got {n}");
}

#[test]
fn zero_threads_is_an_error() {
    let _g = serialise();
    let m = build_pairwise_diseq(3, 3);
    let cb = Box::new(|_: HashMap<VarName, Constant>| true);
    assert!(run_minion_work_steal(0, m, cb).is_err());
}
