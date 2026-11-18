"""Frame block correspondence and its generic forward-checking negation."""
import copy
import unittest
import test_annotations as trace_tests


class FrameProofTests(unittest.TestCase):
    trace = trace_tests.AnnotationTests.trace
    certify = trace_tests.AnnotationTests.certify

    @classmethod
    def setUpClass(cls):
        trace_tests.AnnotationTests.setUpClass()

    def checked(self, variables, constraints):
        events = self.trace(trace_tests.model(variables, constraints))
        self.certify(events, flags=("--audit-solutions", "10000"))
        return events

    def test_blocks_and_prefix_semantics(self):
        for kind in ("DISCRETE", "BOUND"):
            self.checked(f"{kind} x[4] {{0..2}}", "frameupdate([1,x[0]],[x[1],x[2]],[],[],1)")
            self.checked(f"{kind} x[4] {{0..2}}", "frameupdate([0,1,x[0],x[1]],[x[2],x[3],1,0],[1],[2],2)")
        self.checked("DISCRETE x {0..2}", "frameupdate([0],[x],[1],[],1)")
        self.checked("DISCRETE x {0..2}", "frameupdate([0,x],[0,1],[2],[1],1)")

    def test_invalid_indices_and_unassigned_indices(self):
        for indices in ("[0]", "[3]", "[1,1]"):
            self.checked("BOOL x\nBOOL y", f"frameupdate([x,y],[1,0],{indices},[],1)")
        self.checked("BOOL x\nBOOL y\nDISCRETE i {0..3}", "frameupdate([x,y],[1,0],[i],[],1)")

    def test_aliases_and_boolean_views(self):
        self.checked("BOOL b", "frameupdate([b,!b],[!b,b],[],[],1)")
        self.checked("BOOL b\nDISCRETE i {1..2}", "frameupdate([b,!b],[!b,b],[i],[1],1)")

    def test_both_polarities_and_implication(self):
        for wrapper in ("reify", "reifyimply", "reifyimply-quick"):
            for value in (None, 0, 1):
                with self.subTest(wrapper=wrapper, guard=value):
                    c = f"{wrapper}(frameupdate([x,y],[1,z],[],[],1),b)"
                    if value is not None:
                        c += f"\nw-literal(b,{value})"
                    self.checked("BOOL x\nBOOL y\nBOOL z\nBOOL b", c)
        self.checked("BOOL b", "reify(frameupdate([b],[1],[],[],1),b)")

    def test_invalid_bound_rejected(self):
        events = self.checked("DISCRETE x {0..3}", "frameupdate([1],[x],[],[],1)")
        altered = copy.deepcopy(events)
        pruning = next(e for e in altered if e["event"] == "prune" and e["variable"] == 0)
        pruning["operation"] = "lower_bound"
        pruning["value"] = 2
        result = self.certify(altered, False, flags=())
        self.assertIn("not justified", result.stderr)


if __name__ == "__main__":
    unittest.main()
