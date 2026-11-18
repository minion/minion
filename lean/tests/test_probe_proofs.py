"""Speculative assumptions, galloping bounds and nested failed-probe certificates."""
import copy
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
import test_annotations as trace_tests


TRIANGLE = ("BOOL x\nBOOL y\nBOOL z", "diseq(x,y)\ndiseq(y,z)\ndiseq(z,x)")
RANGE = ("DISCRETE v {0..20}\nBOOL a\nBOOL b",
         "reify(w-inrange(v,[0,10]),b)\nreifyimply(w-literal(a,0),b)\nreifyimply(w-literal(a,1),b)")
NESTED = ("BOOL x\nBOOL a\nBOOL b",
          "watched-or({eq(x,1),eq(a,b)})\nwatched-or({eq(x,1),diseq(a,b)})")


class ProbeProofTests(unittest.TestCase):
    trace = trace_tests.AnnotationTests.trace
    certify = trace_tests.AnnotationTests.certify

    @classmethod
    def setUpClass(cls):
        trace_tests.AnnotationTests.setUpClass()

    def checked(self, case, level="SAC", node=False, tuples=""):
        flags = ("-prop-node" if node else "-preprocess", level)
        events = self.trace(trace_tests.model(*case, tuples), flags)
        result = self.certify(events, flags=("--audit-solutions", "10000"))
        self.assertIn("probe assumptions", result.stdout)
        return events

    @staticmethod
    def refutations(events):
        contexts = {e["id"]: e for e in events
                    if e["event"] == "context_open" and e["rule"] == "probe.refutation"}
        probes = {e["id"]: e for e in events if e["event"] == "probe"}
        return [(e, probes[contexts[e["context"]]["info"]["probe"]]) for e in events
                if e["event"] == "prune" and e["context"] in contexts]

    def test_all_four_probe_modes_and_search_nodes(self):
        for level in ("SAC", "SSAC", "SACBounds", "SSACBounds"):
            for node in (False, True):
                with self.subTest(level=level, node=node):
                    events = self.checked(TRIANGLE, level, node)
                    self.assertTrue(self.refutations(events))
                    self.assertFalse(any(e["event"] == "solution" for e in events))

    def test_galloping_interval_refutations(self):
        for level in ("SAC", "SSAC", "SACBounds", "SSACBounds"):
            with self.subTest(level=level):
                events = self.checked(RANGE, level)
                self.assertTrue(any(p["upper"] > p["lower"] for _, p in self.refutations(events)))
                self.assertEqual(sum(e["event"] == "solution" for e in events), 20)

    def test_interior_value_removal(self):
        tuples = "**TUPLELIST**\nt 5 2\n0 0\n0 1\n1 0\n2 0\n2 1\nu 5 2\n0 0\n0 1\n1 1\n2 0\n2 1"
        events = self.checked(("DISCRETE v {0..2}\nBOOL a", "table([v,a],t)\ntable([v,a],u)"), tuples=tuples)
        self.assertTrue(any(e["operation"] == "remove" and e["value"] == 1
                            for e, _ in self.refutations(events)))

    def test_nested_refutation_and_limited_modes(self):
        for level in ("SSAC", "SSACBounds", "SSAC_limit", "SSACBounds_limit"):
            with self.subTest(level=level):
                events = self.checked(NESTED, level)
                depth = max_depth = 0
                for e in events:
                    if e["event"] == "push":
                        depth += 1
                    elif e["event"] == "pop":
                        depth -= 1
                    elif e["event"] == "probe":
                        max_depth = max(max_depth, depth)
                self.assertGreaterEqual(max_depth, 2)
                if "limit" not in level:
                    self.assertTrue(self.refutations(events))

    def test_fabricated_probe_evidence_is_rejected(self):
        events = self.checked(RANGE)
        for alteration in ("reference", "result", "range", "pruning"):
            with self.subTest(alteration=alteration):
                corrupted = copy.deepcopy(events)
                if alteration == "reference":
                    next(e for e in corrupted if e["event"] == "context_open" and
                         e["rule"] == "probe.refutation")["info"]["probe"] = 999999
                elif alteration == "result":
                    next(e for e in corrupted if e["event"] == "probe_result" and e["failed"])["failed"] = False
                elif alteration == "range":
                    next(e for e in corrupted if e["event"] == "probe")["lower"] = 99
                else:
                    pruning, _ = self.refutations(corrupted)[-1]
                    pruning["value"] = 12
                self.certify(corrupted, False, flags=())

    def test_nested_and_range_certificates_export_to_kernel_proofs(self):
        for case, level in ((TRIANGLE, "SAC"), (NESTED, "SSAC"), (RANGE, "SACBounds")):
            with self.subTest(level=level, case=case[1]), tempfile.TemporaryDirectory() as directory:
                events = self.checked(case, level)
                trace = Path(directory) / "trace.jsonl"
                proof = Path(directory) / "ProbeCertificate.lean"
                trace.write_bytes(trace_tests.binary_trace.encode(events))
                exported = subprocess.run([str(trace_tests.CERTIFY), "--export-lean", str(proof), str(trace)],
                                          capture_output=True, text=True)
                self.assertEqual(exported.returncode, 0, exported.stdout + exported.stderr)
                self.assertIn("Program.refute", proof.read_text())
                checked = subprocess.run(["lake", "env", "lean", str(proof)], cwd=trace_tests.ROOT,
                                         capture_output=True, text=True)
                self.assertEqual(checked.returncode, 0, checked.stdout + checked.stderr)
                self.assertNotIn("sorryAx", checked.stdout)


if __name__ == "__main__":
    unittest.main()
