
Command Line Options
********************


Minion supports a number of switches to augment default behaviour, these
are listed here. We begin with the most useful, and then list other,
less common flags.


Choosing how much search to perform (time, number of solutions)
---------------------------------------------------------------



-nodelimit
^^^^^^^^^^^^^^^^


To stop search after N nodes::

   minion -nodelimit N myinput.minion


-sollimit
^^^^^^^^^^^^^^^^


To stop search after N solutions have been found::

   minion -sollimit N myinput.minion


-timelimit
^^^^^^^^^^^^^^^^


To stop search after N seconds (real time)::

   minion -timelimit N myinput.minion

-cpulimit
^^^^^^^^^^^^^^^^


To stop search after N seconds (CPU time)::

   minion -cpulimit N myinput.minion



-findallsols
^^^^^^^^^^^^^^^^


Find all solutions and count them. This option is ignored if the
problem contains any minimising or maximising objective.


-varorder
^^^^^^^^^^^^^^^^



Enable a particular variable ordering for the search process. 

The available orders are:

- sdf - smallest domain first, break ties lexicographically

- sdf-random - sdf, but break ties randomly

- srf - smallest ratio first, chooses unassigned variable with smallest
  percentage of its initial values remaining, break ties lexicographically

- srf-random - srf, but break ties randomly

- ldf - largest domain first, break ties lexicographically

- ldf-random - ldf, but break ties randomly

- random - random variable ordering

- static - lexicographical ordering

- wdeg - Weighted degree

- domoverwdeg - Domain size over weighted degree



-valorder
^^^^^^^^^^^^^^^^



Choose the value ordering (overruling any selection in the input file).

Current orders are, ascend, descend and random.


-quiet
^^^^^^^^^^^^^^^^


Do not print parser progress (default)

-printsols
^^^^^^^^^^^^^^^^


Print solutions (default).


-noprintsols
^^^^^^^^^^^^^^^^


Do not print solutions.


-printsolsonly
^^^^^^^^^^^^^^^^


Print only solutions and a summary at the end.


-printonlyoptimal
^^^^^^^^^^^^^^^^^


In optimisation problems, only print the optimal value, and
not intermediate values.


-prop-node
^^^^^^^^^^^^^^^^


Allows the user to choose the level of consistency to be enforced
during search.

See `-preprocess` for details of the available
levels of consistency.


-preprocess
^^^^^^^^^^^

This switch allows the user to choose what level of preprocess is
applied to their model before search commences.

The choices are:

- GAC
    - generalised arc consistency (default)
    - all propagators are run to a fixed point
    - if some propagators enforce less than GAC then the model will
    not necessarily be fully GAC at the outset

- SACBounds
    - singleton arc consistency on the bounds of each variable
    - AC can be achieved when any variable lower or upper bound is a
    singleton in its own domain

- SAC
    - singleton arc consistency
    - AC can be achieved in the model if any value is a singleton in
    its own domain

- SSACBounds
    - singleton singleton bounds arc consistency
    - SAC can be achieved in the model when domains are replaced by either
    the singleton containing their upper bound, or the singleton containing
    their lower bound

- SSAC
    - singleton singleton arc consistency
    - SAC can be achieved when any value is a singleton in its own domain

These are listed in order of roughly how long they take to
achieve. Preprocessing is a one off cost at the start of search. The
success of higher levels of preprocessing is problem specific; SAC
preprocesses may take a long time to complete, but may reduce search
time enough to justify the cost.

Each of the SAC variants can have '_limit' added (for example
SACBound_limit). The '_limit' variants of these algorithm add checks
which stop the algorithms in some situations when they are taking a
very long time.


-randomseed
^^^^^^^^^^^^^^^^


Set the pseudorandom seed to N. This allows 'random' behaviour to be
repeated in different runs of minion.


-tableout
^^^^^^^^^^^^^^^^


Append a line of data about the current run of minion to a named file.
This data includes minion version information, arguments to the
executable, build and solve time statistics, etc. See the file itself
for a precise schema of the supplied information.

To add statistics about solving myproblem.minion to mystats.txt::

    minion -tableout mystats.txt myproblem.minion


-solsout
^^^^^^^^^^^^^^^^


Append all solutionsto a named file.
Each solution is placed on a line, with no extra formatting.

To add the solutions of myproblem.minion to mysols.txt::

    minion -solsout mysols.txt myproblem.minion



Less common flags
^^^^^^^^^^^^^^^^^

-outputCompressedDomains
^^^^^^^^^^^^^^^^


Try to reduce the initial domains of variables, and output them.
This is in general not useful for users, but is provided as a pre-preprocessing
step for other systems.


-outputCompressed
^^^^^^^^^^^^^^^^


Output a Minion instance with some basic reasoning performed to
reduce the size of the file. This file should produce identical
output the original instance but may solve faster.

To compress a file 'infile.minion' to a file 'smaller.minion'::

   minion infile.minion -outputCompressed smaller.minion


-redump
^^^^^^^^^^^^^^^^


Print the minion input instance file to standard out. No search is
carried out when this switch is used. This can be used to update
files in old versions of the Minion file format.





-verbose
^^^^^^^^^^^^^^^^


Print parser progress




-map-long-short
^^^^^^^^^^^^^^^^


Automatically generate a short tuple list from each long tuple list.

The methods of compression are:

* none : No short tuple list generated (default)
* eager : Use a fast algorithm to produce a reasonable short tuple list (best
as first choice)
* lazy : Work harder (possibly exponentially) to produce a shorter short tuple
list
* keeplong : Make a 'short tuple list' with no short tuples (only for
benchmarking)


-nocheck
^^^^^^^^^^^^^^^^


Do not check solutions for correctness before printing them out.



-check
^^^^^^^^^^^^^^^^


Check solutions for correctness before printing them out. This should
only make a difference if Minion contains a bug.



-dumptree
^^^^^^^^^^^^^^^^


Print out the branching decisions and variable states at each node.


-dumptreejson
^^^^^^^^^^^^^^^^


Print out the branching decisions and variable states at each node.
Accepts filename to output tree to



-skipautoaux
^^^^^^^^^^^^^^^^


By default Minion adds all variables to the varorder, to ensure that all
variables
are branched assigned before a solution is outputted. This option disables
that
behaviour. This means minion Minion may output solutions incorrectly, or
incorrect
numbers of solutions. This flag is provided because some users require this
low-level control over the search, but is in general useless and dangerous.
In particular,
it will not speed up search (except when the speed up is due to producing
garbage of course!)




-randomiseorder
^^^^^^^^^^^^^^^^


Randomises the ordering of the decision variables, and the value ordering.
If the input file specifies as ordering it will randomly permute this. If no
ordering is
specified a random permutation of all the variables is used.



-jsonsolsout
^^^^^^^^^^^^^^^^


Append all solutions to a named file, as JSON objects.
Each solution is store as a seperate JSON object.

To add the solutions of myproblem.minion to mysols.txt::

    minion -jsonsolsout mysols.json myproblem.minion


-makeresume
^^^^^^^^^^^^^^^^


Write a resume file on timeout or being killed.


-noresume
^^^^^^^^^^^^^^^^


Do not write a resume file on timeout or being killed. (default)


-gap
^^^^^^^^^^^^^^^^


Give name of gap executable (defaults to gap.sh)


-split
^^^^^^^^^^^^^^^^


When Minion is terminated before the end of search, write out two new input
files that split the remaining search space in half. Each of the files will
have
all the variables and constraints of the original file plus constraints that
rule out the search already done. In addition, the domain of the variable
under
consideration when Minion was stopped is split in half with each of the new
input files considering a different half.

This feature is experimental and intended to facilitate parallelisation --
to
parallelise the solving of a single constraint problem, stop and split
repeatedly. Please note that large-scale testing of this feature was limited
to
Linux systems and it might not work on others (especially Windows).

The name of the new input files is composed of the name of the original
instance, the string 'resume', a timestamp, the process ID of Minion, the
name
of the variable whose domain is being split and 0 or 1. Each of the new
input
files has a comment identifying the name of the input file which it was
split
from. Similarly, Minion's output identifies the new input files it writes
when
splitting.

The new input files can be run without any special flags.

This flag is intended to be used with the -timelimit, -sollimit, -nodelimit
or -cpulimit flags. Please note that changing other flags between
runs (such as -varorder) may have unintended consequences.

Implies -makeresume.


-split-stderr
^^^^^^^^^^^^^^^^


The flag -split-stderr has the same function as the flag -split, however the
two new Minion input files are sent to standard error rather than written to
files.

See documentation for -split.



