"""A search which stops at its first solutions is certified up to that point."""
import copy
import unittest
import test_annotations as trace_tests

QUEENS = trace_tests.model(
    "\n".join(f"DISCRETE q{i} {{0..5}}" for i in range(6)),
    "\n".join(f"diseq(q{i},q{j})\nwatched-or({{ineq(q{i},q{j},{-(j - i) - 1}),ineq(q{j},q{i},{(j - i) - 1}),"
              f"sumleq([q{i},{j - i - 1}],q{j})}})" for i in range(6) for j in range(i + 1, 6)))
SUMS = trace_tests.model("DISCRETE x {0..4}\nDISCRETE y {0..4}\nDISCRETE z {0..4}",
                         "sumgeq([x,y,z],9)\ndiseq(x,y)\ngacalldiff([x,y,z])")
DETOUR = trace_tests.model(
    "BOOL x\n" + "\n".join(f"DISCRETE p{i} {{1..3}}" for i in range(4)),
    "\n".join(f"watched-or({{eq(x,1),diseq(p{i},p{j})}})" for i in range(4) for j in range(i + 1, 4)))
UNSAT = trace_tests.model("\n".join(f"DISCRETE p{i} {{1..3}}" for i in range(4)),
                          "\n".join(f"diseq(p{i},p{j})" for i in range(4) for j in range(i + 1, 4)))


class FirstSolutionTests(unittest.TestCase):
    trace = trace_tests.AnnotationTests.trace
    certify = trace_tests.AnnotationTests.certify

    @classmethod
    def setUpClass(cls):
        trace_tests.AnnotationTests.setUpClass()

    def stopped(self, source, flags=(), audit=True):
        events = self.trace(source, flags, all_solutions=False)
        self.assertFalse(events[-1]["complete"])
        result = self.certify(events, flags=("--audit-solutions", "100000") if audit else ())
        self.assertIn("search up to the solution CERTIFIED", result.stdout)
        return events, result

    def test_first_solution_with_and_without_probes(self):
        for source in (QUEENS, SUMS):
            for flags in ((), ("-prop-node", "SAC"), ("-prop-node", "SSAC")):
                with self.subTest(flags=flags):
                    events, result = self.stopped(source, flags, audit=source is SUMS)
                    self.assertEqual(sum(e["event"] == "solution" for e in events), 1)
                    self.assertIn("1 solutions before the search stopped", result.stdout)

    def test_several_solutions_before_stopping(self):
        events, result = self.stopped(SUMS, ("-sollimit", "3"))
        self.assertIn("3 solutions before the search stopped", result.stdout)
        self.assertIn("searched region audited by enumeration", result.stdout)

    def test_an_unsatisfiable_first_solution_search_is_a_complete_refutation(self):
        events = self.trace(UNSAT, all_solutions=False)
        self.assertTrue(events[-1]["complete"])
        result = self.certify(events)
        self.assertIn("complete search tree CERTIFIED", result.stdout)

    def test_a_wrong_solution_is_rejected(self):
        events, _ = self.stopped(SUMS)
        solution = next(e for e in events if e["event"] == "solution")
        solution["values"][0], solution["values"][1] = solution["values"][1], solution["values"][0]
        self.certify(events, False)

    def test_an_unjustified_deduction_before_the_solution_is_rejected(self):
        events, _ = self.stopped(QUEENS, audit=False)
        deduction = [e for e in events if e["event"] == "prune" and e["operation"] == "remove"][2]
        deduction["value"] = (deduction["value"] + 1) % 6
        self.certify(events, False)

    def test_a_branch_refuted_before_the_solution_must_really_be_refuted(self):
        # With x = 0 four pigeons need three holes; the solution is under x = 1.
        events, result = self.stopped(DETOUR)
        self.assertTrue(any(e["event"] == "conflict" for e in events))
        solution = next(i for i, e in enumerate(events) if e["event"] == "solution")
        refuted = [e for e in events[:solution] if e["event"] == "prune" and e["context"]]
        self.assertTrue(refuted)
        refuted[0]["value"] = 3 if refuted[0]["value"] != 3 else 2
        self.certify(events, False)

    def test_a_search_stopped_without_a_solution_is_not_certified(self):
        events, _ = self.stopped(QUEENS, audit=False)
        position = next(i for i, e in enumerate(events) if e["event"] == "solution")
        altered = copy.deepcopy(events[:position] + events[position + 1:])
        for i, event in enumerate(altered, 1):
            event["seq"] = i
        self.certify(altered, False)


if __name__ == "__main__":
    unittest.main()
