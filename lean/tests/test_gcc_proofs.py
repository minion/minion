"""GCC count bounds, joint capacity cuts, and logical activation."""
import copy
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
import test_annotations as trace_tests


class GccProofTests(unittest.TestCase):
    trace = trace_tests.AnnotationTests.trace
    certify = trace_tests.AnnotationTests.certify

    @classmethod
    def setUpClass(cls):
        trace_tests.AnnotationTests.setUpClass()

    def checked(self, variables, constraints):
        events = self.trace(trace_tests.model(variables, constraints))
        self.certify(events, flags=("--audit-solutions", "10000"))
        return events

    def test_both_implementations_and_unrestricted_values(self):
        for name in ("gcc", "gccweak"):
            self.checked("DISCRETE x {-1..2}\nDISCRETE y {0..3}\nDISCRETE n {0..2}", f"{name}([x,y],[0],[n])")
            self.checked("DISCRETE n {-1..2}", f"{name}([],[0],[n])")

    def test_joint_capacity_bounds(self):
        for name in ("gcc", "gccweak"):
            with self.subTest(name=name):
                self.checked("DISCRETE x {0..1}\nDISCRETE y {0..1}\nDISCRETE z {0..2}\nDISCRETE n {0..3}",
                             f"{name}([x,y,z],[0,1,2],[1,1,n])")
                self.checked("DISCRETE x {0..2}\nDISCRETE y {0..2}\nDISCRETE z {1..2}\nDISCRETE n {0..3}",
                             f"{name}([x,y,z],[0,1,2],[n,1,1])")
                self.checked("DISCRETE x {0..1}\nDISCRETE y {0..1}\nDISCRETE z {0..3}",
                             f"{name}([x,y,z],[0,1,2,3],[1,1,1,0])")

    def test_aliases_and_boolean_views(self):
        for name in ("gcc", "gccweak"):
            self.checked("DISCRETE x {0..2}\nDISCRETE n {0..3}", f"{name}([x,x],[0,1],[n,n])")
            self.checked("BOOL b\nDISCRETE n {0..3}", f"{name}([b,!b],[0,1],[n,n])")

    def test_both_polarities_and_implication(self):
        variables = "DISCRETE x {0..2}\nDISCRETE y {0..2}\nDISCRETE n {0..2}\nBOOL b"
        for name in ("gcc", "gccweak"):
            for wrapper in ("reify", "reifyimply", "reifyimply-quick"):
                for value in (None, 0, 1):
                    with self.subTest(name=name, wrapper=wrapper, guard=value):
                        c = f"{wrapper}({name}([x,y],[0,1],[n,1]),b)"
                        if value is not None:
                            c += f"\nw-literal(b,{value})"
                        self.checked(variables, c)

    def test_invalid_capacity_bound_rejected(self):
        events = self.checked("DISCRETE x {0..1}\nDISCRETE y {0..1}\nDISCRETE z {0..2}\nDISCRETE n {0..3}",
                              "gcc([x,y,z],[0,1,2],[1,1,n])")
        altered = copy.deepcopy(events)
        pruning = next(e for e in altered if e["event"] == "prune" and e["variable"] == 3)
        pruning["operation"] = "lower_bound"
        pruning["value"] = 2
        result = self.certify(altered, False, flags=())
        self.assertIn("not justified", result.stderr)

    def test_large_untracked_domains_use_capacity_cuts(self):
        for variables, constraint in (
            ("DISCRETE x[2] {0..1}\nDISCRETE y[2] {2..50000}",
             "gcc([x[0],x[1],y[0],y[1]],[0,1,2],[2,1,1])"),
            ("DISCRETE x[3] {0..1}\nDISCRETE y {2..50000}",
             "gcc([x[0],x[1],x[2],y],[0,1,2],[1,1,0])"),
        ):
            events = self.trace(trace_tests.model(variables, constraint))
            self.assertFalse(any(e["event"] == "solution" for e in events))
            # Exhaustive assignment auditing would defeat this scalability test.
            with tempfile.TemporaryDirectory() as directory:
                path = Path(directory) / "trace.jsonl"
                path.write_bytes(trace_tests.binary_trace.encode(events))
                result = subprocess.run([str(trace_tests.CERTIFY), str(path)],
                                        capture_output=True, text=True, timeout=20)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn("complete search tree CERTIFIED", result.stdout)


if __name__ == "__main__":
    unittest.main()
