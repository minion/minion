use crate::constraint_def;
use crate::run_minion;
use crate::run_minion::MinionOutput;
use crate::run_minion_lib;
extern crate rand;

use self::rand::seq::SliceRandom;

use anyhow::{Result, anyhow};

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Backend {
    /// Spawn the `minion` binary as a subprocess (original behaviour).
    Exec,
    /// Call into libminion directly via minion-sys. Each solve gets its own
    /// `MinionContext`; rayon threads run many solves in parallel.
    InProcess,
}

pub struct MinionConfig<'a> {
    pub minionargs: Vec<String>,
    pub minionexec: &'a str,
    pub maxtuples: usize,
    pub backend: Backend,
}

/// Run one solve with whichever backend the config selects.
///
/// `extraargs` mirrors the exec-mode command line. When the backend is
/// `InProcess`, we currently only translate `-findallsols`; any exec-only
/// flag (e.g. `-parallel`, `-preprocess GAC`) is rejected here so callers
/// must skip those tests in in-process mode.
fn run_solve(
    config: &MinionConfig,
    extraargs: &[&str],
    instance: &constraint_def::ConstraintInstance,
    testname: &str,
) -> Result<MinionOutput> {
    match config.backend {
        Backend::Exec => run_minion::get_minion_solutions(
            config.minionexec,
            &config.minionargs,
            extraargs,
            instance,
            testname,
        ),
        Backend::InProcess => {
            let mut find_all = false;
            for &a in extraargs {
                match a {
                    "-findallsols" => find_all = true,
                    other => {
                        return Err(anyhow!(
                            "in-process backend does not support flag {:?}",
                            other
                        ));
                    }
                }
            }
            run_minion_lib::get_minion_solutions_in_process(instance, find_all, testname)
        }
    }
}

pub fn test_constraint(config: &MinionConfig, c: &constraint_def::ConstraintDef) -> Result<()> {
    let mut instance;
    let tups;
    loop {
        instance = constraint_def::build_random_instance(c);
        let tupstry = instance.tableise(config.maxtuples);
        if let Some(t) = tupstry {
            tups = t;
            break;
        }
    }

    let ret = run_solve(config, &["-findallsols"], &instance, "original")?;
    let ret2 = run_solve(config, &["-findallsols"], &tups, "tuples")?;
    if ret.solutions != ret2.solutions {
        return Err(anyhow!(format!(
            "Solutions not equal in {} vs {}",
            ret.filename, ret2.filename
        )));
    }
    if instance.constraint.gac && ret.nodes != ret2.nodes {
        return Err(anyhow!(format!(
            "Propagator should be GAC, but node counts not equal in {} vs {}",
            ret.filename, ret2.filename
        )));
    }

    ret.cleanup.cleanup();
    ret2.cleanup.cleanup();

    Ok(())
}

pub fn test_constraint_par(config: &MinionConfig, c: &constraint_def::ConstraintDef) -> Result<()> {
    let instance = constraint_def::build_random_instance(c);
    let ret = run_solve(config, &["-findallsols"], &instance, "original")?;
    let ret2 = run_solve(
        config,
        &["-findallsols", "-parallel"],
        &instance,
        "parallel",
    )?;
    let mut sortsols = ret.solutions.clone();
    let mut sortsols2 = ret2.solutions.clone();
    sortsols.sort();
    sortsols2.sort();
    if sortsols != sortsols2 {
        return Err(anyhow!(format!(
            "Solutions not equal in {} vs {}",
            ret.filename, ret2.filename
        )));
    }

    if instance.constraint.gac && ret.nodes != ret2.nodes {
        return Err(anyhow!(format!(
            "Propagator should be GAC, but node counts not equal in {} vs {}",
            ret.filename, ret2.filename
        )));
    }

    ret.cleanup.cleanup();
    ret2.cleanup.cleanup();

    Ok(())
}

pub fn test_constraint_options(
    config: &MinionConfig,
    c: &constraint_def::ConstraintDef,
    options: &Vec<&Vec<&str>>,
) -> Result<()> {
    let instance = constraint_def::build_random_instance(c);
    let ret = run_solve(config, &["-findallsols"], &instance, "original")?;
    let mut alloptions: Vec<&str> = vec![];
    for &i in options {
        for &j in i {
            alloptions.push(j);
        }
    }
    alloptions.push("-findallsols");

    let ret2 = run_solve(config, &alloptions[..], &instance, "options")?;
    let mut sortsols = ret.solutions.clone();
    let mut sortsols2 = ret2.solutions.clone();
    sortsols.sort();
    sortsols2.sort();
    if sortsols != sortsols2 {
        return Err(anyhow!(format!(
            "Solutions not equal in {} vs {}",
            ret.filename, ret2.filename
        )));
    }

    ret.cleanup.cleanup();
    ret2.cleanup.cleanup();

    Ok(())
}

/// Mid-search variable-injection test.
///
/// Solve the instance once to establish a baseline solution set, then
/// solve a second time with a callback that adds `new_var_count` fresh
/// boolean variables after `inject_after` solutions.
///
/// `minion_newVarMidsearch` appends the new variable to the solver's
/// *aux* block, which by design gets a single satisfying assignment per
/// decision leaf (search jumps out of aux vars after each solution).
/// So the properties we verify are:
///   * pre-injection rows equal the baseline's first `inject_after` rows;
///   * post-injection produces *exactly one* row per remaining baseline
///     leaf, in the same order;
///   * each injected var's value lies within its declared domain.
///
/// Skipped when baseline has <= `inject_after` solutions — there would
/// be nothing to verify after injection.
pub fn test_constraint_midsearch_add_vars(
    config: &MinionConfig,
    c: &constraint_def::ConstraintDef,
    inject_after: usize,
    new_var_count: usize,
) -> Result<()> {
    // Exec backend can't mutate a running solver; only in-process supports this.
    if config.backend == Backend::Exec {
        return Ok(());
    }

    let instance = constraint_def::build_random_instance(c);

    let baseline =
        crate::run_minion_lib::get_minion_solutions_in_process(&instance, true, "baseline")?;

    if baseline.solutions.len() <= inject_after {
        // Not enough baseline solutions to have anything post-injection.
        return Ok(());
    }

    let new_vars: Vec<(String, minion_sys::ast::VarDomain)> = (0..new_var_count)
        .map(|_| {
            (
                format!("mid_{}", crate::counter::get_unique_value()),
                minion_sys::ast::VarDomain::Bool,
            )
        })
        .collect();

    let got = crate::run_minion_lib::run_inject_vars_after(
        &instance,
        inject_after,
        &new_vars,
        "midsearch",
    )?;

    if !got.injected {
        return Err(anyhow!(
            "midsearch: expected injection to fire at count {} (baseline has {} solutions)",
            inject_after,
            baseline.solutions.len()
        ));
    }

    // Pre-injection must equal baseline[..inject_after] exactly.
    let expected_pre = &baseline.solutions[..inject_after];
    if got.pre_injection.as_slice() != expected_pre {
        return Err(anyhow!(
            "midsearch: pre-injection differs from baseline prefix ({} vs {} rows)",
            got.pre_injection.len(),
            expected_pre.len()
        ));
    }

    // Post-injection: one row per remaining baseline leaf, in the same order.
    let remaining = &baseline.solutions[inject_after..];
    if got.post_injection.len() != remaining.len() {
        if std::env::var("TESTER_DEBUG").is_ok() {
            eprintln!("baseline: {:?}", baseline.solutions);
            eprintln!("pre:      {:?}", got.pre_injection);
            eprintln!("post:     {:?}", got.post_injection);
        }
        return Err(anyhow!(
            "midsearch: post-injection has {} rows but {} remaining baseline leaves",
            got.post_injection.len(),
            remaining.len()
        ));
    }

    // Split each post row into (decision-var values, new-var values), then
    // check:
    //   * decision-var values match the baseline remaining row at the same index;
    //   * each new-var value is within its declared domain.
    let decision_len = baseline
        .solutions
        .first()
        .map(|r| r.len())
        .unwrap_or(0);
    for (idx, row) in got.post_injection.iter().enumerate() {
        if row.len() != decision_len + new_var_count {
            return Err(anyhow!(
                "midsearch: post-injection row {} has length {} (expected {})",
                idx,
                row.len(),
                decision_len + new_var_count
            ));
        }
        let (decision_part, new_part) = row.split_at(decision_len);
        if decision_part != remaining[idx].as_slice() {
            return Err(anyhow!(
                "midsearch: post-injection row {} decision part {:?} differs from baseline {:?}",
                idx,
                decision_part,
                remaining[idx]
            ));
        }
        for (k, (_name, domain)) in new_vars.iter().enumerate() {
            let v = new_part[k];
            let in_domain = match domain {
                minion_sys::ast::VarDomain::Bool => v == 0 || v == 1,
                minion_sys::ast::VarDomain::Bound(lo, hi) => {
                    v >= *lo as i64 && v <= *hi as i64
                }
                minion_sys::ast::VarDomain::Discrete(lo, hi) => {
                    v >= *lo as i64 && v <= *hi as i64
                }
                _ => true,
            };
            if !in_domain {
                return Err(anyhow!(
                    "midsearch: post-injection row {} new var {} value {} not in domain {:?}",
                    idx,
                    k,
                    v,
                    domain
                ));
            }
        }
    }

    Ok(())
}

/// Mid-search *constraint*-injection test, multi-packet generalisation.
///
/// The purpose is to exercise each Minion constraint as an *injected*
/// constraint, and to confirm that several injections in the same
/// solve compose correctly. The base problem is a bag of random
/// constraints purely as a carrier to give the solver something
/// non-trivial to search through.
///
/// Setup (all with a shared seed `s`):
///   * Build a random instance per `base_defs[i]` — together these are
///     the base model.
///   * Build a random instance per `inject_defs[k]` — these are packets
///     `P_1..P_N`. Their variables are declared up-front (so baseline
///     enumerates them freely), but their top-level constraints are
///     not initially in the model.
///
/// Injection schedule: packet `P_k` is added via
/// `MidSearchContext::add_constraint` once the running solve has
/// reported `inject_after_per_packet[k-1]` solutions. The schedule
/// must be strictly increasing.
///
/// Reference solves under seed `s`:
///   * baseline       — base only; packet vars free.
///   * full_k for k=1..N — base + P_1..P_k from the start; later packet
///                         vars still declared as free.
///   * actual         — base only at start; packet `P_k` injected at
///                      schedule index `k`.
///
/// Invariants (STATIC variable order). Splitting the actual solution
/// stream at the injection points into N+1 segments, with
/// `A_0 = 0`, `A_{N+1} = end`:
///   * Segment 0 (`[A_0, A_1)`): equals `baseline[A_0..A_1]`.
///   * Segment k for `1 ≤ k < N`: first `(A_{k+1} - A_k)` rows of
///     `full_k` minus the rows already in `actual[..A_k]`, in
///     `full_k`'s order.
///   * Segment N: all of `full_N` minus the rows already in
///     `actual[..A_N]`, in `full_N`'s order.
///
/// Each "minus seen, take in full's order" step is exactly the
/// single-packet check repeated with a growing prefix.
///
/// Skipped when the baseline can't produce enough solutions to fire
/// every injection, or when the actual run terminates early because a
/// packet pruned away all remaining branches before the next injection
/// point.
///
/// On failure the seed is logged so the trial can be re-run.
pub fn test_constraint_midsearch_inject_constraints(
    config: &MinionConfig,
    base_defs: &[&constraint_def::ConstraintDef],
    inject_defs: &[&constraint_def::ConstraintDef],
    inject_after_per_packet: &[usize],
    seed: u32,
) -> Result<()> {
    if config.backend == Backend::Exec {
        return Ok(());
    }
    let n = inject_defs.len();
    if n == 0 {
        return Ok(());
    }
    if inject_after_per_packet.len() != n {
        return Err(anyhow!(
            "test_constraint_midsearch_inject_constraints: schedule length {} != packet count {}",
            inject_after_per_packet.len(),
            n
        ));
    }
    if !inject_after_per_packet.windows(2).all(|w| w[0] < w[1]) {
        return Err(anyhow!(
            "test_constraint_midsearch_inject_constraints: schedule {:?} must be strictly increasing",
            inject_after_per_packet
        ));
    }

    let base_instances: Vec<constraint_def::ConstraintInstance> = base_defs
        .iter()
        .map(|d| constraint_def::build_random_instance(d))
        .collect();
    let inject_instances: Vec<constraint_def::ConstraintInstance> = inject_defs
        .iter()
        .map(|d| constraint_def::build_random_instance(d))
        .collect();
    let base_refs: Vec<&constraint_def::ConstraintInstance> = base_instances.iter().collect();
    let inject_refs: Vec<&constraint_def::ConstraintInstance> = inject_instances.iter().collect();

    let log_id = || -> String {
        format!(
            "inject={:?} base={:?} schedule={:?} seed={seed:#x}",
            inject_defs.iter().map(|d| d.name.as_str()).collect::<Vec<_>>(),
            base_defs.iter().map(|d| d.name.as_str()).collect::<Vec<_>>(),
            inject_after_per_packet,
        )
    };

    let last_a = *inject_after_per_packet.last().unwrap();

    // Baseline: no packet constraints, but their vars are declared.
    let baseline = crate::run_minion_lib::run_multi(&base_refs, &inject_refs, seed, "baseline")
        .map_err(|e| anyhow!("{}: baseline: {e}", log_id()))?;

    if baseline.solutions.len() < last_a {
        // Can't fire every injection; trivial trial.
        return Ok(());
    }

    // Actual run: all packets injected at their scheduled callback counts.
    let injections: Vec<(usize, &constraint_def::ConstraintInstance)> =
        inject_after_per_packet
            .iter()
            .zip(inject_instances.iter())
            .map(|(&a, inst)| (a, inst))
            .collect();
    let actual = crate::run_minion_lib::run_multi_injected(
        &base_refs,
        &inject_refs,
        &injections,
        seed,
        "actual",
    )
    .map_err(|e| anyhow!("{}: actual: {e}", log_id()))?;

    if actual.injections_fired != n {
        // A packet's pruning ended search before the next injection point.
        // Not a test failure; just a degenerate trial.
        return Ok(());
    }

    // Reference runs F_k for k=1..N: base + packets[..k] in the model.
    let mut full_runs: Vec<crate::run_minion::MinionOutput> = Vec::with_capacity(n);
    for k in 0..n {
        let mut with_c: Vec<&constraint_def::ConstraintInstance> = base_refs.clone();
        with_c.extend(inject_refs[..=k].iter().copied());
        let vars_only_k: Vec<&constraint_def::ConstraintInstance> =
            inject_refs[k + 1..].iter().copied().collect();
        let f_k = crate::run_minion_lib::run_multi(
            &with_c,
            &vars_only_k,
            seed,
            &format!("F_{}", k + 1),
        )
        .map_err(|e| anyhow!("{}: F_{}: {e}", log_id(), k + 1))?;
        full_runs.push(f_k);
    }

    // Build expected, segment by segment, verifying as we go.
    use std::collections::HashSet;
    let mut expected: Vec<Vec<i64>> = Vec::with_capacity(actual.solutions.len());

    // Segment 0: [0, A_1) from baseline.
    expected.extend(baseline.solutions[..inject_after_per_packet[0]].iter().cloned());

    // Verify segment 0.
    if actual.solutions[..inject_after_per_packet[0]] != expected[..] {
        return Err(anyhow!(
            "{}: segment 0 (baseline prefix) differs",
            log_id()
        ));
    }

    // Segments 1..=N: each uses F_k.
    for seg in 1..=n {
        let a_lo = inject_after_per_packet[seg - 1];
        let a_hi_opt = if seg < n {
            Some(inject_after_per_packet[seg])
        } else {
            None
        };
        let f_k = &full_runs[seg - 1].solutions;

        let prefix_seen: HashSet<&Vec<i64>> = expected.iter().collect();
        let f_unseen: Vec<Vec<i64>> = f_k
            .iter()
            .filter(|r| !prefix_seen.contains(*r))
            .cloned()
            .collect();
        let take_n = match a_hi_opt {
            Some(a_hi) => a_hi - a_lo,
            None => f_unseen.len(),
        };

        if f_unseen.len() < take_n {
            return Err(anyhow!(
                "{}: segment {} expected {} rows from F_{} \\ seen but only {} available",
                log_id(),
                seg,
                take_n,
                seg,
                f_unseen.len()
            ));
        }
        expected.extend(f_unseen[..take_n].iter().cloned());

        let a_hi = expected.len();
        if actual.solutions.len() < a_hi {
            return Err(anyhow!(
                "{}: actual ran out at index {} (segment {} expected ends at {})",
                log_id(),
                actual.solutions.len(),
                seg,
                a_hi
            ));
        }
        if actual.solutions[a_lo..a_hi] != expected[a_lo..a_hi] {
            if std::env::var("TESTER_DEBUG").is_ok() {
                eprintln!(
                    "segment {seg}: expected[{a_lo}..{a_hi}] = {:?}",
                    &expected[a_lo..a_hi]
                );
                eprintln!(
                    "segment {seg}: actual[{a_lo}..{a_hi}] = {:?}",
                    &actual.solutions[a_lo..a_hi]
                );
            }
            return Err(anyhow!(
                "{}: segment {} mismatch (a_lo={a_lo}, a_hi={a_hi})",
                log_id(),
                seg
            ));
        }
    }

    if actual.solutions.len() != expected.len() {
        return Err(anyhow!(
            "{}: total length mismatch: actual {} vs expected {}",
            log_id(),
            actual.solutions.len(),
            expected.len()
        ));
    }

    Ok(())
}

/// Single-packet shim that delegates to the multi-packet test with N=1.
/// Kept so existing callers don't have to construct slices.
pub fn test_constraint_midsearch_inject_constraint(
    config: &MinionConfig,
    base_defs: &[&constraint_def::ConstraintDef],
    inject_def: &constraint_def::ConstraintDef,
    inject_after: usize,
    seed: u32,
) -> Result<()> {
    test_constraint_midsearch_inject_constraints(
        config,
        base_defs,
        &[inject_def],
        &[inject_after],
        seed,
    )
}

pub fn test_constraint_nested(
    config: &MinionConfig,
    c: &constraint_def::ConstraintDef,
) -> Result<()> {
    let nest_type = constraint_def::NESTED_CONSTRAINT_LIST
        .choose(&mut rand::thread_rng())
        .unwrap();
    let mut instance;
    let tups;
    loop {
        instance = constraint_def::build_random_instance_with_children(nest_type, &[c]);
        let tupstry = instance.tableise(config.maxtuples);
        if let Some(t) = tupstry {
            tups = t;
            break;
        }
    }

    let ret = run_solve(config, &["-findallsols"], &instance, "original")?;
    let ret2 = run_solve(config, &["-findallsols"], &tups, "tuples")?;
    if ret.solutions != ret2.solutions {
        return Err(anyhow!(format!(
            "Solutions not equal in {} vs {}",
            ret.filename, ret2.filename
        )));
    }
    if instance.constraint.gac && ret.nodes != ret2.nodes {
        return Err(anyhow!(format!(
            "Propagator should be GAC, but node counts not equal in {} vs {}",
            ret.filename, ret2.filename
        )));
    }

    ret.cleanup.cleanup();
    ret2.cleanup.cleanup();
    Ok(())
}
