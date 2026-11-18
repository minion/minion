"""Hall certificates for GAC all-different and simple assigned-value propagation."""
import copy
import unittest
import test_annotations as trace_tests


class AllDiffProofTests(unittest.TestCase):
    trace = trace_tests.AnnotationTests.trace
    certify = trace_tests.AnnotationTests.certify

    @classmethod
    def setUpClass(cls):
        trace_tests.AnnotationTests.setUpClass()

    def checked(self, variables, constraints):
        events = self.trace(trace_tests.model(variables, constraints))
        self.certify(events, flags=("--audit-solutions", "10000"))
        return events

    def test_both_implementations_empty_and_constants(self):
        for name in ("alldiff", "gacalldiff"):
            self.checked("DISCRETE x {0..2}\nDISCRETE y {0..2}\nDISCRETE z {0..2}", f"{name}([x,y,z])")
            self.checked("DISCRETE x {0..1}\nDISCRETE y {0..1}\nDISCRETE z {0..1}", f"{name}([x,y,z])")
            self.checked("", f"{name}([])")
            self.checked("", f"{name}([1,1])")
        self.checked("BOUND x {0..2}\nBOUND y {0..2}", "alldiff([x,y])")

    def test_hall_pruning_without_assigned_variables(self):
        events = self.checked("DISCRETE x {0..1}\nDISCRETE y {0..1}\nDISCRETE z {0..2}", "gacalldiff([x,y,z])")
        reasons = {e["id"]: e["rule"] for e in events if e["event"] == "context_open"}
        self.assertTrue(any(e["event"] == "prune" and e["variable"] == 2 and
                            reasons.get(e["context"]) == "alldiff.hall" for e in events))
        self.checked("DISCRETE x {0..2}\nDISCRETE y {0..2}\nDISCRETE z {0..3}\nDISCRETE w {1..3}",
                     "w-notliteral(x,1)\nw-notliteral(y,1)\nw-notliteral(w,2)\ngacalldiff([x,y,z,w])")

    def test_aliases_and_boolean_views(self):
        for name in ("alldiff", "gacalldiff"):
            for array in ("[x,x]", "[b,!b]", "[x,b,!b]", "[b,!b,b]"):
                with self.subTest(name=name, array=array):
                    self.checked("DISCRETE x {0..2}\nBOOL b", f"{name}({array})")

    def test_both_reified_polarities_and_implication(self):
        variables = "DISCRETE x {0..2}\nDISCRETE y {0..2}\nDISCRETE z {0..2}\nBOOL b"
        for name in ("alldiff", "gacalldiff"):
            for wrapper in ("reify", "reifyimply", "reifyimply-quick"):
                for value in (None, 0, 1):
                    with self.subTest(name=name, wrapper=wrapper, guard=value):
                        c = f"{wrapper}({name}([x,y,z]),b)"
                        if value is not None:
                            c += f"\nw-literal(b,{value})"
                        self.checked(variables, c)

    def test_invalid_hall_deletion_rejected(self):
        events = self.checked("DISCRETE x {0..1}\nDISCRETE y {0..1}\nDISCRETE z {0..2}", "gacalldiff([x,y,z])")
        reasons = {e["id"]: e["rule"] for e in events if e["event"] == "context_open"}
        altered = copy.deepcopy(events)
        pruning = next(e for e in altered if e["event"] == "prune" and e["variable"] == 2 and
                       reasons.get(e["context"]) == "alldiff.hall")
        pruning["operation"] = "remove"
        pruning["value"] = 2
        result = self.certify(altered, False, flags=())
        self.assertIn("not justified", result.stderr)


if __name__ == "__main__":
    unittest.main()
