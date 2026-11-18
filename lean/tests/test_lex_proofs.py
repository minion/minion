"""Prefix support proofs for all lex implementations, including alias witnesses."""
import copy
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
import test_annotations as trace_tests


NAMES = ("lexleq", "lexleq[rv]", "lexleq[quick]", "lexless", "lexless[quick]")


class LexProofTests(unittest.TestCase):
    trace = trace_tests.AnnotationTests.trace
    certify = trace_tests.AnnotationTests.certify

    @classmethod
    def setUpClass(cls):
        trace_tests.AnnotationTests.setUpClass()

    def checked(self, variables, constraints):
        events = self.trace(trace_tests.model(variables, constraints))
        self.certify(events, flags=("--audit-solutions", "10000"))
        return events

    def test_all_implementations_and_empty_vectors(self):
        for name in NAMES:
            with self.subTest(name=name):
                self.checked("DISCRETE x {0..3}\nDISCRETE y {1..3}\nDISCRETE z {-1..2}",
                             f"{name}([x,z],[y,1])")
                self.checked("", f"{name}([],[])")

    def test_aliases_across_positions_and_boolean_views(self):
        variables = "DISCRETE x {0..2}\nDISCRETE y {0..2}\nDISCRETE z {0..2}\nBOOL b"
        for name in NAMES:
            for left, right in (("[x,y]", "[y,x]"), ("[x,x,y]", "[y,x,x]"),
                                ("[x,y,z]", "[y,z,x]"), ("[b,!b]", "[!b,b]"),
                                ("[x,x]", "[x,x]"), ("[2,x]", "[1,y]")):
                with self.subTest(name=name, left=left, right=right):
                    self.checked(variables, f"{name}({left},{right})")

    def test_both_reified_polarities_and_implication(self):
        variables = "DISCRETE x {0..2}\nDISCRETE y {0..2}\nBOOL b"
        for name in NAMES:
            for wrapper in ("reify", "reifyimply", "reifyimply-quick"):
                for value in (None, 0, 1):
                    with self.subTest(name=name, wrapper=wrapper, guard=value):
                        c = f"{wrapper}({name}([x,y],[y,x]),b)"
                        if value is not None:
                            c += f"\nw-literal(b,{value})"
                        self.checked(variables, c)

    def test_holes_and_bound_variables(self):
        for name in NAMES:
            self.checked("DISCRETE x {-2..3}\nDISCRETE y {-1..2}",
                         f"w-notliteral(x,0)\nw-notliteral(y,1)\n{name}([x,y],[1,0])")
            self.checked("BOUND x {-2..3}\nBOUND y {-1..2}", f"{name}([x,y],[y,x])")

    def test_invalid_pruning_and_conflict_rejected(self):
        events = self.checked("DISCRETE x {0..4}", "lexless[quick]([x],[2])")
        reasons = {e["id"]: e["rule"] for e in events if e["event"] == "context_open"}
        for conflict in (False, True):
            altered = copy.deepcopy(events)
            pruning = next(e for e in altered if e["event"] == "prune" and
                           reasons.get(e["context"]) == "lex.support")
            if conflict:
                pruning["event"] = "conflict"
            else:
                self.assertEqual(pruning["operation"], "upper_bound")
                pruning["value"] -= 1
            result = self.certify(altered, False, flags=())
            self.assertIn("not justified", result.stderr)

    def test_duplicate_view_prefix_single_conflict_exports_to_kernel(self):
        # General JSON views: 24 duplicate prefixes used to expand 2^24 paths.
        # A single conflict also ensures the cost is inside one inference, not
        # hidden by prior solver pruning or the bounds/prefix fast path.
        n = 24
        var = lambda i: {"kind": "var", "index": i}
        constant = lambda v: {"kind": "constant", "value": v}
        left = [{"kind": "scale", "factor": 0, "term": var(2 * i)} for i in range(n)] + [var(1)]
        right = [var(2 * i + 1) for i in range(n)] + [constant(0)]
        for reverse in (False, True):
            with self.subTest(reverse=reverse), tempfile.TemporaryDirectory() as directory:
                child = {"name": "lexless" if reverse else "lexleq", "source_path": "0/0",
                         "args": [right, left] if reverse else [left, right], "children": []}
                model = {"format": "minion-solutions-v1",
                         "variables": [{"name": f"v{i}", "domain": [0, 1]} for i in range(2 * n)],
                         "constraints": [{"name": "reify", "source_path": "0",
                                          "args": [[constant(int(reverse))]], "children": [child]}],
                         "solutions": []}
                events = [
                    {"event": "model", "format": "minion-annotations-v1", "model": model},
                    {"event": "constraint", "id": 1, "name": "reify", "source_path": "0", "variables": []},
                    {"event": "context_open", "id": 3, "parent": 0, "constraint": 1,
                     "rule": "lex.support", "info": {}},
                    {"event": "conflict", "context": 3},
                    {"event": "context_close", "id": 3},
                    {"event": "end", "complete": True, "open_worlds": 0, "open_contexts": 0}]
                for i, event in enumerate(events, 1):
                    event["seq"] = i
                trace = Path(directory) / "trace.jsonl"
                proof = Path(directory) / "Certificate.lean"
                trace.write_bytes(trace_tests.binary_trace.encode(events))
                exported = subprocess.run([str(trace_tests.CERTIFY), "--export-lean", str(proof), str(trace)],
                                          capture_output=True, text=True, timeout=10)
                self.assertEqual(exported.returncode, 0, exported.stdout + exported.stderr)
                checked = subprocess.run(["lake", "env", "lean", str(proof)], cwd=trace_tests.ROOT,
                                         capture_output=True, text=True, timeout=30)
                self.assertEqual(checked.returncode, 0, checked.stdout + checked.stderr)
                self.assertNotIn("sorryAx", checked.stdout)
                self.assertNotIn("Lean.ofReduceBool", checked.stdout)
                # The all-zero assignment now satisfies the altered constraint.
                altered = copy.deepcopy(events)
                changed = altered[0]["model"]["constraints"][0]["children"][0]
                changed["args"][0 if reverse else 1][-1] = constant(-1)
                self.certify(altered, False, flags=())


if __name__ == "__main__":
    unittest.main()
