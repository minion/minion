//! Export the model itself, not the tester's Rust truth table. Lean supplies an
//! independent interpretation of each constraint and enumerates all assignments.
use crate::constraint_def::{
    self, Arg, ConstraintDef, ConstraintInstance, MinionVariable, VarType,
};
use crate::run_minion;
use crate::run_minion_lib;
use crate::test_types::{Backend, MinionConfig};
use anyhow::{bail, ensure, Context, Result};
use serde_json::{json, Value};
use std::collections::HashMap;
use std::fs;
use std::path::{Path, PathBuf};
use std::process::Command;
use std::sync::Arc;

fn variables(instance: &ConstraintInstance) -> Result<Vec<Arc<MinionVariable>>> {
    let mut result: Vec<Arc<MinionVariable>> = Vec::new();
    let mut seen = HashMap::new();
    // Same first-declaration order as both the text printer and the FFI backend.
    for variable in instance.vars().iter().flatten() {
        if variable.var_type == VarType::Constant {
            continue;
        }
        if let Some(&index) = seen.get(&variable.name) {
            ensure!(
                Arc::ptr_eq(&result[index], variable),
                "distinct variables share a name"
            );
        } else {
            seen.insert(variable.name.clone(), result.len());
            result.push(variable.clone());
        }
    }
    Ok(result)
}

fn constraint_json(instance: &ConstraintInstance, ids: &HashMap<String, usize>) -> Result<Value> {
    let mut args = Vec::new();
    for (slot, kind) in instance.constraint.arg.iter().enumerate() {
        if !matches!(kind, Arg::Var(_) | Arg::List(_)) {
            continue;
        }
        let terms: Result<Vec<_>> = instance.top_level_vars()[slot]
            .iter()
            .enumerate()
            .map(|(index, variable)| {
                if variable.var_type == VarType::Constant {
                    ensure!(variable.domain.len() == 1, "non-singleton constant");
                    Ok(json!({"kind": "constant", "value": variable.domain[0]}))
                } else {
                    let id = ids.get(&variable.name).context("undeclared variable")?;
                    let kind = if instance.negated(slot, index) {
                        "not_bool"
                    } else {
                        "var"
                    };
                    Ok(json!({"kind": kind, "index": id}))
                }
            })
            .collect();
        args.push(terms?);
    }
    let children: Result<Vec<_>> = instance
        .child_constraints
        .iter()
        .map(|child| constraint_json(child, ids))
        .collect();
    Ok(json!({
        "name": instance.constraint.name,
        "args": args,
        "children": children?,
        "tuples": instance.tuples.as_ref().map(|t| &t.tupledata),
        "short_tuples": instance.short_tuples.as_ref().map(|t| &t.data),
    }))
}

fn solution_file(instance: &ConstraintInstance, solutions: &[Vec<i64>]) -> Result<Value> {
    let vars = variables(instance)?;
    ensure!(
        solutions.iter().all(|row| row.len() == vars.len()),
        "solution width mismatch"
    );
    let ids = vars
        .iter()
        .enumerate()
        .map(|(i, v)| (v.name.clone(), i))
        .collect();
    Ok(json!({
        "format": "minion-solutions-v1",
        "variables": vars.iter().map(|v| json!({"name": v.name, "domain": v.domain})).collect::<Vec<_>>(),
        "constraints": [constraint_json(instance, &ids)?],
        "solutions": solutions,
    }))
}

/// A separate sweep: do not use Rust tableisation to choose or validate models.
/// Reject incomplete runs and retry oversized domains before running Minion.
pub fn export_sweep(
    config: &MinionConfig,
    definitions: &[ConstraintDef],
    count: u64,
    nest_depth: usize,
    directory: &Path,
    max_assignments: usize,
    checker: Option<&Path>,
) -> Result<()> {
    ensure!(
        max_assignments > 0,
        "--semantics-max-assignments must be positive"
    );
    fs::create_dir_all(directory)?;
    let mut exported = 0;
    for definition in definitions {
        for depth in 0..=nest_depth {
            for _ in 0..count {
                let mut selected = None;
                for _ in 0..1000 {
                    let instance = if depth == 0 {
                        constraint_def::build_random_instance(definition)
                    } else {
                        constraint_def::build_random_nested_tree(definition, depth)
                    };
                    let vars = variables(&instance)?;
                    let size = vars
                        .iter()
                        .try_fold(1usize, |n, v| n.checked_mul(v.domain.len()));
                    if size.is_some_and(|n| n <= max_assignments) {
                        selected = Some(instance);
                        break;
                    }
                }
                let instance = selected.with_context(|| format!(
                    "could not generate {} at depth {depth} within {max_assignments} assignments after 1000 attempts",
                    definition.name
                ))?;
                let output = match config.backend {
                    Backend::Exec => run_minion::get_semantics_solutions(
                        config.minionexec,
                        &config.minionargs,
                        &instance,
                        config.max_solutions,
                    )?,
                    Backend::InProcess => run_minion_lib::get_semantics_solutions_in_process(
                        &instance,
                        config.run_options_no_seed(),
                        config.max_solutions,
                    )?,
                };
                ensure!(
                    !output.hit_solution_cap,
                    "semantic export incomplete at solution cap: {}",
                    output.command
                );
                let safe_name: String = definition
                    .name
                    .chars()
                    .map(|c| if c.is_ascii_alphanumeric() { c } else { '_' })
                    .collect();
                let path: PathBuf = directory.join(format!(
                    "{}-{}-depth{}-{}.json",
                    safe_name,
                    std::process::id(),
                    depth,
                    crate::counter::get_unique_value()
                ));
                let certificate = solution_file(&instance, output.raw_sols())?;
                let mut file = fs::OpenOptions::new()
                    .write(true)
                    .create_new(true)
                    .open(&path)?;
                serde_json::to_writer_pretty(&mut file, &certificate)?;
                // Keep the exact solver input next to the claim for debugging discrepancies.
                let mut model_text = Vec::new();
                crate::minion_instance::print_minion_file_pair(&mut model_text, &instance)?;
                fs::write(path.with_extension("minion"), model_text)?;
                if let Some(checker) = checker {
                    let result = Command::new(checker)
                        .args(["--max-assignments", &max_assignments.to_string()])
                        .arg(&path)
                        .output()
                        .with_context(|| format!("running {}", checker.display()))?;
                    if !result.status.success() {
                        bail!(
                            "Lean rejected {}\n{}{}",
                            path.display(),
                            String::from_utf8_lossy(&result.stdout),
                            String::from_utf8_lossy(&result.stderr)
                        );
                    }
                }
                output.cleanup.cleanup();
                exported += 1;
            }
            println!("Exported {} depth {depth}: {count} cases", definition.name);
        }
    }
    println!(
        "Exported {exported} complete solution sets to {}{}",
        directory.display(),
        if checker.is_some() {
            " (all accepted by Lean)"
        } else {
            ""
        }
    );
    Ok(())
}
