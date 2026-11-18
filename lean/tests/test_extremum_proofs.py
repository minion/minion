"""Min/max feasibility, shared negated-view normalization and false polarity."""
import copy
import unittest
import test_annotations as trace_tests


class ExtremumProofTests(unittest.TestCase):
    trace = trace_tests.AnnotationTests.trace
    certify = trace_tests.AnnotationTests.certify

    @classmethod
    def setUpClass(cls):
        trace_tests.AnnotationTests.setUpClass()

    def checked(self, variables, constraints):
        events = self.trace(trace_tests.model(variables, constraints))
        self.certify(events, flags=("--audit-solutions", "10000"))
        return events

    def test_both_extrema_and_domain_kinds(self):
        for name in ("min", "max"):
            for kind in ("DISCRETE", "BOUND"):
                self.checked(f"{kind} x {{-2..2}}\n{kind} y {{1..3}}\n{kind} r {{-3..4}}", f"{name}([x,y],r)")
            self.checked("BOOL b\nDISCRETE r {-1..2}", f"{name}([b,!b],r)")

    def test_aliases_holes_and_constants(self):
        for name in ("min", "max"):
            self.checked("DISCRETE x {-2..2}\nDISCRETE r {-3..3}", f"{name}([x,x,r],r)")
            self.checked("DISCRETE x {-2..2}\nDISCRETE r {-3..3}", f"{name}([x,1],x)")
            self.checked("DISCRETE x {-2..2}\nDISCRETE r {-3..3}",
                         f"w-notliteral(x,0)\nw-notliteral(r,1)\n{name}([x,1],r)")
            self.checked("", f"{name}([-2,1],1)")

    def test_empty_and_singleton_arrays(self):
        for name in ("min", "max"):
            self.checked("DISCRETE r {-1..1}", f"{name}([],r)")
            self.checked("DISCRETE x {-1..2}\nDISCRETE r {-2..3}", f"{name}([x],r)")

    def test_reification_and_implication(self):
        variables = "DISCRETE x {-1..2}\nDISCRETE y {0..2}\nDISCRETE r {-2..3}\nBOOL b"
        for name in ("min", "max"):
            for wrapper in ("reify", "reifyimply", "reifyimply-quick"):
                for value in (None, 0, 1):
                    with self.subTest(name=name, wrapper=wrapper, guard=value):
                        c = f"{wrapper}({name}([x,y],r),b)"
                        if value is not None:
                            c += f"\nw-literal(b,{value})"
                        self.checked(variables, c)

    def test_invalid_result_bound_rejected(self):
        events = self.checked("DISCRETE x {0..3}\nDISCRETE r {-1..4}", "min([x,2],r)")
        reasons = {e["id"]: e["rule"] for e in events if e["event"] == "context_open"}
        altered = copy.deepcopy(events)
        pruning = next(e for e in altered if e["event"] == "prune" and e["variable"] == 1 and
                       reasons.get(e["context"]) == "extremum.support")
        self.assertIn(pruning["operation"], ("lower_bound", "upper_bound"))
        pruning["value"] += 1 if pruning["operation"] == "lower_bound" else -1
        result = self.certify(altered, False, flags=())
        self.assertIn("not justified", result.stderr)


if __name__ == "__main__":
    unittest.main()
