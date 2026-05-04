#[macro_use]
extern crate lazy_static;

#[macro_use]
extern crate serde_derive;

use clap::Parser;
use rayon::prelude::*;

use anyhow::Context;
use anyhow::Result;

use rand::seq::SliceRandom;
use rand::{thread_rng, Rng};

mod constraint_def;
mod counter;
mod minion_instance;
mod run_minion;
mod run_minion_lib;
mod solution_digest;
mod test_types;

use test_types::Backend;

#[derive(clap::Parser, Debug)]
struct Opt {
    #[arg(long)]
    valgrind: bool,

    #[arg(long)]
    constraints: Vec<String>,

    #[arg(short, long, default_value_t = 30)]
    count: u64,

    #[arg(short, long, default_value_t = 1000)]
    optioncount: u64,

    #[arg(short, long, default_value = "")]
    minion: String,

    #[arg(short = 't', long, default_value_t = 10000)]
    maxtuples: usize,

    #[arg(short = 'n', long, default_value_t = 8)]
    numthreads: usize,

    /// Run minion via libminion (minion-sys) in-process instead of spawning
    /// the `minion` binary. Each test gets its own MinionContext; the rayon
    /// threadpool drives many independent solves in parallel, which exercises
    /// libminion's thread isolation between contexts.
    ///
    /// Incompatible with --valgrind and skips the parallel/option test
    /// sweeps (those are exec-mode features).
    #[arg(long)]
    in_process: bool,

    /// Run the mid-search variable-injection test instead of the usual
    /// baseline-vs-tableised sweep. Requires --in-process. Each constraint
    /// is solved once, then re-solved with a callback that injects
    /// --midsearch-new-vars fresh boolean variables after
    /// --midsearch-inject-after solutions; the resulting solution set is
    /// checked against the expected cross-product.
    #[arg(long)]
    midsearch: bool,

    #[arg(long, default_value_t = 1)]
    midsearch_inject_after: usize,

    #[arg(long, default_value_t = 1)]
    midsearch_new_vars: usize,

    /// Run the mid-search constraint-injection sweep: for every
    /// constraint in the built-in list, generate a random instance,
    /// mid-inject it into a random base problem, and verify the three
    /// solution sets (baseline / injected / full-from-start) agree.
    /// Requires --in-process.
    #[arg(long)]
    midsearch_constraints: bool,

    /// Number of random constraint instances used as the base problem
    /// in each constraint-injection trial.
    #[arg(long, default_value_t = 2)]
    midsearch_base_size: usize,

    /// Number of constraint packets injected during the same solve in
    /// each --midsearch-constraints trial. Packet `k` (1-based) is
    /// injected after `k` solutions have been reported. The first
    /// packet uses the constraint being iterated; the rest are picked
    /// uniformly at random from the full constraint list.
    #[arg(long, default_value_t = 1)]
    midsearch_constraints_num_packets: usize,

    /// Wrap each injected constraint in a random nested parent
    /// (reify / reifyimply / watched-and / watched-or), so what gets
    /// added mid-search is a ParentConstraint with the iterated leaf
    /// as its child. Exercises the mid-search handling of
    /// parent-constraint state (trigger activation, stage-1 vs stage-2
    /// setup) on top of the existing single-constraint verification.
    #[arg(long)]
    midsearch_wrap_nested: bool,

    /// Run the mid-search *added-variable* test sweep. Each packet
    /// adds one boolean variable via minion_newVarMidsearch and a
    /// DisEq constraint that mixes the new var with a randomly-chosen
    /// pre-declared base var. Requires --in-process; mutually
    /// exclusive with --midsearch-constraints and --midsearch.
    /// Verifies weaker invariants (constraint satisfaction, base-vars
    /// projection in baseline, distinctness, upper-bound count) since
    /// aux-block enumeration makes exact equality against an F_k
    /// reference run intractable.
    #[arg(long)]
    midsearch_add_vars: bool,

    /// Variable-ordering heuristic for the in-process backend. The
    /// default (`static`) matches Minion's text-parser default and is
    /// state-free, so baseline-vs-tableised and mid-search invariants
    /// hold as exact slice equality. Any other choice makes the
    /// ordering state-dependent and the tester falls back to
    /// set-based comparisons.
    #[arg(long, value_enum, default_value_t = VarOrderArg::Static)]
    var_order: VarOrderArg,

    /// Value-ordering heuristic for the in-process backend.
    /// `random` needs a seed — the tester pins one per trial —
    /// but even a pinned-seed random val order diverges the search
    /// tree between baseline and injected runs, so the tester uses
    /// set-based comparisons whenever this isn't `ascend` or
    /// `descend`.
    #[arg(long, value_enum, default_value_t = ValOrderArg::Ascend)]
    val_order: ValOrderArg,

    /// Preprocessing level: one-shot propagation before search
    /// begins. Mirrors exec-mode's `-preprocess <level>`. Default
    /// `none` matches minion's `SearchMethod` default.
    #[arg(long, value_enum, default_value_t = PropLevelArg::None)]
    preprocess: PropLevelArg,

    /// Cap preprocessing work with minion's `_limit` modifier
    /// (equivalent to exec-mode's `-preprocess SAC_limit` etc.).
    /// Meaningful only when `--preprocess` is non-default.
    #[arg(long)]
    preprocess_limit: bool,

    /// Propagation level applied at every search node. Mirrors
    /// exec-mode's `-prop-node <level>`. Default `gac` matches
    /// minion's default. `none` is not valid here.
    #[arg(long, value_enum, default_value_t = PropLevelArg::Gac)]
    prop_node: PropLevelArg,

    /// Cap per-node propagation work with minion's `_limit`
    /// modifier. Meaningful only when `--prop-node` is stronger
    /// than the default GAC.
    #[arg(long)]
    prop_node_limit: bool,

    /// Depth of the random parent tree used by the deep-nested
    /// sweep. `0` runs no deep-nested trials. `1` reproduces the
    /// existing `test_constraint_nested` shape (one parent wrapping
    /// the iterated leaf). `2`+ builds cross-type trees where each
    /// child subtree is itself `depth-1` random parents — exercises
    /// interactions between different parent types at multiple
    /// levels (e.g. `reify(watched-or(reifyimply(X), watched-and(Y,
    /// Z)))`). Trees whose tableisation exceeds `--maxtuples` are
    /// discarded and re-rolled, so larger depths just mean more
    /// rejected rolls rather than OOMs.
    #[arg(long, default_value_t = 0)]
    nest_depth: usize,

    /// Scales the size of randomly-generated test instances. `1` is
    /// the historical default — small instances that solve in
    /// microseconds. Higher values multiply integer-domain bounds,
    /// sublist target sizes, and list lengths linearly, so the
    /// search-tree size grows roughly polynomially. Use `--size-factor
    /// 4` or higher when you want trials that take long enough for
    /// work-stealing to actually exercise the donation path.
    ///
    /// Note: the work-steal pass also runs an *adaptive* growth loop
    /// per constraint that overrides this — see `--ws-max-size`.
    #[arg(long, default_value_t = 1)]
    size_factor: u32,

    /// Cap for the per-constraint adaptive growth in the work-steal
    /// pass. Each constraint starts at `--size-factor` (or 1) and
    /// doubles until at least one trial reports a non-zero donation
    /// count, or this cap is hit. Larger caps give donation coverage
    /// even for fast-to-solve constraints, at the cost of wall-clock
    /// per trial.
    #[arg(long, default_value_t = 32)]
    ws_max_size: u32,

    /// Hard cap on the number of solutions any single solver run
    /// will gather. Protects against memory blow-up when a scaled-up
    /// random instance has a huge solution space — without this, a
    /// wide `alldiff` at `--size-factor 4` can produce billions of
    /// solutions and OOM the process. Trials whose sequential or
    /// comparison run hits this cap are abandoned silently (treated
    /// as "instance too large at this size"). Set to 0 to disable
    /// the cap entirely.
    #[arg(long, default_value_t = 10_000_000)]
    max_solutions: i64,

    /// Trials per equivalence group in the propagator-variant sweep.
    /// Each trial generates one random instance for the group's
    /// representative and runs every variant in the group on it,
    /// asserting the solution sets agree. Defaults to `--count`.
    /// Set to 0 to skip the variant-equivalence sweep entirely.
    #[arg(long)]
    variant_count: Option<u64>,
}

#[derive(Clone, Copy, Debug, clap::ValueEnum)]
enum VarOrderArg {
    Static,
    Sdf,
    Srf,
    Ldf,
    Original,
    Wdeg,
    Domoverwdeg,
    Conflict,
}

impl VarOrderArg {
    fn into_minion(self) -> minion_sys::VarOrder {
        match self {
            VarOrderArg::Static => minion_sys::VarOrder::Static,
            VarOrderArg::Sdf => minion_sys::VarOrder::SDF,
            VarOrderArg::Srf => minion_sys::VarOrder::SRF,
            VarOrderArg::Ldf => minion_sys::VarOrder::LDF,
            VarOrderArg::Original => minion_sys::VarOrder::Original,
            VarOrderArg::Wdeg => minion_sys::VarOrder::WDeg,
            VarOrderArg::Domoverwdeg => minion_sys::VarOrder::DOMOverWDeg,
            VarOrderArg::Conflict => minion_sys::VarOrder::Conflict,
        }
    }
}

#[derive(Clone, Copy, Debug, clap::ValueEnum)]
enum ValOrderArg {
    Ascend,
    Descend,
    Random,
}

impl ValOrderArg {
    fn into_minion(self) -> minion_sys::ValOrder {
        match self {
            ValOrderArg::Ascend => minion_sys::ValOrder::Ascend,
            ValOrderArg::Descend => minion_sys::ValOrder::Descend,
            ValOrderArg::Random => minion_sys::ValOrder::Random,
        }
    }
}

#[derive(Clone, Copy, Debug, clap::ValueEnum)]
enum PropLevelArg {
    None,
    Gac,
    Sacbounds,
    Sac,
    Ssacbounds,
    Ssac,
}

impl PropLevelArg {
    fn into_minion(self) -> minion_sys::PropLevel {
        match self {
            PropLevelArg::None => minion_sys::PropLevel::None,
            PropLevelArg::Gac => minion_sys::PropLevel::GAC,
            PropLevelArg::Sacbounds => minion_sys::PropLevel::SACBounds,
            PropLevelArg::Sac => minion_sys::PropLevel::SAC,
            PropLevelArg::Ssacbounds => minion_sys::PropLevel::SSACBounds,
            PropLevelArg::Ssac => minion_sys::PropLevel::SSAC,
        }
    }
}

fn main() -> Result<()> {
    let opt = Opt::parse();
    println!("{:?}", opt);

    // Wire --size-factor into the global so build_random_instance
    // (called from many places without a config arg) picks it up.
    constraint_def::SIZE_FACTOR.store(opt.size_factor.max(1), std::sync::atomic::Ordering::Relaxed);

    rayon::ThreadPoolBuilder::new()
        .num_threads(opt.numthreads)
        .build_global()
        .unwrap();

    let mut v;
    if opt.constraints.is_empty() {
        v = constraint_def::CONSTRAINT_LIST.clone();
    } else {
        v = Vec::new();
        for c in opt.constraints.clone() {
            let con = constraint_def::CONSTRAINT_LIST.iter().find(|x| x.name == c);
            match con {
                None => panic!("Unimplemented constraint: {}", c),
                Some(con) => v.push(con.clone()),
            }
        }
    }

    if opt.in_process && opt.valgrind {
        anyhow::bail!("--in-process cannot be combined with --valgrind");
    }
    if !opt.in_process && opt.minion.is_empty() {
        anyhow::bail!("--minion <path> is required in exec mode (or pass --in-process)");
    }
    if opt.midsearch && !opt.in_process {
        anyhow::bail!("--midsearch requires --in-process");
    }
    if opt.midsearch_constraints && !opt.in_process {
        anyhow::bail!("--midsearch-constraints requires --in-process");
    }
    if opt.midsearch && opt.midsearch_constraints {
        anyhow::bail!("--midsearch and --midsearch-constraints are mutually exclusive");
    }
    if opt.midsearch_add_vars && !opt.in_process {
        anyhow::bail!("--midsearch-add-vars requires --in-process");
    }
    if opt.midsearch_add_vars && (opt.midsearch || opt.midsearch_constraints) {
        anyhow::bail!(
            "--midsearch-add-vars is mutually exclusive with --midsearch and --midsearch-constraints"
        );
    }

    let var_order = opt.var_order.into_minion();
    let val_order = opt.val_order.into_minion();
    let preprocess = minion_sys::Propagation {
        level: opt.preprocess.into_minion(),
        limit: opt.preprocess_limit,
    };
    let prop_node = minion_sys::Propagation {
        level: opt.prop_node.into_minion(),
        limit: opt.prop_node_limit,
    };
    let config = if opt.valgrind {
        test_types::MinionConfig {
            minionargs: vec![
                "--leak-check=full".to_owned(),
                "--show-leak-kinds=all".to_owned(),
                opt.minion.clone(),
            ],
            minionexec: "valgrind",
            maxtuples: opt.maxtuples,
            backend: Backend::Exec,
            var_order,
            val_order,
            preprocess,
            prop_node,
            max_solutions: opt.max_solutions,
        }
    } else {
        test_types::MinionConfig {
            minionargs: vec![],
            minionexec: &opt.minion,
            maxtuples: opt.maxtuples,
            backend: if opt.in_process {
                Backend::InProcess
            } else {
                Backend::Exec
            },
            var_order,
            val_order,
            preprocess,
            prop_node,
            max_solutions: opt.max_solutions,
        }
    };

    if opt.midsearch {
        let ret: Result<()> = v.clone().into_par_iter().try_for_each(|ref c| {
            (0..opt.count)
                .into_par_iter()
                .try_for_each(|_| {
                    test_types::test_constraint_midsearch_add_vars(
                        &config,
                        c,
                        opt.midsearch_inject_after,
                        opt.midsearch_new_vars,
                    )
                })
                .context(format!("midsearch failure in {}", c.name))?;
            println!("Tested {} (midsearch)", c.name);
            Ok(())
        });
        ret?;
        return Ok(());
    }

    if opt.midsearch_add_vars {
        if opt.midsearch_base_size == 0 {
            anyhow::bail!("--midsearch-base-size must be >= 1");
        }
        let pool: Vec<constraint_def::ConstraintDef> = constraint_def::CONSTRAINT_LIST.clone();
        use std::sync::Mutex;
        let failures = std::sync::atomic::AtomicUsize::new(0);
        let first_error: Mutex<Option<String>> = Mutex::new(None);
        let trace = std::env::var("TESTER_TRACE").is_ok();
        // Flat sweep of `count` trials (not per-constraint, since the
        // injected-constraint shape is fixed to DisEq(new, base)).
        (0..opt.count).into_par_iter().for_each(|_| {
            let mut rng = thread_rng();
            let base_defs: Vec<&constraint_def::ConstraintDef> = pool
                .choose_multiple(&mut rng, opt.midsearch_base_size)
                .collect();
            let seed: u32 = rand::random();
            if trace {
                let names: Vec<&str> = base_defs.iter().map(|d| d.name.as_str()).collect();
                eprintln!(
                    "trial add-vars base={names:?} n_packets={} seed={seed:#x}",
                    opt.midsearch_constraints_num_packets
                );
            }
            if let Err(e) = test_types::test_midsearch_add_new_vars_with_constraint(
                &config,
                &base_defs,
                opt.midsearch_constraints_num_packets,
                seed,
            ) {
                failures.fetch_add(1, std::sync::atomic::Ordering::Relaxed);
                let mut first = first_error.lock().unwrap();
                if first.is_none() {
                    *first = Some(format!("{e:#}"));
                }
            }
        });
        let f = failures.load(std::sync::atomic::Ordering::Relaxed);
        println!("\n=== midsearch-add-vars summary ===");
        if f > 0 {
            let err = first_error.lock().unwrap();
            println!(
                "  {f}/{} failed — e.g. {}",
                opt.count,
                err.as_deref().unwrap_or("")
            );
            std::process::exit(1);
        } else {
            println!("Total: 0 failed trials out of {}.", opt.count);
        }
        return Ok(());
    }

    if opt.midsearch_constraints {
        if opt.midsearch_base_size == 0 {
            anyhow::bail!("--midsearch-base-size must be >= 1");
        }
        // Draw bases from the full list of constraints — independent of
        // what the user selected for injection, so `--constraints eq`
        // still gets a non-trivial base problem to inject into.
        let pool: Vec<constraint_def::ConstraintDef> = constraint_def::CONSTRAINT_LIST.clone();
        // Non-fatal: each trial that fails is recorded for the summary
        // but doesn't abort the sweep. The point of this test is to
        // surface minion bugs; aborting on the first one hides the rest.
        use std::sync::Mutex;
        struct Report {
            failures: std::sync::atomic::AtomicUsize,
            first_error: Mutex<Option<String>>,
        }
        let reports: std::collections::HashMap<String, Report> = v
            .iter()
            .map(|c| {
                (
                    c.name.clone(),
                    Report {
                        failures: std::sync::atomic::AtomicUsize::new(0),
                        first_error: Mutex::new(None),
                    },
                )
            })
            .collect();
        // If a C++ abort kills the process mid-sweep, the last trace line
        // tells you which trial triggered it (look for it in stderr).
        let trace = std::env::var("TESTER_TRACE").is_ok();
        v.clone().into_par_iter().for_each(|ref c| {
            (0..opt.count).into_par_iter().for_each(|_| {
                let mut rng = thread_rng();
                let base_defs: Vec<&constraint_def::ConstraintDef> = pool
                    .iter()
                    .filter(|d| d.name != c.name)
                    .collect::<Vec<_>>()
                    .choose_multiple(&mut rng, opt.midsearch_base_size)
                    .copied()
                    .collect();
                let seed: u32 = rand::random();
                // Build N packets: first one is the iterated constraint
                // `c`; the rest are random picks from the full pool.
                let mut inject_defs: Vec<&constraint_def::ConstraintDef> = vec![c];
                for _ in 1..opt.midsearch_constraints_num_packets {
                    inject_defs.push(pool.choose(&mut rng).unwrap());
                }
                let inject_after: Vec<usize> =
                    (1..=opt.midsearch_constraints_num_packets).collect();
                if trace {
                    let names: Vec<&str> = base_defs.iter().map(|d| d.name.as_str()).collect();
                    let inames: Vec<&str> = inject_defs.iter().map(|d| d.name.as_str()).collect();
                    eprintln!(
                        "trial inject={inames:?} base={names:?} schedule={inject_after:?} seed={seed:#x}"
                    );
                }
                if let Err(e) = test_types::test_constraint_midsearch_inject_constraints(
                    &config,
                    &base_defs,
                    &inject_defs,
                    &inject_after,
                    opt.midsearch_wrap_nested,
                    seed,
                ) {
                    let r = &reports[&c.name];
                    r.failures
                        .fetch_add(1, std::sync::atomic::Ordering::Relaxed);
                    let mut first = r.first_error.lock().unwrap();
                    if first.is_none() {
                        *first = Some(format!("{e:#}"));
                    }
                }
            });
            println!("Tested {} (midsearch-constraints)", c.name);
        });

        // Summary.
        println!("\n=== midsearch-constraints summary ===");
        let mut ordered: Vec<&String> = reports.keys().collect();
        ordered.sort();
        let mut total_failures = 0usize;
        for name in ordered {
            let r = &reports[name];
            let f = r.failures.load(std::sync::atomic::Ordering::Relaxed);
            total_failures += f;
            if f > 0 {
                let err = r.first_error.lock().unwrap();
                println!(
                    "  {name}: {f}/{} failed — e.g. {}",
                    opt.count,
                    err.as_deref().unwrap_or("")
                );
            }
        }
        println!(
            "Total: {total_failures} failed trials across {} constraints.",
            reports.len()
        );
        if total_failures > 0 {
            std::process::exit(1);
        }
        return Ok(());
    }

    let ret: Result<()> = v.clone().into_par_iter().try_for_each(|ref c| {
        (0..opt.count)
            .into_par_iter()
            .try_for_each(|_| test_types::test_constraint(&config, c))
            .context(format!("failure in {}", c.name))?;

        (0..opt.count)
            .into_par_iter()
            .try_for_each(|_| test_types::test_constraint_nested(&config, c))
            .context(format!("failure in {} with nesting", c.name))?;

        if opt.nest_depth >= 2 {
            let depth = opt.nest_depth;
            (0..opt.count)
                .into_par_iter()
                .try_for_each(|_| test_types::test_constraint_deep_nested(&config, c, depth))
                .context(format!(
                    "failure in {} with deep-nested depth={depth}",
                    c.name
                ))?;
        }

        println!("Tested {}", c.name);
        Ok(())
    });

    ret?;

    // -parallel is a fork-based exec-only flag, so the parallel test
    // sweep is exec-only. Work-stealing is available in both backends
    // (run_solve translates -X-parallelWorkSteal to the in-process
    // run_minion_work_steal entry), so we run that sweep in both.
    if config.backend == Backend::Exec {
        println!("Parallel tests\n");
        let ret2: Result<()> = v.clone().into_par_iter().try_for_each(|ref c| {
            (0..opt.count)
                .into_par_iter()
                .try_for_each(|_| test_types::test_constraint_par(&config, c))
                .context(format!("failure in {}", c.name))?;

            println!("Tested {}", c.name);
            Ok(())
        });

        ret2?;
    }

    // Work-stealing sweep with adaptive sizing. Each constraint
    // starts at the global SIZE_FACTOR and doubles its instance size
    // until at least one trial reports a donation, or we hit
    // --ws-max-size. Without this, tiny random instances finish in
    // microseconds and the donation/replay path is never exercised
    // (workers haven't finished marking themselves idle by the time
    // the search is done).
    println!("Work-stealing tests\n");
    let ws_initial = opt.size_factor.max(1);
    let ws_max = opt.ws_max_size.max(ws_initial);
    let ret_ws: Result<()> = v.clone().into_par_iter().try_for_each(|ref c| {
        let mut size = ws_initial;
        loop {
            let donated_before =
                test_types::WS_TRIALS_WITH_DONATIONS.load(std::sync::atomic::Ordering::Relaxed);
            let capped_before =
                test_types::WS_TRIALS_CAPPED.load(std::sync::atomic::Ordering::Relaxed);
            (0..opt.count)
                .into_par_iter()
                .try_for_each(|_| test_types::test_constraint_workstal(&config, c, 4, size))
                .context(format!(
                    "failure in {} with -X-parallelWorkSteal 4 (size_factor={size})",
                    c.name
                ))?;
            let donated_after =
                test_types::WS_TRIALS_WITH_DONATIONS.load(std::sync::atomic::Ordering::Relaxed);
            let capped_after =
                test_types::WS_TRIALS_CAPPED.load(std::sync::atomic::Ordering::Relaxed);
            let saw_donation = donated_after > donated_before;
            let capped_this_round = capped_after - capped_before;
            if saw_donation {
                println!(
                    "Tested {} (work-steal, size_factor={size}, donations confirmed)",
                    c.name
                );
                break;
            }
            // If most trials at this size hit the solution cap, the
            // random instance has more solutions than we'll
            // materialise — growing further makes that worse, not
            // better. Stop and report.
            if capped_this_round * 2 >= opt.count {
                println!(
                    "Tested {} (work-steal, size_factor={size}, \
                     {capped_this_round}/{} trials hit --max-solutions — instance too \
                     large to compare; giving up before donations confirmed)",
                    c.name, opt.count
                );
                break;
            }
            if size >= ws_max {
                println!(
                    "Tested {} (work-steal, size_factor={size}, no donations seen — \
                     instance still too small at --ws-max-size cap)",
                    c.name
                );
                break;
            }
            size = (size * 2).min(ws_max);
        }
        Ok(())
    });
    ret_ws?;

    // Work-stealing portfolio sweep: same shared-tree donation/replay
    // mechanism, but each worker runs a different (var-order, val-order,
    // randomisation) combination. Exec-only — the in-process API does
    // not expose the portfolio flag. Reuses the same instance-size
    // adaptation as the plain work-steal sweep so donations get
    // exercised on instances large enough to actually trigger them.
    if config.backend == Backend::Exec {
        println!("Work-stealing portfolio tests\n");
        let ret_wsp: Result<()> = v.clone().into_par_iter().try_for_each(|ref c| {
            let mut size = ws_initial;
            loop {
                let donated_before = test_types::WS_TRIALS_WITH_DONATIONS
                    .load(std::sync::atomic::Ordering::Relaxed);
                let capped_before =
                    test_types::WS_TRIALS_CAPPED.load(std::sync::atomic::Ordering::Relaxed);
                (0..opt.count)
                    .into_par_iter()
                    .try_for_each(|_| {
                        test_types::test_constraint_workstal_portfolio(&config, c, 4, size)
                    })
                    .context(format!(
                        "failure in {} with -X-parallelWorkSteal 4 \
                         -X-parallelWorkStealPortfolio (size_factor={size})",
                        c.name
                    ))?;
                let donated_after = test_types::WS_TRIALS_WITH_DONATIONS
                    .load(std::sync::atomic::Ordering::Relaxed);
                let capped_after =
                    test_types::WS_TRIALS_CAPPED.load(std::sync::atomic::Ordering::Relaxed);
                let saw_donation = donated_after > donated_before;
                let capped_this_round = capped_after - capped_before;
                if saw_donation {
                    println!(
                        "Tested {} (work-steal portfolio, size_factor={size}, \
                         donations confirmed)",
                        c.name
                    );
                    break;
                }
                if capped_this_round * 2 >= opt.count {
                    println!(
                        "Tested {} (work-steal portfolio, size_factor={size}, \
                         {capped_this_round}/{} trials hit --max-solutions — \
                         instance too large; giving up before donations confirmed)",
                        c.name, opt.count
                    );
                    break;
                }
                if size >= ws_max {
                    println!(
                        "Tested {} (work-steal portfolio, size_factor={size}, \
                         no donations seen — instance still too small at \
                         --ws-max-size cap)",
                        c.name
                    );
                    break;
                }
                size = (size * 2).min(ws_max);
            }
            Ok(())
        });
        ret_wsp?;
    }

    {
        let trials = test_types::WS_TRIALS.load(std::sync::atomic::Ordering::Relaxed);
        let donations = test_types::WS_DONATIONS.load(std::sync::atomic::Ordering::Relaxed);
        let with_donations =
            test_types::WS_TRIALS_WITH_DONATIONS.load(std::sync::atomic::Ordering::Relaxed);
        println!(
            "Work-steal coverage: {trials} trials, {donations} donations total \
             ({with_donations} trials had >=1 donation, \
             {} avg donations/trial)",
            if trials > 0 {
                donations as f64 / trials as f64
            } else {
                0.0
            }
        );
        // No bail here — the per-constraint loop already prints
        // "no donations seen — instance still too small at
        // --ws-max-size cap" for any constraint that couldn't
        // structurally exercise work-stealing. Bailing globally
        // misfires when the user runs tester for a single small
        // constraint (eq, false, gaceq, etc.) whose search tree
        // genuinely finishes before workers can mark themselves
        // idle. The user's test orchestrator that runs per-constraint
        // shouldn't fail on those.
        let _ = (trials, donations, with_donations, ws_max);
    }

    // Restart-search sweep: -restarts forces sollimit=1 and rejects
    // -findallsols, so it can't ride the option-sweep machinery.
    // Each trial verifies the restart run finds a single valid
    // solution (or correctly proves UNSAT). Exec-only because
    // -restarts isn't currently exposed via RunOptions in the
    // in-process API.
    if config.backend == Backend::Exec && opt.count > 0 {
        println!("Restart-search tests\n");
        let restart_variants: &[&[&str]] = &[
            &[],
            &["-restarts-multiplier", "1.5"],
            &["-restarts-multiplier", "2.5"],
            &["-no-restarts-bias"],
        ];
        let ret_restart: Result<()> = v.clone().into_par_iter().try_for_each(|ref c| {
            (0..opt.count)
                .into_par_iter()
                .try_for_each(|i| {
                    let extra = restart_variants[(i as usize) % restart_variants.len()];
                    test_types::test_constraint_restart(&config, c, extra)
                })
                .context(format!("restart failure in {}", c.name))?;
            println!("Tested {} (restart)", c.name);
            Ok(())
        });
        ret_restart?;
    }

    // Propagator-equivalence sweep: for each group of semantically
    // identical propagators (alldiff/gacalldiff, table family, etc.),
    // generate one instance for the representative and verify every
    // variant produces the same solution set. Bugs in any one
    // variant's propagator surface as a mismatch against the others.
    // Runs in both backends — the only difference is how the per-run
    // dispatch is plumbed.
    let variant_count = opt.variant_count.unwrap_or(opt.count);
    if variant_count > 0 {
        println!("Variant-equivalence tests\n");
        let groups = constraint_def::EQUIVALENCE_GROUPS;
        let ret_eq: Result<()> = groups.par_iter().try_for_each(|group| {
            (0..variant_count)
                .into_par_iter()
                .try_for_each(|_| test_types::test_constraint_variant_equivalence(&config, group))
                .context(format!("variant-equivalence failure in group {:?}", group))?;
            println!("Tested variant equivalence: {:?}", group);
            Ok(())
        });
        ret_eq?;
    }

    if config.backend == Backend::InProcess {
        println!("In-process mode: skipping option test sweep (exec-mode feature).");
        return Ok(());
    }

    println!("Option tests\n");

    let options = vec![
        vec!["-preprocess", "None"],
        vec!["-preprocess", "GAC"],
        vec!["-preprocess", "SAC"],
        vec!["-preprocess", "SSAC"],
        vec!["-preprocess", "SACBounds"],
        vec!["-preprocess", "SSACBounds"],
        vec!["-preprocess", "None_limit"],
        vec!["-preprocess", "GAC_limit"],
        vec!["-preprocess", "SAC_limit"],
        vec!["-preprocess", "SSAC_limit"],
        vec!["-preprocess", "SACBounds_limit"],
        vec!["-preprocess", "SSACBounds_limit"],
        vec!["-prop-node", "GAC"],
        vec!["-prop-node", "SAC"],
        vec!["-prop-node", "SSAC"],
        vec!["-prop-node", "SACBounds"],
        vec!["-prop-node", "SSACBounds"],
        vec!["-prop-node", "GAC_limit"],
        vec!["-prop-node", "SAC_limit"],
        vec!["-prop-node", "SSAC_limit"],
        vec!["-prop-node", "SACBounds_limit"],
        vec!["-prop-node", "SSACBounds_limit"],
        vec!["-parallel"],
        // -X-parallelWorkSteal is mutually exclusive with -parallel and
        // a few other flags, so it can't safely participate in the
        // random-combination sweep below. The dedicated
        // test_constraint_workstal pass above already exercises it.
        vec!["-printsols"],
        vec!["-noprintsols"],
        vec!["-printsolsonly"],
        vec!["-printonlyoptimal"],
        vec!["-map-long-short", "none"],
        vec!["-map-long-short", "keeplong"],
        vec!["-map-long-short", "eager"],
        vec!["-map-long-short", "lazy"],
        vec!["-nocheck"],
        vec!["-check"],
        vec!["-varorder", "static"],
        vec!["-varorder", "srf"],
        //vec!["-varorder", "staticlimited", "0"],
        //vec!["-varorder", "staticlimited", "1"],
        //vec!["-varorder", "staticlimited", "2"],
        vec!["-varorder", "srf-random"],
        vec!["-varorder", "sdf"],
        vec!["-varorder", "sdf-random"],
        vec!["-varorder", "ldf"],
        vec!["-varorder", "ldf-random"],
        vec!["-varorder", "random"],
        vec!["-varorder", "conflict"],
        vec!["-varorder", "wdeg"],
        vec!["-varorder", "domoverwdeg"],
        vec!["-valorder", "ascend"],
        vec!["-valorder", "descend"],
        vec!["-valorder", "random"],
        vec!["-randomiseorder"],
        vec!["-randomseed", "0"], // Just test a couple of values
        vec!["-randomseed", "6"],
        // -restarts is incompatible with -findallsols (forces
        // sollimit=-1, which BuildCSP.cpp:124 rejects). Tested
        // separately by test_constraint_restart, which uses
        // -sollimit 1 and verifies the found solution lies in the
        // baseline's solution set.
    ];

    let mut testlist = vec![];

    let mut rng = thread_rng();

    for _ in 0..opt.optioncount {
        let tests = rng.gen_range(0..options.len());
        let testargs = (
            v.choose(&mut rng).unwrap(),
            options.choose_multiple(&mut rng, tests).collect::<Vec<_>>(),
        );

        testlist.push(testargs);
    }

    let ret3: Result<()> = testlist.into_par_iter().try_for_each(|(c, ref options)| {
        test_types::test_constraint_options(&config, c, options)
            .context(format!("failure in {} with {:?}", c.name, options))?;

        Ok(())
    });

    ret3?;

    Ok(())
}
