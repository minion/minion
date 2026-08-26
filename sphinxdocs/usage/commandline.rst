Command Line Options
====================

Minion supports a number of switches to augment default behaviour, these
are listed here. We begin with the most useful, and then list other,
less common flags.

Choosing how much search to perform (time, number of solutions)
---------------------------------------------------------------

-nodelimit <N>
~~~~~~~~~~~~~~~~~~~~~~

To stop search after N nodes:


-sollimit <N>
~~~~~~~~~~~~~~~~~~~~~

To stop search after N solutions have been found:


-timelimit <N>
~~~~~~~~~~~~~~~~~~~~~~

To stop search after N seconds (real time):


-cpulimit <N>
~~~~~~~~~~~~~~~~~~~~~

To stop search after N seconds (CPU time):


-findallsols
~~~~~~~~~~~~~~~~~~~~~~~~

Find all solutions and count them. This option is ignored if the problem
contains any minimising or maximising objective.

Control search
---------------------------------------------------------------

-parallel
~~~~~~~~~~~~~~~~~~~~~

Make Minion run in parallel. When using this option, you should use `-solsout` to put the solutions in a file, as solutions may get mixed up on the terminal.

-cores <N>
~~~~~~~~~~~~~~~~~~~~~~
Number of cores to use when running in parallel

-steallow
~~~~~~~~~~~~~~~~~~~~

When doing parallel search, "steal low" in the tree (that is, start new threads from low pieces of the tree) instead of "steal high" (default, take high bits of the tree). Is usually slower.

-varorder <order>
~~~~~~~~~~~~~~~~~~~~~

Enable a particular variable ordering for the search process.

The available orders are:

-  sdf - smallest domain first, break ties lexicographically
-  sdf-random - sdf, but break ties randomly
-  srf - smallest ratio first, chooses unassigned variable with smallest
   percentage of its initial values remaining, break ties
   lexicographically
-  srf-random - srf, but break ties randomly
-  ldf - largest domain first, break ties lexicographically
-  ldf-random - ldf, but break ties randomly
-  random - random variable ordering
-  conflict - conflict-directed variable ordering
-  static - lexicographical ordering
-  wdeg - Weighted degree
-  domoverwdeg - Domain size over weighted degree

-valorder <order>
~~~~~~~~~~~~~~~~~~~~~

Choose the value ordering, overriding any ``VALORDER`` in the input file. It
applies to every search variable.

The available orders are:

-  ascend - try the smallest value in the domain first
-  descend - try the largest value in the domain first
-  random - try the values in a random order

Any other value is an error. If neither this flag nor a ``VALORDER`` is given,
values are tried in ascending order.

-restarts
~~~~~~~~~

Enable restart-based search. Minion abandons the current search periodically
and starts again from the root, with a fresh random value ordering each time.

Two things survive a restart. The weights accumulated by the ``WDEG`` and
``DOMOVERWDEG`` orderings are held on the constraints, which are not rebuilt,
so they keep accumulating across restarts. For an optimisation problem the
best objective value found so far is also kept, so each restart searches under
the bound established by the previous ones.

-restarts-multiplier <N>
~~~~~~~~~~~~~~~~~~~~~~~~

Set the restart multiplier. The schedule is geometric in the number of
*backtracks*: the limit starts at 10, and is multiplied by this value before
each attempt. With the default multiplier of 1.5 the first attempt is
therefore allowed 15 backtracks, the second 22, and so on.

-no-restarts-bias
~~~~~~~~~~~~~~~~~

Turn off the value-choice bias applied at each restart.

Restarts use a random value ordering. By default each restart also draws one
bias in the range -100 to 99 and applies it to every variable for that
attempt: a positive bias b takes the largest value in the domain with
probability b/100, and a negative bias takes the smallest with probability
-b/100, falling back to a random choice otherwise. The effect is that one
restart leans towards the top of the domains, the next towards the bottom.

This flag makes every restart use an unbiased random value ordering instead.
It affects only the value choice, not the restart schedule.

-prop-node <proplevel>
~~~~~~~~~~~~~~~~~~~~~~

Allows the user to choose the level of consistency to be enforced during each node of search.

See -preprocess for the allowed values for `<proplevel>`

-preprocess <proplevel>
~~~~~~~~~~~~~~~~~~~~~~~

This switch allows the user to choose what level of preprocess is
applied to their model before search commences. The default is ``None`` --
no preprocessing beyond the initial propagation. (``GAC`` is the default for
``-prop-node``, which is a different setting.)

The choices are:

-  None
      - Default setting: no preprocessing is done

-  GAC
      - All propagators are run to a fixed point
      - Incorrectly named (but kept for historical reasons), because some propagators do not achieve GAC.
      - Used as the basis for all other, better, levels below.

-  SACBounds
      - singleton arc consistency on the upper and lower bounds of each variable
      - GAC is run on every assignment to the upper and lower bound of each variable

-  SAC
      -  singleton arc consistency
      - GAC is run on every assignment to every variable

-  SSACBounds
      -  singleton singleton bounds arc consistency
      - SAC is run on every assignment to the upper and lower bound of each variable

-  SSAC
      - singleton singleton arc consistency
      - SAC is run on every assignment to every variable

These are listed in order of roughly how long they take to achieve.
Preprocessing is a one off cost at the start of search. The success of
higher levels of preprocessing is problem specific; SAC preprocesses may
take a long time to complete, but may reduce search time enough to
justify the cost.

Each of the SAC variants can have '_limit' added (for example
``SACBounds_limit``). The '_limit' variants of these algorithm add checks
which limit the algorithms if they are taking a very long time and not making progress.

-randomseed
~~~~~~~~~~~~~~~~~~~~~~~

Set the pseudorandom seed to N. This allows 'random' behaviour to be
repeated in different runs of minion.


Control output
---------------------------------------------------------------

-quiet
~~~~~~~~~~~~~~~~~~

Do not print parser progress (default)

-verbose
~~~~~~~~~~~~~~~~~~~~

Print parser progress


-printsols
~~~~~~~~~~~~~~~~~~~~~~

Print solutions (default).

-noprintsols
~~~~~~~~~~~~~~~~~~~~~~~~

Do not print solutions.

-printsolsonly
~~~~~~~~~~~~~~~~~~~~~~~~~~

Print only solutions and a summary at the end.

-printonlyoptimal
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In optimisation problems, only print the optimal value, and not
intermediate values.


-tableout <filename>
~~~~~~~~~~~~~~~~~~~~~

Append a line of data about the current run of minion to a named file.
This data includes minion version information, arguments to the
executable, build and solve time statistics, etc. See the file itself
for a precise schema of the supplied information.

-jsontableout <filename>
~~~~~~~~~~~~~~~~~~~~~~~~

Append a line of data about the current run as JSON to a named file.
Contains the same information as ``-tableout``, but formatted as a JSON
object.

-solsout <filename>
~~~~~~~~~~~~~~~~~~~~

Append all solutions to a named file. Each solution is placed on a line,
with no extra formatting.


Less common flags
-----------------

-outputCompressedDomains
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Try to reduce the initial domains of variables, and output them. This is
in general not useful for users, but is provided as a pre-preprocessing
step for other systems.

-outputCompressed <newname>
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Output a Minion instance with some basic reasoning performed to reduce
the size of the file, such as reducing domain size. This file should produce identical output the
original instance but may solve faster.

-redump
~~~~~~~~~~~~~~~~~~~

Print the minion input instance file to standard out. No search is
carried out when this switch is used. This can be used to update files
in old versions of the Minion file format.

-instancestats
~~~~~~~~~~~~~~~~~~~~~~~~~~

Output various statistics about the minion file, which can be used for machine learning.

-map-long-short
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Automatically generate a short tuple list from each long tuple list.

The methods of compression are:

-  none : No short tuple list generated (default)
- eager : Use a fast algorithm to produce a reasonable short tuple list (best as first choice)
- lazy : Work harder (possibly exponentially) to produce a shorter short tuple list
- keeplong : Make a 'short tuple list' with no short tuples (only useful for benchmarking)

-nocheck
~~~~~~~~~~~~~~~~~~~~

Do not check solutions for correctness before printing them out.

-check
~~~~~~~~~~~~~~~~~~

Check solutions for correctness before printing them out. This should
only make a difference if Minion contains a bug.

-dumptree
~~~~~~~~~~~~~~~~~~~~~

Print out the branching decisions and variable states at each node.

-dumptreejson <filename>
~~~~~~~~~~~~~~~~~~~~~~~~~

Print out the branching decisions and variable states at each node as a JSON file to a filename.


-skipautoaux
~~~~~~~~~~~~~~~~~~~~~~~~

By default Minion adds all variables to the varorder, to ensure that all
variables are branched assigned before a solution is outputted. This
option disables that behaviour. This means minion Minion *may output
solutions incorrectly*, or *incorrect numbers of solutions*. This flag is
provided because some users require this low-level control over the
search, but is in general useless and dangerous. In particular, it will
not speed up search (except when the speed up is due to producing
garbage of course!)

-randomiseorder
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Randomises the ordering of the decision variables, and the value
ordering. If the input file specifies as ordering it will randomly
permute this. If no ordering is specified a random permutation of all
the variables is used.

-jsonsolsout <filename>
~~~~~~~~~~~~~~~~~~~~~~~~

Append all solutions to a named file, as JSON objects. Each solution is
store as a separate JSON object.


-makeresume
~~~~~~~~~~~~~~~~~~~~~~~

Write a resume file on timeout or being killed.

-noresume
~~~~~~~~~~~~~~~~~~~~~

Do not write a resume file on timeout or being killed. (default)


-command-list <infile> <outfile>
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Read a list of commands from ``infile`` and write the results to ``outfile``.
This allows batch processing of multiple Minion commands without restarting
the solver.

-split
~~~~~~~~~~~~~~~~~~

When Minion is terminated before the end of search, write out two new
input files that split the remaining search space in half. Each of the
files will have all the variables and constraints of the original file
plus constraints that rule out the search already done. In addition, the
domain of the variable under consideration when Minion was stopped is
split in half with each of the new input files considering a different
half.

This feature is experimental and intended to facilitate parallelisation
--to parallelise the solving of a single constraint problem, stop and
split repeatedly. Please note that large-scale testing of this feature
was limited to Linux systems and it might not work on others (especially
Windows).

The name of the new input files is composed of the name of the original
instance, the string 'resume', a timestamp, the process ID of Minion,
the name of the variable whose domain is being split and 0 or 1. Each of
the new input files has a comment identifying the name of the input file
which it was split from. Similarly, Minion's output identifies the new
input files it writes when splitting.

The new input files can be run without any special flags.

This flag is intended to be used with the -timelimit, -sollimit,
-nodelimit or -cpulimit flags. Please note that changing other flags
between runs (such as -varorder) may have unintended consequences.

Implies -makeresume.

-split-stderr
~~~~~~~~~~~~~~~~~~~~~~~~~

The flag -split-stderr has the same function as the flag -split, however
the two new Minion input files are sent to standard error rather than
written to files.

See documentation for -split.

Experimental flags
------------------

Flags whose name begins with ``-X-`` are experimental. They are documented
here so that they are not invisible, but their names, their arguments and
their behaviour may change without notice, and they should not be relied on
in production.

-X-parallelThreads <N>
~~~~~~~~~~~~~~~~~~~~~~

Runs portfolio search across N OS threads. Each thread searches the same
problem with a different random seed; the first thread to find the requested
number of solutions (or a contradiction) terminates the others. Works on
Windows and inside a library host process.

Known limitations: with ``-findallsols`` each worker independently enumerates
the search space, so the reported solution count is up to N times the true
count -- there is no cross-thread deduplication. For deterministic
first-solution tests, the recorded solution depends on which thread wins the
race. This is useful primarily for SAT-style ``-sollimit 1`` workloads.

Mutually exclusive with ``-parallel``, ``-X-parallelPreprocess``, ``-split``
and ``-makeresume``. Mid-search mutation (``minion_newVarMidsearch`` /
``minion_addConstraintMidsearch``) is not supported in this mode.

-X-parallelWorkSteal <N>
~~~~~~~~~~~~~~~~~~~~~~~~

Runs work-stealing parallel search across N OS threads. One thread starts at
the root; idle threads block on a shared work queue. When some thread is idle,
busy threads periodically donate one stealable left branch -- the topmost
unstolen left, mirroring the fork-based ``-parallel`` -- by encoding a
path-from-root and pushing it to the queue. Idle threads pop a path,
fast-forward by replaying it, and continue search from the resulting subtree.

Weighted-degree counters drift between threads, as each thread accumulates its
own failure history; this is intended as a source of diversity. The mode helps
when proving unsatisfiability by splitting the tree across cores, where
wall-clock time scales roughly as 1/N, modulo replay overhead.

Mutually exclusive with ``-parallel``, ``-X-parallelThreads``,
``-X-parallelPreprocess``, ``-split``, ``-makeresume`` and ``-restarts``.

-X-parallelWorkStealPortfolio
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Augments ``-X-parallelWorkSteal`` by giving each worker a different
combination of variable ordering, value ordering and randomisation. Worker 0
uses the heuristic you chose, or Minion's default; the remaining workers cycle
through a palette including ``SDF``, ``DOMOVERWDEG``, ``WDEG``, ``STATIC``,
``SRF``, ``CONFLICT``, ``LDF`` and randomised variants.

The path-replay machinery is heuristic-independent, because branches are
recorded as concrete variable-value assignments rather than as heuristic
choices. Workers running different strategies therefore cooperate on one
shared search tree: a donor's prefix is replayed verbatim by any receiver, and
both then explore their own subtrees with their own heuristics. This is best
suited to satisfiable instances where one strategy dramatically outperforms
the others, so that the right strategy wins the race while the rest contribute
spare work-stealing cycles.

Requires ``-X-parallelWorkSteal N`` with N at least 2 to have any effect.

-X-parallelPreprocess [N]
~~~~~~~~~~~~~~~~~~~~~~~~~

Runs ``SAC`` / ``SACBounds`` preprocessing in parallel using N worker
processes, or all online cores if N is omitted. Workers fork from the parent
and each runs the standard SAC algorithm on its slice of the variables, with
prunings collected into shared memory and merged at the end of each round. The
final fixpoint is identical to sequential SAC; only the number of rounds and
the wall-clock time differ.

Unix only. Has no effect on ``SSAC``, ``SSACBounds`` or ``GAC``
preprocessing.

-X-AMO
~~~~~~

Detects at-most-one constraints in the instance and gathers them, so that
propagation can exploit the fact that at most one of a set of Boolean
variables is true.

-X-AMO-extra
~~~~~~~~~~~~

As ``-X-AMO``, but searches a wider range of constraint types for
at-most-one structure.

-X-tabulation
~~~~~~~~~~~~~

Replaces small constraints with equivalent table constraints, which are then
propagated by Minion's table propagators.


-X-instancestats
~~~~~~~~~~~~~~~~

An alias for ``-instancestats``. See the documentation for that flag.

-X-prop-node <proplevel>
~~~~~~~~~~~~~~~~~~~~~~~~

An alias for ``-prop-node``. See the documentation for that flag.

-solsout0 <filename>
~~~~~~~~~~~~~~~~~~~~

An alias for ``-solsout``. See the documentation for that flag.

-tableout0 <filename>
~~~~~~~~~~~~~~~~~~~~~

An alias for ``-tableout``. See the documentation for that flag.
