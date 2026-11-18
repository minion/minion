"""Element support rules, their Boolean wrappers, and invalid deductions."""
import copy
import unittest
import test_annotations as trace_tests


NAMES = ("element", "element_one", "element_undefzero", "watchelement", "watchelement_one",
         "watchelement_undefzero", "watchelement_one_undefzero")


class ElementProofTests(unittest.TestCase):
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

    def test_all_element_variants(self):
        variables = "DISCRETE a {1..3}\nDISCRETE b {2..4}\nDISCRETE i {-1..3}\nDISCRETE r {0..5}"
        for name in NAMES:
            with self.subTest(name=name):
                self.checked(variables, f"{name}([a,b],i,r)")

    def test_reification_both_polarities_and_free_guards(self):
        variables = "DISCRETE a {1..2}\nDISCRETE b {2..3}\nDISCRETE i {-1..3}\nDISCRETE r {0..3}\nBOOL guard"
        for name in NAMES:
            for value in (None, 0, 1):
                with self.subTest(name=name, guard=value):
                    constraint = f"reify({name}([a,b],i,r),guard)"
                    if value is not None:
                        constraint += f"\nw-literal(guard,{value})"
                    self.checked(variables, constraint)

    def test_implication_variants(self):
        variables = "DISCRETE a {1..2}\nDISCRETE i {-1..2}\nDISCRETE r {0..3}\nBOOL guard"
        for wrapper in ("reifyimply", "reifyimply-quick"):
            for name in NAMES:
                with self.subTest(wrapper=wrapper, name=name):
                    self.checked(variables, f"{wrapper}({name}([a,2],i,r),guard)")

    def test_empty_arrays_and_out_of_range(self):
        for name in NAMES:
            with self.subTest(name=name):
                self.checked("DISCRETE i {-2..2}\nDISCRETE r {0..1}", f"{name}([],i,r)")
                self.checked("DISCRETE r {0..1}", f"{name}([1],-1,r)")

    def test_aliases_and_boolean_views(self):
        for name in NAMES:
            for array, index, result in (("[a,a]", "i", "r"), ("[i,a]", "i", "r"),
                                         ("[a,r]", "i", "r"), ("[!b,b]", "i", "b")):
                with self.subTest(name=name, array=array):
                    self.checked("DISCRETE a {0..2}\nDISCRETE i {0..2}\nDISCRETE r {0..2}\nBOOL b",
                                 f"{name}({array},{index},{result})")

    def test_selected_array_domain_and_rhs_holes(self):
        for name in ("element", "watchelement"):
            self.checked("DISCRETE a {0..3}\nDISCRETE r {0..3}",
                         f"w-notliteral(r,1)\nw-notliteral(r,2)\n{name}([a,2],0,r)")
        self.checked("BOUND a {0..3}\nBOUND i {0..1}\nBOUND r {1..2}", "element([a,3],i,r)")

    def test_unsound_result_pruning_is_rejected(self):
        events = self.checked("DISCRETE i {0..1}\nDISCRETE r {0..4}", "watchelement([1,3],i,r)")
        altered = copy.deepcopy(events)
        reasons = {e["id"]: e["rule"] for e in events if e["event"] == "context_open"}
        pruning = next(e for e in altered if e["event"] == "prune" and e["variable"] == 1 and
                       reasons.get(e["context"]) == "element.support")
        pruning["operation"] = "remove"
        pruning["value"] = 1
        result = self.certify(altered, False, flags=())
        self.assertIn("not justified", result.stderr)


if __name__ == "__main__":
    unittest.main()
