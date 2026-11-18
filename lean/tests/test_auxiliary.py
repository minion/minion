"""Auxiliary variables: solutions are certified up to the non-auxiliary variables."""
import unittest
import test_annotations as trace_tests

MODEL = """MINION 3
**VARIABLES**
DISCRETE x {0..3}
DISCRETE y {0..3}
DISCRETE a {0..5}
DISCRETE b {0..5}
**SEARCH**
VARORDER [x,y]
VARORDER AUX [a,b]
**CONSTRAINTS**
diseq(x,y)
sumleq([x,y],a)
ineq(a,b,-1)
**EOF**
"""
# No VARORDER names a or b, so Minion treats them as auxiliary.
IMPLICIT = MODEL.replace("VARORDER AUX [a,b]\n", "")


class AuxiliaryTests(unittest.TestCase):
    trace = trace_tests.AnnotationTests.trace
    certify = trace_tests.AnnotationTests.certify

    @classmethod
    def setUpClass(cls):
        trace_tests.AnnotationTests.setUpClass()

    def projected(self, source, flags=()):
        events = self.trace(source, flags)
        result = self.certify(events, flags=("--audit-solutions", "100000"))
        self.assertIn("projected solution set CERTIFIED", result.stdout)
        self.assertIn("projected solution set audited", result.stdout)
        return events, result

    def test_one_solution_for_each_assignment_of_the_other_variables(self):
        for source in (MODEL, IMPLICIT):
            for flags in ((), ("-prop-node", "SAC")):
                with self.subTest(flags=flags):
                    events, result = self.projected(source, flags)
                    self.assertEqual(events[0]["auxiliary"], [2, 3])
                    solutions = [e["values"] for e in events if e["event"] == "solution"]
                    self.assertEqual(len(solutions), 10)
                    self.assertEqual(len({tuple(s[:2]) for s in solutions}), 10)
                    self.assertTrue(any(e["event"] == "branch_skip" for e in events))

    def test_first_solution_with_auxiliary_variables(self):
        events = self.trace(MODEL, all_solutions=False)
        result = self.certify(events)
        self.assertIn("search up to the solution CERTIFIED", result.stdout)

    def test_a_skip_while_another_variable_is_still_open_is_rejected(self):
        # Claim that only b is auxiliary: branches on a are then skipped while
        # a itself, now a variable that matters, could still take other values.
        events, _ = self.projected(MODEL)
        events[0]["auxiliary"] = [3]
        self.certify(events, False)

    def test_a_skip_cannot_hide_a_branch_on_a_non_auxiliary_variable(self):
        events, _ = self.projected(MODEL)
        # Turn the right branch of a decision on y into a skip.
        position = next(i for i, e in enumerate(events)
                        if e["event"] == "branch" and not e["equal"] and e["variable"] == 1)
        split = events[position]["split"]
        end = next(i for i, e in enumerate(events) if e["event"] == "branch_close" and e["split"] == split)
        altered = events[:position] + [{"event": "branch_skip", "split": split}] + events[end + 1:]
        # Re-number: identifiers in this trace are sequence numbers.
        old = {e["seq"]: i for i, e in enumerate(altered, 1) if "seq" in e}
        old[split] = old.get(split, split)
        for i, e in enumerate(altered, 1):
            e["seq"] = i
            for key in ("id", "split", "parent", "context", "witness"):
                if key in e and e[key] in old:
                    e[key] = old[e[key]]
        self.certify(altered, False)

    def test_a_reported_solution_must_be_a_solution(self):
        events, _ = self.projected(MODEL)
        solution = next(e for e in events if e["event"] == "solution")
        solution["values"][3] = solution["values"][2]
        self.certify(events, False)


if __name__ == "__main__":
    unittest.main()
