use constraint_def;
use run_minion;
extern crate rand;

use self::rand::Rng;

use simple_error::SimpleError;

pub fn test_constraint(c: &constraint_def::ConstraintDef) -> Result<(), SimpleError> {
    let instance = constraint_def::build_random_instance(c);
    let tups = instance.tableise();
    let ret = run_minion::get_minion_solutions(
        "/home/caj/bin/minion",
        &["-findallsols"],
        &instance,
        "original",
    )?;
    let ret2 = run_minion::get_minion_solutions(
        "/home/caj/bin/minion",
        &["-findallsols"],
        &tups,
        "tuples",
    )?;
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
    Ok(())
}

pub fn test_constraint_nested(c: &constraint_def::ConstraintDef) -> Result<(), SimpleError> {
    let nest_type = rand::thread_rng().choose(&constraint_def::NESTED_CONSTRAINT_LIST).unwrap();
    let instance = constraint_def::build_random_instance_with_children(nest_type, &[c]);
    let tups = instance.tableise();
    let ret = run_minion::get_minion_solutions(
        "/home/caj/bin/minion",
        &["-findallsols"],
        &instance,
        "original",
    )?;
    let ret2 = run_minion::get_minion_solutions(
        "/home/caj/bin/minion",
        &["-findallsols"],
        &tups,
        "tuples",
    )?;
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
    Ok(())
}