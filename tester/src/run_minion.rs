#![allow(non_snake_case)]

use crate::constraint_def::ConstraintInstance;
use crate::minion_instance::print_minion_file_pair;
use crate::solution_digest::SolutionDigest;

use anyhow::{anyhow, Context, Result};

use std::fs;
use std::io::*;
use std::process::{Command, Stdio};

use crate::counter::get_unique_value;

extern crate serde_json;

extern crate itertools;
use self::itertools::Itertools;

#[derive(Debug, PartialEq, Eq)]
pub struct MinionOutput {
    /// Compact digest of the solution multiset (sum of per-solution
    /// hashes mod 2^64). Always populated. See `solution_digest.rs`
    /// for the rationale — WE ARE HASHING SOLUTIONS, NOT STORING
    /// THEM, to keep memory O(1) regardless of solution count.
    pub solutions: SolutionDigest,
    /// Full solution list, only populated when the caller passes
    /// `keep_full_solutions=true`. The mid-search tests need indexed
    /// access (prefix slices, membership checks); the heavy parallel
    /// sweeps don't and rely on `solutions` (the digest) only.
    pub raw_solutions: Option<Vec<Vec<i64>>>,
    pub nodes: i64,
    pub filename: String,
    pub cleanup: CleanupFiles,
    /// Number of work-stealing donations the run made (None if the
    /// run wasn't a work-steal run; Some(0) if work-steal was active
    /// but no donation fired — useful for confirming the test
    /// exercises the donation path).
    pub work_steal_donations: Option<i64>,
    /// Total parallel-SAC fork-join rounds. None if the run didn't
    /// emit the key (older binaries); Some(0) when -X-parallelPreprocess
    /// was off; Some(>=1) when parallel SAC fired. Used by the
    /// adaptive parallel-preprocess sweep to detect whether the
    /// parallel code path was actually exercised.
    pub parallel_preprocess_rounds: Option<i64>,
    /// Total prunings applied during parallel-SAC. Same None /
    /// Some(0) / Some(>=1) semantics as `parallel_preprocess_rounds`.
    pub parallel_preprocess_prunings: Option<i64>,
    /// True when the run hit the configured solution cap (`-sollimit`
    /// for exec mode, the per-trial counter for the in-process path)
    /// and stopped before exhausting search. Callers must treat the
    /// solution set as a partial prefix and skip any equality
    /// comparison — the random instance is too large to be a useful
    /// signal at this size.
    pub hit_solution_cap: bool,
}

impl MinionOutput {
    /// Access the full solution Vec. Panics if the run was started
    /// without `keep_full_solutions=true` (digest-only mode). Use only
    /// in callers that explicitly request raw storage via
    /// `run_solve_keep_full` or by passing `keep_full_solutions: true`
    /// to the in-process API directly.
    pub fn raw_sols(&self) -> &[Vec<i64>] {
        self.raw_solutions
            .as_deref()
            .expect("solutions Vec only available with keep_full_solutions=true")
    }
}

#[derive(Debug, PartialEq, Eq)]
pub struct CleanupFiles {
    files: Vec<String>,
}

impl CleanupFiles {
    pub fn cleanup(&self) {
        for file in &self.files {
            let _ = fs::remove_file(file);
        }
    }

    pub fn empty() -> Self {
        CleanupFiles { files: vec![] }
    }
}

// We only have to put here what we care about
#[derive(Deserialize)]
struct MinionJsonOut {
    Nodes: String,
    SolutionsFound: String,
    /// Present only when the run used -X-parallelWorkSteal. Reports
    /// how many donate() calls fired; the tester accumulates these
    /// so we can assert that work-steal actually exercised the
    /// donation/replay path.
    #[serde(default)]
    WorkStealDonations: Option<String>,
    /// Total fork-join rounds run by the parallel-SAC fixpoint.
    /// 0 means the parallel path didn't fire (flag was off, or only
    /// sequential SAC was selected). >= 2 means the first round
    /// found prunings AND a further round was needed to confirm
    /// convergence — the strongest "parallel preprocess actually
    /// did meaningful work" signal we can read out.
    #[serde(default)]
    ParallelPreprocessRounds: Option<String>,
    /// Cumulative prunings (setMin + setMax + removeVal) merged from
    /// worker slots into the parent. Useful diagnostic alongside
    /// rounds.
    #[serde(default)]
    ParallelPreprocessPrunings: Option<String>,
}

pub fn get_minion_solutions(
    minionexec: &str,
    baseargs: &[String],
    extraargs: &[&str],
    instance: &ConstraintInstance,
    testname: &str,
    max_solutions: i64,
    keep_full_solutions: bool,
) -> Result<MinionOutput> {
    let nameid = format!(
        "{:?}_{}_{}",
        std::process::id(),
        get_unique_value(),
        testname
    );
    fs::create_dir_all("tempdir").context("Failed to create 'tempdir'")?;

    let minout = format!("tempdir/input{}.minion", nameid);
    let solsout = format!("tempdir/sols{}.out", nameid);
    let tableout = format!("tempdir/jsontable{}.out", nameid);

    let mut args: Vec<String> = baseargs.to_owned();
    for &e in extraargs {
        args.push(e.to_owned());
    }
    // Cap solution gathering — protects against memory blow-up when a
    // random instance happens to have a huge solution space (e.g. wide
    // alldiff with the cartesian product of many domains). Appended
    // after extraargs so it overrides any earlier `-findallsols`
    // (which sets sollimit = -1). The caller checks
    // `hit_solution_cap` and abandons trials that hit the limit.
    //
    // `-restarts` rejects sollimit != 1 (BuildCSP.cpp:124), so the
    // dedicated restart sweep already passes `-sollimit 1` in
    // extraargs and we must not append a second one — detect and skip.
    let has_sollimit = extraargs.iter().any(|&a| a == "-sollimit");
    if max_solutions > 0 && !has_sollimit {
        args.push("-sollimit".to_string());
        args.push(max_solutions.to_string());
    }
    args.push("-solsout".to_string());
    args.push(solsout.clone());
    args.push("-jsontableout".to_string());
    args.push(tableout.clone());
    args.push("-noprintsols".to_string());
    args.push(minout.clone());

    {
        let mut outfile =
            fs::File::create(&minout).context("Could not open output file for writing")?;
        print_minion_file_pair(&mut outfile, instance).context("failed writing Minion output")?;
    }

    let minioncmd = format!("{} {}", minionexec, args.iter().join(" "));

    let child = Command::new(minionexec)
        .args(args)
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .spawn()
        .context(format!("Failed to start '{}'", minionexec))?;

    let output = child
        .wait_with_output()
        .context(format!("failed to capture Minion output: {}", minioncmd))?;

    if !output.status.success() {
        print!(
            "Minion did not finish successfully (non-zero return value)\n{}\n{}\n",
            String::from_utf8_lossy(&output.stdout),
            String::from_utf8_lossy(&output.stderr)
        );
        return Err(anyhow!(format!(
            "Minion returned non-zero value: {}",
            minioncmd
        )));
    }

    // Stream the -solsout file line by line into the digest. Each line
    // is one solution (whitespace-separated decimal values). We avoid
    // building a `Vec<Vec<i64>>` of every solution unless the caller
    // explicitly asks for it via `keep_full_solutions`. WE ARE HASHING
    // SOLUTIONS, NOT STORING THEM — this is the central memory win:
    // tests that previously OOM'd at the 10 M-solution cap now use
    // O(1) memory regardless of solution count.
    let (digest, raw_solutions) = {
        let f = fs::File::open(&solsout)
            .context(format!("failed to open solution file: {}", minioncmd))?;
        let reader = BufReader::new(f);
        let mut digest = SolutionDigest::new();
        let mut raw: Option<Vec<Vec<i64>>> =
            if keep_full_solutions { Some(Vec::new()) } else { None };
        for tryline in reader.lines() {
            let line = tryline.context(format!("failure reading solutions: {}", minioncmd))?;
            let sol: Vec<i64> = line
                .split_whitespace()
                .map(|x| x.parse::<i64>().unwrap())
                .collect();
            if let Some(v) = raw.as_mut() {
                v.push(sol.clone());
            }
            digest.add(sol);
        }
        (digest, raw)
    };

    let parsed = {
        let f = fs::File::open(&tableout)
            .context(format!("failed to open jsontableout file: {}", minioncmd))?;

        let reader = BufReader::new(f);

        let v: MinionJsonOut = serde_json::from_reader(reader)
            .context(format!("jsontableout not valid json!: {}", minioncmd))?;

        let nodes = v
            .Nodes
            .parse::<i64>()
            .context(format!("invalid node count: {}", minioncmd))?;
        let solcount = v
            .SolutionsFound
            .parse::<i64>()
            .context(format!("invalid solution count: {}", minioncmd))?;

        if solcount != digest.count as i64 {
            return Err(anyhow!(format!(
                "Solutions file contains {} solutions, but SolutionsFound is {}",
                digest.count,
                solcount
            )));
        }

        let donations = v
            .WorkStealDonations
            .as_ref()
            .and_then(|s| s.parse::<i64>().ok());
        let pp_rounds = v
            .ParallelPreprocessRounds
            .as_ref()
            .and_then(|s| s.parse::<i64>().ok());
        let pp_prunings = v
            .ParallelPreprocessPrunings
            .as_ref()
            .and_then(|s| s.parse::<i64>().ok());

        (nodes, donations, pp_rounds, pp_prunings)
    };
    let (nodes, work_steal_donations, parallel_preprocess_rounds, parallel_preprocess_prunings) =
        parsed;

    // hit_solution_cap is true when the search stopped at the
    // -sollimit cap. Solution count == cap → cap was hit (search may
    // or may not have been complete; in either case the caller should
    // treat the prefix as untrustworthy for set-equality checks).
    let hit_solution_cap = max_solutions > 0 && digest.count as i64 >= max_solutions;

    Ok(MinionOutput {
        solutions: digest,
        raw_solutions,
        nodes,
        filename: minout.clone(),
        cleanup: CleanupFiles {
            files: vec![minout, solsout, tableout],
        },
        work_steal_donations,
        parallel_preprocess_rounds,
        parallel_preprocess_prunings,
        hit_solution_cap,
    })
}
