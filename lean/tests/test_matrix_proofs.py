"""Row/column count proofs and projected Hall cuts for all-different matrix."""
import copy
import unittest
import test_annotations as trace_tests


class MatrixProofTests(unittest.TestCase):
    trace = trace_tests.AnnotationTests.trace
    certify = trace_tests.AnnotationTests.certify

    @classmethod
    def setUpClass(cls):
        trace_tests.AnnotationTests.setUpClass()

    def checked(self, variables, constraints):
        events = self.trace(trace_tests.model(variables, constraints))
        self.certify(events, flags=("--audit-solutions", "10000"))
        return events

    def test_counts_and_empty_matrix(self):
        self.checked("BOOL x[4]", "alldiffmatrix(x,1)")
        self.checked("DISCRETE x[4] {-1..1}", "alldiffmatrix(x,0)")
        self.checked("BOOL x", "alldiffmatrix([x],1)")
        self.checked("BOOL x\nBOOL y", "alldiffmatrix([1,1,x,y],1)")
        self.checked("", "alldiffmatrix([],1)")

    def test_projected_hall_cut(self):
        self.checked("BOOL a\nBOOL b\nBOOL c\nBOOL d\nBOOL e\nBOOL f\nBOOL g",
                     "alldiffmatrix([a,b,0,c,d,0,e,f,g],1)")
        self.checked("BOOL a\nBOOL b\nBOOL c\nBOOL d", "alldiffmatrix([a,b,0,c,d,0,1,1,0],1)")

    def test_aliases_and_boolean_views(self):
        self.checked("BOOL b", "alldiffmatrix([b,!b,!b,b],1)")
        self.checked("BOOL b", "alldiffmatrix([b,b,!b,!b],1)")

    def test_both_polarities_and_implication(self):
        for wrapper in ("reify", "reifyimply", "reifyimply-quick"):
            for value in (None, 0, 1):
                with self.subTest(wrapper=wrapper, guard=value):
                    c = f"{wrapper}(alldiffmatrix(x,1),b)"
                    if value is not None:
                        c += f"\nw-literal(b,{value})"
                    self.checked("BOOL x[4]\nBOOL b", c)
        self.checked("BOOL b\nBOOL x\nBOOL y\nBOOL z", "reify(alldiffmatrix([b,x,y,z],1),b)")

    def test_invalid_deletion_rejected(self):
        events = self.checked("BOOL a\nBOOL b\nBOOL c\nBOOL d\nBOOL e\nBOOL f\nBOOL g",
                              "alldiffmatrix([a,b,0,c,d,0,e,f,g],1)")
        reasons = {e["id"]: e["rule"] for e in events if e["event"] == "context_open"}
        altered = copy.deepcopy(events)
        pruning = next(e for e in altered if e["event"] == "prune" and e["variable"] == 6 and
                       reasons.get(e["context"]) == "matrix.hall")
        pruning["operation"] = "remove"
        pruning["value"] = 1
        result = self.certify(altered, False, flags=())
        self.assertIn("not justified", result.stderr)


if __name__ == "__main__":
    unittest.main()
