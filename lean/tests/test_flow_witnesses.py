"""All-different and GCC prunings carry cuts which need no matching discovery."""
import unittest
import test_annotations as trace_tests


class FlowWitnessTests(unittest.TestCase):
    trace = trace_tests.AnnotationTests.trace
    certify = trace_tests.AnnotationTests.certify

    @classmethod
    def setUpClass(cls):
        trace_tests.AnnotationTests.setUpClass()

    def certified(self, variables, constraints, flags=("--no-matching-discovery",)):
        events = self.trace(trace_tests.model(variables, constraints))
        result = self.certify(events, flags=() + flags)
        self.assertIn("complete search tree CERTIFIED", result.stdout)
        return events

    def test_alldiff_pruning_uses_every_variable_reaching_the_component(self):
        # Golomb ruler, 5 marks, length 10: the Hall set of a pruned value is
        # larger than the strongly connected component being split off.
        marks, length = 5, 10
        pairs = [(i, j) for i in range(marks) for j in range(i + 1, marks)]
        variables = "\n".join([f"DISCRETE t{i} {{0..{length}}}" for i in range(marks)] +
                              [f"DISCRETE g{i}_{j} {{1..{length}}}" for i, j in pairs])
        constraints = ["eq(t0,0)"] + [f"ineq(t{i},t{i + 1},-1)" for i in range(marks - 1)]
        for i, j in pairs:
            constraints += [f"sumleq([t{i},g{i}_{j}],t{j})", f"sumgeq([t{i},g{i}_{j}],t{j})"]
        constraints.append("gacalldiff([" + ",".join(f"g{i}_{j}" for i, j in pairs) + "])")
        events = self.certified(variables, "\n".join(constraints))
        self.assertTrue(any(e.get("kind") == "hall" and len(e["positions"]) > 1 for e in events))

    def test_gcc_value_and_capacity_prunings_supply_cuts(self):
        for name in ("gcc", "gccweak"):
            variables = "\n".join([f"DISCRETE x{i} {{0..3}}" for i in range(8)] +
                                  [f"DISCRETE c{v} {{1..3}}" for v in range(4)])
            constraints = [f"{name}([{','.join(f'x{i}' for i in range(8))}],[0,1,2,3],[c0,c1,c2,c3])"]
            constraints += [f"ineq(x{i},x{i + 1},0)" for i in range(7)] + ["eq(c0,2)", "diseq(x5,2)"]
            events = self.certified(variables, "\n".join(constraints))
            kinds = {e.get("kind") for e in events if e.get("event") == "witness"}
            self.assertTrue(kinds & {"gcc_upper", "gcc_lower"}, name)

    def test_gcc_sharing_a_capacity_variable_keeps_the_proved_fallback(self):
        variables = "\n".join(f"DISCRETE x{i} {{0..2}}" for i in range(4)) + "\nDISCRETE c {0..3}"
        self.certified(variables, "gcc([x0,x1,x2,x3,c],[0,1],[c,x0])\nineq(x1,x2,0)",
                       flags=("--audit-solutions", "10000"))


if __name__ == "__main__":
    unittest.main()
