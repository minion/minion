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

/// Mid-search variable-injection test.
///
/// Solve the instance once to establish a baseline solution set, then
/// solve a second time with a callback that adds `new_var_count` fresh
/// boolean variables after `inject_after` solutions.
///
/// `minion_newVarMidsearch` appends the new variable to the solver's
/// *aux* block, which by design gets a single satisfying assignment per
/// decision leaf (search jumps out of aux vars after each solution).
/// So the properties we verify are:
///   * pre-injection rows equal the baseline's first `inject_after` rows;
///   * post-injection produces *exactly one* row per remaining baseline
///     leaf, in the same order;
///   * each injected var's value lies within its declared domain.
///
/// Skipped when baseline has <= `inject_after` solutions — there would
/// be nothing to verify after injection.
pub fn test_constraint_midsearch_add_vars(
    config: &MinionConfig,
    c: &constraint_def::ConstraintDef,
    inject_after: usize,
    new_var_count: usize,
) -> Result<()> {
    // Exec backend can't mutate a running solver; only in-process supports this.
    if config.backend == Backend::Exec {
        return Ok(());
    }

    let instance = constraint_def::build_random_instance(c);

    let baseline =
        crate::run_minion_lib::get_minion_solutions_in_process(&instance, true, "baseline")?;

    if baseline.solutions.len() <= inject_after {
        // Not enough baseline solutions to have anything post-injection.
        return Ok(());
    }

    let new_vars: Vec<(String, minion_sys::ast::VarDomain)> = (0..new_var_count)
        .map(|_| {
            (
                format!("mid_{}", crate::counter::get_unique_value()),
                minion_sys::ast::VarDomain::Bool,
            )
        })
        .collect();

    let got = crate::run_minion_lib::run_inject_vars_after(
        &instance,
        inject_after,
        &new_vars,
        "midsearch",
    )?;

    if !got.injected {
        return Err(anyhow!(
            "midsearch: expected injection to fire at count {} (baseline has {} solutions)",
            inject_after,
            baseline.solutions.len()
        ));
    }

    // Pre-injection must equal baseline[..inject_after] exactly.
    let expected_pre = &baseline.solutions[..inject_after];
    if got.pre_injection.as_slice() != expected_pre {
        return Err(anyhow!(
            "midsearch: pre-injection differs from baseline prefix ({} vs {} rows)",
            got.pre_injection.len(),
            expected_pre.len()
        ));
    }

    // Post-injection: one row per remaining baseline leaf, in the same order.
    let remaining = &baseline.solutions[inject_after..];
    if got.post_injection.len() != remaining.len() {
        if std::env::var("TESTER_DEBUG").is_ok() {
            eprintln!("baseline: {:?}", baseline.solutions);
            eprintln!("pre:      {:?}", got.pre_injection);
            eprintln!("post:     {:?}", got.post_injection);
        }
        return Err(anyhow!(
            "midsearch: post-injection has {} rows but {} remaining baseline leaves",
            got.post_injection.len(),
            remaining.len()
        ));
    }

    // Split each post row into (decision-var values, new-var values), then
    // check:
    //   * decision-var values match the baseline remaining row at the same index;
    //   * each new-var value is within its declared domain.
    let decision_len = baseline
        .solutions
        .first()
        .map(|r| r.len())
        .unwrap_or(0);
    for (idx, row) in got.post_injection.iter().enumerate() {
        if row.len() != decision_len + new_var_count {
            return Err(anyhow!(
                "midsearch: post-injection row {} has length {} (expected {})",
                idx,
                row.len(),
                decision_len + new_var_count
            ));
        }
        let (decision_part, new_part) = row.split_at(decision_len);
        if decision_part != remaining[idx].as_slice() {
            return Err(anyhow!(
                "midsearch: post-injection row {} decision part {:?} differs from baseline {:?}",
                idx,
                decision_part,
                remaining[idx]
            ));
        }
        for (k, (_name, domain)) in new_vars.iter().enumerate() {
            let v = new_part[k];
            let in_domain = match domain {
                minion_sys::ast::VarDomain::Bool => v == 0 || v == 1,
                minion_sys::ast::VarDomain::Bound(lo, hi) => {
                    v >= *lo as i64 && v <= *hi as i64
                }
                minion_sys::ast::VarDomain::Discrete(lo, hi) => {
                    v >= *lo as i64 && v <= *hi as i64
                }
                _ => true,
            };
            if !in_domain {
                return Err(anyhow!(
                    "midsearch: post-injection row {} new var {} value {} not in domain {:?}",
                    idx,
                    k,
                    v,
                    domain
                ));
            }
        }
    }

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
