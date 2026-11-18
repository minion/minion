"""Optimisation: a branch-and-bound trace certifies its optimum."""
import copy
import unittest
import test_annotations as trace_tests


def problem(direction, extra=""):
    return ("MINION 3\n**VARIABLES**\nDISCRETE x {0..5}\nDISCRETE y {0..5}\nDISCRETE z {0..12}\n"
            f"**SEARCH**\n{direction} z\nVARORDER [x,y,z]\n**CONSTRAINTS**\n"
            f"sumleq([x,y],z)\nsumgeq([x,y],z)\ndiseq(x,y)\nineq(2,x,0)\n{extra}**EOF**\n")


class OptimisationTests(unittest.TestCase):
    trace = trace_tests.AnnotationTests.trace
    certify = trace_tests.AnnotationTests.certify

    @classmethod
    def setUpClass(cls):
        trace_tests.AnnotationTests.setUpClass()

    def optimum(self, source, flags=()):
        events = self.trace(source, flags, all_solutions=False)
        result = self.certify(events, flags=("--audit-solutions", "100000"))
        self.assertIn("optimality CERTIFIED", result.stdout)
        self.assertIn("optimum audited by enumeration", result.stdout)
        return events, result

    def test_minimum_and_maximum_with_and_without_probes(self):
        for direction, expected in (("MINIMISING", "minimum z = 2"), ("MAXIMISING", "maximum z = 9")):
            for flags in ((), ("-prop-node", "SAC"), ("-prop-node", "SSAC")):
                with self.subTest(direction=direction, flags=flags):
                    events, result = self.optimum(problem(direction), flags)
                    self.assertIn(expected, result.stdout)
                    self.assertEqual(events[0]["optimise"]["minimising"], direction == "MINIMISING")

    def test_bounds_from_earlier_solutions_are_steps_of_the_refutation(self):
        events, _ = self.optimum(problem("MAXIMISING"))
        self.assertGreater(sum(e["event"] == "solution" for e in events), 1)
        self.assertTrue(any(e["event"] == "context_open" and e["rule"] == "optimise.bound" for e in events))

    def test_infeasible_optimisation_is_an_ordinary_refutation(self):
        events = self.trace(problem("MINIMISING", "ineq(x,1,0)\n"), all_solutions=False)
        result = self.certify(events, flags=("--audit-solutions", "100000"))
        self.assertIn("0 solutions", result.stdout)
        self.assertIn("complete search tree CERTIFIED", result.stdout)

    def test_a_wrong_optimum_is_rejected(self):
        events, _ = self.optimum(problem("MAXIMISING"))
        # Drop the last improving solution: the search no longer refutes anything better than the claim.
        last = max(i for i, e in enumerate(events) if e["event"] == "solution")
        claimed = copy.deepcopy(events)
        claimed[last]["values"][2] -= 1
        self.certify(claimed, False)
        # Claim a better value than any solution attains.
        inflated = copy.deepcopy(events)
        inflated[last]["values"][2] += 1
        self.certify(inflated, False)

    def test_a_bound_not_implied_by_the_optimum_is_rejected(self):
        events, _ = self.optimum(problem("MAXIMISING"))
        contexts = {e["id"] for e in events if e["event"] == "context_open" and e["rule"] == "optimise.bound"}
        bound = [e for e in events if e["event"] == "prune" and e["context"] in contexts][-1]
        bound["value"] += 5
        self.certify(events, False)

    def test_solutions_must_improve(self):
        events, _ = self.optimum(problem("MAXIMISING"))
        solutions = [e for e in events if e["event"] == "solution"]
        solutions[0]["values"], solutions[1]["values"] = solutions[1]["values"], solutions[0]["values"]
        self.certify(events, False)


if __name__ == "__main__":
    unittest.main()
