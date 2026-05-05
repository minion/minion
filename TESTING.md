# Testing Minion

There is one entry point: `./test.sh` at the repo root.

```
./test.sh --light                           # ~30 min, mirrors CI
./test.sh --heavy [--budget-hours N]        # default 24h, up to ~48h
```

`--light` is what you run before pushing. It mirrors `.github/workflows/CI.yml` exactly, so a green local run means CI will be green too. `--heavy` is the release-gate / overnight soak; it delegates to `test_instances/deep_test.sh`, which schedules ten phases of progressively bigger sweeps and gives up later phases gracefully if the budget runs out.

Both modes will build any required binaries (`bin-quick`, plus `bin-debug` for `--light`). Existing builds are reused.

## Layers of testing

Minion has four layers of test infrastructure. Both `--light` and `--heavy` exercise the same four layers; the difference is trial counts, instance sizes, and how many sweep variants are run.

### 1. `.minion` regression suite — `test_instances/`

Hand-curated `.minion` instances with embedded expected results. Each file may contain `#TEST SOLCOUNT n`, `#TEST CHECKONESOL ...`, `#TEST NODECOUNT n`, `#TEST EXITCODE1`, `#FAIL` (expected failure), or `#BUG` (known bug). Counts: ~287 files in `test_instances/`, 4 in `tests-32domains/`, 21 in `tests-64domains/`. About 20 files exercise optimisation (`MAXIMISING` / `MINIMISING`).

The runners are:
- `run_tests.sh <minion-binary>` — full regression, run by both light and heavy.
- `run_random_tests.sh` — solution-count-only subset, safe under randomised orderings or parallel exploration. Used when checking parallel modes.
- `run_big_tests.sh` — same as `run_tests.sh` but pulls in `tests-64domains/` for 64-bit-domain coverage.
- `do_basic_tests.sh`, `do_random_tests.sh`, `big_do_basic_tests.sh`, `big_do_random_tests.sh` — building blocks invoked by the above; not normally run directly.
- `do_valgrind_tests.sh`, `fullprop_test.sh` — specialised, not in the default flows.

### 2. Random constraint tester — `tester/`

Rust harness (~5800 lines) that drives a random-instance fuzz across every constraint in its registry. For each random instance it compares results across multiple oracles:
- **Tableisation comparison**: the constraint instance is solved, then re-encoded as an explicit `table`/`negativetable` and re-solved; solution sets must match exactly.
- **Variant equivalence**: constraints registered as semantically equivalent (`gacalldiff`/`alldiff`, `table`/`gacschema`/`lighttable`/`mddc`, etc.) are run on the same instance and their solution sets compared.
- **Option sweep**: the same instance is solved with random combinations of `-preprocess`, `-prop-node`, `-varorder`, `-valorder`, `-randomseed`, `-parallel`, output flags, etc.; solution counts must agree.
- **Restart sweep**: re-solves with `-restarts` and `-restarts-multiplier` variants.
- **Work-stealing sweep**: drives `-X-parallelWorkSteal` with adaptive instance sizing — instance grows until at least one trial actually triggers a donation, capped by `--ws-max-size`.
- **Work-stealing portfolio sweep**: same but with `-X-parallelWorkStealPortfolio`.
- **Parallel-preprocess sweep**: drives `-X-parallelPreprocess`, growing instance size until multi-round runs are observed.
- **Nested / deep-nested sweep**: wraps the iterated constraint in random parent constraints (`reify`, `reifyimply`, `watched-and`, `watched-or`) and re-runs the comparisons. `--nest-depth k` builds depth-`k` cross-type parent trees.
- **Mid-search variable injection** (in-process only): adds a fresh boolean variable mid-search via `minion_newVarMidsearch` and verifies the resulting solution set.
- **Mid-search constraint injection** (in-process only): injects a random constraint mid-search via `minion_addConstraintMidsearch`; can wrap in a nested parent, can inject multiple packets, can mix with new variables.

Important flags (full list: `cargo run -- --help` from `tester/`):
- `--minion <path>` — run via the binary (subprocess per trial, default).
- `--in-process` — link `libminion` via the Rust FFI (`minion-sys`) and drive solves in-process. Required for any midsearch test.
- `--count N` — trials per constraint per sweep (default 30).
- `--variant-count N` — trials per equivalence group; defaults to `--count`.
- `--optioncount N` — random option-combination trials.
- `--size-factor K` — multiplier on random instance size; large `K` is what actually exercises work-stealing donations.
- `--nest-depth K` — depth of random parent trees.
- `--maxtuples N` — cap on solution count when solving a tableised reference.
- `--max-solutions N` — abandon a trial whose solution set exceeds N (default 10M).

### 3. minion-sys integration tests — `minion-sys/tests/`

Rust unit tests (`cargo test --release`) that drive the C FFI surface directly without going through the random tester. These cover features the random tester can't easily express:
- `test_errors.rs` — duplicate variable rejection, empty-model semantics, error-path coverage.
- `test_parallel_search.rs` — `run_minion_parallel` scaling, output-set correctness.
- `test_work_steal.rs` — work-stealing actually divides the search tree (wall time scales with N workers on hard UNSAT instances).
- `test_seed_determinism.rs` — same seed must produce bit-identical solution sequences (needed for the mid-search injection diff to be meaningful).
- `test_tables.rs`, `test_negative_tables.rs`, `test_watchedor_reifyimply_1.rs` — specific known-good patterns.

### 4. Per-constraint isolation sweep — `tester/big-test-logs/run.sh`

Shell driver that launches one `tester` process per constraint, capped at 5 concurrent + 90-min timeout per constraint. Used in heavy phase 9. Process isolation means a crash on constraint X doesn't kill the sweep for constraint Y. Default workload is roughly 6 hours wall-clock on a 10-core box.

## What the heavy script does

`test_instances/deep_test.sh` runs (in order, each phase honouring the remaining budget):

| # | Phase | Approx budget |
|---|-------|---------------|
| 1 | `.minion` regression suite | 5–15 min |
| 2 | minion-sys cargo test | 5 min |
| 3 | tester baseline (`--count 500 --variant-count 500 --optioncount 5000`) | ~30 min |
| 4 | tester deep-nested (`--nest-depth 3`) | ~20 min |
| 5 | tester `--size-factor 2 --ws-max-size 64` | ~1 h |
| 6 | tester `--size-factor 4 --ws-max-size 128` | ~2 h |
| 7 | tester `--in-process` baseline | ~10 min |
| 8 | tester `--in-process --midsearch-constraints --num-packets 3` | ~30 min |
| 9 | per-constraint isolation sweep | ~4–6 h |
| 10 | tester `--size-factor 8 --ws-max-size 256` | ~4 h |

Heavy is also where you'd run a longer `CI-long.yml` equivalent — that workflow runs weekly on GitHub Actions and exercises the same in-process / midsearch axes with much higher trial counts.

## Feature coverage

### Variable types
| Type | Where tested |
|------|--------------|
| `BOOL` | regression + tester (default) |
| `DISCRETE` | regression + tester |
| `BOUND` | regression + tester (default for many constraints) |
| `SPARSEBOUND` | regression (parser tests) + tester (sparse-arg constraints) |
| `ALIAS` | regression (parser tests) only |

### Constraints

The random tester currently registers 64 user-facing constraint variants (the `CONSTRAINT_LIST` in `tester/src/constraint_def.rs`, plus six equivalence groups in `EQUIVALENCE_GROUPS`). Every one is exercised by the standard, nested, work-steal, parallel-preprocess, restart, and option sweeps in both `--light` and `--heavy`.

**Constraints registered in the source but NOT in the random tester:**
- `haggisgac`, `haggisgac-stable` — short-tuple constraints. Regression coverage: 8 `.minion` files exercising `haggisgac`, but no fuzz coverage.
- `shortstr2`, `shortctuplestr2`, `str2plus` — short-tuple table family. Regression coverage: 1 `.minion` file (`basic_shortstr2_table_1.minion`); `str2plus` is referenced inside the tester as the standard-table reference but not tested as a target.
- `frameupdate` — 1 regression file, no fuzz coverage.
- `forwardchecking` — registered in tester (CT_FORWARD_CHECKING), so this *is* covered.
- `()()collectevents()()`, `__reify_eq`, `__reify_diseq`, `__reify_minuseq`, `check[assign]`, `check[gsa]` — internal helpers, not user-facing.

**Gap to flag:** the random tester does not generate optimisation instances (`MAXIMISING` / `MINIMISING`). All optimisation correctness comes from the ~20 hand-written regression files. If you change anything in the optimisation path, lean on the `.minion` suite.

### Search heuristics
All `-varorder` choices (`static`, `sdf`, `sdf-random`, `srf`, `srf-random`, `ldf`, `ldf-random`, `random`, `conflict`, `wdeg`, `domoverwdeg`) are covered by the option-sweep, and additionally by `run_random_tests.sh` for solution-count agreement under each ordering. All `-valorder` choices (`ascend`, `descend`, `random`) and `-randomiseorder` / `-randomseed` are covered.

### Parallel modes
| Mode | Coverage |
|------|----------|
| `-parallel` (fork) | option sweep (exec only) |
| `-X-parallelThreads N` | option sweep — fuzzed but no dedicated correctness invariant beyond "agrees with sequential" |
| `-X-parallelWorkSteal N` | dedicated adaptive-sizing sweep + minion-sys `test_work_steal` |
| `-X-parallelWorkStealPortfolio` | dedicated sweep (exec only) |
| `-X-parallelPreprocess N` | dedicated adaptive-sizing sweep with multi-round invariant (exec only) |

### Preprocessing / propagation
`-preprocess` and `-prop-node` at every level (`None`, `GAC`, `SAC`, `SSAC`, `SACBounds`, `SSACBounds`, plus all `_limit` variants) are in the option sweep. `-X-prop-node` (the experimental override variant) is **not** explicitly tested.

### Search modes
| Mode | Coverage |
|------|----------|
| Standard depth-first | default everywhere |
| Restart search (`-restarts`, `-restarts-multiplier`, `-no-restarts-bias`) | dedicated tester sweep with 4 restart-flag variants |
| Optimisation (minimising / maximising) | regression suite + tester `--optimisation-sweep` (metamorphic comparison across 5 strategies — see below) |

#### Optimisation sweep — `--optimisation-sweep`

For each random constraint instance, the sweep wraps it with `aux = sum(real_vars)` plus a randomly-chosen `MINIMISING aux` or `MAXIMISING aux`, then solves under six strategies and asserts they all report the same optimum:

- baseline (default everything)
- `-preprocess SAC`
- `-varorder sdf -valorder descend`
- `-X-parallelWorkSteal 4`
- `-X-parallelThreads 4`
- `-restarts -restarts-multiplier 1.5`

A per-trial `-nodelimit 100000` caps pathological rolls; if any strategy hits the cap the trial is abandoned silently (a partial "best so far" can't be metamorphic-compared with a "proven optimal"). Disagreement among complete runs is reported as a failure with both `.minion` files preserved on disk for post-mortem.

The premise: we already trust constraint propagators for satisfaction (the existing tableisation sweep covers that broadly). Optimisation bugs therefore live mostly in optimisation-specific code — bound tracking, parallel bound broadcast, restart-with-optimisation interaction. Different (propagator, heuristic, parallel) combinations stress that surface differently, so disagreement is a strong signal of a bound-tracking bug.

Plumbing: minion's `-jsontableout` reports `OptimumValue` and `OptimumDirection`; the tester's exec-mode runner reads them. Sequential, work-steal, and `-X-parallelThreads` all aggregate cross-worker. `-parallel` (fork-based) is not covered; the in-process backend rejects `--optimisation-sweep` because optimisation isn't yet plumbed through the FFI.

`-restarts` is in the strategy set. The restart manager's solution handler delegates to `standard_dealWith_solution` for optimisation problems (so the bound is tightened on every solution and the LIBMINION callback fires), and the new `optimisationHandler` mirrors `search_control.h`'s `opt_handler` (applying the running bound and the cross-worker shared bound at every right-branch step). Each restart attempt is then a complete DFS under the running bound; a natural exhaustion means the bound is provably optimal.

### Library API (libminion / minion-sys)
| Surface | Coverage |
|---------|----------|
| context create/free/activate | minion-sys cargo tests |
| `minion_newVar` / `minion_newSparseBoundVar` | minion-sys cargo tests + every in-process tester run |
| `runMinion` / `runMinionParallel` / `runMinionWorkSteal` | minion-sys cargo tests + every in-process tester run |
| `minion_newVarMidsearch` | tester `--midsearch`, `--midsearch-add-vars` |
| `minion_addConstraintMidsearch` | tester `--midsearch-constraints` (with N=1, 2, 3 packet variants and nested-parent wrap variants) |
| `tupleList_new` / `instance_addTupleTableSymbol` | minion-sys `test_tables`, `test_negative_tables` |
| `printMatrix_*` | exercised by every in-process run that compares solutions |

### Output / diagnostic features

These are CLI flags. `--light` only checks that they don't change the solution count — the option sweep flips them but doesn't validate the output content.

| Flag | Coverage |
|------|----------|
| `-printsols`, `-noprintsols`, `-printsolsonly`, `-printonlyoptimal` | option sweep (count agreement only) |
| `-tableout`, `-jsontableout`, `-solsout`, `-jsonsolsout` | the tester *uses* these internally as its oracle, so format breakage shows up immediately |
| `-nocheck` / `-check` | option sweep |
| `-map-long-short` (none/keeplong/eager/lazy) | option sweep |
| `-quiet`, `-verbose` | not tested |
| `-instancestats`, `-X-instancestats` | not tested |
| `-dumptree`, `-dumptreejson`, `-dumptreesql` | not tested |
| `-Xgraph` | not tested |
| `-redump`, `-makeresume`, `-noresume` | not tested (some files have `-redump` in comments only) |
| `-outputCompressed`, `-outputCompressedDomains` | not tested |
| `-gap` | not tested |
| `-command-list` | not tested |

### Limit flags

| Flag | Coverage |
|------|----------|
| `-timelimit`, `-cpulimit` | not directly tested (would need a long-running instance) |
| `-nodelimit` | used by `do_valgrind_tests.sh`; not in the default flows |
| `-sollimit` | implicitly used by restart sweep |
| `-skipautoaux` | not tested |

### Experimental / `-X-` flags
| Flag | Coverage |
|------|----------|
| `-X-tabulation` | not tested |
| `-X-AMO`, `-X-AMO-extra` | not tested |
| `-X-prop-node` | not tested |
| `-X-instancestats` | not tested |
| `-X-parallel*` | covered (see parallel modes table) |

## Build configurations

`--light` builds both `bin-quick` (`--quick`, optimised) and `bin-debug` (`--quick --debug`, with internal assertions). Other configurations exist but aren't part of the default flows:
- `bin/` (no flag) — fully optimised.
- `bin-debug/` (`--debug`) — `MINION_DEBUG`, `_GLIBCXX_DEBUG`, checked-integer `DomainInt`.
- `bin-lib/` — library-focused build.
- `bin-pp/`, `bin-ws/` — used in ad-hoc experiments; `tester/big-test-logs/run.sh` defaults to `bin-ws/minion`.
- `--sanitize` — clang address/leak sanitiser.
- `--domains64` — 64-bit `DomainInt`.
- `--constraints <list>` — restrict to a subset of constraints (build-time selection).

The CI matrix builds `bin-quick` and `bin-debug` and runs the full light suite on both. None of the alternative build flags are exercised by either light or heavy by default — if you change `configure.py` itself, build a few of the variants manually and re-run.

## Known gaps to plug

- **In-process backend doesn't expose optimisation.** `--optimisation-sweep` errors out cleanly under `--in-process`; making it work needs `optimiseMinimiseVars`/`optimiseMaximiseVars` bindings in `minion-sys` and a routing path through the existing FFI runner.
- **`-parallel` (fork-based) is not in the optimisation strategy set.** Each forked child writes its own `-jsontableout` and the parent doesn't aggregate cross-process for optimisation. Adding it would need a small post-process step.
- **Resume / dump / Xgraph are completely untested.** A single round-trip test (dump → reload → verify same solutions) per format would suffice.
- **`-X-AMO`, `-X-tabulation`, `-X-prop-node` have no coverage.** These are experimental, so this may be deliberate.
- **Five constraints have no fuzz coverage**: `haggisgac`, `haggisgac-stable`, `shortstr2`, `shortctuplestr2`, `frameupdate`. They have minimal regression coverage. Adding them to `tester/src/constraint_def.rs::CONSTRAINT_LIST` would close this.
- **No sanitiser run is in `--heavy`.** Adding an `--sanitize`-built binary as an extra phase would catch UB and leaks that the debug build misses.
