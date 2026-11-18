"""Certificate layout equivalence, shape diagnostics and checked index exports."""
import json
from pathlib import Path
import subprocess
import tempfile
import unittest

import test_annotations as trace_tests


LAYOUTS = ("auto", "tree", "blocks-array", "blocks-index", "blocks-index-build")


class CertificateLayoutTests(unittest.TestCase):
    trace = trace_tests.AnnotationTests.trace
    certify = trace_tests.AnnotationTests.certify

    @classmethod
    def setUpClass(cls):
        trace_tests.AnnotationTests.setUpClass()

    def test_all_layouts_check_the_same_deductions(self):
        source = trace_tests.model("DISCRETE x {0..8}",
                                   "\n".join(f"diseq(x,{i})" for i in range(8)))
        events = self.trace(source)
        for layout in LAYOUTS:
            with self.subTest(layout=layout):
                self.certify(events, flags=("--certificate-layout", layout,
                                           "--audit-solutions", "100"))

    def shape(self, source):
        result = self.certify(self.trace(source), flags=("--certificate-shape",))
        return json.loads(next(line.removeprefix("SHAPE ") for line in result.stdout.splitlines()
                               if line.startswith("SHAPE ")))

    def test_propagation_depth_becomes_a_list(self):
        shape = self.shape(trace_tests.model("DISCRETE x {0..8}",
                          "\n".join(f"diseq(x,{i})" for i in range(8))))
        self.assertEqual(shape["tree"]["height"], 9)
        self.assertEqual(shape["blocks"]["height"], 2)
        self.assertEqual(shape["blocks"]["maxBlockLength"], 8)
        self.assertEqual(shape["blocks"]["propagationSteps"], 8)
        self.assertEqual(shape["blocks"]["splitDepth"], 0)

    def test_actual_search_nesting_is_preserved(self):
        shape = self.shape(trace_tests.model("DISCRETE x {0..3}", "true()"))
        self.assertEqual(shape["blocks"]["height"], shape["tree"]["height"])
        self.assertEqual(shape["blocks"]["splitDepth"], 3)
        self.assertEqual(shape["blocks"]["maxBlockLength"], 0)

    def test_automatic_layout_amortizes_index_construction(self):
        sources = [
            (trace_tests.model("DISCRETE x {0..64}",
                               "\n".join(f"diseq(x,{i})" for i in range(64))), "program true"),
            (trace_tests.model("", "\n".join(["true()"] * 64 + ["false()"])), "program false"),
        ]
        with tempfile.TemporaryDirectory() as directory:
            for source, expected in sources:
                trace = Path(directory) / "trace.jsonl"
                proof = Path(directory) / "Certificate.lean"
                trace.write_bytes(trace_tests.binary_trace.encode(self.trace(source)))
                result = subprocess.run([str(trace_tests.CERTIFY), "--export-lean", str(proof), str(trace)],
                                        capture_output=True, text=True, timeout=60)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertIn(expected, proof.read_text())
                checked = subprocess.run(["lake", "env", "lean", str(proof)], cwd=trace_tests.ROOT,
                                         capture_output=True, text=True, timeout=60)
                self.assertEqual(checked.returncode, 0, checked.stdout + checked.stderr)

    def test_all_exports_check_in_kernel_and_wrong_index_is_rejected(self):
        events = self.trace(trace_tests.model("DISCRETE x {0..1}", "diseq(x,0)"))
        with tempfile.TemporaryDirectory() as directory:
            trace = Path(directory) / "trace.jsonl"
            proof = Path(directory) / "Certificate.lean"
            trace.write_bytes(trace_tests.binary_trace.encode(events))
            for layout in LAYOUTS:
                with self.subTest(layout=layout):
                    p = subprocess.run([str(trace_tests.CERTIFY), "--certificate-layout", layout,
                                        "--export-lean", str(proof), str(trace)],
                                       capture_output=True, text=True, timeout=60)
                    self.assertEqual(p.returncode, 0, p.stdout + p.stderr)
                    checked = subprocess.run(["lake", "env", "lean", str(proof)], cwd=trace_tests.ROOT,
                                             capture_output=True, text=True, timeout=60)
                    self.assertEqual(checked.returncode, 0, checked.stdout + checked.stderr)
                    self.assertNotIn("sorryAx", checked.stdout)
                    self.assertNotIn("Lean.ofReduceBool", checked.stdout)
                    if layout == "blocks-index":
                        text = proof.read_text()
                        self.assertIn("theorem index_eq", text)
                        self.assertEqual(text.count("Minion.Constraint.compare"), 1)
                        altered = text.replace("(.node (some c0) .empty .empty)", ".empty")
                        self.assertNotEqual(text, altered)
                        proof.write_text(altered)
                        rejected = subprocess.run(["lake", "env", "lean", str(proof)], cwd=trace_tests.ROOT,
                                                  capture_output=True, text=True, timeout=60)
                        self.assertNotEqual(rejected.returncode, 0)

    def test_large_table_payloads_check_and_corruption_is_rejected(self):
        for short in (False, True):
            with self.subTest(short=short), tempfile.TemporaryDirectory() as directory:
                values = range(-64, 65)
                if short:
                    tuples = "**SHORTTUPLELIST**\nt 129\n" + "\n".join(f"[(0,{v})]" for v in values)
                    constraint = "shortstr2([x],t)"
                    marker = "minion_short_rows%"
                    original, corrupt = "[[[0,-64]]", "[[[0,1000]]"
                else:
                    tuples = "**TUPLELIST**\nt 129 1\n" + "\n".join(map(str, values))
                    constraint = "table([x],t)"
                    marker = "minion_int_rows%"
                    original, corrupt = "[[-64]", "[[1000]"
                source = trace_tests.model("DISCRETE x {-64..-64}", f"reify({constraint},1)", tuples)
                trace = Path(directory) / "trace.jsonl"
                proof = Path(directory) / "Certificate.lean"
                trace.write_bytes(trace_tests.binary_trace.encode(self.trace(source)))
                for layout in ("auto", "tree", "blocks-index"):
                    result = subprocess.run([str(trace_tests.CERTIFY), "--certificate-layout", layout,
                                             "--export-lean", str(proof), str(trace)],
                                            capture_output=True, text=True, timeout=60)
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                    text = proof.read_text()
                    self.assertIn(marker, text)
                    checked = subprocess.run(["lake", "env", "lean", str(proof)], cwd=trace_tests.ROOT,
                                             capture_output=True, text=True, timeout=60)
                    self.assertEqual(checked.returncode, 0, checked.stdout + checked.stderr)
                    altered = text.replace(original, corrupt, 1)
                    self.assertNotEqual(altered, text)
                    proof.write_text(altered)
                    rejected = subprocess.run(["lake", "env", "lean", str(proof)], cwd=trace_tests.ROOT,
                                              capture_output=True, text=True, timeout=60)
                    self.assertNotEqual(rejected.returncode, 0)

    def test_compact_data_rejects_noninteger_values_and_negative_positions(self):
        with tempfile.TemporaryDirectory() as directory:
            proof = Path(directory) / "BadData.lean"
            for term in ('minion_int_rows% "[[1.5]]"', 'minion_short_rows% "[[[-1,0]]]"'):
                with self.subTest(term=term):
                    proof.write_text("import Minion.CertificateData\n\ndef bad := " + term + "\n")
                    result = subprocess.run(["lake", "env", "lean", str(proof)], cwd=trace_tests.ROOT,
                                            capture_output=True, text=True, timeout=60)
                    self.assertNotEqual(result.returncode, 0)
                    self.assertIn("invalid certificate data", result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
