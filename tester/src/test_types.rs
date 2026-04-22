use crate::constraint_def;
use crate::run_minion;
use crate::run_minion::MinionOutput;
use crate::run_minion_lib;
extern crate rand;

use self::rand::seq::SliceRandom;

use anyhow::{Result, anyhow};

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
    match config.backend {
        Backend::Exec => run_minion::get_minion_solutions(
            config.minionexec,
            &config.minionargs,
            extraargs,
            instance,
            testname,
        ),
        Backend::InProcess => {
            let mut find_all = false;
            for &a in extraargs {
                match a {
                    "-findallsols" => find_all = true,
                    other => {
                        return Err(anyhow!(
                            "in-process backend does not support flag {:?}",
                            other
                        ));
                    }
                }
            }
            run_minion_lib::get_minion_solutions_in_process(instance, find_all, testname)
        }
    }
}

pub fn test_constraint(config: &MinionConfig, c: &constraint_def::ConstraintDef) -> Result<()> {
    let mut instance;
    let tups;
    loop {
        instance = constraint_def::build_random_instance(c);
        let tupstry = instance.tableise(config.maxtuples);
        if let Some(t) = tupstry {
            tups = t;
            break;
        }
    }

    let ret = run_solve(config, &["-findallsols"], &instance, "original")?;
    let ret2 = run_solve(config, &["-findallsols"], &tups, "tuples")?;
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

pub fn test_constraint_par(config: &MinionConfig, c: &constraint_def::ConstraintDef) -> Result<()> {
    let instance = constraint_def::build_random_instance(c);
    let ret = run_solve(config, &["-findallsols"], &instance, "original")?;
    let ret2 = run_solve(
        config,
        &["-findallsols", "-parallel"],
        &instance,
        "parallel",
    )?;
    let mut sortsols = ret.solutions.clone();
    let mut sortsols2 = ret2.solutions.clone();
    sortsols.sort();
    sortsols2.sort();
    if sortsols != sortsols2 {
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
    let mut sortsols = ret.solutions.clone();
    let mut sortsols2 = ret2.solutions.clone();
    sortsols.sort();
    sortsols2.sort();
    if sortsols != sortsols2 {
        return Err(anyhow!(format!(
            "Solutions not equal in {} vs {}",
            ret.filename, ret2.filename
        )));
    }

    ret.cleanup.cleanup();
    ret2.cleanup.cleanup();

    Ok(())
}

pub fn test_constraint_nested(
    config: &MinionConfig,
    c: &constraint_def::ConstraintDef,
) -> Result<()> {
    let nest_type = constraint_def::NESTED_CONSTRAINT_LIST
        .choose(&mut rand::thread_rng())
        .unwrap();
    let mut instance;
    let tups;
    loop {
        instance = constraint_def::build_random_instance_with_children(nest_type, &[c]);
        let tupstry = instance.tableise(config.maxtuples);
        if let Some(t) = tupstry {
            tups = t;
            break;
        }
    }

    let ret = run_solve(config, &["-findallsols"], &instance, "original")?;
    let ret2 = run_solve(config, &["-findallsols"], &tups, "tuples")?;
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
