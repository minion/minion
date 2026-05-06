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

fn print_minion_short_tuples<F: Write>(f: &mut F, st: &ShortTuples) -> Result<()> {
    f.write_all(b"**SHORTTUPLELIST**\n")?;
    writeln!(f, "{} {}", st.name, st.data.len())?;
    for short in &st.data {
        let pairs = short
            .iter()
            .map(|(i, v)| format!("({},{})", i, v))
            .join(",");
        writeln!(f, "[{}]", pairs)?;
    }
    f.write_all(b"\n")?;
    Ok(())
}

fn print_minion_constraint_tuples<F: Write>(f: &mut F, con: &ConstraintInstance) -> Result<()> {
    if let Some(ref tuples) = con.tuples {
        print_minion_tuples(f, tuples)?;
    }
    if let Some(ref st) = con.short_tuples {
        print_minion_short_tuples(f, st)?;
    }
    // Recurse into nested children so any tuple-table-using constraint
    // (mddc, str2plus, table, etc.) embedded inside a parent
    // (watched-or, reify, ...) emits its **TUPLELIST** definition too;
    // otherwise the parser fails with "Undefined tuplelist: ...".
    for child in &con.child_constraints {
        print_minion_constraint_tuples(f, child)?;
    }

    Ok(())
}

fn print_minion_constraint_contents<F: Write>(f: &mut F, con: &ConstraintInstance) -> Result<()> {
    let mut i: usize = 0;

    // watched-and and watched-or take a constraint LIST (in `{...}`) in
    // minion syntax, not separate Constraint args. The other parent
    // constraints (reify, reifyimply, *-quick) interleave a single
    // Constraint with non-constraint args and are emitted inline.
    let needs_brace_list =
        con.constraint.name == "watched-and" || con.constraint.name == "watched-or";

    let varlist = (0..con.constraint.arg.len())
        .map(|list| match con.constraint.arg[list] {
            Arg::Var(..) => con.vars()[list][0].name.clone(),
            Arg::List(..) /*| Arg::TwoVars(..)*/ => {
                let commalist = con.vars()[list].iter().map(|x| &x.name).join(", ");
                format!("[{}]", commalist)
            }
            Arg::Tuples => con.tuples.as_ref().unwrap().name.clone(),
            Arg::ShortTuples => con.short_tuples.as_ref().unwrap().name.clone(),
            Arg::Constraint => {
                let mut out = Vec::new();
                print_minion_constraint_contents(&mut out, &(con.child_constraints[i])).unwrap();
                i += 1;
                let s = String::from_utf8(out).unwrap();
                // print_minion_constraint_contents writes a trailing
                // newline (from `writeln!`). Strip it when we're
                // embedding inline.
                s.trim_end_matches('\n').to_string()
            }
        })
        .join(", ");

    if needs_brace_list {
        writeln!(f, "{}({{{}}})", con.constraint.name, varlist)?;
    } else {
        writeln!(f, "{}({})", con.constraint.name, varlist)?;
    }

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
    writeln!(f, "**VARIABLES**")?;
    match doms.var_type {
        VarType::Constant => {}
        VarType::Bool => {
            writeln!(f, "BOOL {}", doms.name)?;
        }
        VarType::Bound => {
            writeln!(
                f,
                "BOUND {} {{{}..{}}}",
                doms.name,
                doms.domain.first().unwrap(),
                doms.domain.last().unwrap()
            )?;
        }
        VarType::Discrete => {
            writeln!(
                f,
                "DISCRETE {} {{{}..{}}}",
                doms.name,
                doms.domain.first().unwrap(),
                doms.domain.last().unwrap()
            )?;
        }
        VarType::SparseBound => {
            writeln!(
                f,
                "SPARSEBOUND {} {{ {} }}",
                doms.name,
                doms.domain
                    .iter()
                    .map(|i| i.to_string())
                    .collect::<Vec<String>>()
                    .join(",")
            )?;
        }
    }

    writeln!(f, "**CONSTRAINTS**")?;
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

/// Description of an optimisation wrapper added on top of an existing
/// `ConstraintInstance`: minion's `MINIMISING`/`MAXIMISING` directive on
/// a fresh `aux` BOUND variable, plus the two `sumleq`/`sumgeq`
/// constraints encoding `aux = sum(real_vars)`. The metamorphic
/// optimisation sweep wraps every constraint instance with one of
/// these so it always has a non-trivial objective.
pub struct OptimisationWrapper<'a> {
    pub aux_name: &'a str,
    pub aux_min: i64,
    pub aux_max: i64,
    pub minimise: bool,
    /// Names of the variables whose sum equals `aux`. Drawn from the
    /// instance's flat var list, with constants filtered out.
    pub sum_var_names: Vec<String>,
}

pub fn print_minion_file_pair_optimisation<F: Write>(
    fcon: &mut F,
    con: &ConstraintInstance,
    opt: &OptimisationWrapper,
) -> Result<()> {
    print_minion_prefix(fcon)?;
    print_variables_def(fcon, con)?;

    // Aux variable plus its containing **VARIABLES**/**CONSTRAINTS**
    // pair (mirroring print_variable_def's per-var emission shape).
    writeln!(fcon, "**VARIABLES**")?;
    writeln!(
        fcon,
        "BOUND {} {{{}..{}}}",
        opt.aux_name, opt.aux_min, opt.aux_max
    )?;
    writeln!(fcon, "**CONSTRAINTS**")?;

    // Search section. VARORDER must contain at least the objective
    // var so it gets assigned at solution nodes (otherwise minion
    // aborts with "The optimisation variable isn't assigned at a
    // solution node!"). Listing *only* the aux var is fine — minion
    // augments with the real vars at finaliseModel.
    writeln!(fcon, "**SEARCH**")?;
    writeln!(fcon, "VARORDER [{}]", opt.aux_name)?;
    if opt.minimise {
        writeln!(fcon, "MINIMISING {}", opt.aux_name)?;
    } else {
        writeln!(fcon, "MAXIMISING {}", opt.aux_name)?;
    }

    // Re-open **CONSTRAINTS** for the original constraint plus the
    // pair encoding aux = sum(vars).
    print_minion_constraint(fcon, con)?;
    let names = opt.sum_var_names.iter().join(", ");
    writeln!(fcon, "sumleq([{}], {})", names, opt.aux_name)?;
    writeln!(fcon, "sumgeq([{}], {})", names, opt.aux_name)?;

    print_minion_postfix(fcon)?;

    Ok(())
}
