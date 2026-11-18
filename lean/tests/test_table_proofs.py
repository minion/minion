"""Shared positive/negative tuple support, including generated negative children."""
import copy
import unittest
import test_annotations as trace_tests


NAMES = ("table", "str2plus", "gacschema", "lighttable", "mddc", "negativetable", "negativemddc")


class TableProofTests(unittest.TestCase):
    trace = trace_tests.AnnotationTests.trace
    certify = trace_tests.AnnotationTests.certify

    @classmethod
    def setUpClass(cls):
        trace_tests.AnnotationTests.setUpClass()

    def checked(self, variables, constraints, rows=((0, 2), (1, 1), (2, 0)), arity=2):
        tuples = f"**TUPLELIST**\nt {len(rows)} {arity}\n" + "\n".join(" ".join(map(str, r)) for r in rows)
        events = self.trace(trace_tests.model(variables, constraints, tuples))
        self.certify(events, flags=("--audit-solutions", "10000"))
        return events

    def test_all_full_table_implementations(self):
        for name in NAMES:
            with self.subTest(name=name):
                self.checked("DISCRETE x {0..3}\nDISCRETE y {0..3}", f"{name}([x,y],t)")

    def test_empty_tables_and_constant_arguments(self):
        for name in NAMES:
            with self.subTest(name=name):
                self.checked("DISCRETE x {0..2}", f"{name}([x,1],t)")
                self.checked("DISCRETE x {0..2}\nDISCRETE y {0..2}", f"{name}([x,y],t)", rows=())

    def test_aliases_and_boolean_views(self):
        for name in NAMES:
            with self.subTest(name=name):
                self.checked("DISCRETE x {0..2}", f"{name}([x,x],t)")
                self.checked("BOOL b", f"{name}([b,!b],t)", rows=((0, 1), (1, 0)))

    def test_both_reified_polarities_and_implication(self):
        variables = "DISCRETE x {0..2}\nDISCRETE y {0..2}\nBOOL b"
        for name in NAMES:
            for wrapper in ("reify", "reifyimply", "reifyimply-quick"):
                for value in (None, 0, 1):
                    with self.subTest(name=name, wrapper=wrapper, guard=value):
                        c = f"{wrapper}({name}([x,y],t),b)"
                        if value is not None:
                            c += f"\nw-literal(b,{value})"
                        self.checked(variables, c)

    def test_invalid_supported_value_deletion_is_rejected(self):
        events = self.checked("DISCRETE x {0..3}\nDISCRETE y {0..2}", "table([x,y],t)")
        reasons = {e["id"]: e["rule"] for e in events if e["event"] == "context_open"}
        altered = copy.deepcopy(events)
        pruning = next(e for e in altered if e["event"] == "prune" and e["variable"] == 0 and
                       reasons.get(e["context"]) == "table.support")
        pruning["operation"] = "remove"
        pruning["value"] = 1
        result = self.certify(altered, False, flags=())
        self.assertIn("not justified", result.stderr)


if __name__ == "__main__":
    unittest.main()
