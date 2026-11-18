"""Trace/replay integration and deliberate trace corruption.

MINION_ANNOTATIONS_BINARY=/path/to/new/minion python3 lean/tests/test_annotations.py
"""
import copy
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest
import binary_trace

ROOT = Path(__file__).resolve().parents[1]
BINARY = Path(os.environ.get("MINION_ANNOTATIONS_BINARY", ROOT.parent / "bin-annotations/minion"))
CERTIFY = ROOT / ".lake/build/bin/minion-certify"


def model(variables, constraints, tuples=""):
    return f"MINION 3\n**VARIABLES**\n{variables}\n{tuples}\n**CONSTRAINTS**\n{constraints}\n**EOF**\n"


ELEMENT = model("DISCRETE i {0..1}\nDISCRETE r {1..3}\nBOOL b",
                "reify(watchelement([1,3],i,r),b)")
EQUALITY = model("DISCRETE x {0..2}\nDISCRETE y {1..3}", "eq(x,y)")


class AnnotationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not BINARY.is_file() or not CERTIFY.is_file():
            raise RuntimeError("Build Minion and minion-certify; set MINION_ANNOTATIONS_BINARY")

    def trace(self, source, flags=(), all_solutions=True):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "model.minion"
            trace = Path(directory) / "trace.bin"
            path.write_text(source)
            args = [str(BINARY), "-noprintsols", *flags]
            if all_solutions:
                args.append("-findallsols")
            result = subprocess.run(args + ["-annotations", str(trace), str(path)],
                                    capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            # Minion writes the compact binary form. Tests inspect and corrupt
            # the equivalent JSON events.
            readable = Path(directory) / "events.jsonl"
            converted = subprocess.run([str(CERTIFY), "--to-json", str(readable), str(trace)],
                                       capture_output=True, text=True)
            self.assertEqual(converted.returncode, 0, converted.stdout + converted.stderr)
            events = [json.loads(line) for line in readable.read_text().splitlines()]
            # Logging must preserve the solver's behaviour.
            baseline = subprocess.run(args + [str(path)], capture_output=True, text=True)
            self.assertEqual(baseline.returncode, 0, baseline.stderr)
            for key in ("Solutions Found", "Total Nodes"):
                self.assertEqual(re.findall(key + r": (\d+)", result.stdout),
                                 re.findall(key + r": (\d+)", baseline.stdout))
            return events

    def certify(self, events, accepted=True, flags=("--audit-solutions", "10000")):
        with tempfile.TemporaryDirectory() as directory:
            # The checker reads the form Minion writes. Events the binary form
            # cannot express are rejected before reaching it.
            path = Path(directory) / "trace.bin"
            try:
                path.write_bytes(binary_trace.encode(events))
            except (ValueError, KeyError, TypeError) as error:
                result = subprocess.CompletedProcess([], 1, "", f"REJECT: not a well-formed trace: {error}")
            else:
                result = subprocess.run([str(CERTIFY), *flags, str(path)], capture_output=True, text=True)
            self.assertEqual(result.returncode == 0, accepted, result.stdout + result.stderr)
            return result

    def test_reified_element_and_empty_explanatory_contexts(self):
        events = self.trace(ELEMENT, ("-annotations-all-contexts",))
        result = self.certify(events)
        self.assertIn("UNCHECKED", result.stdout)
        self.assertEqual(sum(e["event"] == "solution" for e in events), 6)
        self.assertTrue(any(a["event"] == "context_open" and b["event"] == "context_close"
                            and a["id"] == b["id"] for a, b in zip(events, events[1:])))
        self.assertEqual(sum(e["event"] == "model" for e in events), 1)
        self.assertTrue(any(e["event"] == "constraint" and e["source_path"] is None for e in events))

    def test_branch_assumptions_are_not_duplicated(self):
        compact = self.trace(ELEMENT)
        verbose = self.trace(ELEMENT, ("-annotations-all-contexts",))
        for events in (compact, verbose):
            self.certify(events, flags=("--audit-solutions", "10000"))
        self.assertLess(sum(e["event"] == "prune" for e in compact),
                        sum(e["event"] == "prune" for e in verbose))
        self.assertEqual([(e["variable"], e["value"], e["equal"]) for e in compact if e["event"] == "branch"],
                         [(e["variable"], e["value"], e["equal"]) for e in verbose if e["event"] == "branch"])

    def test_unused_contexts_are_omitted_by_default(self):
        compact = self.trace(ELEMENT)
        verbose = self.trace(ELEMENT, ("-annotations-all-contexts",))
        self.certify(compact, flags=("--audit-solutions", "10000"))
        self.certify(verbose, flags=("--audit-solutions", "10000"))
        self.assertLess(sum(e["event"] == "context_open" for e in compact),
                        sum(e["event"] == "context_open" for e in verbose))
        self.assertEqual([e["values"] for e in compact if e["event"] == "solution"],
                         [e["values"] for e in verbose if e["event"] == "solution"])
        active = []
        used = set()
        for event in compact:
            if event["event"] == "context_open":
                active.append(event["id"])
            elif event["event"] in ("prune", "conflict", "table_reduce"):
                used.update(active)
            elif event["event"] == "context_close":
                self.assertEqual(active.pop(), event["id"])
                self.assertIn(event["id"], used)
        self.assertFalse(active)

    def test_pruning_sites_survive_type_erasure(self):
        events = self.trace(EQUALITY)
        self.certify(events)
        sites = [e["rule"] for e in events if e["event"] == "prune" and e["context"]]
        self.assertTrue(sites)
        self.assertTrue(all("/constraints/" in site for site in sites), sites)

    def test_tables_are_declared_once(self):
        source = model("DISCRETE x {0..2}\nDISCRETE y {0..2}", "table([x,y],t)",
                       "**TUPLELIST**\nt 3 2\n0 2\n1 1\n2 0")
        events = self.trace(source)
        self.certify(events)
        self.assertEqual(events[0]["model"]["constraints"][0]["tuples"], [[0, 2], [1, 1], [2, 0]])
        self.assertFalse(any("tuples" in json.dumps(event) for event in events[1:]))

    def test_root_failure_and_zero_variable_solution(self):
        for constraint, count in [("true()", 1), ("false()", 0)]:
            events = self.trace(model("", constraint))
            self.certify(events)
            self.assertEqual(sum(e["event"] == "solution" for e in events), count)

    def test_boolean_views_bound_and_sparse_domains(self):
        for source in [
            model("BOOL x\nBOOL y", "eq(!x,y)"),
            model("BOUND x {-2..2}\nSPARSEBOUND y {-2,0,2}", "minuseq(x,y)"),
            model("DISCRETE x {-3..3}\nDISCRETE y {-2..2}\nDISCRETE z {-3..3}", "div(x,y,z)"),
        ]:
            self.certify(self.trace(source))

    def test_speculative_worlds_under_sac(self):
        source = model("DISCRETE x {0..2}\nDISCRETE y {0..2}\nDISCRETE z {0..2}",
                       "alldiff([x,y,z])\nineq(x,y,-1)")
        for level in ("SAC", "SSAC"):
            self.certify(self.trace(source, ("-preprocess", level)))

    def test_missing_end_and_reordered_events_are_rejected(self):
        events = self.trace(EQUALITY)
        self.certify(events[:-1], False)
        reordered = copy.deepcopy(events)
        reordered[2], reordered[3] = reordered[3], reordered[2]
        self.certify(reordered, False)

    def test_invalid_context_and_constraint_references_are_rejected(self):
        events = self.trace(ELEMENT)
        for kind, field in [("context_close", "id"), ("context_open", "constraint"),
                            ("prune", "context")]:
            corrupted = copy.deepcopy(events)
            next(e for e in corrupted if e["event"] == kind)[field] = 999999
            self.certify(corrupted, False)

    def test_replay_handles_unsorted_initial_domains(self):
        events = self.trace(model("DISCRETE x {0..4}", "watchless(0,x)\nwatchless(x,4)"))
        variable = events[0]["model"]["variables"][0]
        variable.pop("interval", None)
        variable["domain"] = [4, 2, 0, 3, 1]
        self.certify(events, flags=("--audit-solutions", "10000"))
        self.certify(events, flags=("--audit-solutions", "10000"))

    def test_wrong_branch_complement_is_rejected(self):
        events = self.trace(ELEMENT)
        next(e for e in events if e["event"] == "branch" and not e["equal"])["value"] += 1
        self.certify(events, False)

    def test_incorrect_domain_update_is_rejected(self):
        events = self.trace(EQUALITY)
        pruning = next(e for e in events if e["event"] == "prune" and e["context"])
        pruning["value"] += 1 if pruning["operation"] == "lower_bound" else -1
        self.certify(events, False)

    def test_prefix_is_not_accepted_as_complete(self):
        events = self.trace(ELEMENT, all_solutions=False)
        self.assertFalse(events[-1]["complete"])
        result = self.certify(events, flags=())
        self.assertIn("search up to the solution CERTIFIED", result.stdout)
        self.assertNotIn("complete search tree CERTIFIED", result.stdout)
        # The same events cannot be passed off as an exhausted search.
        events[-1]["complete"] = True
        self.certify(events, False, flags=())


if __name__ == "__main__":
    unittest.main()
