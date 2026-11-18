"""Minion's compact binary trace, as written by Minion and read by the checker."""
from pathlib import Path
import subprocess
import tempfile
import unittest
import test_annotations as trace_tests

DIRECT = "complete search tree CERTIFIED"
QUEENS = trace_tests.model(
    "\n".join(f"DISCRETE q{i} {{0..4}}" for i in range(5)),
    "\n".join(f"diseq(q{i},q{j})\nwatchneq(q{i},q{j})" for i in range(5) for j in range(i + 1, 5)) +
    "\n" + "\n".join(f"ineq(q{i},q{i + 1},2)" for i in range(4)))
GLOBALS = trace_tests.model(
    "\n".join(f"DISCRETE x{i} {{0..3}}" for i in range(5)) + "\nDISCRETE c0 {0..3}\nDISCRETE c1 {1..2}",
    "gacalldiff([x0,x1,x2])\ngcc([x0,x1,x2,x3,x4],[0,1],[c0,c1])\nsumleq([x3,x4],x0)\nineq(x1,x2,-1)")
UNSAT = trace_tests.model("\n".join(f"DISCRETE p{i} {{1..3}}" for i in range(4)),
                          "\n".join(f"diseq(p{i},p{j})" for i in range(4) for j in range(i + 1, 4)))
NESTED = trace_tests.model(
    "BOOL a\nBOOL b\nDISCRETE x {0..3}\nDISCRETE y {0..3}\nDISCRETE z {0..3}",
    "watched-or({eq(x,y),sumleq([y,z],2)})\nreify(watched-and({diseq(x,z),ineq(y,z,-1)}),a)\n"
    "reifyimply(gacalldiff([x,y,z]),b)\ndiseq(a,b)")
TABLES = trace_tests.model(
    "DISCRETE x {0..3}\nDISCRETE y {0..3}\nDISCRETE z {0..3}\nBOOL b",
    "table([x,y],t)\nnegativetable([y,z],t)\nreify(str2plus([x,z],t),b)",
    "**TUPLELIST**\nt 5 2\n0 1\n1 2\n2 3\n3 0\n2 2")
REIFIED = trace_tests.model("BOOL b\nDISCRETE x {0..2}\nDISCRETE y {0..2}", "reify(sumleq([x,y],1),b)")


class BinaryTraceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        trace_tests.AnnotationTests.setUpClass()

    def run_model(self, source, directory, flags=()):
        model = Path(directory) / "model.minion"
        trace = Path(directory) / "trace.bin"
        model.write_text(source)
        result = subprocess.run([str(trace_tests.BINARY), "-findallsols", "-noprintsols", *flags,
                                 "-annotations", str(trace), str(model)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        return trace

    def certify(self, trace, *flags):
        return subprocess.run([str(trace_tests.CERTIFY), "--audit-solutions", "100000",
                               *flags, str(trace)], capture_output=True, text=True)

    def test_plain_searches_and_probes_certify(self):
        cases = [(source, ()) for source in (QUEENS, GLOBALS, UNSAT)]
        cases += [(source, ("-prop-node", level)) for source in (QUEENS, GLOBALS, UNSAT)
                  for level in ("SAC", "SSAC", "SACBounds", "SSACBounds")]
        cases += [(QUEENS, ("-preprocess", "SSAC"))]
        for source, flags in cases:
            with tempfile.TemporaryDirectory() as directory:
                trace = self.run_model(source, directory, flags)
                self.assertTrue(trace.read_bytes().startswith(b"MINIONTR\x01"))
                direct = self.certify(trace)
                self.assertEqual(direct.returncode, 0, direct.stdout + direct.stderr)
                self.assertIn(DIRECT, direct.stdout)
                self.assertIn("complete search tree CERTIFIED", direct.stdout)

    def test_nested_constraints_and_tables_certify(self):
        for source in (REIFIED, NESTED, TABLES):
            for flags in ((), ("-prop-node", "SAC"), ("-prop-node", "SSAC")):
                with tempfile.TemporaryDirectory() as directory:
                    trace = self.run_model(source, directory, flags)
                    direct = self.certify(trace)
                    self.assertEqual(direct.returncode, 0, direct.stdout + direct.stderr)
                    self.assertIn(DIRECT, direct.stdout)

    def test_a_damaged_deletion_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            trace = self.run_model(UNSAT, directory)
            data = bytearray(trace.read_bytes())
            start = data.index(b'"solutions":[]}') + 16
            position = next(i for i in range(start, len(data) - 4) if data[i] == 5)
            data[position + 2] ^= 2
            trace.write_bytes(bytes(data))
            result = self.certify(trace)
            self.assertNotEqual(result.returncode, 0)

    def test_truncated_traces_are_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            trace = self.run_model(QUEENS, directory)
            data = trace.read_bytes()
            for length in (len(data) - 1, len(data) // 2, 9):
                trace.write_bytes(data[:length])
                self.assertNotEqual(self.certify(trace).returncode, 0, length)

    def test_damaged_bytes_never_certify_a_wrong_solution_set(self):
        # Every accepted run below also passed exhaustive enumeration of the
        # model, so a damaged trace is either rejected or still correct.
        with tempfile.TemporaryDirectory() as directory:
            trace = self.run_model(GLOBALS, directory, ("-prop-node", "SSAC"))
            data = trace.read_bytes()
            model_end = data.index(b'"solutions":[]}') + 16
            rejected = 0
            for position in range(model_end, len(data), max(1, (len(data) - model_end) // 120)):
                damaged = bytearray(data)
                damaged[position] ^= 1
                trace.write_bytes(bytes(damaged))
                result = self.certify(trace)
                if result.returncode != 0:
                    rejected += 1
                else:
                    self.assertIn("complete solution set audited", result.stdout)
            self.assertGreater(rejected, 0)


if __name__ == "__main__":
    unittest.main()
