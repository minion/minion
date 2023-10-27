#[macro_use]
extern crate lazy_static;

#[macro_use]
extern crate serde_derive;

use structopt::StructOpt;

use rayon::prelude::*;

use anyhow::Context;
use anyhow::Result;

use rand::seq::SliceRandom;
use rand::{thread_rng, Rng};

mod constraint_def;
mod counter;
mod minion_instance;
mod run_minion;
mod test_types;

#[derive(StructOpt, Debug)]
#[structopt(name = "basic")]
struct Opt {
    #[structopt(short = "v", long = "valgrind")]
    valgrind: bool,

    #[structopt(name = "constraints")]
    constraints: Vec<String>,

    #[structopt(short = "c", long = "count", default_value = "30")]
    count: u64,

    #[structopt(short = "o", long = "optioncount", default_value = "1000")]
    optioncount: u64,

    #[structopt(short = "m", long = "minion")]
    minion: String,

    #[structopt(short = "t", long = "maxtuples", default_value = "10000")]
    maxtuples: usize,

    #[structopt(short = "n", long = "number of threads", default_value = "8")]
    numthreads: usize,
}

fn main() -> Result<()> {
    let opt = Opt::from_args();
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

    let config = if opt.valgrind {
        test_types::MinionConfig {
            minionargs: vec![
                "--leak-check=full".to_owned(),
                "--show-leak-kinds=all".to_owned(),
                opt.minion.clone(),
            ],
            minionexec: "valgrind",
            maxtuples: opt.maxtuples,
        }
    } else {
        test_types::MinionConfig {
            minionargs: vec![],
            minionexec: &opt.minion,
            maxtuples: opt.maxtuples,
        }
    };

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
