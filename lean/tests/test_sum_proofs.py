"""Proved sum-rule integration, including intentionally unsound proof steps.

MINION_ANNOTATIONS_BINARY=/path/to/new/minion python3 lean/tests/test_sum_proofs.py
"""
import os
import re
import unittest
import test_annotations as trace_tests


class SumProofTests(unittest.TestCase):
    trace = trace_tests.AnnotationTests.trace
    certify = trace_tests.AnnotationTests.certify

    @classmethod
    def setUpClass(cls):
        trace_tests.AnnotationTests.setUpClass()

    def checked(self, source, expect_deduction=True):
        events = self.trace(source)
        result = self.certify(events, flags=("--audit-solutions", "10000"))
        self.assertIn("0 prunings and 0 conflicts UNCHECKED", result.stdout)
        if expect_deduction:
            # Some deduction or conflict was checked, not only solution leaves.
            match = re.search(r"(\d+) deductions, (\d+) leaves", result.stdout)
            self.assertIsNotNone(match)
            solutions = int(re.search(r": (\d+) solutions", result.stdout).group(1))
            self.assertGreater(int(match.group(1)) + int(match.group(2)) - solutions, 0)
        return events

    def test_general_bounds_both_directions(self):
        for op in ("sumleq", "sumgeq"):
            self.checked(trace_tests.model(
                "DISCRETE x {-2..3}\nDISCRETE y {1..3}\nDISCRETE z {-1..2}\nDISCRETE r {0..2}",
                f"{op}([x,y,z],r)",
            ))

    def test_two_term_light_implementation(self):
        for op, rhs in [("sumleq", 3), ("sumgeq", 6)]:
            self.checked(trace_tests.model("DISCRETE x {0..5}\nDISCRETE y {2..4}", f"{op}([x,y],{rhs})"))

    def test_result_bounds(self):
        for op in ("sumleq", "sumgeq"):
            self.checked(trace_tests.model("DISCRETE r {-3..8}", f"{op}([1,2,1],r)"))

    def test_boolean_specialisation(self):
        for op, rhs in [("sumleq", 0), ("sumgeq", 3)]:
            events = self.checked(trace_tests.model("BOOL x\nBOOL y\nBOOL z", f"{op}([x,y,z],{rhs})"))
            if os.environ.get("MINION_EXPECT_BOOL_SPECIALISATION"):
                self.assertTrue(any(e["event"] == "context_open" and
                                    e["rule"] == "sum.bounds.boolean_force" for e in events))

    def test_watched_sums_and_clause_specialisation(self):
        for op, rhs in (("watchsumleq", 0), ("watchsumleq", 2),
                        ("watchsumgeq", 1), ("watchsumgeq", 3)):
            self.checked(trace_tests.model("BOOL x\nBOOL y\nBOOL z", f"{op}([x,!y,z],{rhs})"))

    def test_empty_sums_and_initial_conflicts(self):
        for op, rhs in [("sumleq", -1), ("sumgeq", 1)]:
            self.checked(trace_tests.model("", f"{op}([],{rhs})"))
        self.checked(trace_tests.model("", "sumleq([],0)"), expect_deduction=False)
        for op, rhs in [("sumleq", 1), ("sumgeq", 5)]:
            self.checked(trace_tests.model("DISCRETE x {1..2}\nDISCRETE y {1..2}", f"{op}([x,y],{rhs})"))

    def test_aliases_and_result_in_left_hand_side(self):
        for op in ("sumleq", "sumgeq"):
            self.checked(trace_tests.model("DISCRETE x {-2..2}\nDISCRETE y {0..3}", f"{op}([x,x,y],x)"))

    def test_holes_constants_and_negated_booleans(self):
        for op in ("sumleq", "sumgeq"):
            self.checked(trace_tests.model("SPARSEBOUND x {-3,0,2}\nBOOL b\nDISCRETE r {-2..3}",
                                           f"{op}([x,!b,2],r)"))

    def test_unsound_pruning_is_rejected_by_the_rule_before_replay(self):
        events = self.checked(trace_tests.model("DISCRETE x {0..5}\nDISCRETE y {2..4}", "sumleq([x,y],3)"))
        reasons = {e["id"]: e["rule"] for e in events if e["event"] == "context_open"}
        pruning = next(e for e in events if e["event"] == "prune" and
                       reasons.get(e["context"]) == "sum.bounds.term")
        self.assertEqual(pruning["operation"], "upper_bound")
        pruning["value"] -= 1
        result = self.certify(events, False, flags=())
        self.assertIn("not justified", result.stderr)

    def test_fabricated_conflict_is_rejected(self):
        events = self.checked(trace_tests.model("DISCRETE x {0..5}\nDISCRETE y {2..4}", "sumleq([x,y],3)"))
        reasons = {e["id"]: e["rule"] for e in events if e["event"] == "context_open"}
        event = next(e for e in events if e["event"] == "prune" and
                     reasons.get(e["context"]) == "sum.bounds.term")
        event["event"] = "conflict"
        result = self.certify(events, False, flags=())
        self.assertIn("conflict not justified", result.stderr)

    def test_solver_cache_info_is_not_trusted(self):
        events = self.checked(trace_tests.model("DISCRETE x {0..5}\nDISCRETE y {2..4}", "sumleq([x,y],3)"))
        for event in events:
            if event["event"] == "context_open" and event["rule"].startswith("sum.bounds."):
                event["info"] = {"min_sum": 999999, "cached_min_sum": -999999, "slack": 999999}
        self.certify(events, flags=("--audit-solutions", "10000"))

    def test_reified_activation_uses_the_proved_enclosing_relation(self):
        for value in (0, 1):
            source = trace_tests.model("DISCRETE x {0..5}\nDISCRETE y {2..4}\nBOOL b",
                                       f"reify(sumleq([x,y],3),b)\nw-literal(b,{value})")
            events = self.trace(source)
            self.certify(events, flags=("--audit-solutions", "10000"))


if __name__ == "__main__":
    unittest.main()
