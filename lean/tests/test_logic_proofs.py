"""Compositional reification and Boolean activation, including generated children."""
import copy
import unittest
import test_annotations as trace_tests


class LogicProofTests(unittest.TestCase):
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

    def test_reification_both_polarities_and_free_guards(self):
        variables = "DISCRETE x {-2..3}\nDISCRETE y {-1..2}\nBOOL b"
        for child in ("eq(x,y)", "diseq(x,y)", "watchless(x,y)", "ineq(x,y,-1)",
                      "minuseq(x,y)", "abs(x,y)", "sumleq([x,y],1)", "sumgeq([x,y],2)",
                      "w-inset(x,[-1,2])", "w-notinrange(x,[-1,1])"):
            for value in (None, 0, 1):
                constraint = f"reify({child},b)"
                if value is not None:
                    constraint += f"\nw-literal(b,{value})"
                self.checked(variables, constraint)

    def test_implication_variants(self):
        for wrapper in ("reifyimply", "reifyimply-quick"):
            for child in ("sumleq([x,y],1)", "eq(x,y)", "watchless(x,y)"):
                self.checked("DISCRETE x {0..3}\nDISCRETE y {1..2}\nBOOL b", f"{wrapper}({child},b)")

    def test_guard_inference_and_boolean_views(self):
        for child in ("eq(x,9)", "sumleq([x,x],-1)", "sumgeq([x,x],9)"):
            self.checked("DISCRETE x {0..3}\nBOOL b", f"reify({child},!b)")
        self.checked("BOOL b", "reify(true(),b)")
        self.checked("BOOL b", "reify(false(),b)")

    def test_guard_aliases_child_variables(self):
        for c in ("reify(eq(b,0),b)", "reify(diseq(b,0),b)",
                  "reify(watchsumgeq([b],1),b)", "reify(watchsumgeq([b],1),!b)",
                  "reifyimply(watchless(b,!b),b)"):
            self.checked("BOOL b", c)

    def test_specialised_reification_retains_original_source(self):
        events = self.checked("DISCRETE x {0..2}\nDISCRETE y {1..3}\nBOOL b", "reify(eq(x,y),b)")
        specialised = [e for e in events if e["event"] == "constraint" and e["name"] == "__reify_eq"]
        self.assertTrue(specialised)
        self.assertTrue(all(e["source_path"] == "0" for e in specialised))
        for name in ("__reify_eq", "__reify_diseq", "__reify_minuseq"):
            self.checked("DISCRETE x {-2..2}\nDISCRETE y {-1..1}\nBOOL b", f"{name}(x,y,b)")

    def test_boolean_combinations_and_nested_guards(self):
        variables = "DISCRETE x {0..3}\nDISCRETE y {0..3}\nBOOL b\nBOOL c"
        for constraint in (
            "watched-and({ineq(x,y,-1),sumleq([x,y],3)})",
            "watched-or({eq(x,0),eq(x,3)})",
            "watched-or({false(),sumleq([x,y],1)})",
            "reify(reify(sumgeq([x,y],5),b),c)",
            "reifyimply(watched-and({eq(x,y),sumleq([x,y],3)}),b)",
            "watched-or({reify(sumleq([x,y],1),b),eq(x,0)})",
        ):
            self.checked(variables, constraint)

    def test_transparent_propagation_wrappers(self):
        for wrapper in ("forwardchecking", "check[assign]", "check[gsa]"):
            for child in ("eq(x,y)", "sumleq([x,y],1)", "reify(watchless(x,y),b)"):
                self.checked("DISCRETE x {0..3}\nDISCRETE y {0..3}\nBOOL b", f"{wrapper}({child})")

    def test_unsound_enclosing_reason_is_rejected(self):
        events = self.checked("DISCRETE x {0..3}\nDISCRETE y {1..2}\nBOOL b",
                              "reify(sumleq([x,y],2),b)\nw-literal(b,1)")
        reasons = {e["id"]: e["rule"] for e in events if e["event"] == "context_open"}
        for conflict in (False, True):
            altered = copy.deepcopy(events)
            pruning = next(e for e in altered if e["event"] == "prune" and
                           reasons.get(e["context"], "").startswith("sum.bounds."))
            if conflict:
                pruning["event"] = "conflict"
            else:
                self.assertEqual(pruning["operation"], "upper_bound")
                pruning["value"] -= 1
            result = self.certify(altered, False, flags=())
            self.assertIn("not justified", result.stderr)

    def test_matrix_child_now_has_checked_activation(self):
        events = self.trace(trace_tests.model(
            "DISCRETE x {0..1}\nDISCRETE y {0..1}\nDISCRETE n {0..2}\nDISCRETE m {0..2}\nBOOL b",
            "reify(alldiffmatrix([x,y,n,m],1),b)"))
        self.certify(events)
        self.certify(events, flags=("--audit-solutions", "10000"))


if __name__ == "__main__":
    unittest.main()
