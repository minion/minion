"""Shared local support-rule integration and hostile trace checks."""
import copy
import unittest
import test_annotations as trace_tests


class LocalProofTests(unittest.TestCase):
    trace = trace_tests.AnnotationTests.trace
    certify = trace_tests.AnnotationTests.certify

    @classmethod
    def setUpClass(cls):
        trace_tests.AnnotationTests.setUpClass()

    def checked(self, variables, constraints):
        events = self.trace(trace_tests.model(variables, constraints))
        result = self.certify(events, flags=("--audit-solutions", "10000"))
        self.assertIn("0 prunings and 0 conflicts UNCHECKED", result.stdout)
        return events

    def test_truth_and_literals(self):
        for constraint in ("true()", "false()"):
            self.checked("", constraint)
        for name in ("w-literal", "w-notliteral"):
            self.checked("DISCRETE x {-2..3}", f"{name}(x,1)")
            self.checked("BOOL x", f"{name}(x,0)")
            self.checked("BOUND x {-2..3}", f"{name}(x,-2)")

    def test_equality_domain_and_bound_variants(self):
        for name in ("eq", "gaceq"):
            self.checked("DISCRETE x {-2..3}\nDISCRETE y {-1..4}",
                         f"w-notliteral(x,-1)\nw-notliteral(x,1)\nw-notliteral(x,2)\n{name}(x,y)")
            self.checked("BOOL x\nBOOL y", f"{name}(x,!y)")
        self.checked("BOUND x {-2..3}\nBOUND y {1..4}", "eq(x,y)")
        self.checked("SPARSEBOUND x {-2,0,3}\nDISCRETE y {-1..4}", "eq(x,y)")

    def test_disequality_variants(self):
        for name in ("diseq", "watchneq"):
            self.checked("DISCRETE x {-2..3}\nDISCRETE y {-1..2}", f"{name}(x,y)")
            self.checked("BOOL x\nBOOL y", f"{name}(x,!y)")
        self.checked("BOUND x {-2..3}\nBOUND y {1..2}", "diseq(x,y)")

    def test_order_and_offset(self):
        variables = "DISCRETE x {-3..3}\nDISCRETE y {-2..2}"
        self.checked(variables, "watchless(x,y)")
        for offset in (-2, 0, 2):
            self.checked(variables, f"ineq(x,y,{offset})")

    def test_membership_sets_and_ranges(self):
        for kind in ("DISCRETE", "BOUND", "SPARSEBOUND"):
            domain = "{-3,0,2,4}" if kind == "SPARSEBOUND" else "{-3..4}"
            for name, args in (("w-inset", "[-2,0,2]"), ("w-notinset", "[-2,0,2]"),
                               ("w-inrange", "[-1,2]"), ("w-notinrange", "[-1,2]"),
                               ("w-inintervalset", "[-2,-1,2,3]")):
                self.checked(f"{kind} x {domain}", f"{name}(x,{args})")
        self.checked("DISCRETE x {-2..3}", "w-inset(x,[])")
        self.checked("DISCRETE x {-2..3}", "w-notinset(x,[])")

    def test_arithmetic_negation_and_absolute(self):
        variables = "DISCRETE x {-3..3}\nDISCRETE y {-2..2}"
        self.checked(variables, "minuseq(x,y)")
        self.checked(variables, "abs(x,y)")
        self.checked("BOUND x {-3..3}\nBOUND y {-2..2}", "abs(x,y)")

    def test_aliases_and_initial_conflicts(self):
        for c in ("eq(x,x)", "diseq(x,x)", "watchneq(x,x)", "watchless(x,x)",
                  "minuseq(x,x)", "abs(x,x)", "ineq(x,x,-1)"):
            self.checked("DISCRETE x {-2..2}", c)
        self.checked("DISCRETE x {0..1}\nDISCRETE y {2..3}", "eq(x,y)")

    def test_unsound_pruning_and_conflicts_rejected_by_rule(self):
        events = self.checked("DISCRETE x {0..3}\nDISCRETE y {1..2}", "eq(x,y)")
        reasons = {e["id"]: e["rule"] for e in events if e["event"] == "context_open"}
        for conflict in (False, True):
            altered = copy.deepcopy(events)
            pruning = next(e for e in altered if e["event"] == "prune" and
                           reasons.get(e["context"]) == "local.support")
            if conflict:
                pruning["event"] = "conflict"
            else:
                self.assertIn(pruning["operation"], ("lower_bound", "upper_bound"))
                pruning["value"] += 1 if pruning["operation"] == "lower_bound" else -1
            result = self.certify(altered, False, flags=())
            self.assertIn("not justified", result.stderr)


if __name__ == "__main__":
    unittest.main()
