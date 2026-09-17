#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root/minion-sys"
export CARGO_TARGET_WASM32_UNKNOWN_EMSCRIPTEN_RUNNER="node $root/minion-sys/tools/emscripten-runner.mjs"
# Final-link flags belong to the executable, not the dependency's build script.
export CARGO_TARGET_WASM32_UNKNOWN_EMSCRIPTEN_RUSTFLAGS="-C link-arg=-sDEFAULT_TO_CXX=1 -C link-arg=-fwasm-exceptions -C link-arg=-sALLOW_MEMORY_GROWTH=1 -C link-arg=-sSTACK_SIZE=8388608"
cargo +"${MINION_WASM_RUST:-1.98.0}" test --target wasm32-unknown-emscripten \
  --test test_emscripten --test test_gcc --test test_errors --test test_limits \
  --test test_midsearch_gcc_reinit --test test_midsearch_bound_branch \
  --test test_midsearch_backtrack_state "$@" -- --test-threads=1
