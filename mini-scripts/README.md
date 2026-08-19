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

### How they are built

Each soak builds whatever it needs (`bin-quick`, `bin-debug`, the Rust
tester, and for `soak-debug` a second tester linked against a
`DEBUG_MINION=1` libminion). Existing builds are reused, so re-running
after a crash doesn't recompile from scratch.

The constraint list is harvested from the tester at startup rather than
kept here. A hardcoded list quietly stops covering anything added later
— `tester/big-test-logs/run.sh` still names 64 constraints where the
tester now registers 78.

### Budgets, not trial counts

Each soak runs *rounds* of increasing difficulty until its budget runs
out, rather than a fixed number of trials. A trial count tuned on one
machine is meaningless on another; a budget is not. Rounds that the
budget doesn't reach are reported as skipped, so the summary always says
how far the run actually got.

### Parallelism

Work is one tester process per item, run `--jobs` at a time. Items are
per-constraint (and per-mode where a soak sweeps modes), which gives
hundreds of independent items — enough to keep any core count busy.

`--jobs` defaults to `nproc / --numthreads`, because each tester process
itself runs `--numthreads` solves concurrently. The product is what
lands on the cores.

Process-per-item also buys crash isolation: these soaks mostly catch
assertion aborts, and an abort in one constraint must not take the sweep
with it.

### Reading the result

Every item writes `<round>-<item>.log` and `<round>-<item>.exit` under
the log directory, and each soak ends with a summary plus the tail of
every failure. Exit status is zero only if nothing failed *and* nothing
timed out.

A timeout is deliberately not a pass. On these budgets it means the item
never finished, which is a result worth looking at — particularly in
`soak-parallel.sh`, where instance growth is unbounded by design and the
per-item timeout is the only thing stopping it.

### Requirements

GNU `timeout` (`brew install coreutils` on macOS, where it is
`gtimeout`). The soaks refuse to start without it: an unattended
multi-day run with no per-item timeout will eventually wedge on a single
trial and waste the whole budget. `stdbuf` is used when present so logs
stay readable mid-run.

## See also

`TESTING.md` at the repo root describes the four layers of test
infrastructure and what each covers. `./test.sh --light` is the
pre-push check; `./test.sh --heavy` is the 24–48h gate that these soaks
extend.
