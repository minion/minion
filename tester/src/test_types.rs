use constraint_def;
use run_minion;
extern crate rand;

use self::rand::Rng;

use simple_error::SimpleError;

use run_minion::{NodeCheck, SolCheck};

pub struct MinionConfig<'a> {
    pub minionexec: &'a str,
    pub maxtuples: usize
}

pub fn test_constraint_with_flags(
    config: &MinionConfig,
    c: &constraint_def::ConstraintDef,
    flags: &[&str],
    node_check: NodeCheck,
    sol_check: SolCheck,
) -> Result<(), SimpleError> {
    let instance = constraint_def::build_random_instance(c);
    let ret =
        run_minion::get_minion_solutions(config.minionexec, &["-findallsols"], &instance, "original")?;

    let mut args = flags.to_vec();
    args.push("-findallsols");
    let ret2 = run_minion::get_minion_solutions(config.minionexec, &args, &instance, "flags")?;

    try_with!(
        node_check(ret.nodes, ret2.nodes),
        "in {} vs {}",
        ret.filename,
        ret2.filename
    );
    try_with!(
        sol_check(ret.solutions, ret2.solutions),
        "in {} vs {}",
        ret.filename,
        ret2.filename
    );

    ret.cleanup.cleanup();
    ret2.cleanup.cleanup();
    Ok(())
}

pub fn test_constraint(
    config: &MinionConfig,
    c: &constraint_def::ConstraintDef,
) -> Result<(), SimpleError> {
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

    let ret =
        run_minion::get_minion_solutions(config.minionexec, &["-findallsols"], &instance, "original")?;
    let ret2 = run_minion::get_minion_solutions(config.minionexec, &["-findallsols"], &tups, "tuples")?;
    if ret.solutions != ret2.solutions {
        return Err(SimpleError::new(format!(
            "Solutions not equal in {} vs {}",
            ret.filename, ret2.filename
        )));
    }
    if instance.constraint.gac && ret.nodes != ret2.nodes {
        return Err(SimpleError::new(format!(
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
) -> Result<(), SimpleError> {
    let instance = constraint_def::build_random_instance(c);
    let ret =
        run_minion::get_minion_solutions(config.minionexec, &["-findallsols"], &instance, "original")?;
    let ret2 = run_minion::get_minion_solutions(config.minionexec, &["-findallsols","-parallel"], &instance, "parallel")?;
    let mut sortsols = ret.solutions.clone();
    let mut sortsols2 = ret2.solutions.clone();
    sortsols.sort();
    sortsols2.sort();
    if sortsols != sortsols2 {
        return Err(SimpleError::new(format!(
            "Solutions not equal in {} vs {}",
            ret.filename, ret2.filename
        )));
    }/*
    if instance.constraint.gac && ret.nodes != ret2.nodes {
        return Err(SimpleError::new(format!(
            "Propagator should be GAC, but node counts not equal in {} vs {}",
            ret.filename, ret2.filename
        )));
    }*/

    ret.cleanup.cleanup();
    ret2.cleanup.cleanup();

    Ok(())
}

pub fn test_constraint_nested(
    config: &MinionConfig,
    c: &constraint_def::ConstraintDef,
) -> Result<(), SimpleError> {
    let nest_type = rand::thread_rng()
        .choose(&constraint_def::NESTED_CONSTRAINT_LIST)
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

    let ret =
        run_minion::get_minion_solutions(config.minionexec, &["-findallsols"], &instance, "original")?;
    let ret2 = run_minion::get_minion_solutions(config.minionexec, &["-findallsols"], &tups, "tuples")?;
    if ret.solutions != ret2.solutions {
        return Err(SimpleError::new(format!(
            "Solutions not equal in {} vs {}",
            ret.filename, ret2.filename
        )));
    }
    if instance.constraint.gac && ret.nodes != ret2.nodes {
        return Err(SimpleError::new(format!(
            "Propagator should be GAC, but node counts not equal in {} vs {}",
            ret.filename, ret2.filename
        )));
    }

    ret.cleanup.cleanup();
    ret2.cleanup.cleanup();
    Ok(())
}
