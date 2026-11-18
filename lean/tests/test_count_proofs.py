"""Must/may count bounds shared by occurrence, literal sums and Hamming."""
import copy
import unittest
import test_annotations as trace_tests


class CountProofTests(unittest.TestCase):
    trace = trace_tests.AnnotationTests.trace
    certify = trace_tests.AnnotationTests.certify

    @classmethod
    def setUpClass(cls):
        trace_tests.AnnotationTests.setUpClass()

    def checked(self, variables, constraints):
        events = self.trace(trace_tests.model(variables, constraints))
        self.certify(events, flags=("--audit-solutions", "10000"))
        return events

    def test_occurrence_result_and_constant_bounds(self):
        self.checked("DISCRETE x {0..2}\nDISCRETE y {0..2}\nDISCRETE n {-1..3}", "occurrence([x,y,1],1,n)")
        for name in ("occurrenceleq", "occurrencegeq"):
            for count in (-1, 0, 1, 2, 3):
                self.checked("DISCRETE x {0..2}\nDISCRETE y {0..2}", f"{name}([x,y],1,{count})")
        self.checked("DISCRETE n {-1..2}", "occurrence([],1,n)")

    def test_literal_sums_and_hamming(self):
        self.checked("DISCRETE x {-1..1}\nDISCRETE y {-1..1}\nDISCRETE z {-1..1}",
                     "litsumgeq([x,y,z],[0,1,-1],2)")
        for name in ("hamming", "not-hamming"):
            for count in (0, 1, 2, 3):
                self.checked("DISCRETE x {0..2}\nDISCRETE y {0..2}", f"{name}([x,y],[y,1],{count})")
            self.checked("", f"{name}([],[],1)")
        self.checked("", "litsumgeq([],[],1)")

    def test_aliases_and_boolean_views(self):
        self.checked("DISCRETE x {0..3}\nDISCRETE y {0..2}", "occurrence([x,x,y],1,x)")
        self.checked("BOOL b", "litsumgeq([b,b],[0,1],2)")
        self.checked("BOOL b", "hamming([b,!b],[!b,b],2)")
        self.checked("DISCRETE x {0..2}", "hamming([x,x],[x,x],1)")

    def test_vector_exists_predicates(self):
        for name in ("watchvecneq", "watchvecexists_less"):
            self.checked("DISCRETE x {0..2}\nDISCRETE y {0..2}", f"{name}([x,y],[y,x])")
            self.checked("BOOL b", f"{name}([b,!b],[!b,b])")
            self.checked("", f"{name}([],[])")

    def test_reification_and_implication(self):
        variables = "DISCRETE x {0..2}\nDISCRETE y {0..2}\nDISCRETE n {0..3}\nBOOL b"
        children = ("occurrence([x,y],1,n)", "occurrencegeq([x,y],1,1)",
                    "occurrenceleq([x,y],1,1)", "litsumgeq([x,y],[0,1],1)",
                    "hamming([x,y],[y,1],1)", "not-hamming([x,y],[y,1],1)",
                    "watchvecneq([x,y],[y,x])", "watchvecexists_less([x,y],[y,x])")
        for child in children:
            for wrapper in ("reify", "reifyimply", "reifyimply-quick"):
                for value in (None, 0, 1):
                    with self.subTest(child=child, wrapper=wrapper, guard=value):
                        c = f"{wrapper}({child},b)"
                        if value is not None:
                            c += f"\nw-literal(b,{value})"
                        self.checked(variables, c)

    def test_unsound_count_bound_rejected(self):
        events = self.checked("DISCRETE x {0..2}\nDISCRETE n {-1..2}", "occurrence([x],1,n)")
        reasons = {e["id"]: e["rule"] for e in events if e["event"] == "context_open"}
        altered = copy.deepcopy(events)
        pruning = next(e for e in altered if e["event"] == "prune" and e["variable"] == 1 and
                       reasons.get(e["context"]) == "count.bounds")
        self.assertIn(pruning["operation"], ("lower_bound", "upper_bound"))
        pruning["value"] += 1 if pruning["operation"] == "lower_bound" else -1
        result = self.certify(altered, False, flags=())
        self.assertIn("not justified", result.stderr)


if __name__ == "__main__":
    unittest.main()
