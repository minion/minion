#!/usr/bin/env python3
"""Check that unchanged minion-sys sources do not trigger another build."""

import json
from pathlib import Path
import subprocess


crate = Path(__file__).resolve().parents[1]
command = [
    "cargo", "build", "--manifest-path", str(crate / "Cargo.toml"),
    "--release", "--locked", "--message-format=json",
]


def build():
    result = subprocess.run(command, check=True, text=True, stdout=subprocess.PIPE)
    return [json.loads(line) for line in result.stdout.splitlines()]


build()

# Generated headers once caused the C++ build to rerun on every invocation.
# Check Cargo's public JSON freshness flag rather than its internal cache files.
artifacts = [
    message for message in build()
    if message["reason"] == "compiler-artifact"
    and Path(message["manifest_path"]).resolve() == crate / "Cargo.toml"
    and "lib" in message["target"]["kind"]
]
assert artifacts, "Cargo did not report the minion-sys library"
assert all(message["fresh"] for message in artifacts), (
    "minion-sys rebuilt despite unchanged sources", artifacts
)
print("minion-sys reused the previous build: unchanged sources did not rebuild")
