# The annotation trace

A Minion configured with `--annotations` accepts `-annotations FILE` and
writes a record of the search to `FILE`. This note describes what is recorded,
the file format, and how `minion-certify` reads each kind of search.
[README.md](README.md) covers building and running.

The format is experimental and may change between releases.

## What is recorded

The trace is a sequence of events. Their order is the order in which Minion
did things.

| Event | Fields | Meaning |
| --- | --- | --- |
| `model` | `format`, `model`, optional `auxiliary`, `optimise` | The variables and the parsed constraint tree, written once |
| `constraint` | `id`, `name`, `source_path`, `variables` | A constraint object Minion built, and the input constraint it came from |
| `child` | `parent`, `child`, `position` | Links a constraint to a child, such as the constraint inside a reification |
| `context_open` | `constraint`, `rule`, sometimes `info` | A constraint starts some reasoning |
| `context_close` | | That reasoning ends |
| `witness` | `constraint`, `kind`, `positions` or `values` | The structure behind the deductions which follow |
| `prune` | `variable`, `operation`, `value`, `rule` | A domain deletion |
| `conflict` | | The constraint in the current context found a contradiction |
| `push`, `pop` | | Minion saved or restored its state |
| `probe` | `id`, `variable`, `lower`, `upper` | SAC or SSAC assumes the variable lies in this interval |
| `probe_result` | `id`, `failed` | Whether that assumption led to a contradiction |
| `branch` | `variable`, `value`, `equal`, `split` | A search decision: the left branch assigns, the right branch removes |
| `branch_close` | `split` | Both sides of a decision are finished |
| `branch_skip` | `split` | The right side of a decision on an auxiliary variable is abandoned |
| `solution` | `values` | A solution, one value for each variable |
| `backtrack` | | Search moves on after a solution; not a contradiction |
| `end` | `complete` | Whether search ran to completion |

Variables are numbered by their position in the model. A deletion is one of
`assign`, `remove`, `lower_bound` or `upper_bound`. It is written in terms of
the declared variable: a propagator acting through a view, such as a negation,
has its operation translated first. An operation which changes nothing is not
written.

Every deletion and conflict lies inside a context, and so belongs to a
constraint. Contexts nest: when a reification propagates its child, the
child's context lies inside the reification's. A context which contains no
deletion or conflict is not written at all, unless
`-annotations-all-contexts` is given.

Tables appear once, in the model. Deductions refer to constraints by number.

### Witnesses

Some propagators reason from a structure which the checker would otherwise
have to rediscover. They record it:

| Kind | Content | Supplied by |
| --- | --- | --- |
| `hall` | Positions of a set of variables whose domains together hold no more values than the set has variables | all-different |
| `gcc_upper` | Positions of variables which can only take values that are already full | GCC |
| `gcc_lower` | Values at or below their required count which no further variable can take | GCC |

A witness is not trusted. The checker confirms that it is the cut it claims to
be in the current domains, and that the deduction follows from it. Witnesses
are written only for all-different and GCC constraints at the top level of the
model, and not for a GCC whose capacity variable also occurs elsewhere in the
constraint. In the other cases the checker finds a cut itself.

## File format

A file starts with the bytes `MINIONTR` and a version byte, currently 1. It is
then a sequence of records, each starting with a tag byte. Unsigned integers
are LEB128 (seven bits per byte, low bits first); signed integers are zigzag
mapped to unsigned first. Text is a length followed by UTF-8 bytes.

Every record except a string definition is one event. Events are numbered from
1 in file order. The identifiers of contexts, saved states, decisions and
witnesses are the numbers of the events which introduced them, so they are not
stored: a context's parent is the innermost open context, a deletion or
conflict belongs to the innermost open context, and a `pop` restores the most
recent `push`. A reference to an earlier decision or witness is stored as the
distance back to it.

| Tag | Event | Fields after the tag |
| ---: | --- | --- |
| 0 | any other event | text: its fields as JSON |
| 1 | (defines the next string, numbered from 0) | text |
| 2 | `context_open` | constraint, rule string |
| 3 | `context_open` with `info` | constraint, rule string, text |
| 4 | `context_close` | |
| 5–8 | `prune`: remove, assign, lower bound, upper bound | variable, value, site string |
| 9–12 | `prune` with a witness, same four operations | variable, value, site string, distance to witness |
| 13, 14 | `push`, `pop` | |
| 15 | `branch`, equal | variable, value |
| 16 | `branch`, not equal | variable, value, distance to its decision |
| 17 | `branch_close` | distance to its decision |
| 18 | `conflict` | |
| 19 | `conflict` with a witness | distance to witness |
| 20 | `backtrack` | |
| 21 | `solution` | count, then that many values |
| 22 | `branch_skip` | distance to its decision |
| 23 | `witness` | constraint, kind string, 0 for positions or 1 for values, count, items |

The model, constraint declarations, probes and the end marker use tag 0. A
deletion's site string names the source file and line of the call which made
it.

`minion-certify --to-json OUT.jsonl FILE` writes the events as JSON lines.
`tests/binary_trace.py` encodes such a rendering again; the tests use it to
corrupt traces.

## How a trace is read

`minion-certify` turns the records into a proof: a tree whose internal nodes
are search decisions and whose leaves are conflicts and solutions, with the
deductions made at each node in between. The proved checker then accepts or
rejects it. The reader is not trusted; a mistake in it can only cause a
rejection.

- A deletion is attributed to the top-level constraint enclosing its context.
  When that constraint is a reification, conjunction or disjunction, the path
  to the child which acted is kept as a hint for the checker.
- A right branch must be the negation of its left branch.
- A failed probe becomes a refutation: a subproof that the assumed interval
  contains no solution. The deletion which follows must remove exactly what
  the probe assumed. Successful probes are discarded.
- Tables of 512 rows or more are narrowed to the rows still possible at each
  search node, so that later checks scan fewer rows.
- The rule names and site strings are not checked. A deduction is confirmed
  against its constraint, not against the reason Minion gave for it.
- `push`, `pop` and `backtrack` are not needed and are read past.

### Stopping at a solution

A search without `-findallsols`, or with `-sollimit N`, stops once it has its
solutions and leaves the rest of the tree unexplored. Its trace ends with
`"complete": false` straight after a solution, with decisions still open.

Each open decision is one where the search went left and never came back for
the right side. For each, the reader adds a constraint to the model, "below
this point in the search, the variable has this value", and closes the
unexplored side as a conflict with it. What was searched is then a complete
search of the extended model. By `PartialProof.check_correct`, the solutions
found are exactly the model's solutions which satisfy the added constraints.
Each is therefore a solution, every deduction on the way was justified, and
every branch abandoned before a solution was correctly refuted. Nothing is
claimed about the unexplored part.

A search which stops without a solution has nothing to certify and is
rejected.

### Auxiliary variables

Variables in a `VARORDER AUX` block, and variables named in no `VARORDER`, are
auxiliary: once the other variables are fixed, Minion finds one assignment to
them and does not look for more. The model event lists them in `auxiliary`.
When a solution is found, each open decision on an auxiliary variable is
abandoned, and the trace records a `branch_skip` where a right branch would
otherwise follow.

For each skip the reader adds a constraint to the model and closes the skipped
side as a conflict with it. The constraint says: either the search is not at
this point, or the decision holds, or some non-auxiliary variable differs from
the solution just found. The skipped side can only conflict with it where
every non-auxiliary variable is already fixed to that solution's value, so a
skip made too early is rejected. `ProjectionProof.check_correct` then gives
the result: every reported solution is a solution, and every solution of the
model agrees with a reported one on the non-auxiliary variables.

### Optimisation

A run with a single `MINIMISING` or `MAXIMISING` variable is traced like any
other search. The model event gains an `optimise` field giving the direction
and the objective variable. The bound Minion applies after each solution is
recorded in a context whose rule is `optimise.bound`, with no constraint.

Such a trace is read as a refutation. Once the last, best solution is known,
the reader adds to the model the constraint "the objective is strictly better
than that solution's value". Each recorded bound is a step justified by that
constraint, and each solution found on the way is a leaf which violates it.
The search theorem then says the extended model has no solution, and
`OptimalProof.check_optimal` turns that, with a check that the best solution
satisfies the original model, into optimality. The solutions reported on the
way must each satisfy the model and strictly improve.
