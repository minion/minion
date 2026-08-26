#!/bin/bash
# Top-level entry point for Minion's test suite. See TESTING.md for the
# layered structure beneath these two modes.
#
# Usage:
#   ./test.sh --light                          # ~30 min, mirrors CI.yml
#   ./test.sh --heavy [--budget-hours N]       # default 24h, up to ~48h
#                     [--budget-minutes M]
#                     [--logdir DIR]
#
# --light builds bin-quick if missing and runs the same set of phases as
# .github/workflows/CI.yml: regression suite, library tests, random
# tester (default + nested smoke), minion-sys cargo tests, in-process
# tester smoke. Use this as the local "have I broken anything obvious"
# check.
#
# --heavy delegates to test_instances/deep_test.sh, which runs a
# layered sequence of bigger sweeps (size-factor 1/2/4/8, deep-nested
# trees, per-constraint isolation). Each phase honours the remaining
# budget so a slow phase doesn't starve later ones.

set -u
set -o pipefail

REPO=$(cd "$(dirname "$0")" && pwd)

MODE=""
HEAVY_ARGS=()

while [ $# -gt 0 ]; do
  case "$1" in
    --light) MODE=light; shift ;;
    --heavy) MODE=heavy; shift ;;
    --budget-hours|--budget-minutes|--logdir)
      HEAVY_ARGS+=("$1" "$2"); shift 2 ;;
    -h|--help)
      sed -n '2,/^$/{ s/^# \{0,1\}//; p; }' "$0"
      exit 0 ;;
    *)
      echo "Unknown argument: $1" >&2
      echo "Usage: $0 --light | --heavy [--budget-hours N]" >&2
      exit 2 ;;
  esac
done

if [ -z "$MODE" ]; then
  echo "Usage: $0 --light | --heavy [--budget-hours N]" >&2
  exit 2
fi

NPROC=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)

build_dir() {
  # build_dir <dir> <configure-flags...>
  local d="$1"; shift
  if [ -x "$REPO/$d/minion" ]; then
    echo "Reusing existing build at $d/"
    return 0
  fi
  echo "Building $d/ ($*)..."
  mkdir -p "$REPO/$d"
  (cd "$REPO/$d" && python3 ../configure.py "$@" && make -j"$NPROC")
}

run_phase() {
  # run_phase <name> <command...>
  local name="$1"; shift
  echo
  echo "=== $name ==="
  if ! "$@"; then
    echo
    echo "FAILED: $name" >&2
    exit 1
  fi
}

if [ "$MODE" = "light" ]; then
  run_phase "documentation sync check" \
    python3 "$REPO/sphinxdocs/check-docs-sync.py"

  build_dir bin-quick --quick
  build_dir bin-debug --quick --debug

  run_phase "regression suite (bin-quick)" \
    bash -c "cd '$REPO/test_instances' && bash ./run_tests.sh '$REPO/bin-quick/minion'"

  run_phase "regression suite (bin-debug)" \
    bash -c "cd '$REPO/test_instances' && bash ./run_tests.sh '$REPO/bin-debug/minion'"

  run_phase "libminion library tests (bin-quick)" \
    bash -c "cd '$REPO/bin-quick' && make test"

  run_phase "libminion library tests (bin-debug)" \
    bash -c "cd '$REPO/bin-debug' && make test"

  run_phase "random tester: default sweep" \
    bash -c "cd '$REPO/tester' && cargo run --release -- --minion '$REPO/bin-quick/minion'"

  run_phase "random tester: nested + size-factor smoke" \
    bash -c "cd '$REPO/tester' && cargo run --release -- --minion '$REPO/bin-quick/minion' \
      --count 10 --variant-count 20 --optioncount 100 \
      --nest-depth 2 --size-factor 2 --numthreads 4"

  run_phase "minion-sys cargo test" \
    bash -c "cd '$REPO/minion-sys' && cargo test --release"

  run_phase "in-process tester: baseline" \
    bash -c "cd '$REPO/tester' && cargo run --release -- \
      --in-process --count 3 --numthreads 4 --maxtuples 10000"

  run_phase "in-process tester: midsearch add-vars (no constraint on new vars)" \
    bash -c "cd '$REPO/tester' && cargo run --release -- \
      --in-process --midsearch --count 5 --numthreads 4 --maxtuples 10000"

  run_phase "in-process tester: midsearch constraint injection N=1" \
    bash -c "cd '$REPO/tester' && cargo run --release -- \
      --in-process --midsearch-constraints \
      --count 10 --numthreads 4 --maxtuples 10000"

  run_phase "in-process tester: midsearch constraint injection N=2" \
    bash -c "cd '$REPO/tester' && cargo run --release -- \
      --in-process --midsearch-constraints --midsearch-constraints-num-packets 2 \
      --count 5 --numthreads 4 --maxtuples 10000"

  run_phase "in-process tester: midsearch wrapped in nested parent" \
    bash -c "cd '$REPO/tester' && cargo run --release -- \
      --in-process --midsearch-constraints --midsearch-wrap-nested \
      --count 5 --numthreads 4 --maxtuples 10000"

  run_phase "in-process tester: midsearch add-vars + mixed-var constraint" \
    bash -c "cd '$REPO/tester' && cargo run --release -- \
      --in-process --midsearch-add-vars --midsearch-constraints-num-packets 2 \
      --count 30 --numthreads 4 --maxtuples 10000"

  run_phase "in-process tester: debug build smoke" \
    bash -c "cd '$REPO/tester' && DEBUG_MINION=1 cargo run --release -- \
      --in-process --count 2 --numthreads 4 --maxtuples 10000"

  run_phase "metamorphic optimisation sweep (exec)" \
    bash -c "cd '$REPO/tester' && cargo run --release -- \
      --minion '$REPO/bin-quick/minion' \
      --optimisation-sweep --count 10 --numthreads 4"

  run_phase "metamorphic optimisation sweep (in-process)" \
    bash -c "cd '$REPO/tester' && cargo run --release -- \
      --in-process --optimisation-sweep --count 5 --numthreads 4"

  echo
  echo "Light tests passed."
  exit 0
fi

if [ "$MODE" = "heavy" ]; then
  build_dir bin-quick --quick
  exec bash "$REPO/test_instances/deep_test.sh" \
    "$REPO/bin-quick/minion" \
    ${HEAVY_ARGS[@]+"${HEAVY_ARGS[@]}"}
fi
