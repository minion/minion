# Lean-checked certificates for Minion

This directory holds a checker, written and proved correct in Lean 4, for the
traces Minion writes with `-annotations`. A trace records every branch, every
domain deletion with the constraint which made it, every conflict and every
solution. `minion-certify` checks each recorded deduction against a proved
rule for its constraint, and then the search tree built from them.

The aim is to check the solver's reasoning step by step, not only its answer.
A run is accepted only if every deletion Minion made was one its constraint
was entitled to make.

This is experimental. The trace format may change between Minion releases.

## Quick start

From the repository root, with a C++ compiler, Python and
[elan](https://github.com/leanprover/elan) installed:

```sh
mkdir -p bin-annotations
(cd bin-annotations && python3 ../configure.py --annotations --quick && make -j4)
(cd lean && lake build)

bin-annotations/minion -findallsols -noprintsols \
  -annotations /tmp/run.trace lean/examples/reified-element.minion
lean/.lake/build/bin/minion-certify /tmp/run.trace
```

Tracing must be requested when Minion is configured, because the hooks slow
search even when no trace is written. A Minion built without `--annotations`
rejects the `-annotations` option.

`minion-certify` prints one line for each trace:

| Line starts | Meaning |
| --- | --- |
| `CERTIFIED` | A complete search: the reported solutions are exactly the model's solutions |
| `OPTIMAL` | An optimisation search: the last solution is optimal |
| `SOLUTION` | A search which stopped at a solution: everything up to that point is justified |
| `PROJECTED` | A search with auxiliary variables: one solution for each assignment of the other variables which extends to a solution |
| `REJECT` | Some step does not hold; the message names the first one |

Options:

- `--audit-solutions N` also enumerates the model, when its domains have at
  most `N` assignments in total, and compares the result.
- `--export-lean FILE.lean` writes the certificate as a Lean file whose
  theorem is checked by Lean's kernel. This is practical for small searches
  only. `examples/Certified*.lean` are certificates of this kind.
- `--to-json OUT.jsonl TRACE` renders a trace as JSON lines for inspection.
- `--no-matching-discovery` rejects all-different and GCC deductions which
  arrive without a witness from Minion.
- `--trim` first removes deductions the final result does not need. The
  result is a smaller proof of the same answer; it no longer checks every step
  Minion took.

## What is proved, and what is trusted

[PROOFS.md](PROOFS.md) describes the Lean development. In outline:

- `Minion/Semantics.lean` gives every registered constraint a meaning: a
  Boolean function of an assignment.
- For each constraint family there is an executable check that a deletion or
  conflict is justified in the current domains, with a proof that an accepted
  check excludes no solution.
- `SearchProof.verify_correct` proves that an accepted search tree reports
  exactly the solutions of the model. The checker which runs day to day is a
  faster one, proved equal to it.

The proofs use no `sorry` and no axioms beyond Lean's standard ones.

What a `CERTIFIED` line does not establish:

- **The model is read, not verified.** Minion writes its parsed model into
  the trace. Nothing checks that this matches the `.minion` file.
- **The compiled checker trusts Lean's compiler.** `--export-lean` avoids
  this for a particular run, at much greater cost.
- **Only this run is checked.** A propagator which was right on every step of
  one search can be wrong on another. This is not a verification of Minion's
  C++ code.
- **Nothing is said about strength.** Deletions are checked to be sound, not
  that a propagator achieved GAC or any other level of consistency.
- **The checker's rules can be weaker than a propagator.** A correct deduction
  the rules cannot justify is rejected. Witnesses from Minion close this gap
  for all-different and GCC.

## Scope

Tracing supports sequential search, including SAC and SSAC preprocessing, a
single optimisation variable, and auxiliary variables. Minion refuses to trace
parallel search, restarts, tabulation, command search and several objective
variables. Optimisation together with auxiliary variables is not certified.
A search which stops without a solution, such as one which timed out, is
rejected.

The checker holds the whole trace in memory, which limits it to traces of a
few hundred megabytes.

[CONSTRAINTS.md](CONSTRAINTS.md) lists every registered constraint name with
its Lean semantics and proof family, and [COVERAGE.md](COVERAGE.md) the rule
used for each family. [ANNOTATIONS.md](ANNOTATIONS.md) describes the trace
format and how each kind of search is read.

## Tests

```sh
(cd lean && for f in *Tests.lean examples/Certified*.lean; do lake env lean "$f" || break; done)
MINION_ANNOTATIONS_BINARY="$PWD/bin-annotations/minion" \
  python3 -m unittest discover -s lean/tests -p 'test_*.py'
python3 lean/tests/constraint_inventory.py --check
```

The `*Tests.lean` files are small examples checked by Lean itself. The Python
tests run Minion on models for each constraint family, certify the traces, and
check that corrupted traces are rejected: an unjustified deletion, a fabricated
conflict, a missing solution, a wrong witness. The inventory check compares
Minion's constraint registry with the Lean decoder and the proof-family map.

`tests/check_corpus.py` traces and certifies a directory of random models
written by the tester, and compares the solutions with the tester's.
`tests/kernel_corpus.py` does the same through `--export-lean`.

## Checking solution sets by enumeration

`minion-semantics` is a second, independent checker. It takes a model and a
claimed complete list of solutions, enumerates every assignment, and accepts
only if the claim is exactly the set of solutions.
`Check.verifySolutions_correct` proves this. It does not look at how the
solutions were found, and is limited to small models.

The Rust tester writes such claims:

```sh
cargo build --manifest-path tester/Cargo.toml
tester/target/debug/tester --minion "$PWD/bin/minion" \
  --export-solutions /tmp/minion-semantics \
  --check-solutions-with "$PWD/lean/.lake/build/bin/minion-semantics" \
  --count 10 --nest-depth 2
```

At each nesting depth from zero to `--nest-depth` it generates `--count`
models for each constraint, keeping those whose domains have at most
`--semantics-max-assignments` assignments (10,000 by default). Each claim is a
JSON file with a matching `.minion` file.

### Claim format: `minion-solutions-v1`

The top-level object contains `format`, `variables`, `constraints` and
`solutions`. Each variable is `{ "name": "x", "domain": [0, 1] }`; its
position in `variables` is its index and its column in every solution.

A term is one of:

- `{ "kind": "var", "index": 0 }`
- `{ "kind": "constant", "value": -3 }`
- `{ "kind": "not_bool", "index": 0 }`
- `{ "kind": "neg", "term": T }`
- `{ "kind": "shift", "term": T, "offset": 1 }`
- `{ "kind": "scale", "term": T, "factor": -2 }`

A constraint has `name`, `args` (a list of lists of terms) and `children`
(nested constraints). Tables also have `tuples`, or `short_tuples` holding
arrays of `[position, value]` pairs. `examples/element.json` is a complete
hand-written case.
