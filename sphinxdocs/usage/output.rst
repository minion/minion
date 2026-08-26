Output
======

Minion writes two different kinds of output, and you should know which is
which before you build anything on top of them.

.. warning::

   **Do not parse Minion's standard output.** The text Minion prints to the
   terminal is meant to be read by a person. Its wording, its ordering and the
   set of lines it contains may change in any release, without notice and
   without being called out in the changelog.

   If you want to consume Minion's results from another program, use the JSON
   files described below. Those have a stability promise.

Standard output
---------------

A normal run prints a header of comment lines beginning with ``#``, then
timings, then any solutions, then a summary::

   # Minion Version 2
   # Git version: "717f3cfb (2026-08-25 14:58:38 +0100)"
   #  Run at: UTC Tue Aug 25 21:39:47 2026
   # Input filename: myproblem.minion
   # Command line: minion -findallsols myproblem.minion
   Using seed: 3634443012
   Parsing Time: 0.000151
   Setup Time: 0.000035
   Initial Propagate: 0.000002
   Preprocess Time: 0.000004
   Sol: 0 2 0 3

   Solution Number: 1
   Time:0.000020
   Nodes: 21

   Solve Time: 0.000028
   Total Time: 0.000220
   Total System Time: 0.000285
   Total Wall Time: 0.000705
   Maximum RSS (kB): 3136
   Total Nodes: 21
   Solutions Found: 1

``-printsolsonly`` reduces this to the ``Sol:`` lines alone, and ``-quiet``
suppresses the parser's progress messages.

Every whole-run figure shown above is also in the JSON statistics file, so
there is no need to scrape the terminal for those.

The one exception is the per-solution block -- ``Solution Number``, ``Time:``
and ``Nodes:``, printed after each solution as it is found. These record the
cumulative time and node count at the moment each solution was reached, and at
present they have no JSON equivalent: ``-jsonsolsout`` records the solutions
themselves but not the cost of reaching each one.

Solutions as JSON -- ``-jsonsolsout <file>``
--------------------------------------------

Writes every solution found to a file, **one solution per line**. Each line is
a complete JSON document; the file as a whole is not valid JSON, so read it a line at a
time rather than handing the whole file to a JSON parser.

Each line is a list of rows, and each row is a list of single-entry
``{"name": value}`` objects. The rows are exactly the rows of the ``PRINT``
specification in the input file, so for::

   PRINT [[a[0],a[1]],[c]]

each solution is written as::

   [[{"a_0__":0}, {"a_1__":0}], [{"c":0}]]
   [[{"a_0__":0}, {"a_1__":0}], [{"c":1}]]

With the default ``PRINT ALL``, there is one row per declared variable or
matrix. Names are the flattened forms described under Vectors in the input
format, so ``a[0]`` is written ``a_0__``.

.. warning::

   The file is opened in **append** mode, so a second run adds its solutions
   to the end of an existing file rather than replacing them. Delete the file,
   or use a fresh name, if you want only the current run's solutions.

``-solsout <file>`` writes the same solutions as plain text, one solution per
line, as space-separated values with the rows flattened together and no
variable names.

Statistics as JSON -- ``-jsontableout <file>``
-----------------------------------------------

Writes one JSON object describing the run::

   {"CommandLineArguments" : "-findallsols,-jsontableout,out.json,myproblem.minion" ,
    "Filename" : "myproblem.minion" ,
    "GitVersion" : "717f3cfb (2026-08-25 14:58:38 +0100)" ,
    "MinionVersion" : "Minion Version 2" ,
    "Nodes" : "21" ,
    "SolutionsFound" : "1" ,
    "Satisfiable" : "1" ,
    "TotalTime" : "0.000789" }

.. note::

   **Every value is a JSON string**, including the numeric ones: ``Nodes`` is
   ``"21"``, not ``21``. Convert on the way in.

The keys are:

.. list-table::
   :header-rows: 1
   :widths: 28 72

   * - Key
     - Meaning
   * - ``CommandLineArguments``
     - The arguments Minion was given, joined with commas.
   * - ``Filename``
     - The instance file that was solved.
   * - ``MinionVersion``
     - The Minion version string.
   * - ``GitVersion``
     - The git revision and commit date Minion was built from.
   * - ``RandomSeed``
     - The seed used. Pass it back with ``-randomseed`` to reproduce a run.
   * - ``Preprocess``
     - The preprocessing level applied, or ``none``.
   * - ``ParsingTime``
     - Seconds spent reading the instance.
   * - ``SetupTime``
     - Seconds spent building variables and constraints.
   * - ``InitialPropagate``
     - Seconds spent on the first propagation to a fixpoint.
   * - ``PreprocessTime``
     - Seconds spent in preprocessing.
   * - ``SolveTime``
     - Seconds spent in search.
   * - ``TotalTime``
     - Total CPU time, in seconds.
   * - ``TotalSystemTime``
     - Total system CPU time, in seconds.
   * - ``TotalWallTime``
     - Elapsed real time, in seconds.
   * - ``MaxRSSkB``
     - Peak resident set size, in kilobytes.
   * - ``Nodes``
     - Search nodes explored.
   * - ``SolutionsFound``
     - Number of solutions found.
   * - ``Satisfiable``
     - ``1`` if at least one solution was found, ``0`` otherwise.
   * - ``TimeOut``
     - ``1`` if a time or node limit stopped the search, ``0`` otherwise.
   * - ``OptimumValue``
     - For a single-objective problem with at least one solution, the best
       objective value found. Absent otherwise.
   * - ``OptimumDirection``
     - ``min`` or ``max``. Present exactly when ``OptimumValue`` is.
   * - ``ParallelPreprocessRounds``
     - Rounds performed by ``-X-parallelPreprocess``; ``0`` when unused.
   * - ``ParallelPreprocessPrunings``
     - Values pruned by ``-X-parallelPreprocess``; ``0`` when unused.
   * - ``WorkStealDonations``
     - Work-stealing diagnostic: sub-trees donated by busy workers.
   * - ``WorkStealItemsTaken``
     - Work-stealing diagnostic: sub-trees taken off the queue by idle workers.
   * - ``WorkStealReplayFailures``
     - Work-stealing diagnostic: donated paths that failed to replay.
   * - ``WorkStealQueueLockWaitNs``
     - Work-stealing diagnostic: nanoseconds waiting on the queue lock,
       summed over workers.
   * - ``WorkStealIdleWaitNs``
     - Work-stealing diagnostic: nanoseconds spent idle waiting for a donation,
       summed over workers. Divided by (workers x ``TotalWallTime``) this gives
       the fraction of available CPU spent waiting; if it is high, donations
       are not keeping up.
   * - ``WorkStealCallbackLockWaitNs``
     - Work-stealing diagnostic: nanoseconds waiting on the callback lock,
       summed over workers.

The six ``WorkSteal`` keys are present only when ``-X-parallelWorkSteal`` is
used. They are diagnostics for an experimental mode, and unlike the rest of
this table they may change or disappear along with it.

What is promised
~~~~~~~~~~~~~~~~

**New keys may be added at any time.** Treat the object as open: read the keys
you care about and ignore the rest. A consumer that fails when it meets an
unfamiliar key will break on a future release.

Beyond that, within a major version, an existing key will not be removed, and
will not change its meaning or its units. Keys documented as conditional
(``OptimumValue``, ``OptimumDirection``) may be absent; the rest are always
present.

``-tableout <file>`` writes the same data as a space-separated table instead,
with a ``#`` header line naming the columns. Values containing spaces are
quoted. It appends, so repeated runs accumulate rows under one header.

Search tree as JSON -- ``-dumptreejson <file>``
------------------------------------------------

Writes the whole search tree as a single nested JSON object. Each node has::

   {"Node": 0,
    "branchVar": "q_0__",
    "branchVal": 2,
    "Domains": {"b": [[1,3]], "bool": [[0,0]], "q_0__": [[2,2]]},
    "left": { ... },
    "right": { ... }}

``Domains`` maps each variable to its domain at that node, as a list of
inclusive ``[low, high]`` intervals. ``left`` and ``right`` are the child
nodes, and are ``{}`` at a leaf. A node where a solution was found also has
``"solution": 1``.

This can be very large -- it records every variable's domain at every node --
so use it on small instances.

``-dumptree`` prints a human-readable version to standard output, and
``-dumptreesql`` emits SQL insert statements instead.
