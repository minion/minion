#![allow(non_snake_case)]

use constraint_def::ConstraintInstance;
use minion_instance::print_minion_file_pair;
use simple_error::SimpleError;
use simple_error::SimpleResult;
use std;
use std::fs;
use std::io::*;
use std::process::{Command, Stdio};

extern crate serde_json;

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct MinionOutput {
    pub solutions: Vec<Vec<i64>>,
    pub nodes: i64,
    pub filename: String,
}

// We only have to put here what we care about
#[derive(Deserialize)]
struct MinionJsonOut {
    Nodes: String,
}

pub fn get_minion_solutions(
    minionexec: &str,
    minionargs: &[&str],
    instance: &ConstraintInstance,
    testname: &str,
) -> SimpleResult<MinionOutput> {
    let nameid = format!(
        "{:?}_{}_{}",
        std::thread::current().id(),
        std::process::id(),
        testname
    );
    try_with!(fs::create_dir_all("tempdir"), "Failed to create 'tempdir'");

    let minout = format!("tempdir/input{}.minion", nameid);
    let solsout = format!("tempdir/sols{}.out", nameid);
    let tableout = format!("tempdir/jsontable{}.out", nameid);

    let _ = fs::remove_file(&solsout);
    let _ = fs::remove_file(&minout);
    let _ = fs::remove_file(&tableout);

    let mut args: Vec<String> = minionargs.iter().map(|x| x.to_string()).collect();
    args.push("-solsout".to_string());
    args.push(solsout.clone());
    args.push("-jsontableout".to_string());
    args.push(tableout.clone());
    args.push(minout.clone());

    {
        let mut outfile = try_with!(
            fs::File::create(&minout),
            "Could not open output file for writing"
        );
        try_with!(
            print_minion_file_pair(&mut outfile, &instance),
            "failed writing Minion output"
        );
    }

    let child = try_with!(
        Command::new(minionexec)
            .args(args)
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn(),
        format!("Failed to start '{}'", minionexec)
    );

    let output = try_with!(child.wait_with_output(), "failed to capture Minion output");

    if !output.status.success() {
        print!(
            "{}\n{}\n",
            String::from_utf8_lossy(&output.stdout),
            String::from_utf8_lossy(&output.stderr)
        );
        return Err(SimpleError::new("Minion returned non-zero value"));
    }

    let solutions = {
        let f = try_with!(fs::File::open(&solsout), "failed to open solution file");

        let reader = BufReader::new(f);

        let mut solutions: Vec<Vec<i64>> = Vec::new();
        for tryline in reader.lines() {
            let line = try_with!(tryline, "failure reading solutions");
            solutions.push(
                line.split_whitespace()
                    .map(|x| x.parse::<i64>().unwrap())
                    .collect(),
            )
        }
        solutions
    };

    let nodes = {
        let f = try_with!(
            fs::File::open(&tableout),
            "failed to open jsontableout file"
        );

        let reader = BufReader::new(f);

        let v: MinionJsonOut = try_with!(
            serde_json::from_reader(reader),
            "jsontableout not valid json!"
        );

        try_with!(v.Nodes.parse::<i64>(), "invalid node count")
    };

    Ok(MinionOutput {
        solutions,
        nodes,
        filename: minout,
    })
}
