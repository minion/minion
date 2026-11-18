#![cfg(unix)]

use std::fs;
use std::os::unix::fs::PermissionsExt;
use std::process::Command;
use std::sync::atomic::{AtomicUsize, Ordering};

static NEXT: AtomicUsize = AtomicUsize::new(0);

fn run_export(timeout: bool, extra: &[&str]) -> (std::process::Output, Vec<serde_json::Value>) {
    let directory = std::env::temp_dir().join(format!(
        "minion-semantics-test-{}-{}",
        std::process::id(),
        NEXT.fetch_add(1, Ordering::Relaxed)
    ));
    fs::create_dir_all(&directory).unwrap();
    let solver = directory.join("solver.sh");
    fs::write(
        &solver,
        r#"#!/bin/sh
while [ "$#" -gt 0 ]; do
  case "$1" in
    -solsout) sols=$2; shift 2 ;;
    -jsontableout) stats=$2; shift 2 ;;
    *) shift ;;
  esac
done
# The zero-variable true model has one solution: the empty assignment.
echo '' > "$sols"
echo "{\"Nodes\":\"1\",\"SolutionsFound\":\"1\",\"TimeOut\":\"$SEMANTICS_TIMEOUT\"}" > "$stats"
"#,
    )
    .unwrap();
    fs::set_permissions(&solver, fs::Permissions::from_mode(0o755)).unwrap();
    let output = Command::new(env!("CARGO_BIN_EXE_tester"))
        .current_dir(&directory)
        .arg("--minion")
        .arg(&solver)
        .args([
            "--export-solutions",
            "claims",
            "--constraints",
            "true",
            "--count",
            "1",
            "--numthreads",
            "1",
        ])
        .args(extra)
        .env("SEMANTICS_TIMEOUT", if timeout { "1" } else { "0" })
        .output()
        .unwrap();
    let claims = fs::read_dir(directory.join("claims"))
        .unwrap()
        .map(|entry| entry.unwrap().path())
        .filter(|path| {
            path.extension()
                .is_some_and(|extension| extension == "json")
        })
        .map(|path| serde_json::from_slice(&fs::read(path).unwrap()).unwrap())
        .collect();
    fs::remove_dir_all(directory).unwrap();
    (output, claims)
}

#[test]
fn exports_the_empty_assignment_for_a_zero_variable_true_model() {
    let (output, claims) = run_export(false, &[]);
    assert!(
        output.status.success(),
        "{}",
        String::from_utf8_lossy(&output.stderr)
    );
    assert_eq!(claims.len(), 1);
    assert_eq!(claims[0]["format"], "minion-solutions-v1");
    assert_eq!(claims[0]["variables"], serde_json::json!([]));
    assert_eq!(claims[0]["constraints"][0]["name"], "true");
    assert_eq!(claims[0]["solutions"], serde_json::json!([[]]));
}

#[test]
fn never_exports_a_timeout_as_a_complete_solution_set() {
    let (output, claims) = run_export(true, &[]);
    assert!(!output.status.success());
    assert!(claims.is_empty());
    assert!(String::from_utf8_lossy(&output.stderr).contains("complete enumeration timed out"));
}

#[test]
fn never_exports_a_capped_prefix_as_a_complete_solution_set() {
    let (output, claims) = run_export(false, &["--max-solutions", "1"]);
    assert!(!output.status.success());
    assert!(claims.is_empty());
    assert!(String::from_utf8_lossy(&output.stderr).contains("incomplete at solution cap"));
}

#[test]
fn checker_failure_fails_the_sweep_and_preserves_the_claim() {
    let (output, claims) = run_export(false, &["--check-solutions-with", "/usr/bin/false"]);
    assert!(!output.status.success());
    assert_eq!(claims.len(), 1);
    assert!(String::from_utf8_lossy(&output.stderr).contains("Lean rejected"));
}
