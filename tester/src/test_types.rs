use crate::constraint_def;
use crate::run_minion;
extern crate rand;

use self::rand::seq::SliceRandom;

use anyhow::{anyhow, Result};

pub struct MinionConfig<'a> {
    pub minionargs: Vec<String>,
    pub minionexec: &'a str,
    pub maxtuples: usize,
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

    let ret = run_minion::get_minion_solutions(
        config.minionexec,
        &config.minionargs,
        &["-findallsols"],
        &instance,
        "original",
    )?;
    let ret2 = run_minion::get_minion_solutions(
        config.minionexec,
        &config.minionargs,
        &["-findallsols"],
        &tups,
        "tuples",
    )?;
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
    let ret = run_minion::get_minion_solutions(
        config.minionexec,
        &config.minionargs,
        &["-findallsols"],
        &instance,
        "original",
    )?;
    let ret2 = run_minion::get_minion_solutions(
        config.minionexec,
        &config.minionargs,
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

    let ret = run_minion::get_minion_solutions(
        config.minionexec,
        &config.minionargs,
        &["-findallsols"],
        &instance,
        "original",
    )?;
    let ret2 = run_minion::get_minion_solutions(
        config.minionexec,
        &config.minionargs,
        &["-findallsols"],
        &tups,
        "tuples",
    )?;
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
