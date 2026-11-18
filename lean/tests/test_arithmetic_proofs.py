"""Local support rules for nonlinear arithmetic and their logical wrappers."""
import copy
import unittest
import test_annotations as trace_tests


class ArithmeticProofTests(unittest.TestCase):
    trace = trace_tests.AnnotationTests.trace
    certify = trace_tests.AnnotationTests.certify

    @classmethod
    def setUpClass(cls):
        trace_tests.AnnotationTests.setUpClass()

    def checked(self, variables, constraints):
        events = self.trace(trace_tests.model(variables, constraints))
        self.certify(events, flags=("--audit-solutions", "10000"))
        return events

    def test_products_and_boolean_specialisation(self):
        self.checked("DISCRETE x {-2..2}\nDISCRETE y {-2..2}\nDISCRETE z {-3..3}", "product(x,y,z)")
        self.checked("BOOL x\nBOOL y\nBOOL z", "product(x,!y,z)")
        self.checked("BOUND x {-2..2}\nBOUND y {-2..2}\nBOUND z {-3..3}", "product(x,y,z)")

    def test_division_modulo_signs_and_zero(self):
        for name in ("div", "div_undefzero", "modulo", "modulo_undefzero"):
            with self.subTest(name=name):
                self.checked("DISCRETE x {-3..3}\nDISCRETE y {-2..2}\nDISCRETE z {-3..3}",
                             f"{name}(x,y,z)")
                self.checked("DISCRETE z {-2..2}", f"{name}(1,0,z)")
                self.checked("DISCRETE z {-3..3}", f"{name}(-3,2,z)")
                self.checked("DISCRETE z {-3..3}", f"{name}(3,-2,z)")

    def test_powers_and_difference(self):
        self.checked("DISCRETE x {-2..2}\nDISCRETE y {-2..3}\nDISCRETE z {-3..4}", "pow(x,y,z)")
        self.checked("DISCRETE x {-2..2}\nDISCRETE y {-2..2}\nDISCRETE z {0..3}", "difference(x,y,z)")

    def test_aliases(self):
        for name in ("product", "div", "div_undefzero", "modulo", "modulo_undefzero", "difference", "pow"):
            self.checked("DISCRETE x {-2..2}\nDISCRETE z {-2..3}", f"{name}(x,x,z)")

    def test_reification_both_polarities(self):
        variables = "DISCRETE x {-2..2}\nDISCRETE y {-1..2}\nDISCRETE z {-2..3}\nBOOL b"
        for name in ("product", "div", "div_undefzero", "modulo", "modulo_undefzero", "difference", "pow"):
            for wrapper in ("reify", "reifyimply"):
                for value in (None, 0, 1):
                    with self.subTest(name=name, wrapper=wrapper, guard=value):
                        c = f"{wrapper}({name}(x,y,z),b)"
                        if value is not None:
                            c += f"\nw-literal(b,{value})"
                        self.checked(variables, c)

    def test_unsound_integer_rounding_is_rejected(self):
        events = self.checked("DISCRETE x {0..5}", "div(x,2,1)")
        altered = copy.deepcopy(events)
        reasons = {e["id"]: e["rule"] for e in events if e["event"] == "context_open"}
        pruning = next(e for e in altered if e["event"] == "prune" and
                       reasons.get(e["context"]) == "local.support")
        self.assertIn(pruning["operation"], ("lower_bound", "upper_bound"))
        pruning["value"] += 1 if pruning["operation"] == "lower_bound" else -1
        result = self.certify(altered, False, flags=())
        self.assertIn("not justified", result.stderr)


if __name__ == "__main__":
    unittest.main()
