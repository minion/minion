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
}

// We only have to put here what we care about
#[derive(Deserialize)]
struct MinionJsonOut {
    Nodes: String,
    SolutionsFound: String,
}

pub fn get_minion_solutions(
    minionexec: &str,
    baseargs: &[String],
    extraargs: &[&str],
    instance: &ConstraintInstance,
    testname: &str,
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

    let nodes = {
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

        nodes
    };

    Ok(MinionOutput {
        solutions,
        nodes,
        filename: minout.clone(),
        cleanup: CleanupFiles {
            files: vec![minout, solsout, tableout],
        },
    })
}
