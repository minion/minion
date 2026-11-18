# The Lean development

This note describes what is defined and proved under `Minion/`. Everything
here is checked by Lean when the project is built. There is no `sorry`, and no
axiom beyond Lean's standard ones.

## Semantics

`Semantics.lean` defines models: finite-domain variables and a tree of
constraints. A term is a variable, a constant, or a view of another term:
Boolean complement, negation, a shift or a scaling. Every registered Minion
constraint has a constructor with an executable meaning, `Constraint.eval`,
a Boolean function of an assignment. Propagator variants with the same meaning
share a constructor. `m.IsSolution a` says that `a` takes each variable's
value from its domain and satisfies every constraint.

Integers are unbounded. Division and modulo round towards negative infinity,
and the variants which define division by zero have that definition written
out.

## Rules for single deductions

`DomainProof.lean` relates domains to assignments, including through views.
`LocalProof.lean` gives the shape every rule has: a conflict test with a proof
that, when it accepts, no assignment in the current domains satisfies the
constraint. From that one proof the library derives the rule for deletions.
To check that a constraint may delete some values of a variable, restrict the
variable to exactly those values and test for a conflict.
`LocalProof.Rule.prune_preservesSolutions` states the result: an accepted
deletion removes no solution of the model.

The conflict tests, by family:

| Module | Constraints | Test |
| --- | --- | --- |
| `SumProof`, `WeightedProof` | sums and weighted sums | The least possible total exceeds the largest right-hand side, or the reverse for `sumgeq`. Weighted sums become sums over scaled views |
| `BoundsProof`, `SupportProof`, `FunctionalProof`, `PowerProof` | comparisons, equality, absolute value, difference, product, division, modulo, power, literals and set membership | Interval reasoning where it suffices, and otherwise a search for a support among the argument domains |
| `ElementProof` | element | No index, entry and result agree |
| `LexProof` | lexicographic orderings | No first differing position is feasible |
| `CountProof`, `GccProof` | occurrence, literal sums, Hamming, vector-exists, GCC counts | Bounds on how many positions must and may satisfy a predicate |
| `ExtremumProof` | min, max | No argument can equal the result with the others on the right side of it |
| `NvalueProof` | number of distinct values | Bounds from forced values and from the union of the domains |
| `AllDiffProof` | all-different | A Hall set: a set of variables with too few values between them |
| `GccCutProof`, `CapacityProof` | GCC | A set of variables confined to full values, or a set of values which cannot reach their required counts |
| `MatrixProof` | all-different matrix | Row and column counts, and Hall sets on the possible columns |
| `TableProof`, `ShortTableProof` | tables, negative tables, short tables | No row is still possible; for negative tables, every possible tuple is forbidden |
| `FrameProof`, `BoundedProof` | frame update, generic negations | Evaluation once enough arguments are fixed, with a bounded split otherwise |

Searches for supports bind each variable before looking at later arguments, so
a variable which occurs twice, or occurs through two views, is handled
correctly. None of the rules assumes distinct variables.

`LogicProof.lean` composes these through `reify`, `reifyimply`, conjunction
and disjunction. Each family provides a test for both the constraint and its
negation; `LogicProof.possible_sound` proves that a constraint which can take
a truth value in the current domains passes the test for that value.

### Witnesses

`InferenceWitness.lean` checks a deduction from supplied evidence: a Hall set
or a GCC cut from Minion, a path to the child of a logical constraint which
acted, or a request to use interval reasoning alone. `check_excludes` proves
each kind sound. The evidence is checked, never trusted: a set which is not a
Hall set in the current domains justifies nothing.

Where Minion supplies no evidence, `Matching.lean` and `FlowWitness.lean` look
for a Hall set or cut. They are not proved correct and need not be, because
whatever they return goes through the same check.

## Search

`SearchProof.lean` defines a search tree: a leaf is an empty domain, a
conflict, or a solution; an internal node is a deduction followed by a
subtree, a split on `x = v` against `x ≠ v`, or a refutation, which removes
part of a domain given a subtree showing that part holds no solution.
`check` runs over the tree from the model's initial domains and accepts if
every step passes its rule. The main theorems are:

```lean
theorem verify_correct (m : Model) (tree : Tree) (reported : List Assignment)
    (hc : verify m tree reported = true) : ∀ a, a ∈ reported ↔ m.IsSolution a

theorem verify_unsat (m : Model) (tree : Tree) (hc : verify m tree [] = true) :
    ∀ a, ¬m.IsSolution a

theorem verify_nodup (m : Model) (tree : Tree) (reported : List Assignment)
    (hc : verify m tree reported = true) : reported.Nodup
```

`refutation_preserves` justifies refutation nodes, which is how SAC and SSAC
probes are checked. `TableProjection.solution_iff` justifies replacing a table
by the rows still possible in the current domains.

The checker which runs is `BlockSearchProof.verify`. It holds the deductions
at a node as a list, keeps the domains in an array, and caches constraint
lookup. `BlockSearchProof.verify_eq` proves it equal to `SearchProof.verify`,
so the theorems above apply to it unchanged. `FastSearchProof.lean` is an
intermediate step in that proof.

Three further theorems reuse the search theorem on an extended model:

| Theorem | Statement |
| --- | --- |
| `OptimalProof.check_optimal` | The best solution satisfies the model, and no solution has a better objective value |
| `PartialProof.check_correct` | For a search which stopped early, the reported solutions are exactly the model's solutions which satisfy the constraints added for the unexplored branches |
| `ProjectionProof.check_correct` | With auxiliary variables, every reported solution is a solution, and every solution agrees with a reported one on the other variables |

[ANNOTATIONS.md](ANNOTATIONS.md) describes how the reader builds the extended
model in each case.

## Enumeration

`Check.lean` defines the independent solution-set checker, which evaluates the
model on every assignment. `verifySolutions_correct` proves that it accepts a
claimed list only if that list is exactly the set of solutions.

## Code which is not proved

These modules produce data for the proved checker and are not trusted:

- `Json.lean`, `JsonTables.lean`: decoding of models.
- `BinaryTrace.lean`: reading a trace and building the proof.
- `BoundsEvidence.lean`: choosing where interval reasoning suffices.
- `Trim.lean`: removing deductions the result does not need.
- `TraceCertificate.lean`: writing a certificate as a Lean file.

A mistake in any of them can cause a rejection, or a certificate for a model
other than the one intended, since the decoded model is taken as given. It
cannot make the checker accept a proof which does not hold for the model it
was given.
