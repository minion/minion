# mini-scripts

Small helpers, plus the long-running release soak suite.

## Output filters

Two one-liners for pulling numbers out of minion's output. Both read
stdin:

```sh
minion foo.minion | ./get_info.sh nodes        # setuptime|solutions|solvetime|nodes|totaltime
minion foo.minion | ./print_sol.sh             # just the Sol: lines
```

## Release soak suite

Long unattended runs for pre-release confidence. These are much heavier
than anything in CI: the whole suite is budgeted at about 5.75 days, and
no single soak is meant to exceed two days.

```sh
./run-all-soaks.sh                        # everything, ~138h
./run-all-soaks.sh --budget-hours 48      # everything, squeezed into 48h
./mini-scripts/soak-midsearch.sh --budget-hours 12
./run-all-soaks.sh --smoke                # ~25 min: checks the scripts work
```

### The soaks

| Script | Default | What it is for |
|---|---:|---|
| `soak-constraints.sh` | 36h | Breadth. Every constraint, rounds of growing instance size, with variable reuse and negated bools turned up. |
| `soak-midsearch.sh` | 36h | The mid-search injection matrix: packet count 1/2/3, with and without a nested wrapper, with and without added variables. |
| `soak-parallel.sh` | 24h | Work-stealing, portfolio and parallel-preprocess at sizes big enough that donations actually happen. |
| `soak-optimisation.sh` | 18h | Metamorphic optimisation: several strategies must agree on the optimum. |
| `soak-debug.sh` | 24h | The same ground again with assertions live — debug binary, plus both in-process assertion layers. |

Common options: `--budget-hours N`, `--budget-minutes M`, `--jobs N`,
`--numthreads N`, `--logdir DIR`, `--smoke`.

### Design notes

Each soak builds what it needs (`bin-quick`, `bin-debug`, the tester,
and for `soak-debug` a second tester linked against a `DEBUG_MINION=1`
libminion). Existing builds are reused.

The constraint list is harvested from the tester at startup rather than
kept here. A hardcoded list quietly stops covering anything added later
— `tester/big-test-logs/run.sh` still names 64 constraints where the
tester now registers 78.

Soaks run *rounds* of increasing difficulty until the budget is gone,
rather than a fixed number of trials: a count tuned on one machine means
nothing on another. Rounds the budget never reaches are reported as
skipped, so the summary says how far the run got.

Work is one tester process per item, `--jobs` at a time, defaulting to
`nproc / --numthreads` because each tester runs `--numthreads` solves
itself. Items are per-constraint, and per-mode where modes are swept, so
there are hundreds of them — enough to keep any core count busy, and
enough isolation that an assertion abort in one constraint doesn't take
the sweep with it.

### Reading the result

Every item writes `<round>-<item>.log` and `<round>-<item>.exit` under
the log directory, and each soak ends with a summary and the tail of
every failure.

Exit status is zero only if nothing failed and nothing timed out. A
timeout is not a pass: it means the item never finished. That matters
most in `soak-parallel.sh`, where instance growth is unbounded by design
and the timeout is the only bound.

### Requirements

GNU `timeout` (`brew install coreutils` on macOS, where it is
`gtimeout`). The soaks refuse to start without it — an unattended
multi-day run with no per-item timeout eventually wedges on one trial
and wastes the budget. `stdbuf` is used when present so logs stay
readable mid-run.

## See also

`TESTING.md` at the repo root describes the four layers of test
infrastructure and what each covers. `./test.sh --light` is the
pre-push check; `./test.sh --heavy` is the 24–48h gate that these soaks
extend.
