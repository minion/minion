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
}

fn main() -> Result<()> {
    let opt = Opt::parse();
    println!("{:?}", opt);

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

    if opt.midsearch_constraints {
        if opt.midsearch_base_size == 0 {
            anyhow::bail!("--midsearch-base-size must be >= 1");
        }
        // Draw bases from the full list of constraints — independent of
        // what the user selected for injection, so `--constraints eq`
        // still gets a non-trivial base problem to inject into.
        let pool: Vec<constraint_def::ConstraintDef> =
            constraint_def::CONSTRAINT_LIST.clone();
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
                if trace {
                    let names: Vec<&str> = base_defs.iter().map(|d| d.name.as_str()).collect();
                    eprintln!(
                        "trial inject={} base={names:?} seed={seed:#x}",
                        c.name
                    );
                }
                if let Err(e) = test_types::test_constraint_midsearch_inject_constraint(
                    &config,
                    &base_defs,
                    c,
                    opt.midsearch_inject_after,
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
            let f = r
                .failures
                .load(std::sync::atomic::Ordering::Relaxed);
            total_failures += f;
            if f > 0 {
                let err = r.first_error.lock().unwrap();
                println!("  {name}: {f}/{} failed — e.g. {}", opt.count, err.as_deref().unwrap_or(""));
            }
        }
        println!("Total: {total_failures} failed trials across {} constraints.", reports.len());
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

        println!("Tested {}", c.name);
        Ok(())
    });

    ret?;

    if config.backend == Backend::InProcess {
        println!("In-process mode: skipping parallel and option test sweeps (exec-mode features).");
        return Ok(());
    }

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
