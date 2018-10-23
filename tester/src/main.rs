#![allow(dead_code)]

#[macro_use]
extern crate lazy_static;
extern crate atomic_counter;
extern crate tempfile;

extern crate serde;
extern crate serde_json;

#[macro_use]
extern crate serde_derive;

#[macro_use]
extern crate simple_error;

extern crate num_integer;
extern crate rayon;

#[macro_use]
extern crate structopt;
use structopt::StructOpt;

use rayon::prelude::*;

mod constraint_def;
mod counter;
mod minion_instance;
mod run_minion;
mod test_types;


#[derive(StructOpt, Debug)]
#[structopt(name = "basic")]
struct Opt {
    #[structopt(short = "v", long = "verbose")]
    verbose: bool,

    #[structopt(name = "constraints")]
    constraints: Vec<String>,

    #[structopt(short = "c", long = "count", default_value = "30")]
    count: u64,

    #[structopt(short = "m", long = "minion")]
    minion: String,

    #[structopt(short = "t", long = "maxtuples", default_value = "10000")]
    maxtuples: usize,
    

}

fn main() -> Result<(), simple_error::SimpleError> {
    let opt = Opt::from_args();
    println!("{:?}", opt);

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

    let config = test_types::MinionConfig { minionexec: &opt.minion, maxtuples: opt.maxtuples };

    v.clone().into_par_iter().try_for_each(|ref c| {
        try_with!(
            (0..opt.count)
                .into_par_iter()
                .try_for_each(|_| test_types::test_constraint(&config, &c)),
            format!("failure in {}", c.name)
        );
        try_with!(
            (0..opt.count)
                .into_par_iter()
                .try_for_each(|_| test_types::test_constraint_nested(&config, &c)),
            format!("failure in {} with nesting", c.name)
        );
        println!("Tested {}", c.name);
        Ok(())
    })?;

    println!("Parallel tests\n");
    v.clone().into_par_iter().try_for_each(|ref c| {
        try_with!(
            (0..opt.count)
                .into_par_iter()
                .try_for_each(|_| test_types::test_constraint_par(&config, &c)),
            format!("failure in {}", c.name)
        );

        println!("Tested {}", c.name);
        Ok(())
    })?;


    Ok(())
}
