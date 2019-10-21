extern crate itertools;

use std::io::prelude::*;
use std::io::Result;

use self::itertools::Itertools;
use crate::constraint_def::*;

fn print_minion_tuples<F: Write>(f: &mut F, tuples: &Tuples) -> Result<()> {
    f.write_all(b"**TUPLELIST**\n")?;
    let tups = &tuples.tupledata;
    if tups.is_empty() {
        f.write_all(format!("{}  0 0\n", tuples.name).as_bytes())?;
    } else {
        f.write_all(format!("{}  {} {}\n", tuples.name, tups.len(), tups[0].len()).as_bytes())?;
        for tup in tups {
            for val in tup {
                f.write_all(format!("{} ", val).as_bytes())?;
            }
            f.write_all(b"\n")?;
        }
        f.write_all(b"\n")?;
    }
    Ok(())
}

fn print_minion_constraint_tuples<F: Write>(f: &mut F, con: &ConstraintInstance) -> Result<()> {
    if let Some(ref tuples) = con.tuples {
        print_minion_tuples(f, tuples)?;
    }

    Ok(())
}

fn print_minion_constraint_contents<F: Write>(f: &mut F, con: &ConstraintInstance) -> Result<()> {
    let mut i: usize = 0;

    let varlist = (0..con.constraint.arg.len())
        .map(|list| match con.constraint.arg[list] {
            Arg::Var(..) => con.vars()[list][0].name.clone(),
            Arg::List(..) /*| Arg::TwoVars(..)*/ => {
                let commalist = con.vars()[list].iter().map(|x| &x.name).join(", ");
                format!("[{}]", commalist)
            }
            Arg::Tuples => con.tuples.as_ref().unwrap().name.clone(),
            Arg::Constraint => {
                let mut out = Vec::new();
                print_minion_constraint_contents(&mut out, &(con.child_constraints[i])).unwrap();
                i += 1;
                String::from_utf8(out).unwrap()
            }
        })
        .join(", ");

    writeln!(f, "{}({})", con.constraint.name, varlist)?;

    Ok(())
}

fn print_minion_constraint<F: Write>(f: &mut F, con: &ConstraintInstance) -> Result<()> {
    print_minion_constraint_tuples(f, con)?;
    f.write_all(b"**CONSTRAINTS**\n")?;
    print_minion_constraint_contents(f, con)?;
    Ok(())
}

fn print_minion_prefix<F: Write>(f: &mut F) -> Result<()> {
    f.write_all(b"MINION 3\n")?;
    Ok(())
}

fn print_minion_postfix<F: Write>(f: &mut F) -> Result<()> {
    f.write_all(b"**EOF**\n")?;
    Ok(())
}

fn print_variable_def<F: Write>(f: &mut F, doms: &MinionVariable) -> Result<()> {
    if doms.var_type == VarType::Constant {
        return Ok(());
    }
    write!(f, "**VARIABLES**\n")?;
    match doms.var_type {
        VarType::Constant => {}
        VarType::Bool => {
            write!(f, "BOOL {}\n", doms.name)?;
        }
        VarType::Bound => {
            write!(
                f,
                "BOUND {} {{{}..{}}}\n",
                doms.name,
                doms.domain.first().unwrap(),
                doms.domain.last().unwrap()
            )?;
        }
        VarType::Discrete => {
            write!(
                f,
                "DISCRETE {} {{{}..{}}}\n",
                doms.name,
                doms.domain.first().unwrap(),
                doms.domain.last().unwrap()
            )?;
        }
    }

    write!(f, "**CONSTRAINTS**\n")?;
    let range = if doms.var_type == VarType::Bool {
        0..2
    } else {
        *doms.domain.first().unwrap()..*doms.domain.last().unwrap()
    };

    for val in range {
        if !doms.domain.contains(&val) {
            f.write_all(format!("diseq({}, {})\n", doms.name, val).as_bytes())?;
        }
    }
    Ok(())
}

fn print_variables_def<F: Write>(f: &mut F, con: &ConstraintInstance) -> Result<()> {
    for list in con.vars().iter() {
        for item in list.iter() {
            print_variable_def(f, item)?;
        }
    }

    Ok(())
}

pub fn print_minion_file_pair<F: Write>(fcon: &mut F, con: &ConstraintInstance) -> Result<()> {
    print_minion_prefix(fcon)?;
    print_variables_def(fcon, con)?;

    print_minion_constraint(fcon, con)?;

    print_minion_postfix(fcon)?;

    Ok(())
}
