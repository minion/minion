#!/usr/bin/env python3
"""Check Cargo's actual build-script dependencies and no-op build behaviour."""

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


messages = build()
scripts = [
    message for message in messages
    if message["reason"] == "build-script-executed"
    and "minion-sys" in message["package_id"]
]
assert len(scripts) == 1, scripts
out_dir = Path(scripts[0]["out_dir"]).resolve()
directives = (out_dir.parent / "output").read_text().splitlines()
headers = []
environment = set()
for line in directives:
    if line.startswith("cargo:rerun-if-changed="):
        path = Path(line.split("=", 1)[1])
        if not path.is_absolute():
            path = crate / path
        path = path.resolve()
        assert not path.is_relative_to(out_dir), f"tracking own output: {path}"
        headers.append(path)
    elif line.startswith("cargo:rerun-if-env-changed="):
        environment.add(line.split("=", 1)[1])

assert any(path.name == "libwrapper.h" for path in headers), headers
assert any(path.name == "minion.h" for path in headers), headers
assert "MINION_SRC" in environment, environment
assert "BINDGEN_EXTRA_CLANG_ARGS" in environment, environment

# Checking the build-script artifact's freshness catches a rerun even if the
# generated Rust bindings happen to be identical.
artifacts = [
    message for message in build()
    if message["reason"] == "compiler-artifact"
    and "minion-sys" in message["package_id"]
]
assert artifacts, "Cargo did not report minion-sys artifacts"
assert all(message["fresh"] for message in artifacts), artifacts
print("minion-sys dependencies are tracked and a second build is fresh")
