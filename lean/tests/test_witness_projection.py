"""Supplied cuts and branch-local STR2+ tables, including hostile evidence."""
import copy
import itertools
import json
from pathlib import Path
import subprocess
import tempfile
import unittest

import test_annotations as trace_tests


HALL = trace_tests.model("DISCRETE x {0..1}\nDISCRETE y {0..1}\nDISCRETE z {0..3}",
                         "gacalldiff([x,y,z])")
UPPER = trace_tests.model("DISCRETE x[3] {0..1}\nDISCRETE y {0..3}",
                          "gcc([x[0],x[1],x[2],y],[0,1],[1,1])")
LOWER = trace_tests.model("DISCRETE x[2] {0..1}\nDISCRETE y[2] {2..3}",
                          "gcc([x[0],x[1],y[0],y[1]],[0,1,2],[2,1,1])")
PROBE = trace_tests.model("DISCRETE v {0..2}\nBOOL a",
                          "str2plus([v,a],t)\nstr2plus([v,a],u)",
                          "**TUPLELIST**\nt 5 2\n0 0\n0 1\n1 0\n2 0\n2 1\n"
                          "u 5 2\n0 0\n0 1\n1 1\n2 0\n2 1")


def table_source(constraints="str2plus([x,y,z],t)"):
    tuples = "**TUPLELIST**\nt 8 3\n" + "\n".join(
        " ".join(map(str, r)) for r in itertools.product((0, 1), repeat=3))
    return trace_tests.model("BOOL x\nBOOL y\nBOOL z", constraints, tuples)


class WitnessProjectionTests(unittest.TestCase):
    trace = trace_tests.AnnotationTests.trace
    certify = trace_tests.AnnotationTests.certify

    @classmethod
    def setUpClass(cls):
        trace_tests.AnnotationTests.setUpClass()

    def checked(self, source, solver_flags=(), strict=False):
        events = self.trace(source, solver_flags)
        flags = ("--audit-solutions", "10000")
        if strict:
            flags += ("--no-matching-discovery",)
        self.certify(events, flags=flags)
        return events

    def test_hall_and_gcc_without_matching_discovery(self):
        for source, kind in ((HALL, "hall"), (UPPER, "gcc_upper"), (LOWER, "gcc_lower")):
            with self.subTest(kind=kind):
                events = self.checked(source, strict=True)
                declarations = {e["id"] for e in events if e["event"] == "witness"}
                uses = [e["witness"] for e in events if "witness" in e]
                self.assertTrue(declarations)
                self.assertEqual(declarations, set(uses))
                self.assertTrue(any(e.get("kind") == kind for e in events))
                if kind == "hall":
                    self.assertGreater(len(uses), len(declarations))

    def test_unused_hall_candidate_is_not_emitted(self):
        events = self.checked(trace_tests.model("DISCRETE x[3] {0..3}", "gacalldiff(x)"))
        # Root propagation has nothing to delete. Branches later need reasons.
        root = events[:next(i for i, e in enumerate(events) if e["event"] == "push")]
        self.assertFalse(any(e["event"] == "witness" for e in root))

    def test_missing_or_corrupted_supplied_hall_evidence(self):
        events = self.checked(HALL, strict=True)
        missing = copy.deepcopy(events)
        for e in missing:
            e.pop("witness", None)
        self.certify(missing, flags=())
        result = self.certify(missing, False, flags=("--no-matching-discovery",))
        self.assertIn("supplied witness required", result.stderr)
        for positions in ([], [99], [0, 0]):
            with self.subTest(positions=positions):
                altered = copy.deepcopy(events)
                next(e for e in altered if e["event"] == "witness")["positions"] = positions
                self.certify(altered, False, flags=())
        altered = copy.deepcopy(events)
        next(e for e in altered if "witness" in e)["witness"] = 999999
        self.certify(altered, False, flags=())

    def test_simple_alldiff_singleton_witness(self):
        events = self.checked(trace_tests.model("DISCRETE x[3] {0..2}", "alldiff(x)"), strict=True)
        self.assertTrue(any(e["event"] == "witness" for e in events))

    def test_computed_table_filter_with_original_row_ids_and_wrappers(self):
        tuples = "**TUPLELIST**\nt 512 9\n" + "\n".join(
            " ".join(map(str, row)) for row in itertools.product((0, 1), repeat=9))
        terms = "[x,x,x,x,x,x,x,y,y]"
        for relation, count in ((f"str2plus({terms},t)", 8),
                                (f"table({terms},t)", 8),
                                (f"reify(table({terms},t),b)", 4),
                                (f"reify(negativetable({terms},t),b)", 4),
                                (f"reifyimply(table({terms},t),b)", 8)):
            with self.subTest(relation=relation):
                source = trace_tests.model("BOOL x\nBOOL y\nBOOL b", relation, tuples)
                events = self.checked(source)
                result = self.certify(events, flags=())
                self.assertRegex(result.stdout, r"[1-9]\d* computed table filters CHECKED")
                self.assertEqual(sum(e["event"] == "solution" for e in events), count)
                if relation.startswith("reify("):
                    with tempfile.TemporaryDirectory() as directory:
                        trace = Path(directory) / "trace.jsonl"
                        proof = Path(directory) / "Filtered.lean"
                        trace.write_bytes(trace_tests.binary_trace.encode(events))
                        exported = subprocess.run([str(trace_tests.CERTIFY), "--export-lean", str(proof), str(trace)],
                                                  capture_output=True, text=True, timeout=60)
                        self.assertEqual(exported.returncode, 0, exported.stderr)
                        self.assertIn("Step.filterTables", proof.read_text())
                        checked = subprocess.run(["lake", "env", "lean", str(proof)], cwd=trace_tests.ROOT,
                                                 capture_output=True, text=True, timeout=60)
                        self.assertEqual(checked.returncode, 0, checked.stdout + checked.stderr)
                        self.assertNotIn("sorryAx", checked.stdout)

    def test_two_computed_table_roots_preserve_both_polarities(self):
        tuples = "**TUPLELIST**\nt 512 9\n" + "\n".join(
            " ".join(map(str, row)) for row in itertools.product((0, 1), repeat=9))
        terms = "[x,x,x,x,x,x,x,y,y]"
        source = trace_tests.model("BOOL x\nBOOL y\nBOOL b\nBOOL c",
            f"reify(table({terms},t),b)\nreify(negativetable({terms},t),c)", tuples)
        events = self.checked(source)
        result = self.certify(events, flags=())
        self.assertRegex(result.stdout, r"[1-9]\d* computed table filters CHECKED")
        self.assertEqual(sum(e["event"] == "solution" for e in events), 4)

    def test_initial_alias_filter_and_solver_row_ids_inside_failed_probes(self):
        sections = []
        for name, pairs in (("t", [(0,0), (0,1), (1,0), (2,0), (2,1)]),
                            ("u", [(0,0), (0,1), (1,1), (2,0), (2,1)])):
            rows = [(v,a,z,w) for v,a in pairs for z in range(16) for w in range(16)]
            sections.append(f"{name} {len(rows)} 4\n" + "\n".join(" ".join(map(str,r)) for r in rows))
        source = trace_tests.model("DISCRETE v {0..2}\nBOOL a\nDISCRETE z {0..15}",
            "str2plus([v,a,z,z],t)\nstr2plus([v,a,z,z],u)",
            "**TUPLELIST**\n" + "\n".join(sections))
        events = self.checked(source, ("-preprocess", "SAC"))
        self.assertTrue(any(e["event"] == "probe_result" and e["failed"] for e in events))
        self.assertEqual(sum(e["event"] == "solution" for e in events), 64)
        result = self.certify(events, flags=())
        self.assertRegex(result.stdout, r"[1-9]\d* computed table filters CHECKED")

    def test_supplied_witness_and_table_exports_check_in_kernel(self):
        for source, flags, expected in ((HALL, (), "Step.explainedPrune"),
                                         (UPPER, (), "Program.explainedConflict"),
                                         (LOWER, (), "Program.explainedConflict"),
                                         (table_source(), (), "Program.solution"),
                                         (PROBE, ("-preprocess", "SAC"), "Program.refute")):
            with self.subTest(expected=expected), tempfile.TemporaryDirectory() as directory:
                events = self.checked(source, flags)
                trace = Path(directory) / "trace.jsonl"
                proof = Path(directory) / "Certificate.lean"
                trace.write_bytes(trace_tests.binary_trace.encode(events))
                exported = subprocess.run([str(trace_tests.CERTIFY), "--export-lean", str(proof), str(trace)],
                                          capture_output=True, text=True, timeout=60)
                self.assertEqual(exported.returncode, 0, exported.stdout + exported.stderr)
                self.assertIn(expected, proof.read_text())
                checked = subprocess.run(["lake", "env", "lean", str(proof)], cwd=trace_tests.ROOT,
                                         capture_output=True, text=True, timeout=60)
                self.assertEqual(checked.returncode, 0, checked.stdout + checked.stderr)
                self.assertNotIn("sorryAx", checked.stdout)
                self.assertNotIn("Lean.ofReduceBool", checked.stdout)
                self.assertNotIn("native_decide", proof.read_text())


if __name__ == "__main__":
    unittest.main()
