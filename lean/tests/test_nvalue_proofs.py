"""Distinct-value bounds, including batch pruning outside a forced value set."""
import copy
import unittest
import test_annotations as trace_tests


class NvalueProofTests(unittest.TestCase):
    trace = trace_tests.AnnotationTests.trace
    certify = trace_tests.AnnotationTests.certify

    @classmethod
    def setUpClass(cls):
        trace_tests.AnnotationTests.setUpClass()

    def checked(self, variables, constraints):
        events = self.trace(trace_tests.model(variables, constraints))
        self.certify(events, flags=("--audit-solutions", "10000"))
        return events

    def test_both_nvalue_directions(self):
        for name in ("nvalueleq", "nvaluegeq"):
            self.checked("DISCRETE x {-1..2}\nDISCRETE y {0..2}\nDISCRETE n {-1..4}", f"{name}([x,y,1],n)")
            self.checked("DISCRETE n {-1..2}", f"{name}([],n)")

    def test_forced_values_batch_bounds_and_holes(self):
        for kind in ("DISCRETE", "BOUND"):
            self.checked(f"{kind} x {{0..20}}", "nvalueleq([10,x],1)")
            self.checked(f"{kind} x {{0..20}}\nDISCRETE n {{0..4}}", "nvaluegeq([1,1,x],n)")
        self.checked("DISCRETE x {0..5}", "nvalueleq([1,4,x],2)")

    def test_aliases_and_boolean_views(self):
        for name in ("nvalueleq", "nvaluegeq"):
            self.checked("DISCRETE x {0..3}\nDISCRETE n {0..3}", f"{name}([x,x],n)")
            self.checked("BOOL b\nDISCRETE n {0..3}", f"{name}([b,!b],n)")
            self.checked("BOOL b", f"{name}([b,!b],b)")

    def test_reification_and_implication(self):
        variables = "DISCRETE x {0..3}\nDISCRETE y {0..2}\nDISCRETE n {0..3}\nBOOL b"
        for name in ("nvalueleq", "nvaluegeq"):
            for wrapper in ("reify", "reifyimply", "reifyimply-quick"):
                for value in (None, 0, 1):
                    with self.subTest(name=name, wrapper=wrapper, guard=value):
                        c = f"{wrapper}({name}([x,y,1],n),b)"
                        if value is not None:
                            c += f"\nw-literal(b,{value})"
                        self.checked(variables, c)

    def test_invalid_bound_is_rejected(self):
        events = self.checked("DISCRETE x {0..5}", "nvalueleq([2,x],1)")
        reasons = {e["id"]: e["rule"] for e in events if e["event"] == "context_open"}
        altered = copy.deepcopy(events)
        pruning = next(e for e in altered if e["event"] == "prune" and
                       reasons.get(e["context"]) == "nvalue.bounds")
        self.assertEqual(pruning["operation"], "lower_bound")
        pruning["value"] += 1
        result = self.certify(altered, False, flags=())
        self.assertIn("not justified", result.stderr)


if __name__ == "__main__":
    unittest.main()
