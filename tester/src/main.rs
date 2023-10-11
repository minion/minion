#[macro_use]
extern crate lazy_static;

#[macro_use]
extern crate serde_derive;

use structopt::StructOpt;

use rayon::prelude::*;

use anyhow::Context;
use anyhow::Result;

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
    let ret2: Result<()> = v.into_par_iter().try_for_each(|ref c| {
        (0..opt.count)
            .into_par_iter()
            .try_for_each(|_| test_types::test_constraint_par(&config, c))
            .context(format!("failure in {}", c.name))?;

        println!("Tested {}", c.name);
        Ok(())
    });

    ret2?;

    Ok(())
}
