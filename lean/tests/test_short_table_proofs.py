"""Short table semantics, compressed alternatives, and forward-checking negation."""
import copy
import unittest
import test_annotations as trace_tests


NAMES = ("shortstr2", "shortctuplestr2", "haggisgac", "haggisgac-stable")


class ShortTableProofTests(unittest.TestCase):
    trace = trace_tests.AnnotationTests.trace
    certify = trace_tests.AnnotationTests.certify

    @classmethod
    def setUpClass(cls):
        trace_tests.AnnotationTests.setUpClass()

    def checked(self, variables, constraints, rows=(((0, 0),), ((1, 1),))):
        tuples = f"**SHORTTUPLELIST**\nt {len(rows)}\n" + "\n".join(
            "[" + ",".join(f"({i},{v})" for i, v in row) + "]" for row in rows)
        events = self.trace(trace_tests.model(variables, constraints, tuples))
        self.certify(events, flags=("--audit-solutions", "10000"))
        return events

    def test_all_implementations_and_empty_tables(self):
        for name in NAMES:
            with self.subTest(name=name):
                self.checked("DISCRETE x {0..2}\nDISCRETE y {0..2}", f"{name}([x,y],t)")
                self.checked("DISCRETE x {0..2}\nDISCRETE y {0..2}", f"{name}([x,y],t)", rows=())
                self.checked("DISCRETE x {0..2}\nDISCRETE y {0..2}", f"{name}([x,y],t)", rows=((),))

    def test_compressed_alternatives(self):
        self.checked("DISCRETE x {0..3}\nDISCRETE y {0..2}", "shortctuplestr2([x,y],t)",
                     rows=(((0, 0), (0, 2), (1, 1)),))
        self.checked("DISCRETE x {0..3}\nBOOL b", "reify(shortctuplestr2([x],t),b)",
                     rows=(((0, 0), (0, 2)),))

    def test_aliases_and_boolean_views(self):
        for name in NAMES:
            with self.subTest(name=name):
                self.checked("DISCRETE x {0..2}", f"{name}([x,x],t)")
                self.checked("BOOL b", f"{name}([b,!b],t)")

    def test_wrappers_and_union_of_rows(self):
        variables = "DISCRETE x {0..2}\nDISCRETE y {0..2}\nBOOL b"
        for name in NAMES:
            for wrapper in ("reify", "reifyimply", "reifyimply-quick"):
                for value in (None, 0, 1):
                    with self.subTest(name=name, wrapper=wrapper, guard=value):
                        c = f"{wrapper}({name}([x,y],t),b)"
                        if value is not None:
                            c += f"\nw-literal(b,{value})"
                        self.checked(variables, c)
            self.checked("DISCRETE x {0..20}\nBOOL b", f"reify({name}([x],t),b)\nw-literal(b,0)",
                         rows=tuple(((0, v),) for v in range(10)))

    def test_invalid_deletion_is_rejected(self):
        events = self.checked("DISCRETE x {0..3}", "shortstr2([x],t)", rows=(((0, 0),), ((0, 2),)))
        reasons = {e["id"]: e["rule"] for e in events if e["event"] == "context_open"}
        altered = copy.deepcopy(events)
        pruning = next(e for e in altered if e["event"] == "prune" and e["variable"] == 0 and
                       reasons.get(e["context"]) == "table.support")
        pruning["operation"] = "remove"
        pruning["value"] = 2
        result = self.certify(altered, False, flags=())
        self.assertIn("not justified", result.stderr)


if __name__ == "__main__":
    unittest.main()
