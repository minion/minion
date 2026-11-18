"""Independent small examples and adversarial files for the built Lean checker.

Run from anywhere: python3 lean/tests/test_semantics.py
"""
import copy
import json
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
CHECKER = ROOT / ".lake/build/bin/minion-semantics"


def var(index):
    return {"kind": "var", "index": index}


def const(value):
    return {"kind": "constant", "value": value}


def constraint(name, args=(), children=(), **data):
    return {"name": name, "args": list(args), "children": list(children), **data}


def model(domains, constraints, solutions):
    return {
        "format": "minion-solutions-v1",
        "variables": [{"name": f"x{i}", "domain": d} for i, d in enumerate(domains)],
        "constraints": constraints,
        "solutions": solutions,
    }


class SemanticsTests(unittest.TestCase):
    def check(self, claim, accepted=True, budget=None):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "claim.json"
            path.write_text(json.dumps(claim))
            args = [str(CHECKER)]
            if budget is not None:
                args += ["--max-assignments", str(budget)]
            result = subprocess.run(args + [str(path)], capture_output=True, text=True)
            self.assertEqual(result.returncode == 0, accepted, result.stdout + result.stderr)

    def test_element_support_and_indexing(self):
        for offset, name in [(0, "element"), (1, "element_one")]:
            self.check(model(
                [[offset, offset + 1], [1, 2, 3]],
                [constraint(name, [[const(1), const(3)], [var(0)], [var(1)]])],
                [[offset, 1], [offset + 1, 3]],
            ))

    def test_undefined_element_and_empty_arrays(self):
        self.check(model(
            [[-1, 0, 1], [0, 5]],
            [constraint("element_undefzero", [[const(5)], [var(0)], [var(1)]])],
            [[-1, 0], [0, 5], [1, 0]],
        ))
        self.check(model([], [constraint("element", [[], [const(0)], [const(0)]])], []))
        self.check(model([], [constraint("element_undefzero", [[], [const(0)], [const(0)]])], [[]]))

    def test_reification_and_implication(self):
        child = constraint("eq", [[var(0)], [const(1)]])
        for name, solutions in [
            ("reify", [[0, 0], [1, 1]]),
            ("reifyimply", [[0, 0], [1, 0], [1, 1]]),
            ("reifyimply-quick", [[0, 0], [1, 0], [1, 1]]),
        ]:
            self.check(model([[0, 1], [0, 1]], [constraint(name, [[var(1)]], [child])], solutions))

    def test_registry_internal_aliases(self):
        for name, solutions in (
            ("__reify_eq", [[-1, 0], [0, 0], [1, 1]]),
            ("__reify_diseq", [[-1, 1], [0, 1], [1, 0]]),
            ("__reify_minuseq", [[-1, 1], [0, 0], [1, 0]]),
        ):
            self.check(model([[-1, 0, 1], [0, 1]],
                             [constraint(name, [[var(0)], [const(1)], [var(1)]])], solutions))
        self.check(model([[0, 1]], [constraint("()()collectevents()()", [[var(0)]])], [[0], [1]]))

    def test_aliases_negation_and_conjunction_of_constraints(self):
        neg = {"kind": "not_bool", "index": 0}
        self.check(model([[0, 1]], [constraint("eq", [[var(0)], [neg]])], []))
        self.check(model([[0, 1]], [constraint("eq", [[var(0)], [var(0)]])], [[0], [1]]))
        self.check(model([[0, 1]], [constraint("eq", [[var(0)], [const(0)]]),
                                   constraint("eq", [[var(0)], [const(1)]])], []))

    def test_arithmetic_views_compose_and_validate_underlying_references(self):
        def neg(term):
            return {"kind": "neg", "term": term}

        self.check(model([[-2, 0, 3]], [constraint("eq", [[neg(var(0))], [const(2)]])], [[-2]]))
        self.check(model([[-2, 0, 3]], [constraint("eq", [[neg(neg(var(0)))], [var(0)]])],
                         [[-2], [0], [3]]))
        self.check(model([[0, 1]], [constraint("eq", [[neg({"kind": "not_bool", "index": 0})],
                                                    [const(-1)]])], [[0]]))
        shifted = {"kind": "shift", "term": neg(var(0)), "offset": 3}
        self.check(model([[-2, 0, 3]], [constraint("eq", [[shifted], [const(5)]])], [[-2]]))
        scaled = {"kind": "scale", "term": shifted, "factor": -2}
        self.check(model([[-2, 0, 3]], [constraint("eq", [[scaled], [const(-10)]])], [[-2]]))
        for term in [neg(var(1)), neg({"kind": "not_bool", "index": 0})]:
            self.check(model([[-2, 3]], [constraint("eq", [[term], [const(0)]])], []), False)

    def test_floor_division_modulo_and_zero(self):
        for x, y, quotient, remainder in [(7, -3, -3, -2), (-7, 3, -3, 2), (-7, -3, 2, -1)]:
            for name, result in [("div", quotient), ("modulo", remainder)]:
                self.check(model([], [constraint(name, [[const(x)], [const(y)], [const(result)]])], [[]]))
        for name in ["div", "modulo"]:
            self.check(model([], [constraint(name, [[const(1)], [const(0)], [const(0)]])], []))
            self.check(model([], [constraint(name + "_undefzero", [[const(1)], [const(0)], [const(0)]])], [[]]))

    def test_power_negative_exponents(self):
        for x, y, z, solutions in [(0, 0, 1, [[]]), (0, -1, 0, []),
                                    (-1, -3, -1, [[]]), (2, -1, 0, [])]:
            self.check(model([], [constraint("pow", [[const(x)], [const(y)], [const(z)]])], solutions))

    def test_table_boundaries_and_compressed_short_tuples(self):
        for name, tuples, solutions in [("table", [], []), ("table", [[]], [[]]),
                                         ("negativetable", [], [[]]), ("negativetable", [[]], [])]:
            self.check(model([], [constraint(name, [[]], tuples=tuples)], solutions))
        for short, solutions in [([], []), ([[]], [[0], [1], [2]])]:
            self.check(model([[0, 1, 2]], [constraint("shortstr2", [[var(0)]], short_tuples=short)], solutions))
        # Two literals for one position have OR semantics in compressed tuples.
        self.check(model([[0, 1, 2]], [constraint("shortctuplestr2", [[var(0)]],
                         short_tuples=[[[0, 0], [0, 2]]])], [[0], [2]]))

    def test_zero_variables_and_empty_domains(self):
        self.check(model([], [], [[]]))
        self.check(model([], [constraint("false")], []))
        self.check(model([[]], [], []))
        self.check(model([], [constraint("watched-and")], [[]]))
        self.check(model([], [constraint("watched-or")], []))

    def test_exact_set_rejects_missing_extra_duplicate_and_bad_width(self):
        base = model([[0, 1]], [], [[1], [0]])  # Order is immaterial.
        self.check(base)
        for solutions in [[[0]], [[0], [1], [2]], [[0], [1], [1]], [[0], [1, 0]], []]:
            bad = copy.deepcopy(base)
            bad["solutions"] = solutions
            self.check(bad, False)

    def test_unknown_constraints_and_invalid_references_fail_closed(self):
        for child in [constraint("unimplemented"), constraint("eq", [[var(9)], [const(0)]])]:
            # Even an inactive implication must not conceal unsupported input.
            self.check(model([], [constraint("reifyimply", [[const(0)]], [child])], [[]]), False)
        self.check(model([[0, 0]], [], [[0]]), False)
        self.check(model([[2]], [constraint("eq", [[{"kind": "not_bool", "index": 0}], [const(-1)]])], [[2]]), False)
        self.check(model([[0]], [constraint("table", [[var(0)]], tuples=[[]])], []), False)

    def test_budget_is_a_rejection(self):
        self.check(model([[0, 1]], [], [[0], [1]]), False, budget=1)

    def test_decoder_covers_every_registered_tester_constraint(self):
        import re
        source = (ROOT.parent / "tester/src/constraint_def.rs").read_text()
        names = re.findall(r'ConstraintDef::new\w*\(\s*"([^\"]+)"', source)
        decoder = (ROOT / "Minion/Json.lean").read_text()
        self.assertEqual(len(names), 78)
        self.assertFalse([name for name in names if json.dumps(name) not in decoder])


if __name__ == "__main__":
    unittest.main()
