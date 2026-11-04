import contextlib
import io
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import unittest
from unittest.mock import Mock, patch

import run_basic_tests as basic
import run_tests as suite


class RunnerTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.here = Path(self.temp.name)
        self.solver = Mock(exe="minion")
        self.output = io.StringIO()
        self.capture = contextlib.redirect_stdout(self.output)
        self.capture.__enter__()
        self.addCleanup(self.capture.__exit__, None, None, None)

    def instance(self, text, name="test.minion"):
        path = self.here / name
        path.write_text(text)
        return path

    def response(self, stdout="", rc=0):
        self.solver.run.return_value = subprocess.CompletedProcess(
            ["minion", "test.minion"], rc, stdout, "stderr marker")

    def test_directives_require_expected_output_and_status(self):
        for directive, output, expected_rc in [
                ("SOLCOUNT 2", "Solutions Found: 2\n", 0),
                ("CHECKONESOL 1 2", "Sol: 1 2\n", 0),
                ("NODECOUNT 3", "Total Nodes: 3\n", 0),
                ("EXITCODE0", "", 0), ("EXITCODE1", "", 1)]:
            for rc in (0, 1):
                with self.subTest(directive=directive, rc=rc):
                    path = self.instance("#TEST " + directive)
                    self.response(output, rc)
                    passed, message = basic.check(self.solver, path)
                    self.assertEqual(passed, rc == expected_rc)
                    if not passed:
                        for token in ("Command:", "Exit code:", "stdout:",
                                      "stderr:", "stderr marker"):
                            self.assertIn(token, message)

    def test_basic_failure_markers(self):
        for marker in ("#FAIL", "#BUG", "#FAIL\n#BUG"):
            for matches in (True, False):
                with self.subTest(marker=marker, matches=matches):
                    self.instance(marker + "\n#TEST SOLCOUNT 2\n")
                    self.response(f"Solutions Found: {2 if matches else 3}\n")
                    expected_match = "#FAIL" not in marker
                    failed = matches == expected_match if "#BUG" in marker else matches != expected_match
                    self.assertEqual(basic.run_suite(self.solver, self.here), int(failed))
                    if failed:
                        self.assertIn("stderr marker", self.output.getvalue())

    def test_extra_flags_keep_arguments(self):
        path = self.instance("#TEST SOLCOUNT 2\n#TEST EXTRAFLAGS -X-parallelWorkSteal 4\n")
        self.response("Solutions Found: 2\n")
        self.assertTrue(basic.check(self.solver, path, ["-preprocess", "SAC"])[0])
        args = self.solver.run.call_args.args[1]
        self.assertEqual(args[1:5], ["-X-parallelWorkSteal", "4", "-preprocess", "SAC"])
        with patch.object(suite, "HERE", self.here):
            self.assertEqual(suite.random_sweep(self.solver, 2, ["-varorder", "sdf"], []), 0)
        calls = self.solver.run.call_args_list[-2:]
        for seed, call in enumerate(calls):
            self.assertEqual(call.args[1][1:5],
                             ["-X-parallelWorkSteal", "4", "-randomseed", str(seed)])

    def test_random_sweep_rejects_nonzero_exit_with_matching_count(self):
        self.instance("#TEST SOLCOUNT 2\n")
        self.response("Solutions Found: 2\n", 1)
        with patch.object(suite, "HERE", self.here):
            self.assertEqual(suite.random_sweep(self.solver, 1, [], []), 1)
        self.assertIn("stderr marker", self.output.getvalue())

    def test_random_candidates_exclude_order_sensitive_cases_and_include_both_widths(self):
        self.instance("#TEST SOLCOUNT 2", "normal.minion")
        for i, marker in enumerate(("#FAIL", "#BUG", "MINIMISING x", "maximising x",
                                     "#TEST NODECOUNT 2")):
            self.instance(marker + ("\n#TEST SOLCOUNT 2" if i < 4 else ""), f"skip{i}.minion")
        for width in (32, 64):
            directory = self.here / f"tests-{width}domains"
            directory.mkdir()
            (directory / "wide.minion").write_text("#TEST SOLCOUNT 2")
        with patch.object(suite, "HERE", self.here):
            self.assertEqual(len(suite.random_candidates()), 2)
            self.assertEqual(len(suite.random_candidates(big=True)), 3)

    def test_empty_suites_fail(self):
        with self.assertRaisesRegex(ValueError, "No .minion"):
            basic.run_suite(self.solver, self.here)
        with patch.object(suite, "HERE", self.here):
            with self.assertRaisesRegex(ValueError, "No randomised"):
                suite.random_sweep(self.solver, 1, [], [])

    def test_malformed_instance_fails(self):
        with self.assertRaises(SystemExit):
            basic.check(self.solver, self.instance("MINION 3\n"))

    def special_setup(self):
        directory = self.here / "special_tests"
        directory.mkdir()
        (directory / "bibd.minion-amo").write_text("AMO expected\n")
        self.response("SAC x 36\nSSAC x 244\nAMO expected\n")

    def test_special_expected_results_and_nonzero_exits(self):
        self.special_setup()
        with patch.object(suite, "HERE", self.here):
            self.assertEqual(suite.special_tests(self.solver), 0)
            self.solver.run.return_value.returncode = 1
            self.assertEqual(suite.special_tests(self.solver), 4)
        self.assertIn("stderr marker", self.output.getvalue())

    def test_corrupt_amo_golden_fails(self):
        self.special_setup()
        (self.here / "special_tests/bibd.minion-amo").write_text("AMO corrupted\n")
        with patch.object(suite, "HERE", self.here):
            self.assertEqual(suite.special_tests(self.solver), 1)

    def test_special_failure_makes_main_fail(self):
        with patch.object(basic, "Solver", return_value=self.solver), \
                patch.object(basic, "run_suite", return_value=0), \
                patch.object(suite, "has_wdeg", return_value=False), \
                patch.object(suite, "RANDOM_SWEEPS", []), \
                patch.object(suite, "special_tests", return_value=1):
            self.assertEqual(suite.main(["run_tests.py", "minion"]), 1)

    def test_basic_only_big_checks_both_directories_and_forwards_flags(self):
        with patch.object(basic, "Solver", return_value=self.solver), \
                patch.object(basic, "run_suite", return_value=0) as run_suite, \
                patch.object(suite, "special_tests") as special, \
                patch.object(suite, "has_wdeg") as wdeg:
            self.assertEqual(suite.main(["run_tests.py", "minion", "--basic-only", "--big",
                                         "-preprocess", "SAC"]), 0)
            self.assertEqual([call.args[1] for call in run_suite.call_args_list],
                             [str(suite.HERE), str(suite.HERE / "tests-64domains")])
            self.assertTrue(all(call.args[2] == ["-preprocess", "SAC"]
                                for call in run_suite.call_args_list))
            special.assert_not_called()
            wdeg.assert_not_called()

    def test_random_only_skips_basic_and_special_checks(self):
        with patch.object(basic, "Solver", return_value=self.solver), \
                patch.object(basic, "run_suite") as run_suite, \
                patch.object(suite, "special_tests") as special, \
                patch.object(suite, "has_wdeg", return_value=False), \
                patch.object(suite, "random_sweep", return_value=0) as sweep:
            self.assertEqual(suite.main(["run_tests.py", "minion", "--random-only"]), 0)
            self.assertEqual(sweep.call_count, len(suite.RANDOM_SWEEPS))
            run_suite.assert_not_called()
            special.assert_not_called()

    def test_failure_count_cannot_wrap_to_success(self):
        code = ("import run_tests, run_basic_tests; "
                "run_basic_tests.run_suite=lambda *args: 256; "
                "raise SystemExit(run_tests.main(['run_tests.py', "
                "__import__('sys').executable, '--basic-only']))")
        p = subprocess.run([sys.executable, "-c", code], cwd=Path(__file__).parent,
                           capture_output=True, text=True, timeout=10)
        self.assertEqual(p.returncode, 1, p.stdout + p.stderr)

    def test_wdeg_detection_requires_build_information(self):
        for enabled in (True, False):
            self.response("this one is " + ("such a build." if enabled else "not."))
            self.assertEqual(suite.has_wdeg(self.solver), enabled)
        self.response("Usage: minion")
        with self.assertRaisesRegex(RuntimeError, "Missing wdeg"):
            suite.has_wdeg(self.solver)

    def test_crashes_cannot_be_accepted_as_expected_failures(self):
        solver = basic.Solver(sys.executable)
        with self.assertRaisesRegex(RuntimeError, "stderr marker"):
            solver.run(self.here, ["-c", "import sys; print('stderr marker', file=sys.stderr); sys.exit(139)"])
        crash = subprocess.CompletedProcess(["minion"], -11, "", "crash marker")
        for marker in ("#FAIL", "#BUG"):
            self.instance(marker + "\n#TEST SOLCOUNT 0")
            with patch.object(subprocess, "run", return_value=crash):
                with self.assertRaisesRegex(RuntimeError, "crash marker"):
                    basic.run_suite(solver, self.here)

    def test_exception_status_requires_explicit_expectation(self):
        solver = basic.Solver(sys.executable)
        args = ["-c", "import sys; sys.exit(9)"]
        with self.assertRaises(RuntimeError):
            solver.run(self.here, args)
        self.assertEqual(solver.run(self.here, args, expected_exit=9).returncode, 9)
        path = self.instance("#TEST EXITCODE9")
        self.response(rc=9)
        self.assertTrue(basic.check(self.solver, path)[0])
        self.solver.run.assert_called_with(str(self.here), [path.name], expected_exit=9)

    def test_timeout_reports_partial_output(self):
        solver = basic.Solver(sys.executable, timeout=0.5)
        timeout = subprocess.TimeoutExpired([sys.executable], 0.5,
                                            output=b"stdout marker", stderr=b"stderr marker")
        with patch.object(subprocess, "run", side_effect=timeout):
            with self.assertRaisesRegex(RuntimeError, "stdout marker") as error:
                solver.run(self.here, [])
        self.assertIn("Timed out", str(error.exception))
        self.assertIn("stderr marker", str(error.exception))

    def test_exhausted_budget_starts_no_process(self):
        solver = basic.Solver(sys.executable)
        solver.deadline = time.monotonic() - 1
        with patch.object(subprocess, "run") as run:
            with self.assertRaisesRegex(RuntimeError, "Suite budget exhausted"):
                solver.run(self.here, [])
            run.assert_not_called()

    def test_remaining_budget_bounds_next_process(self):
        solver = basic.Solver(sys.executable, timeout=60)
        solver.deadline = time.monotonic() + 1
        self.response()
        with patch.object(subprocess, "run", return_value=self.solver.run.return_value) as run:
            solver.run(self.here, [])
            self.assertGreater(run.call_args.kwargs["timeout"], 0)
            self.assertLessEqual(run.call_args.kwargs["timeout"], 1)


if __name__ == "__main__":
    unittest.main()
