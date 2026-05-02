#![allow(non_snake_case)]

use crate::constraint_def::ConstraintInstance;
use crate::minion_instance::print_minion_file_pair;

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
    pub solutions: Vec<Vec<i64>>,
    pub nodes: i64,
    pub filename: String,
    pub cleanup: CleanupFiles,
    /// Number of work-stealing donations the run made (None if the
    /// run wasn't a work-steal run; Some(0) if work-steal was active
    /// but no donation fired — useful for confirming the test
    /// exercises the donation path).
    pub work_steal_donations: Option<i64>,
    /// True when the run hit the configured solution cap (`-sollimit`
    /// for exec mode, the per-trial counter for the in-process path)
    /// and stopped before exhausting search. Callers must treat the
    /// solution set as a partial prefix and skip any equality
    /// comparison — the random instance is too large to be a useful
    /// signal at this size.
    pub hit_solution_cap: bool,
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
}

pub fn get_minion_solutions(
    minionexec: &str,
    baseargs: &[String],
    extraargs: &[&str],
    instance: &ConstraintInstance,
    testname: &str,
    max_solutions: i64,
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

    let solutions = {
        let f = fs::File::open(&solsout)
            .context(format!("failed to open solution file: {}", minioncmd))?;

        let reader = BufReader::new(f);

        let mut solutions: Vec<Vec<i64>> = Vec::new();
        for tryline in reader.lines() {
            let line = tryline.context(format!("failure reading solutions: {}", minioncmd))?;
            solutions.push(
                line.split_whitespace()
                    .map(|x| x.parse::<i64>().unwrap())
                    .collect(),
            )
        }
        solutions
    };

    let nodes: (i64, Option<i64>) = {
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

        if solcount != solutions.len() as i64 {
            return Err(anyhow!(format!(
                "Solutions files contains {} solutions, but SolutionsFound is {}",
                solutions.len(),
                solcount
            )));
        }

        let donations = v
            .WorkStealDonations
            .as_ref()
            .and_then(|s| s.parse::<i64>().ok());

        (nodes, donations)
    };
    let (nodes, work_steal_donations) = nodes;

    // hit_solution_cap is true when the search stopped at the
    // -sollimit cap. Solution count == cap → cap was hit (search may
    // or may not have been complete; in either case the caller should
    // treat the prefix as untrustworthy for set-equality checks).
    let hit_solution_cap = max_solutions > 0 && solutions.len() as i64 >= max_solutions;

    Ok(MinionOutput {
        solutions,
        nodes,
        filename: minout.clone(),
        cleanup: CleanupFiles {
            files: vec![minout, solsout, tableout],
        },
        work_steal_donations,
        hit_solution_cap,
    })
}
