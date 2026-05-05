use crate::constraint_def;
use crate::minion_instance::OptimisationWrapper;
use crate::run_minion;
use crate::run_minion::MinionOutput;
use crate::run_minion_lib;
extern crate rand;

use self::rand::seq::SliceRandom;
use self::rand::Rng;

use anyhow::{anyhow, Result};

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
    pub var_order: minion_sys::VarOrder,
    pub val_order: minion_sys::ValOrder,
    pub preprocess: minion_sys::Propagation,
    pub prop_node: minion_sys::Propagation,
    /// Hard cap on the number of solutions any single solver run will
    /// gather. Protects against memory blow-up when scaled-up random
    /// instances happen to have huge solution spaces. A run that hits
    /// the cap sets `MinionOutput.hit_solution_cap`; callers (test
    /// functions) skip the trial silently rather than comparing
    /// possibly-divergent prefixes.
    pub max_solutions: i64,
}

impl<'a> MinionConfig<'a> {
    /// Returns true when the configured variable/value ordering is
    /// (a) state-independent and (b) deterministic — i.e. any two runs
    /// of the same model under the same seed visit decision leaves in
    /// the same order. Holds for Static + Ascend/Descend.
    ///
    /// When false, even baseline-vs-tableised solves on the same
    /// instance may report solutions in different orders, so tests
    /// must compare sets rather than ordered slices.
    pub fn deterministic_ordering(&self) -> bool {
        self.var_order == minion_sys::VarOrder::Static
            && self.val_order != minion_sys::ValOrder::Random
    }

    /// Returns true when search-tree alignment between a
    /// mid-search-injected run and its "built with all packets from
    /// the start" reference is guaranteed: static ordering *and*
    /// default preprocessing. Non-default preprocess/prop-node
    /// changes the initial domain state of the reference run
    /// relative to actual's state post-injection, which diverges the
    /// trees even under Static + Ascend.
    pub fn midsearch_slice_exact(&self) -> bool {
        self.deterministic_ordering()
            && self.preprocess.is_default_preprocess()
            && self.prop_node.is_default_prop_node()
    }

    /// [`minion_sys::RunOptions`] derived from this config, with the
    /// given seed.
    pub fn run_options(&self, seed: u32) -> minion_sys::RunOptions {
        minion_sys::RunOptions {
            seed: Some(seed),
            var_order: self.var_order,
            val_order: self.val_order,
            preprocess: self.preprocess,
            prop_node: self.prop_node,
        }
    }

    /// [`minion_sys::RunOptions`] with no pinned seed (for
    /// `run_minion` callers that never needed reproducibility).
    pub fn run_options_no_seed(&self) -> minion_sys::RunOptions {
        minion_sys::RunOptions {
            seed: None,
            var_order: self.var_order,
            val_order: self.val_order,
            preprocess: self.preprocess,
            prop_node: self.prop_node,
        }
    }
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
    // Default: digest-only — WE ARE HASHING SOLUTIONS, NOT STORING
    // THEM (see solution_digest.rs). Callers that need indexed access
    // to all solutions go through `run_solve_keep_full` below.
    run_solve_inner(config, extraargs, instance, testname, false)
}

/// Same as [`run_solve`], but populates `MinionOutput::raw_solutions`
/// with the full Vec of solutions. Use only when the caller needs
/// indexed access (mid-search prefix slices, membership checks, etc.).
/// Heavy-traffic set-equality call sites should use [`run_solve`] and
/// compare digests.
fn run_solve_keep_full(
    config: &MinionConfig,
    extraargs: &[&str],
    instance: &constraint_def::ConstraintInstance,
    testname: &str,
) -> Result<MinionOutput> {
    run_solve_inner(config, extraargs, instance, testname, true)
}

fn run_solve_inner(
    config: &MinionConfig,
    extraargs: &[&str],
    instance: &constraint_def::ConstraintInstance,
    testname: &str,
    keep_full_solutions: bool,
) -> Result<MinionOutput> {
    match config.backend {
        Backend::Exec => run_minion::get_minion_solutions(
            config.minionexec,
            &config.minionargs,
            extraargs,
            instance,
            testname,
            config.max_solutions,
            keep_full_solutions,
        ),
        Backend::InProcess => {
            // Tiny flag parser: -findallsols is recognised as before;
            // -X-parallelWorkSteal <N> routes through the work-steal
            // variant of the in-process entry. Other flags are
            // exec-only and rejected.
            let mut find_all = false;
            let mut work_steal: Option<usize> = None;
            let mut i = 0;
            while i < extraargs.len() {
                match extraargs[i] {
                    "-findallsols" => {
                        find_all = true;
                        i += 1;
                    }
                    "-X-parallelWorkSteal" => {
                        if i + 1 >= extraargs.len() {
                            return Err(anyhow!(
                                "-X-parallelWorkSteal needs a thread-count argument"
                            ));
                        }
                        let n: usize = extraargs[i + 1].parse().map_err(|_| {
                            anyhow!(
                                "-X-parallelWorkSteal: invalid thread count {:?}",
                                extraargs[i + 1]
                            )
                        })?;
                        work_steal = Some(n);
                        i += 2;
                    }
                    other => {
                        return Err(anyhow!(
                            "in-process backend does not support flag {:?}",
                            other
                        ));
                    }
                }
            }
            if let Some(n) = work_steal {
                let _ = find_all;
                run_minion_lib::get_minion_solutions_in_process_work_steal(
                    instance,
                    config.run_options_no_seed(),
                    n,
                    testname,
                    config.max_solutions,
                    keep_full_solutions,
                )
            } else {
                run_minion_lib::get_minion_solutions_in_process(
                    instance,
                    find_all,
                    config.run_options_no_seed(),
                    testname,
                    config.max_solutions,
                    keep_full_solutions,
                )
            }
        }
    }
}

pub fn test_constraint(config: &MinionConfig, c: &constraint_def::ConstraintDef) -> Result<()> {
    let mut instance;
    let tups;
    let mut attempts = 0;
    loop {
        instance = constraint_def::build_random_instance(c);
        let tupstry = instance.tableise(config.maxtuples);
        if let Some(t) = tupstry {
            tups = t;
            break;
        }
        attempts += 1;
        // At high --size-factor most random rolls produce instances
        // whose Cartesian product exceeds maxtuples and tableise
        // refuses. Skip the trial after a bounded retry budget rather
        // than spinning forever.
        if attempts > 100 {
            return Ok(());
        }
    }

    let ret = run_solve(config, &["-findallsols"], &instance, "original")?;
    let ret2 = run_solve(config, &["-findallsols"], &tups, "tuples")?;
    // If either side hit the solution cap, the random instance is too
    // big to compare reliably — solution lists are partial prefixes.
    // Skip the trial silently rather than report a spurious mismatch.
    if ret.hit_solution_cap || ret2.hit_solution_cap {
        ret.cleanup.cleanup();
        ret2.cleanup.cleanup();
        return Ok(());
    }
    // Comparison is now a digest equality check. The digest sums each
    // solution's hash mod 2^64 (order-independent), so the previously-
    // distinct deterministic-vs-set comparison paths collapse into one:
    // both check multiset equality. The bounded `sample` field gives us
    // up to 16 concrete solutions per side for failure messages.
    if ret.solutions != ret2.solutions {
        let name = &c.name;
        let n = ret.solutions.sample.len().min(10);
        let m = ret2.solutions.sample.len().min(10);
        return Err(anyhow!(format!(
            "Solutions not equal in {}/{} ({name}): orig={}×{:?} tup={}×{:?}",
            ret.filename,
            ret2.filename,
            ret.solutions.count,
            &ret.solutions.sample[..n],
            ret2.solutions.count,
            &ret2.solutions.sample[..m],
        )));
    }
    // Node-count equality is a GAC-propagator invariant only when the
    // ordering is state-free (Static + Ascend/Descend); state-dependent
    // heuristics explore different trees against the weaker tabulated
    // propagator even when the solution sets match.
    if instance.constraint.gac && config.deterministic_ordering() && ret.nodes != ret2.nodes {
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
    if ret.hit_solution_cap || ret2.hit_solution_cap {
        ret.cleanup.cleanup();
        ret2.cleanup.cleanup();
        return Ok(());
    }
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

use std::sync::atomic::{AtomicI64, AtomicU64, Ordering};

/// Process-wide diagnostic counters for the work-steal sweep:
/// - `WS_TRIALS` counts how many test_constraint_workstal calls ran.
/// - `WS_DONATIONS` accumulates the total `donate()` calls across all
///   trials (only meaningful for Exec backend; in-process path doesn't
///   currently surface this).
/// - `WS_TRIALS_WITH_DONATIONS` counts trials whose donation count was
///   non-zero.
///
/// `main` reads these after the sweep and reports them so the user
/// can confirm work-stealing actually exercised the donation path.
pub static WS_TRIALS: AtomicU64 = AtomicU64::new(0);
pub static WS_DONATIONS: AtomicI64 = AtomicI64::new(0);
pub static WS_TRIALS_WITH_DONATIONS: AtomicU64 = AtomicU64::new(0);
/// Trials abandoned because the random instance hit the configured
/// solution cap. Tracked separately from `WS_TRIALS` so the adaptive
/// growth loop can detect when a constraint's instances at the
/// current size are too big to compare and stop growing.
pub static WS_TRIALS_CAPPED: AtomicU64 = AtomicU64::new(0);

/// Parallel-preprocess sweep diagnostics (mirrors the WS_* set above).
/// `PP_TRIALS` counts trials that ran the parallel-preprocess
/// comparison. `PP_TRIALS_WITH_ROUNDS` counts trials whose parallel
/// run reported `rounds >= 2` — the strongest "parallel SAC actually
/// did meaningful work" signal (one round found prunings, another
/// confirmed convergence). `PP_TOTAL_PRUNINGS` accumulates merged
/// prunings across trials. `PP_TRIALS_CAPPED` mirrors WS_TRIALS_CAPPED
/// for the parallel-preprocess sweep's adaptive-sizing loop.
pub static PP_TRIALS: AtomicU64 = AtomicU64::new(0);
pub static PP_TRIALS_WITH_ROUNDS: AtomicU64 = AtomicU64::new(0);
pub static PP_TOTAL_PRUNINGS: AtomicI64 = AtomicI64::new(0);
pub static PP_TRIALS_CAPPED: AtomicU64 = AtomicU64::new(0);

/// Work-stealing parallel torture test: solve the same instance with
/// `-findallsols` sequentially and under `-X-parallelWorkSteal N`, and
/// require the two solution sets to match. Unlike `-parallel` (fork
/// portfolio), work-stealing divides the search tree across workers, so
/// the solution count must equal sequential exactly — not be multiplied
/// by N. Node counts are NOT compared here even when the constraint is
/// GAC: work-stealing changes which sub-tree each worker explores, and
/// per-worker propagation can re-derive the same prunings against
/// slightly different traversal orders, so node-count equality is not
/// guaranteed.
pub fn test_constraint_workstal(
    config: &MinionConfig,
    c: &constraint_def::ConstraintDef,
    n: u32,
    size_factor: u32,
) -> Result<()> {
    let instance = constraint_def::build_random_instance_sized(c, size_factor);
    let ret = run_solve(config, &["-findallsols"], &instance, "original")?;
    if ret.hit_solution_cap {
        // Sequential blew through the cap — the random instance has
        // more solutions than we're willing to materialise. Skip
        // comparison and record the trial as capped so the adaptive
        // growth loop knows not to grow further.
        WS_TRIALS_CAPPED.fetch_add(1, Ordering::Relaxed);
        ret.cleanup.cleanup();
        return Ok(());
    }
    let n_str = n.to_string();
    let ret2 = run_solve(
        config,
        &["-findallsols", "-X-parallelWorkSteal", &n_str],
        &instance,
        "workstal",
    )?;
    if ret2.hit_solution_cap {
        WS_TRIALS_CAPPED.fetch_add(1, Ordering::Relaxed);
        ret.cleanup.cleanup();
        ret2.cleanup.cleanup();
        return Ok(());
    }
    if ret.solutions != ret2.solutions {
        return Err(anyhow!(format!(
            "Work-stealing solutions not equal in {} vs {}: seq={} ws={}",
            ret.filename,
            ret2.filename,
            ret.solutions.count,
            ret2.solutions.count
        )));
    }

    // Accumulate donation diagnostics. Both backends now surface the
    // donation count: exec via -jsontableout, in-process via the
    // WorkStealStats returned from run_minion_work_steal_with_options.
    WS_TRIALS.fetch_add(1, Ordering::Relaxed);
    if let Some(donations) = ret2.work_steal_donations {
        WS_DONATIONS.fetch_add(donations, Ordering::Relaxed);
        if donations > 0 {
            WS_TRIALS_WITH_DONATIONS.fetch_add(1, Ordering::Relaxed);
        }
    }

    ret.cleanup.cleanup();
    ret2.cleanup.cleanup();

    Ok(())
}

/// Work-stealing portfolio variant: same as `test_constraint_workstal`,
/// but the parallel run also passes `-X-parallelWorkStealPortfolio` so
/// each worker uses a different (var-order, val-order, randomisation)
/// strategy. The crucial invariant — portfolio must enumerate exactly
/// the same solution set as sequential — is unchanged: path replay is
/// heuristic-independent, so cooperating workers on diverse strategies
/// are still required to cover the whole tree.
///
/// Exec-only: the in-process API does not expose the portfolio flag.
pub fn test_constraint_workstal_portfolio(
    config: &MinionConfig,
    c: &constraint_def::ConstraintDef,
    n: u32,
    size_factor: u32,
) -> Result<()> {
    let instance = constraint_def::build_random_instance_sized(c, size_factor);
    let ret = run_solve(config, &["-findallsols"], &instance, "original")?;
    if ret.hit_solution_cap {
        WS_TRIALS_CAPPED.fetch_add(1, Ordering::Relaxed);
        ret.cleanup.cleanup();
        return Ok(());
    }
    let n_str = n.to_string();
    let ret2 = run_solve(
        config,
        &[
            "-findallsols",
            "-X-parallelWorkSteal",
            &n_str,
            "-X-parallelWorkStealPortfolio",
        ],
        &instance,
        "workstal_portfolio",
    )?;
    if ret2.hit_solution_cap {
        WS_TRIALS_CAPPED.fetch_add(1, Ordering::Relaxed);
        ret.cleanup.cleanup();
        ret2.cleanup.cleanup();
        return Ok(());
    }
    if ret.solutions != ret2.solutions {
        return Err(anyhow!(format!(
            "Work-steal portfolio solutions not equal in {} vs {}: seq={} portfolio={}",
            ret.filename,
            ret2.filename,
            ret.solutions.count,
            ret2.solutions.count
        )));
    }

    WS_TRIALS.fetch_add(1, Ordering::Relaxed);
    if let Some(donations) = ret2.work_steal_donations {
        WS_DONATIONS.fetch_add(donations, Ordering::Relaxed);
        if donations > 0 {
            WS_TRIALS_WITH_DONATIONS.fetch_add(1, Ordering::Relaxed);
        }
    }

    ret.cleanup.cleanup();
    ret2.cleanup.cleanup();

    Ok(())
}

/// Per-trial outcome from `test_constraint_parallel_preprocess`. The
/// global PP_* counters update too, but the per-constraint adaptive-
/// sizing loop in `main.rs` needs a *local* signal — global atomics
/// would observe other constraints' rounds in the parallel sweep and
/// declare every constraint "multi-round confirmed" the moment any
/// constraint fires.
#[derive(Debug, Clone, Copy, Default)]
pub struct PPTrialOutcome {
    pub had_rounds_ge2: bool,
    pub was_capped: bool,
}

/// Parallel-preprocess correctness test.
///
/// Solve the same instance with `-preprocess SAC` sequentially and
/// then with `-preprocess SAC -X-parallelPreprocess N`. Parallel SAC
/// is mathematically required to reach the same fixpoint as sequential
/// SAC, so the solution sets must match exactly — same correctness
/// contract as work-stealing.
///
/// Random tester instances at `size_factor = 1` typically have
/// SAC-trivial domains (the fixpoint converges in one round before any
/// pruning happens), so the parallel path can run without ever doing
/// meaningful work. The caller drives an adaptive-sizing loop in
/// `main.rs` that grows the instance until at least one trial reports
/// `rounds >= 2` — the strongest "parallel SAC actually fired" signal
/// available, because rounds == 1 just means "first round found
/// nothing" while rounds >= 2 means "first round found prunings AND a
/// second round was needed to confirm convergence".
///
/// Exec-only: the in-process API doesn't surface the parallel-
/// preprocess counters (and the flag itself isn't plumbed through
/// `RunOptions`). The exec backend's `-jsontableout` already reports
/// `ParallelPreprocessRounds` and `ParallelPreprocessPrunings`.
pub fn test_constraint_parallel_preprocess(
    config: &MinionConfig,
    c: &constraint_def::ConstraintDef,
    n: u32,
    size_factor: u32,
) -> Result<PPTrialOutcome> {
    let instance = constraint_def::build_random_instance_sized(c, size_factor);
    let ret = run_solve(
        config,
        &["-findallsols", "-preprocess", "SAC"],
        &instance,
        "pp_seq",
    )?;
    if ret.hit_solution_cap {
        PP_TRIALS_CAPPED.fetch_add(1, Ordering::Relaxed);
        ret.cleanup.cleanup();
        return Ok(PPTrialOutcome {
            was_capped: true,
            ..Default::default()
        });
    }
    let n_str = n.to_string();
    let ret2 = run_solve(
        config,
        &[
            "-findallsols",
            "-preprocess",
            "SAC",
            "-X-parallelPreprocess",
            &n_str,
        ],
        &instance,
        "pp_par",
    )?;
    if ret2.hit_solution_cap {
        PP_TRIALS_CAPPED.fetch_add(1, Ordering::Relaxed);
        ret.cleanup.cleanup();
        ret2.cleanup.cleanup();
        return Ok(PPTrialOutcome {
            was_capped: true,
            ..Default::default()
        });
    }

    if ret.solutions != ret2.solutions {
        return Err(anyhow!(format!(
            "Parallel-preprocess solutions not equal in {} vs {}: seq={} par={}",
            ret.filename,
            ret2.filename,
            ret.solutions.count,
            ret2.solutions.count
        )));
    }

    PP_TRIALS.fetch_add(1, Ordering::Relaxed);
    let mut outcome = PPTrialOutcome::default();
    if let Some(rounds) = ret2.parallel_preprocess_rounds {
        // rounds >= 2 means the parallel path fired AND did meaningful
        // work (a round found prunings, another confirmed convergence).
        // rounds == 1 means parallel SAC ran but the instance was
        // already SAC-stable so nothing was pruned — still a valid run
        // through the parallel code path, but doesn't tell us anything
        // about the merge / cross-worker prune handling.
        if rounds >= 2 {
            PP_TRIALS_WITH_ROUNDS.fetch_add(1, Ordering::Relaxed);
            outcome.had_rounds_ge2 = true;
        }
    }
    if let Some(prunings) = ret2.parallel_preprocess_prunings {
        PP_TOTAL_PRUNINGS.fetch_add(prunings, Ordering::Relaxed);
    }

    ret.cleanup.cleanup();
    ret2.cleanup.cleanup();

    Ok(outcome)
}

/// Exercise the restart-search code path (`restartNewSearchManager.h`).
///
/// `-restarts` forces `sollimit = 1` and is therefore incompatible
/// with the option-sweep machinery (which always passes
/// `-findallsols`). Instead, we run a dedicated test:
///   - baseline: `-findallsols` collects the full solution set;
///   - restart:  `-restarts -sollimit 1` finds at most one solution.
///
/// Soundness invariant: the restart run finds exactly one solution
/// and that solution lies in the baseline set.
///
/// Skipped on UNSAT instances: `-restarts` has no nogood learning,
/// so the restart strategy loops indefinitely on UNSAT instances
/// without making progress (it just keeps re-exploring with a
/// different seed). Restart search isn't designed for proving
/// infeasibility — it's a SAT-style "find one solution fast"
/// heuristic. We test only the SAT case.
///
/// `extra_restart_args` lets the caller cycle the restart-tuning
/// flags (`-restarts-multiplier`, `-no-restarts-bias`); empty slice
/// runs the bare `-restarts` configuration.
pub fn test_constraint_restart(
    config: &MinionConfig,
    c: &constraint_def::ConstraintDef,
    extra_restart_args: &[&str],
) -> Result<()> {
    let instance = constraint_def::build_random_instance(c);

    // Restart needs membership testing (does the restart's single
    // solution lie in the baseline set?), so the baseline opts into
    // raw_solutions storage. The restart itself uses -sollimit 1 so
    // its single-row Vec is stored regardless of digest mode.
    let baseline =
        run_solve_keep_full(config, &["-findallsols"], &instance, "restart_baseline")?;
    if baseline.hit_solution_cap || baseline.solutions.count == 0 {
        baseline.cleanup.cleanup();
        return Ok(());
    }

    let mut args: Vec<&str> = vec!["-restarts", "-sollimit", "1"];
    args.extend_from_slice(extra_restart_args);
    let restart = run_solve_keep_full(config, &args, &instance, "restart")?;

    let restart_sols = restart
        .raw_solutions
        .as_ref()
        .expect("run_solve_keep_full populates raw_solutions");
    if restart_sols.len() != 1 {
        return Err(anyhow!(format!(
            "Restart found {} solutions, expected 1 (baseline has {}, in {} vs {})",
            restart_sols.len(),
            baseline.solutions.count,
            baseline.filename,
            restart.filename
        )));
    }
    let found = &restart_sols[0];
    let baseline_sols = baseline
        .raw_solutions
        .as_ref()
        .expect("run_solve_keep_full populates raw_solutions");
    if !baseline_sols.iter().any(|s| s == found) {
        return Err(anyhow!(format!(
            "Restart solution {:?} not in baseline ({} sols) ({} vs {})",
            found,
            baseline.solutions.count,
            baseline.filename,
            restart.filename
        )));
    }

    baseline.cleanup.cleanup();
    restart.cleanup.cleanup();
    Ok(())
}

/// Run the same random instance under every propagator variant in an
/// equivalence group and verify all variants produce the same
/// solution set. A mismatch is a real propagator bug in one of the
/// group members — solution sets are an algorithm-independent
/// invariant.
///
/// `variants[0]` is the representative whose `ConstraintDef` is used
/// to generate the instance; its argument types must be strict
/// enough that every other variant accepts them (e.g. Discrete vars
/// for the alldiff group, since `gacalldiff` rejects Bound).
///
/// Skipped silently when any run hits the `hit_solution_cap` — partial
/// prefixes can disagree even between two correct propagators.
pub fn test_constraint_variant_equivalence(config: &MinionConfig, variants: &[&str]) -> Result<()> {
    if variants.len() < 2 {
        return Err(anyhow!(
            "equivalence group needs >=2 members, got {:?}",
            variants
        ));
    }
    let representative = constraint_def::find_constraint_def(variants[0]).ok_or_else(|| {
        anyhow!(
            "unknown representative constraint {:?} in equivalence group",
            variants[0]
        )
    })?;

    let instance = constraint_def::build_random_instance(representative);
    let mut runs: Vec<MinionOutput> = Vec::with_capacity(variants.len());

    for &name in variants {
        let mut variant_instance = instance.clone();
        variant_instance.constraint.name = name.to_string();
        let ret = run_solve(config, &["-findallsols"], &variant_instance, name)?;
        runs.push(ret);
    }

    if runs.iter().any(|r| r.hit_solution_cap) {
        for r in runs {
            r.cleanup.cleanup();
        }
        return Ok(());
    }

    for i in 1..runs.len() {
        if runs[i].solutions != runs[0].solutions {
            let ref_sample = &runs[0].solutions.sample;
            let var_sample = &runs[i].solutions.sample;
            let n = ref_sample.len().min(5);
            let m = var_sample.len().min(5);
            return Err(anyhow!(format!(
                "Propagator {} disagrees with {}: \
                 ref={}×{:?} variant={}×{:?} (files {} vs {})",
                variants[i],
                variants[0],
                runs[0].solutions.count,
                &ref_sample[..n],
                runs[i].solutions.count,
                &var_sample[..m],
                runs[0].filename,
                runs[i].filename,
            )));
        }
    }

    for r in runs {
        r.cleanup.cleanup();
    }
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
    if ret.hit_solution_cap || ret2.hit_solution_cap {
        ret.cleanup.cleanup();
        ret2.cleanup.cleanup();
        return Ok(());
    }
    if ret.solutions != ret2.solutions {
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

    // Pin a seed so baseline and injected runs agree under dynamic /
    // randomised orderings. Under the default Static + Ascend it's
    // redundant (order is state-free) but cheap.
    let seed: u32 = rand::random();

    let baseline = crate::run_minion_lib::get_minion_solutions_in_process(
        &instance,
        true,
        config.run_options(seed),
        "baseline",
        config.max_solutions,
        true, // keep_full_solutions: mid-search needs indexed access
    )?;

    if baseline.solutions.count as usize <= inject_after {
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
        config.run_options(seed),
        "midsearch",
    )?;

    let baseline_sols = baseline.raw_sols();

    if !got.injected {
        return Err(anyhow!(
            "midsearch: expected injection to fire at count {} (baseline has {} solutions)",
            inject_after,
            baseline_sols.len()
        ));
    }

    // Pre-injection must equal baseline[..inject_after] exactly.
    let expected_pre = &baseline_sols[..inject_after];
    if got.pre_injection.as_slice() != expected_pre {
        return Err(anyhow!(
            "midsearch: pre-injection differs from baseline prefix ({} vs {} rows)",
            got.pre_injection.len(),
            expected_pre.len()
        ));
    }

    // Post-injection: one row per remaining baseline leaf, in the same order.
    let remaining = &baseline_sols[inject_after..];
    if got.post_injection.len() != remaining.len() {
        if std::env::var("TESTER_DEBUG").is_ok() {
            eprintln!("baseline: {:?}", baseline_sols);
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
    let decision_len = baseline_sols.first().map(|r| r.len()).unwrap_or(0);
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
                minion_sys::ast::VarDomain::Bound(lo, hi) => v >= *lo as i64 && v <= *hi as i64,
                minion_sys::ast::VarDomain::Discrete(lo, hi) => v >= *lo as i64 && v <= *hi as i64,
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
/// Wrap a simple constraint def in a random nested parent chosen from
/// `reify`, `reifyimply`, `watched-and`, `watched-or`, returning a
/// `ConstraintInstance` whose top-level is the parent and whose
/// child(ren) use `inner_def` (plus a random other leaf for two-child
/// parents).
fn wrap_in_random_nested(
    inner_def: &constraint_def::ConstraintDef,
) -> constraint_def::ConstraintInstance {
    let mut rng = rand::thread_rng();
    // Fixed shortlist of parents we want to exercise here. Picked
    // explicitly (not from NESTED_CONSTRAINT_LIST) so we don't
    // accidentally pull in forwardchecking / check[gsa] / check[assign]
    // which aren't the focus of this flag.
    let wrappers: &[&str] = &["reify", "reifyimply", "watched-and", "watched-or"];
    let wname = wrappers.choose(&mut rng).unwrap();
    let parent_def = constraint_def::NESTED_CONSTRAINT_LIST
        .iter()
        .find(|d| &d.name.as_str() == wname)
        .expect("nested parent not in NESTED_CONSTRAINT_LIST");

    // Count the number of Constraint args the parent expects.
    let n_children = parent_def
        .arg
        .iter()
        .filter(|a| matches!(a, constraint_def::Arg::Constraint))
        .count();

    // First child is the iterated leaf; any additional children are
    // random picks from the main constraint list.
    let mut children: Vec<&constraint_def::ConstraintDef> = vec![inner_def];
    for _ in 1..n_children {
        let extra = constraint_def::CONSTRAINT_LIST
            .choose(&mut rng)
            .expect("CONSTRAINT_LIST empty");
        children.push(extra);
    }
    constraint_def::build_random_instance_with_children(parent_def, &children)
}

pub fn test_constraint_midsearch_inject_constraints(
    config: &MinionConfig,
    base_defs: &[&constraint_def::ConstraintDef],
    inject_defs: &[&constraint_def::ConstraintDef],
    inject_after_per_packet: &[usize],
    wrap_nested: bool,
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
        .map(|d| {
            if wrap_nested {
                wrap_in_random_nested(d)
            } else {
                constraint_def::build_random_instance(d)
            }
        })
        .collect();
    let base_refs: Vec<&constraint_def::ConstraintInstance> = base_instances.iter().collect();
    let inject_refs: Vec<&constraint_def::ConstraintInstance> = inject_instances.iter().collect();

    let log_id = || -> String {
        format!(
            "inject={:?} base={:?} schedule={:?} seed={seed:#x}",
            inject_defs
                .iter()
                .map(|d| d.name.as_str())
                .collect::<Vec<_>>(),
            base_defs
                .iter()
                .map(|d| d.name.as_str())
                .collect::<Vec<_>>(),
            inject_after_per_packet,
        )
    };

    let last_a = *inject_after_per_packet.last().unwrap();

    // Cap per-run solution count so a cartesian-exploded baseline (or
    // F_k) can't eat tens of GB. If any run hits the cap, we can't
    // verify the invariant against a complete set, so we skip the
    // trial. `config.maxtuples` is the shared knob with the exec-mode
    // tester's tableise limit.
    let cap = Some(config.maxtuples);

    let options = config.run_options(seed);

    // Baseline: no packet constraints, but their vars are declared.
    let baseline =
        crate::run_minion_lib::run_multi(&base_refs, &inject_refs, options, cap, "baseline")
            .map_err(|e| anyhow!("{}: baseline: {e}", log_id()))?;

    if baseline.stopped_at_limit {
        // Baseline would have been enormous; skip this trial.
        return Ok(());
    }

    if baseline.solutions.len() < last_a {
        // Can't fire every injection; trivial trial.
        return Ok(());
    }

    // Actual run: all packets injected at their scheduled callback counts.
    let injections: Vec<(usize, crate::run_minion_lib::InjectionPacket<'_>)> =
        inject_after_per_packet
            .iter()
            .zip(inject_instances.iter())
            .map(|(&a, inst)| {
                (
                    a,
                    crate::run_minion_lib::InjectionPacket::ExistingVarsConstraint(inst),
                )
            })
            .collect();
    let actual = crate::run_minion_lib::run_multi_injected(
        &base_refs,
        &inject_refs,
        &injections,
        options,
        cap,
        "actual",
    )
    .map_err(|e| anyhow!("{}: actual: {e}", log_id()))?;

    if actual.stopped_at_limit {
        return Ok(());
    }

    if actual.injections_fired != n {
        // A packet's pruning ended search before the next injection point.
        // Not a test failure; just a degenerate trial.
        return Ok(());
    }

    // Reference runs F_k for k=1..N: base + packets[..k] in the model.
    let mut full_runs: Vec<crate::run_minion_lib::CappedOutput> = Vec::with_capacity(n);
    for k in 0..n {
        let mut with_c: Vec<&constraint_def::ConstraintInstance> = base_refs.clone();
        with_c.extend(inject_refs[..=k].iter().copied());
        let vars_only_k: Vec<&constraint_def::ConstraintInstance> =
            inject_refs[k + 1..].iter().copied().collect();
        let f_k = crate::run_minion_lib::run_multi(
            &with_c,
            &vars_only_k,
            options,
            cap,
            &format!("F_{}", k + 1),
        )
        .map_err(|e| anyhow!("{}: F_{}: {e}", log_id(), k + 1))?;
        if f_k.stopped_at_limit {
            return Ok(());
        }
        full_runs.push(f_k);
    }

    use std::collections::HashSet;

    if config.midsearch_slice_exact() {
        // Static + non-random ordering AND default preprocess/
        // prop-node: the search tree is fixed and the initial state
        // is identical across baseline / actual / F_k, so
        // "actual[a_lo..a_hi] is F_k minus what we've already seen,
        // in F_k's order" holds exactly. Keep the strong slice check.
        let mut expected: Vec<Vec<i64>> = Vec::with_capacity(actual.solutions.len());

        // Segment 0: [0, A_1) from baseline.
        expected.extend(
            baseline.solutions[..inject_after_per_packet[0]]
                .iter()
                .cloned(),
        );

        if actual.solutions[..inject_after_per_packet[0]] != expected[..] {
            return Err(anyhow!("{}: segment 0 (baseline prefix) differs", log_id()));
        }

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
    } else {
        // Under state-dependent / randomised orderings, actual's
        // search tree diverges from F_k's after any injection (their
        // propagator states differ — actual reaches each post-A_k
        // leaf from a very different history), so per-segment slice
        // equality is not an invariant. What still holds with a
        // shared seed:
        //
        //   (a) actual[..A_1] == baseline[..A_1] exact slice equality.
        //       Before any packet fires the model, seed and ordering
        //       state are byte-for-byte identical in both runs.
        //   (b) Segment containment: for each row actual[i], let
        //       seg(i) = largest k such that A_k ≤ i (with A_0 := 0,
        //       so seg(0) = 0 when A_1 > 0). Then
        //         actual[i] ∈ set(F_{seg(i)})   (F_0 := baseline).
        //       Rationale: actual's model at the moment it reported
        //       row i is exactly base + P_1..P_{seg(i)} = F_{seg(i)}.
        //   (c) Bracket: set(F_N) ⊆ set(actual) ⊆ set(baseline). Every
        //       F_N solution has to appear — DFS reports each reachable
        //       leaf exactly once, and a leaf that satisfies all
        //       packets is still a leaf once any subset is active. And
        //       actual's rows are all reachable under some F_k, all of
        //       which are subsets of baseline.
        //   (d) No duplicates.
        //
        // These are strictly weaker than the static-ordering slice
        // invariant, but they still catch "actual invented a row that
        // shouldn't exist" and "F_N was missed" bugs.
        let a1 = inject_after_per_packet[0];
        if actual.solutions[..a1] != baseline.solutions[..a1] {
            if std::env::var("TESTER_DEBUG").is_ok() {
                eprintln!("baseline[..{a1}] = {:?}", &baseline.solutions[..a1]);
                eprintln!("actual[..{a1}]   = {:?}", &actual.solutions[..a1]);
            }
            return Err(anyhow!(
                "{}: pre-first-injection prefix differs from baseline",
                log_id()
            ));
        }

        // (b): segment containment.
        let fk_sets: Vec<HashSet<&Vec<i64>>> = (0..n)
            .map(|k| full_runs[k].solutions.iter().collect())
            .collect();
        let baseline_set: HashSet<&Vec<i64>> = baseline.solutions.iter().collect();
        for (idx, row) in actual.solutions.iter().enumerate() {
            // seg(idx) = number of schedule points ≤ idx.
            let seg = inject_after_per_packet
                .iter()
                .take_while(|&&a| a <= idx)
                .count();
            let in_model: bool = if seg == 0 {
                baseline_set.contains(row)
            } else {
                fk_sets[seg - 1].contains(row)
            };
            if !in_model {
                return Err(anyhow!(
                    "{}: row {} (segment {}) not in F_{} solution set",
                    log_id(),
                    idx,
                    seg,
                    seg
                ));
            }
        }

        // (c): F_N ⊆ actual ⊆ baseline. actual ⊆ baseline already
        // follows from (b) since every F_k is a subset of baseline —
        // so only the F_N ⊆ actual side is a new check.
        let actual_set: HashSet<&Vec<i64>> = actual.solutions.iter().collect();
        for row in &full_runs[n - 1].solutions {
            if !actual_set.contains(row) {
                return Err(anyhow!(
                    "{}: F_{} solution {:?} missing from actual",
                    log_id(),
                    n,
                    row
                ));
            }
        }

        // (d): no duplicates.
        if actual_set.len() != actual.solutions.len() {
            return Err(anyhow!(
                "{}: actual contains duplicates ({} rows, {} distinct)",
                log_id(),
                actual.solutions.len(),
                actual_set.len(),
            ));
        }
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
        false,
        seed,
    )
}

/// Mid-search test: each packet adds one Bool variable via
/// `minion_newVarMidsearch` (aux-block) and a `DisEq(new_var, X)`
/// constraint where `X` is a randomly-chosen non-constant base
/// variable — so the injected constraint mixes newly-added and
/// pre-declared variables.
///
/// Aux semantics mean we can't do an exact `expected == actual` check
/// (F_k built with those vars as decisions has many more rows), so we
/// verify the weaker invariants:
///
/// * `actual[..A_1]` (pre-any-injection rows) equals `baseline[..A_1]`
///   exactly — same columns, same order.
/// * each post-injection row has `|base_vars| + k` columns where `k`
///   is the number of packets that have fired by then.
/// * each post-injection row's base-vars projection is a solution of
///   the baseline (every row's base part corresponds to some leaf
///   that baseline would have visited).
/// * each active packet's `DisEq(new_var, paired_base_var)` holds on
///   every row whose packet has fired.
/// * rows are distinct.
/// * `actual.solutions.len() <= baseline.solutions.len()` (aux
///   produces at most one post-injection row per remaining decision
///   leaf).
///
/// Skipped when any run hits `config.maxtuples`, when baseline has
/// fewer rows than there are packets, when the base model has no
/// non-constant variables to pair against, or when an injection
/// pruned away all remaining branches before the next schedule point.
pub fn test_midsearch_add_new_vars_with_constraint(
    config: &MinionConfig,
    base_defs: &[&constraint_def::ConstraintDef],
    n_packets: usize,
    seed: u32,
) -> Result<()> {
    if config.backend == Backend::Exec {
        return Ok(());
    }
    if n_packets == 0 {
        return Ok(());
    }

    let base_instances: Vec<constraint_def::ConstraintInstance> = base_defs
        .iter()
        .map(|d| constraint_def::build_random_instance(d))
        .collect();
    let base_refs: Vec<&constraint_def::ConstraintInstance> = base_instances.iter().collect();

    // Pool of pre-declared non-constant base-var names to pair
    // DisEq against.
    let base_var_names: Vec<String> = base_instances
        .iter()
        .flat_map(|inst| {
            inst.vars()
                .iter()
                .flatten()
                .filter(|v| v.var_type != constraint_def::VarType::Constant)
                .map(|v| v.name.clone())
                .collect::<Vec<_>>()
        })
        .collect();
    if base_var_names.is_empty() {
        return Ok(());
    }

    let log_id = || -> String {
        format!(
            "mode=add-vars base={:?} n_packets={n_packets} seed={seed:#x}",
            base_defs
                .iter()
                .map(|d| d.name.as_str())
                .collect::<Vec<_>>(),
        )
    };

    // Build packets: each one adds 1 bool var and DisEq(new_var, base_var).
    let mut rng = rand::thread_rng();
    // Remember each packet's (new_var_name, paired_base_var_name) for
    // constraint checking.
    let mut packet_pairs: Vec<(String, String)> = Vec::with_capacity(n_packets);
    let mut injections: Vec<(usize, crate::run_minion_lib::InjectionPacket<'_>)> =
        Vec::with_capacity(n_packets);
    for k in 0..n_packets {
        let new_var = format!("midv_{}", crate::counter::get_unique_value());
        let paired_base = base_var_names.choose(&mut rng).unwrap().clone();
        let constraint = minion_sys::ast::Constraint::DisEq(
            minion_sys::ast::Var::NameRef(new_var.clone()),
            minion_sys::ast::Var::NameRef(paired_base.clone()),
        );
        packet_pairs.push((new_var.clone(), paired_base));
        injections.push((
            k + 1,
            crate::run_minion_lib::InjectionPacket::AddVarsAndConstraint {
                new_vars: vec![(new_var, minion_sys::ast::VarDomain::Bool)],
                constraint,
            },
        ));
    }

    let cap = Some(config.maxtuples);
    let options = config.run_options(seed);

    // Baseline: only base constraints, no packets.
    let baseline =
        crate::run_minion_lib::run_multi(&base_refs, &[], options, cap, "add-vars-baseline")
            .map_err(|e| anyhow!("{}: baseline: {e}", log_id()))?;
    if baseline.stopped_at_limit {
        return Ok(());
    }
    if baseline.solutions.len() < n_packets {
        return Ok(());
    }

    let actual = crate::run_minion_lib::run_multi_injected(
        &base_refs,
        &[],
        &injections,
        options,
        cap,
        "add-vars-actual",
    )
    .map_err(|e| anyhow!("{}: actual: {e}", log_id()))?;
    if actual.stopped_at_limit {
        return Ok(());
    }
    if actual.injections_fired != n_packets {
        return Ok(());
    }

    let base_var_count = baseline.solutions.first().map(|r| r.len()).unwrap_or(0);

    // Invariant 1: actual[..A_1] == baseline[..A_1] exactly.
    // A_1 = 1, so that's just actual[0] == baseline[0] with
    // base_var_count columns.
    let a1 = injections[0].0;
    for i in 0..a1 {
        if actual.solutions[i].len() != base_var_count
            || actual.solutions[i] != baseline.solutions[i]
        {
            if std::env::var("TESTER_DEBUG").is_ok() {
                eprintln!("baseline[..{a1}] = {:?}", &baseline.solutions[..a1]);
                eprintln!("actual[..{a1}]   = {:?}", &actual.solutions[..a1]);
            }
            return Err(anyhow!(
                "{}: pre-injection row {} differs from baseline",
                log_id(),
                i
            ));
        }
    }

    // Map var names to column indices in actual's final_variable_order.
    let col_of: std::collections::HashMap<&str, usize> = actual
        .final_variable_order
        .iter()
        .enumerate()
        .map(|(i, n)| (n.as_str(), i))
        .collect();

    // Baseline base-rows as a lookup set.
    let baseline_base_rows: std::collections::HashSet<&Vec<i64>> =
        baseline.solutions.iter().collect();

    let mut seen_actual: std::collections::HashSet<Vec<i64>> = std::collections::HashSet::new();

    for (idx, row) in actual.solutions.iter().enumerate() {
        // Number of packets fired by (and including) this row:
        // packet k (1-based count A_k = k) fires in the callback for
        // the A_k-th solution. So row at 0-based `idx` follows the
        // `idx+1`-th callback. Packet k is active at row `idx` iff
        // A_k <= idx (0-based): A_k is the callback count post-which
        // the constraint is live. With A_k = k (1-based packet
        // number), packet k is active when idx >= k. Number of active
        // packets at row idx = min(idx, n_packets) — idx==0 gives 0
        // active, idx==1 gives 1 active, etc.
        let active = idx.min(n_packets);

        // Column-count invariant.
        let expected_cols = base_var_count + active;
        if row.len() != expected_cols {
            return Err(anyhow!(
                "{}: row {} has {} cols, expected {} (base {} + {} active packets)",
                log_id(),
                idx,
                row.len(),
                expected_cols,
                base_var_count,
                active
            ));
        }

        if idx < a1 {
            // Pre-injection rows already checked above.
            continue;
        }

        // Base-vars projection is in baseline's solution set.
        let base_proj: Vec<i64> = row[..base_var_count].to_vec();
        if !baseline_base_rows.contains(&base_proj) {
            return Err(anyhow!(
                "{}: row {} base projection {:?} not in baseline",
                log_id(),
                idx,
                base_proj
            ));
        }

        // All active DisEq packets hold.
        for k in 0..active {
            let (new_var_name, paired_base_name) = &packet_pairs[k];
            let new_col = *col_of.get(new_var_name.as_str()).ok_or_else(|| {
                anyhow!(
                    "{}: row {}: new var {} not in final_variable_order",
                    log_id(),
                    idx,
                    new_var_name
                )
            })?;
            let paired_col = *col_of.get(paired_base_name.as_str()).ok_or_else(|| {
                anyhow!(
                    "{}: row {}: paired base var {} not in final_variable_order",
                    log_id(),
                    idx,
                    paired_base_name
                )
            })?;
            if new_col >= row.len() || paired_col >= row.len() {
                return Err(anyhow!(
                    "{}: row {} packet {} column out of bounds",
                    log_id(),
                    idx,
                    k
                ));
            }
            if row[new_col] == row[paired_col] {
                return Err(anyhow!(
                    "{}: row {} packet {} DisEq violated: {}={} == {}={}",
                    log_id(),
                    idx,
                    k,
                    new_var_name,
                    row[new_col],
                    paired_base_name,
                    row[paired_col]
                ));
            }
        }

        // Distinctness.
        if !seen_actual.insert(row.clone()) {
            return Err(anyhow!("{}: row {} duplicate", log_id(), idx));
        }
    }

    // Upper-bound invariant: aux gives at most one post-injection row
    // per remaining decision leaf, so `actual` can't be longer than
    // `baseline`.
    if actual.solutions.len() > baseline.solutions.len() {
        return Err(anyhow!(
            "{}: actual has {} rows but baseline has only {}",
            log_id(),
            actual.solutions.len(),
            baseline.solutions.len()
        ));
    }

    Ok(())
}

/// Depth-N cross-type nesting test. Builds a random parent tree of
/// depth `depth` with `c` at the bottom of one branch
/// (`build_random_nested_tree`), tableises, and checks
/// original-vs-tableised. `depth == 1` matches
/// `test_constraint_nested`; `depth >= 2` is the new coverage.
///
/// Uses the same tableise-loop as the other nested tests so a tree
/// that exceeds `config.maxtuples` is just re-rolled rather than
/// failing the trial.
pub fn test_constraint_deep_nested(
    config: &MinionConfig,
    c: &constraint_def::ConstraintDef,
    depth: usize,
) -> Result<()> {
    let mut instance;
    let tups;
    let mut attempts = 0;
    loop {
        instance = constraint_def::build_random_nested_tree(c, depth);
        let tupstry = instance.tableise(config.maxtuples);
        if let Some(t) = tupstry {
            tups = t;
            break;
        }
        attempts += 1;
        if attempts > 100 {
            return Ok(());
        }
    }

    let ret = run_solve(config, &["-findallsols"], &instance, "original")?;
    let ret2 = run_solve(config, &["-findallsols"], &tups, "tuples")?;
    if ret.hit_solution_cap || ret2.hit_solution_cap {
        ret.cleanup.cleanup();
        ret2.cleanup.cleanup();
        return Ok(());
    }
    // Multiset digest comparison — order-independent regardless of
    // ordering heuristic, so the deterministic vs randomised paths
    // collapse into one.
    if ret.solutions != ret2.solutions {
        return Err(anyhow!(format!(
            "Solutions not equal in {} vs {} (deep nest depth={depth})",
            ret.filename, ret2.filename
        )));
    }
    // GAC node-count check intentionally skipped here: the `gac` flag
    // on the top-level parent in NESTED_CONSTRAINT_LIST is set to
    // `false` for every wrapper, so the assertion would never fire
    // even with a GAC-propagated leaf.

    ret.cleanup.cleanup();
    ret2.cleanup.cleanup();
    Ok(())
}

pub fn test_constraint_nested(
    config: &MinionConfig,
    c: &constraint_def::ConstraintDef,
) -> Result<()> {
    let mut rng = rand::thread_rng();
    let nest_type = constraint_def::NESTED_CONSTRAINT_LIST
        .choose(&mut rng)
        .unwrap();
    // Build enough children for the parent's Constraint-arg slots:
    // the iterated constraint `c` goes in the first slot; any extra
    // slots (e.g. watched-and / watched-or with two children) get
    // random picks from the leaf constraint list.
    let n_children = nest_type
        .arg
        .iter()
        .filter(|a| matches!(a, constraint_def::Arg::Constraint))
        .count();
    let mut children: Vec<&constraint_def::ConstraintDef> = vec![c];
    for _ in 1..n_children {
        let extra = constraint_def::CONSTRAINT_LIST
            .choose(&mut rng)
            .expect("CONSTRAINT_LIST empty");
        children.push(extra);
    }

    let mut instance;
    let tups;
    let mut attempts = 0;
    loop {
        instance = constraint_def::build_random_instance_with_children(nest_type, &children);
        let tupstry = instance.tableise(config.maxtuples);
        if let Some(t) = tupstry {
            tups = t;
            break;
        }
        attempts += 1;
        if attempts > 100 {
            return Ok(());
        }
    }

    let ret = run_solve(config, &["-findallsols"], &instance, "original")?;
    let ret2 = run_solve(config, &["-findallsols"], &tups, "tuples")?;
    if ret.hit_solution_cap || ret2.hit_solution_cap {
        ret.cleanup.cleanup();
        ret2.cleanup.cleanup();
        return Ok(());
    }
    if ret.solutions != ret2.solutions {
        let name = &c.name;
        let n = ret.solutions.sample.len().min(10);
        let m = ret2.solutions.sample.len().min(10);
        return Err(anyhow!(format!(
            "Nested solutions not equal in {}/{} ({name}): orig={}×{:?} tup={}×{:?}",
            ret.filename,
            ret2.filename,
            ret.solutions.count,
            &ret.solutions.sample[..n],
            ret2.solutions.count,
            &ret2.solutions.sample[..m],
        )));
    }
    if instance.constraint.gac && config.deterministic_ordering() && ret.nodes != ret2.nodes {
        return Err(anyhow!(format!(
            "Propagator should be GAC, but node counts not equal in {} vs {}",
            ret.filename, ret2.filename
        )));
    }

    ret.cleanup.cleanup();
    ret2.cleanup.cleanup();
    Ok(())
}

/// Metamorphic optimisation sweep: build a random constraint instance,
/// wrap it with `aux = sum(real_vars)` plus `MINIMISING`/`MAXIMISING aux`,
/// and verify every strategy in a fixed set reports the same optimum.
///
/// The premise: we already trust the constraint propagators for
/// satisfaction (the existing tableisation sweep covers that broadly).
/// Optimisation bugs therefore live mostly in *optimisation-specific*
/// code — bound tracking, parallel bound broadcast, restart-with-
/// optimisation interaction. Different propagator/heuristic/parallel
/// combinations stress that surface differently, so disagreement
/// across them is a strong signal of a bound-tracking bug.
///
/// Skips silently when:
///   - backend is in-process (FFI doesn't yet expose optimisation);
///   - instance has no real (non-constant) variables (no objective);
///   - aux range is degenerate (all real vars constant);
///   - any strategy hits the per-trial node limit (incomplete run —
///     "best so far" can't be metamorphic-compared with "proven
///     optimal").
///
/// Fails when complete runs disagree on the optimum value (bug).
pub fn test_constraint_optimisation(
    config: &MinionConfig,
    c: &constraint_def::ConstraintDef,
) -> Result<()> {
    let instance = constraint_def::build_random_instance(c);

    // Flatten and pick out the real (non-constant) variables.
    // Constants contribute a fixed offset to the sum but no slack to
    // optimise over, so dropping them is fine.
    let flat: Vec<_> = instance
        .vars()
        .concat()
        .into_iter()
        .filter(|v| v.var_type != constraint_def::VarType::Constant)
        .collect();
    if flat.is_empty() {
        return Ok(());
    }

    let mut sum_min: i64 = 0;
    let mut sum_max: i64 = 0;
    for v in &flat {
        let lo = *v.domain.first().unwrap();
        let hi = *v.domain.last().unwrap();
        sum_min += lo;
        sum_max += hi;
    }
    if sum_min >= sum_max {
        // No slack to optimise over — every assignment yields the
        // same aux value, so the test wouldn't shake out propagator
        // bugs.
        return Ok(());
    }

    let mut rng = rand::thread_rng();
    let minimise: bool = rng.gen();

    // Aux name uses a `__opt_` prefix that can't collide with any
    // generated variable name (those use `var_*`, `tup_*`, etc. via
    // get_unique_name).
    let aux_name = format!("__opt_aux_{}", crate::counter::get_unique_value());
    let names: Vec<String> = flat.iter().map(|v| v.name.clone()).collect();
    let wrapper = OptimisationWrapper {
        aux_name: &aux_name,
        aux_min: sum_min,
        aux_max: sum_max,
        minimise,
        sum_var_names: names,
    };

    // Per-trial node cap. Big enough that small random instances
    // complete (typical needs <1k nodes), small enough that a
    // pathological roll doesn't eat tens of seconds. If any strategy
    // hits this we abandon the trial — comparing partial results
    // would manufacture false bugs.
    const NODE_LIMIT: i64 = 100_000;
    let nl_str = NODE_LIMIT.to_string();

    // Strategy set: combinations chosen to hit different code paths
    // in the optimisation flow. `-restarts` is in: each restart
    // attempt is a complete depth-first search under the running
    // best bound, so the optimum found must agree with all the
    // other strategies.
    //
    // The in-process backend can only express a subset (the FFI
    // doesn't expose `-restarts`, parallel modes, prop_node levels,
    // or `-nodelimit`). It picks up the in-process flavour of the
    // exec sweep via `run_with_options`-style varorder/valorder/
    // preprocess overrides — see the dispatch below.
    let exec_strategies: [(&str, &[&str]); 6] = [
        ("baseline", &[]),
        ("preprocess-SAC", &["-preprocess", "SAC"]),
        ("var-sdf-val-desc", &["-varorder", "sdf", "-valorder", "descend"]),
        ("worksteal-4", &["-X-parallelWorkSteal", "4"]),
        ("threads-4", &["-X-parallelThreads", "4"]),
        ("restarts", &["-restarts", "-restarts-multiplier", "1.5"]),
    ];
    let in_process_strategies: [(&str, minion_sys::VarOrder, minion_sys::ValOrder,
                                 minion_sys::PropLevel); 4] = [
        ("baseline",
         minion_sys::VarOrder::Static, minion_sys::ValOrder::Ascend,
         minion_sys::PropLevel::None),
        ("preprocess-SAC",
         minion_sys::VarOrder::Static, minion_sys::ValOrder::Ascend,
         minion_sys::PropLevel::SAC),
        ("preprocess-SACBounds",
         minion_sys::VarOrder::Static, minion_sys::ValOrder::Ascend,
         minion_sys::PropLevel::SACBounds),
        ("var-sdf-val-desc",
         minion_sys::VarOrder::SDF, minion_sys::ValOrder::Descend,
         minion_sys::PropLevel::None),
    ];

    let mut results: Vec<(String, MinionOutput)> = Vec::new();
    if config.backend == Backend::Exec {
        for (name, args) in &exec_strategies {
            let mut full: Vec<&str> = (*args).to_vec();
            full.push("-nodelimit");
            full.push(&nl_str);

            // max_solutions=0 disables the solution cap. Optimisation
            // runs emit one solution per non-dominated improvement;
            // with bounded-domain random instances the count is small
            // (tens at most), so the cap would never fire usefully and
            // we'd lose the "complete run" signal.
            let ret = run_minion::get_minion_solutions_optimisation(
                config.minionexec,
                &config.minionargs,
                &full,
                &instance,
                &format!("opt_{}", name),
                0,
                &wrapper,
            )?;
            results.push(((*name).to_string(), ret));
        }
    } else {
        // In-process: smaller strategy set (no -restarts, no parallel
        // modes via FFI, no -nodelimit). Still useful — covers the
        // FFI optimisation plumbing under different propagator
        // settings, and any disagreement vs the Model::optimise
        // contract surfaces immediately.
        for (name, vo, valo, pp) in &in_process_strategies {
            let opts = minion_sys::RunOptions {
                seed: None,
                var_order: *vo,
                val_order: *valo,
                preprocess: minion_sys::Propagation { level: *pp, limit: false },
                prop_node: minion_sys::Propagation {
                    level: minion_sys::PropLevel::GAC,
                    limit: false,
                },
            };
            let ret = run_minion_lib::get_minion_solutions_in_process_optimisation(
                &instance,
                opts,
                &format!("opt_{}", name),
                &wrapper,
            )?;
            results.push(((*name).to_string(), ret));
        }
    }

    // If any run hit the node cap, abandon the trial. We can't tell
    // whether divergent "best so far" values are bugs or just
    // different cut-off points.
    //
    // Only meaningful for the exec strategies; the in-process flow
    // doesn't currently apply -nodelimit, but its small instances
    // typically complete in <1k nodes so this check is harmless.
    let any_capped = results.iter().any(|(_, r)| r.nodes >= NODE_LIMIT);
    if any_capped {
        for (_, r) in &results {
            r.cleanup.cleanup();
        }
        return Ok(());
    }

    // All runs are complete. They MUST agree on the optimum (or all
    // agree the problem is UNSAT, which surfaces as None).
    let baseline = results[0].1.optimum_value;
    for (name, r) in &results[1..] {
        if r.optimum_value != baseline {
            let dir = if minimise { "MIN" } else { "MAX" };
            let msg = format!(
                "optimisation disagreement on {} ({}): \
                 baseline ({}, {:?}) vs {} ({}, {:?})",
                c.name,
                dir,
                results[0].1.filename,
                baseline,
                name,
                r.filename,
                r.optimum_value,
            );
            // Leave .minion files in place for post-mortem.
            return Err(anyhow!(msg));
        }
    }

    for (_, r) in &results {
        r.cleanup.cleanup();
    }
    Ok(())
}
