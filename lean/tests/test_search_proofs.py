"""Certified complete search trees, independently of structural replay."""
import copy
from pathlib import Path
import re
import subprocess
import tempfile
import unittest
import test_annotations as trace_tests


class SearchProofTests(unittest.TestCase):
    trace = trace_tests.AnnotationTests.trace
    certify = trace_tests.AnnotationTests.certify

    @classmethod
    def setUpClass(cls):
        trace_tests.AnnotationTests.setUpClass()

    def certified(self, variables, constraints, tuples=""):
        events = self.trace(trace_tests.model(variables, constraints, tuples))
        result = self.certify(events, flags=("--audit-solutions", "10000"))
        self.assertIn("complete search tree CERTIFIED", result.stdout)
        return events

    def test_empty_model_solution_and_root_unsat(self):
        self.certified("", "true()")
        self.certified("", "false()")
        self.certified("BOOL x", "false()")

    def test_unconstrained_and_equality_search(self):
        self.certified("DISCRETE x {0..3}\nBOOL y", "true()")
        self.certified("DISCRETE x {0..3}\nDISCRETE y {0..3}", "eq(x,y)")
        self.certified("DISCRETE x {0..3}\nDISCRETE y {0..3}", "eq(x,y)\ndiseq(x,y)")

    def test_nested_reification_and_generated_negative_objects(self):
        self.certified("BOOL b\nBOOL c\nDISCRETE x {0..2}\nDISCRETE y {0..2}",
                       "reifyimply(reify(gacalldiff([x,y]),b),!c)")
        self.certified("BOOL b\nDISCRETE x {0..2}\nDISCRETE y {0..2}",
                       "reify(sumleq([x,y],1),b)")

    def test_table_polarities_and_shared_runtime_declarations(self):
        tuples = "**TUPLELIST**\nt 2 2\n0 2\n2 0"
        self.certified("BOOL b\nDISCRETE x {0..2}\nDISCRETE y {0..2}",
                       "reify(table([x,y],t),b)", tuples)
        self.certified("DISCRETE x {0..2}\nDISCRETE y {0..2}", "negativetable([x,y],t)", tuples)

    def test_global_capacity_and_frame_rules(self):
        self.certified("DISCRETE x {0..1}\nDISCRETE y {0..1}\nDISCRETE z {0..2}\nDISCRETE n {0..3}",
                       "gccweak([x,y,z],[0,1,2],[1,1,n])")
        self.certified("BOOL x[4]", "alldiffmatrix(x,1)")
        self.certified("BOOL b\nBOOL x\nBOOL y", "reify(frameupdate([x],[y],[],[],1),b)")

    def test_fabricated_solution_or_missing_subtree_rejected(self):
        events = self.certified("BOOL x\nBOOL y", "eq(x,y)")
        altered = copy.deepcopy(events)
        next(e for e in altered if e["event"] == "solution")["values"] = [0, 1]
        self.certify(altered, False, flags=())
        altered = [e for e in events if e["event"] != "branch_close"]
        self.certify(altered, False, flags=())

    def test_incomplete_search_is_not_certified_as_complete(self):
        events = self.trace(trace_tests.model("BOOL x\nBOOL y", "true()"), all_solutions=False)
        result = self.certify(events, flags=())
        self.assertNotIn("complete search tree CERTIFIED", result.stdout)
        self.assertIn("2 decisions left unexplored", result.stdout)

    def test_duplicate_solution_event_rejected_with_deferred_uniqueness(self):
        events = self.certified("", "true()")
        position = next(i for i, e in enumerate(events) if e["event"] == "solution")
        altered = copy.deepcopy(events)
        altered.insert(position + 1, copy.deepcopy(altered[position]))
        # All context IDs precede the root solution; there are no split/world
        # references after this insertion in a zero-variable enumeration.
        for i, event in enumerate(altered, 1):
            event["seq"] = i
        result = self.certify(altered, False, flags=())
        self.assertIn("solution after a closed subtree", result.stderr)
        result = self.certify(altered, False, flags=())
        self.assertIn("solution after a closed subtree", result.stderr)

    def test_exported_certificates_are_checked_by_the_kernel(self):
        for variables, constraints in (
                ("DISCRETE i {0..1}\nDISCRETE r {1..3}\nBOOL b", "reify(watchelement([1,3],i,r),b)"),
                ("DISCRETE x[3] {0..1}", "gacalldiff(x)")):
            with self.subTest(constraints=constraints), tempfile.TemporaryDirectory() as directory:
                events = self.certified(variables, constraints)
                trace = Path(directory) / "trace.jsonl"
                proof = Path(directory) / "Certificate.lean"
                import json
                trace.write_bytes(trace_tests.binary_trace.encode(events))
                exported = subprocess.run([str(trace_tests.CERTIFY), "--export-lean", str(proof), str(trace)],
                                          capture_output=True, text=True)
                self.assertEqual(exported.returncode, 0, exported.stdout + exported.stderr)
                checked = subprocess.run(["lake", "env", "lean", str(proof)], cwd=trace_tests.ROOT,
                                         capture_output=True, text=True)
                self.assertEqual(checked.returncode, 0, checked.stdout + checked.stderr)
                self.assertNotIn("sorryAx", checked.stdout)
                self.assertNotIn("native_decide", proof.read_text())
                self.assertIn("theorem solutions_nodup", proof.read_text())
                corrupted = re.sub(r"(def reported : List Minion.Assignment :=\n).*?(\n\n(?:set_option|theorem))",
                                   r"\g<1>[[999]]\2", proof.read_text(), flags=re.S)
                self.assertNotEqual(corrupted, proof.read_text())
                proof.write_text(corrupted)
                rejected = subprocess.run(["lake", "env", "lean", str(proof)], cwd=trace_tests.ROOT,
                                          capture_output=True, text=True)
                self.assertNotEqual(rejected.returncode, 0)


if __name__ == "__main__":
    unittest.main()
