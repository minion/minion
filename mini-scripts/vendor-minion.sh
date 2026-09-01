#!/bin/bash
#
# Copy Minion's C++ source into minion-sys/vendor/, ready for `cargo publish`.
#
# minion-sys embeds Minion rather than looking for an installed copy: the
# libminion C interface is not stable, so a crate version and a system Minion
# that happen to be paired at build time would disagree silently. vendor/ is
# gitignored -- an explicit `include` in Cargo.toml is what gets it into the
# published .crate, since cargo skips gitignore rules when `include` is set.
#
# Run this before `cargo publish` (or `cargo package`), from anywhere.

set -e
set -u

REPO="$(cd "$(dirname "$0")/.." && pwd)"
VENDOR="$REPO/minion-sys/vendor"

ALLOW_DIRTY=0
if [[ "${1-}" == "--allow-dirty" ]]; then
  ALLOW_DIRTY=1
elif [[ $# -gt 0 ]]; then
  echo "usage: $(basename "$0") [--allow-dirty]" >&2
  exit 2
fi

# What is vendored must correspond to a commit, or the GIT_VERSION stamped into
# the library is a lie about which source is in the crate. --allow-dirty is for
# local dry-runs, never for a release.
if [[ $ALLOW_DIRTY -eq 0 && -n "$(git -C "$REPO" status --porcelain --untracked-files=no)" ]]; then
  echo "vendor-minion: the working tree has uncommitted changes." >&2
  echo "  Commit them first: the vendored source is stamped with HEAD's revision." >&2
  git -C "$REPO" status --short --untracked-files=no >&2
  exit 1
fi

rm -rf "$VENDOR"
mkdir -p "$VENDOR/minion"

# Everything under minion/ except:
#   system/minlib/tests/  - minlib's own unit tests, never compiled
#   CT_ALLDIFF_CIARAN.cpp - in no source list and #included nowhere
tar -c -C "$REPO" \
    --exclude='minion/system/minlib/tests' \
    --exclude='minion/build_constraints/CT_ALLDIFF_CIARAN.cpp' \
    minion | tar -x -C "$VENDOR"

cp "$REPO/LICENSE.txt" "$VENDOR/LICENSE.txt"

# build.rs reads this instead of asking git, which is absent in a published
# crate. Same format as configure.py's GIT_VER.
git -C "$REPO" log -1 --pretty=format:'%h (%ai)' > "$VENDOR/GIT_VERSION"
echo >> "$VENDOR/GIT_VERSION"

# A missing source file here becomes a compile error for whoever installs the
# crate, so check now rather than at `cargo publish --dry-run`.
for required in minion/libwrapper.h minion/libwrapper.cpp minion/minion.cpp; do
  if [[ ! -f "$VENDOR/$required" ]]; then
    echo "vendor-minion: $required is missing from the vendored copy" >&2
    exit 1
  fi
done

files=$(find "$VENDOR" -type f | wc -l | tr -d ' ')
size=$(du -sk "$VENDOR" | cut -f1)
echo "vendored $files files (${size}K) into minion-sys/vendor"
echo "revision: $(cat "$VENDOR/GIT_VERSION")"
echo
echo "Package with:  cd minion-sys && cargo package --allow-dirty"
echo "(--allow-dirty because \`include\` overrides gitignore, so cargo counts"
echo " the gitignored vendor/ as uncommitted.)"
echo
echo "Note: build.rs prefers vendor/ over ../, so minion-sys now builds against"
echo "this copy rather than the working tree. Edits under minion/ will not be"
echo "picked up until you remove it:"
echo "    rm -rf minion-sys/vendor"
