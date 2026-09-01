#!/bin/bash
#
# minion-sys generates the constraint sources that configure.py generates, so
# that the crate builds with nothing but a C++ compiler. Two implementations of
# one generator can drift, and a drift here is not a build failure -- it is a
# library that disagrees with the minion binary about which constraints exist.
#
# This runs both and diffs them. Everything must match byte for byte except:
#   - the #include of the declaring header: configure.py emits a path relative
#     to the build directory, build.rs emits one relative to minion/ and lets
#     -I resolve it. Normalised below.
#   - BuildDefines.h, which carries the git revision.

set -e
set -u

REPO="$(cd "$(dirname "$0")/.." && pwd)"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

echo "--- configure.py (reference) ---"
mkdir -p "$WORK/ref"
(cd "$WORK/ref" && python3 "$REPO/configure.py" --quick >/dev/null)

echo "--- build.rs (candidate) ---"
(cd "$REPO/minion-sys" && cargo build)
GEN="$(find "$REPO/minion-sys/target" -type d -name minion_generated -print -quit)"
if [[ -z "$GEN" ]]; then
  echo "no minion_generated directory found under minion-sys/target" >&2
  exit 1
fi

# "…/anything/minion/constraints/foo.h" -> "constraints/foo.h"
normalise() {
  sed -E 's|#include "[^"]*/minion/|#include "|'
}

status=0
compare() {
  local name="$1"
  normalise < "$WORK/ref/src/$name" > "$WORK/a"
  normalise < "$GEN/$name" > "$WORK/b"
  if diff -u "$WORK/a" "$WORK/b" > "$WORK/d"; then
    echo "ok   $name"
  else
    echo "DIFF $name"
    head -40 "$WORK/d"
    status=1
  fi
}

for f in ConstraintEnum.h constraint_defs.h BuildStaticStart.cpp; do
  compare "$f"
done

# The chunk count is derived from the constraint count, so a mismatch in the
# set of files is itself a failure.
ref_chunks=$(find "$WORK/ref/src" -name 'build_constraint_*.cpp' | wc -l | tr -d ' ')
gen_chunks=$(find "$GEN" -name 'build_constraint_*.cpp' | wc -l | tr -d ' ')
if [[ "$ref_chunks" != "$gen_chunks" ]]; then
  echo "DIFF chunk count: configure.py $ref_chunks, build.rs $gen_chunks"
  status=1
fi
for i in $(seq 1 "$ref_chunks"); do
  compare "build_constraint_$i.cpp"
done

if [[ $status -eq 0 ]]; then
  echo
  echo "generators agree ($ref_chunks constraint files)"
fi
exit $status
