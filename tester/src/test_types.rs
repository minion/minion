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

/// Mid-search *constraint*-injection test.
///
/// The purpose is to exercise each Minion constraint as an *injected*
/// constraint: does the solver correctly apply a constraint that was
/// added mid-search? The base problem is a bag of random constraints
/// purely as a carrier to give the solver something non-trivial to
/// search through.
///
/// Setup (all with a shared seed `s`):
///   * Build `base_defs.len()` random instances — these form the base
///     model. All their variables and top-level constraints go into
///     the model.
///   * Build one random instance of `inject_def`. Its variables are
///     also declared up-front (so baseline enumerates over them as
///     free search vars), but its top-level constraint is NOT in the
///     model at the start.
///
/// Three runs under seed `s`:
///   * baseline  — base model only; inject-instance's vars are free.
///   * full      — base model + inject constraint from the start.
///   * injected  — base model; inject constraint added via
///                 MidSearchContext::add_constraint after
///                 `inject_after` solutions.
///
/// Invariants (STATIC variable order):
///   * `injected[0..A] == baseline[0..A]` exactly.
///   * `injected[A..]` equals `full` with every element of
///     `full ∩ baseline[0..A]` dropped once, in `full`'s order.
///
/// Tests are skipped when baseline has fewer than `inject_after`
/// solutions (nothing post-injection to compare).
///
/// On failure the seed is logged so the trial can be re-run.
pub fn test_constraint_midsearch_inject_constraint(
    config: &MinionConfig,
    base_defs: &[&constraint_def::ConstraintDef],
    inject_def: &constraint_def::ConstraintDef,
    inject_after: usize,
) -> Result<()> {
    if config.backend == Backend::Exec {
        return Ok(());
    }

    let seed: u32 = rand::random();

    let base_instances: Vec<constraint_def::ConstraintInstance> = base_defs
        .iter()
        .map(|d| constraint_def::build_random_instance(d))
        .collect();
    let base_refs: Vec<&constraint_def::ConstraintInstance> = base_instances.iter().collect();
    let inject_instance = constraint_def::build_random_instance(inject_def);

    let log_seed = |msg: &str| -> String {
        format!(
            "inject={} base={:?} seed={seed:#x} inject_after={inject_after} — {msg}",
            inject_def.name,
            base_defs.iter().map(|d| d.name.as_str()).collect::<Vec<_>>(),
            msg = msg,
        )
    };

    // Baseline.
    let baseline = crate::run_minion_lib::run_multi(
        &base_refs,
        &[&inject_instance],
        seed,
        "baseline",
    )
    .map_err(|e| anyhow!("{}", log_seed(&format!("baseline: {e}"))))?;

    if baseline.solutions.len() <= inject_after {
        // Not enough baseline solutions to exercise the post-injection
        // branch. Silently skip — this isn't a constraint failure, just
        // a trivial trial.
        return Ok(());
    }

    // Injected run.
    let injected = crate::run_minion_lib::run_multi_injected(
        &base_refs,
        &[&inject_instance],
        &[(inject_after, &inject_instance)],
        seed,
        "injected",
    )
    .map_err(|e| anyhow!("{}", log_seed(&format!("injected: {e}"))))?;

    if injected.injections_fired != 1 {
        return Err(anyhow!(
            "{}",
            log_seed(&format!(
                "injection did not fire ({} solutions produced)",
                injected.solutions.len()
            ))
        ));
    }

    // Full run.
    let mut full_with_constraints: Vec<&constraint_def::ConstraintInstance> = base_refs.clone();
    full_with_constraints.push(&inject_instance);
    let full = crate::run_minion_lib::run_multi(
        &full_with_constraints,
        &[],
        seed,
        "full",
    )
    .map_err(|e| anyhow!("{}", log_seed(&format!("full: {e}"))))?;

    // Invariant 1: pre-injection equals baseline prefix exactly.
    if injected.solutions[..inject_after] != baseline.solutions[..inject_after] {
        if std::env::var("TESTER_DEBUG").is_ok() {
            eprintln!("baseline[..A]: {:?}", &baseline.solutions[..inject_after]);
            eprintln!("injected[..A]: {:?}", &injected.solutions[..inject_after]);
        }
        return Err(anyhow!(
            "{}",
            log_seed("pre-injection rows differ from baseline prefix")
        ));
    }

    // Invariant 2: injected[A..] == full minus (full ∩ baseline[0..A]).
    use std::collections::HashSet;
    let seen: HashSet<&Vec<i64>> = baseline.solutions[..inject_after].iter().collect();
    let expected_post: Vec<Vec<i64>> = full
        .solutions
        .iter()
        .filter(|r| !seen.contains(*r))
        .cloned()
        .collect();
    let got_post = &injected.solutions[inject_after..];
    if got_post != expected_post.as_slice() {
        if std::env::var("TESTER_DEBUG").is_ok() {
            eprintln!(
                "baseline ({} sols): {:?}",
                baseline.solutions.len(),
                baseline.solutions
            );
            eprintln!("full ({} sols): {:?}", full.solutions.len(), full.solutions);
            eprintln!(
                "injected[..{inject_after}]: {:?}",
                &injected.solutions[..inject_after]
            );
            eprintln!(
                "injected[{inject_after}..] ({} sols): {:?}",
                got_post.len(),
                got_post
            );
            eprintln!(
                "expected post ({} sols): {:?}",
                expected_post.len(),
                expected_post
            );
        }
        return Err(anyhow!(
            "{}",
            log_seed(&format!(
                "post-injection differs (got {} rows, expected {})",
                got_post.len(),
                expected_post.len()
            ))
        ));
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
