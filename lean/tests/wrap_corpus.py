"""Derive Boolean-wrapper claims from independently checked leaf solution sets.

The truth of the leaf is read from its complete solution set. The generated
claims must subsequently be checked by Lean, and their Minion executions must
be compared with them using check_corpus.py.
"""
import argparse
from collections import Counter
import copy
import itertools
import json
from pathlib import Path
import subprocess


VARIANTS = {
    "reify": [("reify", False)],
    "reify-not": [("reify", True)],
    "imply": [("reifyimply", False)],
    "imply-quick": [("reifyimply-quick", False)],
    "reify-reify": [("reify", False), ("reify", False)],
    "imply-reify": [("reifyimply", True), ("reify", False)],
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("claims", type=Path, nargs="+")
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--per-family", type=int, default=10)
    parser.add_argument("--budget", type=int, default=10000)
    parser.add_argument("--checker", type=Path,
                        default=Path(__file__).resolve().parents[1] / ".lake/build/bin/minion-semantics")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    selected = Counter()
    paths = []
    for directory in args.claims:
        for source in sorted(directory.glob("*.json")):
            base = json.loads(source.read_text())
            if len(base["constraints"]) != 1:
                continue
            family = base["constraints"][0]["name"]
            if selected[family] >= args.per_family:
                continue
            size = 1
            for variable in base["variables"]:
                size *= len(variable["domain"])
            if size * 4 > args.budget:
                continue
            selected[family] += 1
            text = source.with_suffix(".minion").read_text()
            prefix, separator, final = text.rpartition("**CONSTRAINTS**")
            if not separator or "**EOF**" not in final:
                raise RuntimeError(f"unexpected input layout: {source}")
            target = final.split("**EOF**")[0].strip()
            base_solutions = {tuple(row) for row in base["solutions"]}
            base_assignments = list(itertools.product(*(v["domain"] for v in base["variables"])))
            for name, layers in VARIANTS.items():
                claim = copy.deepcopy(base)
                wrapped = target
                root = claim["constraints"][0]
                declarations = []
                known_names = {v["name"] for v in claim["variables"]}
                for i, (kind, negate_guard) in enumerate(layers):
                    guard = f"proofGuard{i}"
                    if guard in known_names:
                        raise RuntimeError("generated guard name collision")
                    index = len(claim["variables"])
                    claim["variables"].append({"name": guard, "domain": [0, 1]})
                    declarations.append(f"BOOL {guard}")
                    root = {"name": kind, "args": [[{"kind": "not_bool" if negate_guard else "var",
                                                        "index": index}]], "children": [root]}
                    wrapped = f"{kind}({wrapped},{'!' if negate_guard else ''}{guard})"
                claim["constraints"] = [root]
                solutions = []
                for assignment in base_assignments:
                    for guards in itertools.product((0, 1), repeat=len(layers)):
                        truth = assignment in base_solutions
                        for (kind, negate_guard), value in zip(layers, guards):
                            actual_guard = 1 - value if negate_guard else value
                            truth = actual_guard == int(truth) if kind == "reify" else actual_guard != 1 or truth
                        if truth:
                            solutions.append(list(assignment + guards))
                claim["solutions"] = solutions
                path = args.output / f"{source.stem}-{name}.json"
                # Never silently overwrite an earlier claim or its reproducer.
                with path.open("x") as handle:
                    json.dump(claim, handle, indent=2)
                    handle.write("\n")
                with path.with_suffix(".minion").open("x") as handle:
                    handle.write(prefix + "**VARIABLES**\n" + "\n".join(declarations) +
                                 "\n**CONSTRAINTS**\n" + wrapped + "\n**EOF**\n")
                paths.append(path)
    for start in range(0, len(paths), 100):
        result = subprocess.run([str(args.checker), "--max-assignments", str(args.budget),
                                 *map(str, paths[start:start + 100])], capture_output=True, text=True)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
    print(f"Generated and Lean-checked {len(paths)} wrapper claims from {dict(selected)}")


if __name__ == "__main__":
    main()
