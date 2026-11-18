"""Export and kernel-check small actual executions across constraint names."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess


ROOT = Path(__file__).resolve().parents[1]


def sources_hash():
    digest = hashlib.sha256()
    for path in sorted((ROOT / "Minion").glob("*.lean")):
        digest.update(path.name.encode())
        digest.update(path.read_bytes())
    return digest.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("traces", type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--certify", type=Path, default=ROOT / ".lake/build/bin/minion-certify")
    parser.add_argument("--jobs", type=int, default=2)
    parser.add_argument("--timeout", type=int, default=60)
    parser.add_argument("--both-results", action="store_true")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    previous = json.loads((args.traces / "report.json").read_text())
    if previous["accepted"] != previous["inputs"]:
        raise SystemExit("original trace/solution comparison has failures")
    initial_hash = sources_hash()
    checker = (args.output / "minion-proof-export-under-test").resolve()
    shutil.copy2(args.certify, checker)
    groups = {}
    for record in previous["records"]:
        names = record["constraints"]
        if len(names) != 1:
            raise SystemExit("choose a corpus with one original root per claim")
        result = "sat" if record["solutions"] else "unsat"
        groups.setdefault((names[0], result if args.both_results else "any"), []).append(record)
    chosen = []
    for key, records in sorted(groups.items()):
        def rank(record):
            useful = record["counts"].get("deductions", 0)
            return (not useful, Path(record["trace"]).stat().st_size)
        chosen.append((key, min(records, key=rank)))

    def check(item):
        (name, result), record = item
        safe = re.sub(r"[^a-zA-Z0-9]", "_", name)
        proof = (args.output / f"{safe}-{result}.lean").resolve()
        output = {"name": name, "result": result, "trace": record["trace"],
                  "proof": str(proof), "ok": False}
        try:
            exported = subprocess.run([str(checker), "--export-lean", str(proof), record["trace"]],
                                      capture_output=True, text=True, timeout=args.timeout)
            if exported.returncode:
                raise RuntimeError(exported.stdout + exported.stderr)
            checked = subprocess.run(["lake", "env", "lean", str(proof)], cwd=ROOT,
                                     capture_output=True, text=True, timeout=args.timeout)
            if checked.returncode or "sorryAx" in checked.stdout or "Lean.ofReduceBool" in checked.stdout:
                raise RuntimeError(checked.stdout + checked.stderr)
            output.update(ok=True, axioms=checked.stdout.strip())
        except Exception as error:
            output["error"] = str(error)
        return output

    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        records = list(pool.map(check, chosen))
    failures = [r for r in records if not r["ok"]]
    stable = initial_hash == sources_hash()
    report = {"inputs": len(records), "accepted": len(records) - len(failures),
              "names": len({r["name"] for r in records}), "sources_stable": stable,
              "proof_sources_sha256": initial_hash,
              "checker_sha256": hashlib.sha256(checker.read_bytes()).hexdigest(),
              "records": records}
    path = args.output / "report.json"
    path.write_text(json.dumps(report, indent=2) + "\n")
    print(f"Kernel-checked {report['accepted']}/{report['inputs']} certificates across {report['names']} names; sources stable: {stable}")
    print(f"Report: {path}")
    for record in failures[:10]:
        print(f"FAIL {record['name']} ({record['result']}): {record['error']}")
    return bool(failures) or not stable


if __name__ == "__main__":
    raise SystemExit(main())
