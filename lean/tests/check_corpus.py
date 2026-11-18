"""Trace saved tester inputs, certify the traces, and compare solution claims.

Each input is a `.minion` file with a `.json` claim of its complete solution
set, as written by the tester's `--export-solutions`. Large generated data
belongs outside the checkout. Example:

  python3 lean/tests/check_corpus.py /tmp/claims /tmp/traces --minion /path/to/minion
"""
import argparse
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("claims", type=Path)
    parser.add_argument("traces", type=Path)
    parser.add_argument("--minion", required=True, type=Path)
    parser.add_argument("--certify", type=Path,
                        default=Path(__file__).resolve().parents[1] / ".lake/build/bin/minion-certify")
    parser.add_argument("--no-matching-discovery", action="store_true")
    parser.add_argument("--preprocess")
    parser.add_argument("--prop-node")
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("--timeout", type=int, default=60)
    args = parser.parse_args()
    args.traces.mkdir(parents=True, exist_ok=True)
    # Keep a run stable while development rebuilds the original executables.
    minion_snapshot = (args.traces / "minion-under-test").resolve()
    certify_snapshot = (args.traces / "minion-certify-under-test").resolve()
    shutil.copy2(args.minion, minion_snapshot)
    shutil.copy2(args.certify, certify_snapshot)

    def run(command):
        result = subprocess.run(list(map(str, command)), text=True, capture_output=True,
                                timeout=args.timeout)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        return result

    def check(source):
        trace = args.traces / (source.stem + ".trace")
        events_path = args.traces / (source.stem + ".events.jsonl")
        record = {"input": str(source), "trace": str(trace), "ok": False}
        try:
            claim = json.loads(source.with_suffix(".json").read_text())
            record["constraints"] = [c["name"] for c in claim["constraints"]]
            propagation = []
            if args.preprocess:
                propagation += ["-preprocess", args.preprocess]
            if args.prop_node:
                propagation += ["-prop-node", args.prop_node]
            start = time.perf_counter()
            solver = run([minion_snapshot, *propagation, "-findallsols", "-noprintsols",
                          "-annotations", trace, source])
            record["minion_ms"] = (time.perf_counter() - start) * 1000
            nodes = re.search(r"Total Nodes: (\d+)", solver.stdout)
            record["solver_nodes"] = int(nodes[1]) if nodes else None
            flags = ["--no-matching-discovery"] if args.no_matching_discovery else []
            start = time.perf_counter()
            result = run([certify_snapshot, *flags, trace])
            record["checker_ms"] = (time.perf_counter() - start) * 1000
            if "complete search tree CERTIFIED" not in result.stdout:
                raise RuntimeError("missing search-certificate acceptance")
            # The solutions the certificate covers, in the trace's own column order.
            run([certify_snapshot, "--to-json", events_path, trace])
            observed_names = None
            solutions = []
            event_count = pruning_count = 0
            with events_path.open() as stream:
                for line in stream:
                    event = json.loads(line)
                    if event_count == 0:
                        observed_names = [v["name"] for v in event["model"]["variables"]]
                    event_count += 1
                    pruning_count += event["event"] == "prune"
                    if event["event"] == "solution":
                        solutions.append(event["values"])
            events_path.unlink()
            record["events"] = event_count
            record["trace_bytes"] = trace.stat().st_size
            record["prune_events"] = pruning_count
            record["model_variables"] = len(claim["variables"])
            product = 1
            for variable in claim["variables"]:
                product *= len(variable["domain"])
            record["model_assignments"] = product

            def names(constraint):
                return [constraint["name"], *(name for child in constraint["children"] for name in names(child))]

            record["semantic_constraints"] = [name for c in claim["constraints"] for name in names(c)]
            expected_names = [v["name"] for v in claim["variables"]]
            if observed_names != expected_names:
                raise RuntimeError("variable column order differs from exported solution claim")
            if sorted(solutions) != sorted(claim["solutions"]):
                raise RuntimeError("trace solutions differ from independently checked solution claim")
            record.update(ok=True, solutions=len(solutions), certified=result.stdout.strip())
            record["counts"] = {name: int(n) for n, name in re.findall(
                r"(\d+) (deductions|leaves|probe assumptions|computed table filters)", result.stdout)}
        except Exception as error:
            record["error"] = str(error)
        return record

    sources = sorted(args.claims.glob("*.minion"))
    if not sources:
        raise SystemExit("no matching Minion inputs")
    records = []
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        for record in pool.map(check, sources):
            records.append(record)
            if len(records) % 100 == 0:
                print(f"Checked {len(records)}/{len(sources)}", flush=True)
    totals = Counter()
    for record in records:
        totals.update(record.get("counts", {}))
    failures = [r for r in records if not r["ok"]]
    report = {"inputs": len(records), "accepted": len(records) - len(failures),
              "no_matching_discovery": args.no_matching_discovery,
              "preprocess": args.preprocess, "prop_node": args.prop_node,
              "timing": "native full-process wall time, concurrent first-pass screening; includes startup and trace IO",
              "jobs": args.jobs,
              "totals": dict(totals), "records": records,
              "producer_sha256": hashlib.sha256(minion_snapshot.read_bytes()).hexdigest(),
              "checker_sha256": hashlib.sha256(certify_snapshot.read_bytes()).hexdigest()}
    report_path = args.traces / "report.json"
    report_path.write_text(json.dumps(report, indent=2) + "\n")
    print(f"Accepted {report['accepted']}/{len(records)} traces; {dict(totals)}", flush=True)
    print(f"Report: {report_path}")
    for record in failures[:10]:
        print(f"FAIL {record['input']}: {record['error']}")
    return bool(failures)


if __name__ == "__main__":
    raise SystemExit(main())
