"""Backward trimming is untrusted: its output must pass the proved checker."""
import json
import unittest
import test_annotations as trace_tests


class TrimTests(unittest.TestCase):
    trace = trace_tests.AnnotationTests.trace
    certify = trace_tests.AnnotationTests.certify

    @classmethod
    def setUpClass(cls):
        trace_tests.AnnotationTests.setUpClass()

    def trimmed(self, variables, constraints, tuples=""):
        events = self.trace(trace_tests.model(variables, constraints, tuples))
        result = self.certify(events, flags=("--trim", "--audit-solutions", "10000"))
        self.assertIn("complete search tree CERTIFIED", result.stdout)
        line = next(l for l in result.stdout.splitlines() if l.startswith("TRIM "))
        report = json.loads(line[5:])
        return report["before"], report["after"]

    def test_independent_refutation_drops_irrelevant_branching(self):
        variables = "\n".join(f"DISCRETE {v} {{0..3}}" for v in "abc") + "\n" + \
            "\n".join(f"DISCRETE {v} {{0..2}}" for v in "xyzw")
        clique = "\n".join(f"diseq({p},{q})" for i, p in enumerate("xyzw") for q in "xyzw"[i + 1:])
        before, after = self.trimmed(variables, "ineq(a,b,-1)\nineq(b,c,-1)\n" + clique)
        self.assertEqual(before["splitDepth"], 6)
        self.assertEqual(after["splitDepth"], 3)
        self.assertLess(after["propagationSteps"], before["propagationSteps"])

    def test_solutions_keep_their_deductions(self):
        before, after = self.trimmed("DISCRETE x {0..3}\nDISCRETE y {0..3}", "eq(x,y)")
        self.assertEqual(before["nodes"], after["nodes"])

    def test_tables_probes_and_witnesses_survive_trimming(self):
        tuples = "**TUPLELIST**\nt 2 2\n0 2\n2 0"
        self.trimmed("BOOL b\nDISCRETE x {0..2}\nDISCRETE y {0..2}\nDISCRETE z {0..2}",
                     "reify(table([x,y],t),b)\nstr2plus([y,z],t)\ndiseq(x,z)", tuples)
        self.trimmed("\n".join(f"DISCRETE p{i} {{0..2}}" for i in range(4)) + "\nBOOL f",
                     "gacalldiff([p0,p1,p2,p3])")
        self.trimmed("BOOL x\nBOOL y\nBOOL z", "diseq(x,y)\ndiseq(y,z)\ndiseq(z,x)")


if __name__ == "__main__":
    unittest.main()
