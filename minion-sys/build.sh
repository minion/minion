#!/bin/bash

set -e
set -x

SCRIPT_DIR="$(readlink -f "$(dirname "$0")")"
cd "$SCRIPT_DIR"

# MINION_SRC is normally set by build.rs, which picks between vendor/
# (bundled copy, used for packaged releases) and .. (the parent minion
# repository, used during development). Fall back to the same logic here
# so build.sh can still be run standalone.
if [[ -z "${MINION_SRC-}" ]]; then
  if [[ -f "$SCRIPT_DIR/vendor/configure.py" ]]; then
    MINION_SRC="$SCRIPT_DIR/vendor"
  elif [[ -f "$SCRIPT_DIR/../configure.py" ]]; then
    MINION_SRC="$(readlink -f "$SCRIPT_DIR/..")"
  else
    echo "minion-sys: cannot locate Minion source tree." >&2
    echo "  Place a Minion checkout at minion-sys/vendor/," >&2
    echo "  run from within the Minion repository," >&2
    echo "  or set MINION_SRC to point at a Minion checkout." >&2
    exit 1
  fi
fi

if [[ ! -f "$MINION_SRC/configure.py" ]]; then
  echo "minion-sys: MINION_SRC=$MINION_SRC does not contain configure.py" >&2
  exit 1
fi

if [[ -z "$OUT_DIR" ]]; then
  echo "OUT_DIR env variable does not exist - did you run this script through cargo build?"
  exit 1
fi

echo "------ CONFIGURE STEP ------"
echo "Using Minion source at: $MINION_SRC"

mkdir -p "$OUT_DIR/build"
cd "$OUT_DIR/build"

if [[ ${DEBUG_MINION-default} != "default" ]]; then
  python3 "$MINION_SRC/configure.py" --quick --debug
else
  python3 "$MINION_SRC/configure.py" --quick --unoptimised --extraflags "-g -DDOM_ASSERT"
fi

echo "------ BUILD STEP ------"
cd "$OUT_DIR/build"
make libminion.a
