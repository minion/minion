"""Weighted sums share the sum proof through composable scaling views."""
import copy
import unittest
import test_annotations as trace_tests


class WeightedProofTests(unittest.TestCase):
    trace = trace_tests.AnnotationTests.trace
    certify = trace_tests.AnnotationTests.certify

    @classmethod
    def setUpClass(cls):
        trace_tests.AnnotationTests.setUpClass()

    def checked(self, variables, constraints):
        events = self.trace(trace_tests.model(variables, constraints))
        self.certify(events, flags=("--audit-solutions", "10000"))
        return events

    def test_mixed_coefficients_and_result_bounds(self):
        variables = "DISCRETE x {-3..3}\nDISCRETE y {-2..2}\nDISCRETE z {0..2}\nDISCRETE r {-2..3}"
        for name in ("weightedsumleq", "weightedsumgeq"):
            self.checked(variables, f"{name}([2,-3,0],[x,y,z],r)")

    def test_rounding_negative_domains_and_coefficients(self):
        for name in ("weightedsumleq", "weightedsumgeq"):
            for weight in (-3, -1, 1, 2, 3):
                for rhs in (-2, 2):
                    self.checked("DISCRETE x {-3..3}", f"{name}([{weight}],[x],{rhs})")

    def test_zero_weights_empty_sums_and_constants(self):
        for name in ("weightedsumleq", "weightedsumgeq"):
            self.checked("DISCRETE x {-2..2}\nDISCRETE r {-1..1}", f"{name}([0],[x],r)")
            for rhs in (-1, 0, 1):
                self.checked("", f"{name}([],[],{rhs})")
            self.checked("DISCRETE r {-2..3}", f"{name}([2,-1],[1,2],r)")

    def test_aliases_and_boolean_complements(self):
        for name in ("weightedsumleq", "weightedsumgeq"):
            self.checked("DISCRETE x {-2..2}\nDISCRETE y {0..2}", f"{name}([2,-1,3],[x,x,y],x)")
            self.checked("BOOL b\nDISCRETE r {-2..3}", f"{name}([2,-3],[b,!b],r)")

    def test_reification_and_implication(self):
        variables = "DISCRETE x {-2..2}\nDISCRETE y {0..2}\nBOOL b"
        for wrapper in ("reify", "reifyimply", "reifyimply-quick"):
            for name in ("weightedsumleq", "weightedsumgeq"):
                for value in (None, 0, 1):
                    constraint = f"{wrapper}({name}([2,-3],[x,y],1),b)"
                    if value is not None:
                        constraint += f"\nw-literal(b,{value})"
                    self.checked(variables, constraint)

    def test_unsound_rounding_is_rejected(self):
        events = self.checked("DISCRETE x {0..3}", "weightedsumleq([2],[x],3)")
        altered = copy.deepcopy(events)
        reasons = {e["id"]: e["rule"] for e in events if e["event"] == "context_open"}
        pruning = next(e for e in altered if e["event"] == "prune" and
                       reasons.get(e["context"], "").startswith("sum.bounds."))
        self.assertEqual(pruning["operation"], "upper_bound")
        pruning["value"] -= 1
        result = self.certify(altered, False, flags=())
        self.assertIn("not justified", result.stderr)


if __name__ == "__main__":
    unittest.main()
