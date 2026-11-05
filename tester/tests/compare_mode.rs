#![cfg(unix)]

use std::fs;
use std::os::unix::fs::PermissionsExt;
use std::process::Command;
use std::sync::atomic::{AtomicUsize, Ordering};

static NEXT: AtomicUsize = AtomicUsize::new(0);

// A solver stub isolates CLI coverage from solver correctness. It records each
// rendered input and emits valid output files without searching the random tree.
fn comparison(args: &[&str], cap: bool) -> (std::process::Output, Vec<String>) {
    let dir = std::env::temp_dir().join(format!(
        "minion-compare-test-{}-{}",
        std::process::id(),
        NEXT.fetch_add(1, Ordering::Relaxed)
    ));
    fs::create_dir_all(&dir).unwrap();
    let solver = dir.join("solver.sh");
    fs::write(
        &solver,
        r#"#!/bin/sh
while [ "$#" -gt 0 ]; do
    case "$1" in
        -solsout) sols=$2; shift 2 ;;
        -jsontableout) stats=$2; shift 2 ;;
        *) model=$1; shift ;;
    esac
done
cp "$model" "$model.captured"
if [ "$MINION_COMPARE_TEST_CAP" = 1 ]; then
    echo 0 > "$sols"
    count=1
else
    : > "$sols"
    count=0
fi
echo "{\"Nodes\":\"1\",\"SolutionsFound\":\"$count\",\"TimeOut\":\"0\"}" > "$stats"
"#,
    )
    .unwrap();
    fs::set_permissions(&solver, fs::Permissions::from_mode(0o755)).unwrap();
    let output = Command::new(env!("CARGO_BIN_EXE_tester"))
        .current_dir(&dir)
        .arg("--minion")
        .arg(&solver)
        .arg("--compare-with")
        .arg(&solver)
        .args(["--constraints", "gcc", "--count", "1", "--numthreads", "1"])
        .args(args)
        .env("MINION_COMPARE_TEST_CAP", if cap { "1" } else { "0" })
        .output()
        .unwrap();
    let models = fs::read_dir(dir.join("tempdir"))
        .unwrap()
        .map(|entry| entry.unwrap().path())
        .filter(|path| path.extension().is_some_and(|ext| ext == "captured"))
        .map(|path| fs::read_to_string(path).unwrap())
        .collect();
    fs::remove_dir_all(dir).unwrap();
    (output, models)
}

#[test]
fn comparison_runs_flat_and_each_requested_nesting_depth() {
    let (output, models) = comparison(&["--nest-depth", "2"], false);
    assert!(
        output.status.success(),
        "{}",
        String::from_utf8_lossy(&output.stderr)
    );
    let stdout = String::from_utf8_lossy(&output.stdout);
    for depth in 0..=2 {
        assert!(
            stdout.contains(&format!("comparison depth {depth}")),
            "{stdout}"
        );
    }
    assert_eq!(models.len(), 6); // primary and comparison, at each of three depths
    assert!(models.iter().any(|model| model.contains("reify(")
        || model.contains("reifyimply(")
        || model.contains("reifyimply-quick(")
        || model.contains("watched-and(")
        || model.contains("watched-or(")));
}

#[test]
fn matching_capped_prefixes_do_not_pass_a_solver_comparison() {
    let (output, _) = comparison(&["--max-solutions", "1"], true);
    assert!(!output.status.success());
    assert!(
        String::from_utf8_lossy(&output.stderr).contains("comparison incomplete at solution cap")
    );
}
