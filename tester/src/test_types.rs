use crate::constraint_def;
use crate::run_minion;
extern crate rand;

use self::rand::seq::SliceRandom;

use crate::run_minion::{NodeCheck, SolCheck};

use anyhow::{Result, anyhow, Context};

pub struct MinionConfig<'a> {
    pub minionexec: &'a str,
    pub maxtuples: usize,
}

pub fn test_constraint_with_flags(
    config: &MinionConfig,
    c: &constraint_def::ConstraintDef,
    flags: &[&str],
    node_check: NodeCheck,
    sol_check: SolCheck,
) -> Result<()> {
    let instance = constraint_def::build_random_instance(c);
    let ret = run_minion::get_minion_solutions(
        config.minionexec,
        &["-findallsols"],
        &instance,
        "original",
    )?;

    let mut args = flags.to_vec();
    args.push("-findallsols");
    let ret2 = run_minion::get_minion_solutions(config.minionexec, &args, &instance, "flags")?;

        node_check(ret.nodes, ret2.nodes).context(format!(
        "in {} vs {}",
        ret.filename,
        ret2.filename))?;
    
        sol_check(ret.solutions, ret2.solutions).context(format!(
        "in {} vs {}",
        ret.filename,
        ret2.filename))?;

    ret.cleanup.cleanup();
    ret2.cleanup.cleanup();
    Ok(())
}

pub fn test_constraint(
    config: &MinionConfig,
    c: &constraint_def::ConstraintDef,
) -> Result<()> {
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
        &["-findallsols"],
        &instance,
        "original",
    )?;
    let ret2 =
        run_minion::get_minion_solutions(config.minionexec, &["-findallsols"], &tups, "tuples")?;
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

pub fn test_constraint_par(
    config: &MinionConfig,
    c: &constraint_def::ConstraintDef,
) -> Result<()> {
    let instance = constraint_def::build_random_instance(c);
    let ret = run_minion::get_minion_solutions(
        config.minionexec,
        &["-findallsols"],
        &instance,
        "original",
    )?;
    let ret2 = run_minion::get_minion_solutions(
        config.minionexec,
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
        &["-findallsols"],
        &instance,
        "original",
    )?;
    let ret2 =
        run_minion::get_minion_solutions(config.minionexec, &["-findallsols"], &tups, "tuples")?;
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
