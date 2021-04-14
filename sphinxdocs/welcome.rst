Introduction to Minion
======================

Minion is a solver for constraint satisfaction problems. First we
introduce constraints, then give a general overview of Minion. Following
this we give instructions for installation and basic use.

What are constraints?
---------------------

Constraints are a powerful and natural means of knowledge representation
and inference in many areas of industry and academia. Consider, for
example, the production of a university timetable. This problem’s
constraints include: the maths lecture theatre has a capacity of 100
students; art history lectures require a venue with a slide projector;
no student can attend two lectures simultaneously. Constraint solving of
a combinatorial problem proceeds in two phases. First, the problem is
*modelled* as a set of *decision variables*, and a set of *constraints*
on those variables that a solution must satisfy. A decision variable
represents a choice that must be made in order to solve the problem. The
*domain* of potential values associated with each decision variable
corresponds to the options for that choice. In our example one might
have two decision variables per lecture, representing the time and the
venue. For each class of students, the time variables of the lectures
they attend may have an AllDifferent constraint on them to ensure that
the class is not timetabled to be in two places at once. The second
phase consists of using a constraint solver to search for *solutions*:
assignments of values to decision variables satisfying all constraints.
The simplicity and generality of this approach is fundamental to the
successful application of constraint solving to a wide variety of
disciplines such as scheduling, industrial design and combinatorial
mathematics :cite:`wallace:Survey`.

To illustrate, below we show a simple puzzle, where two six-digit
numbers (DONALD and GERALD) are added together to form another six-digit
number (ROBERT). Each letter A, B, D, E, G, L, N, O, R and T represents
a distinct digit :math:`0\ldots9`. The puzzle can be represented with
the expressions below, given by Bessière and Régin
:cite:`bessiere-gac-schema`.

.. math::

   \begin{aligned}
   100000*D+10000*O+1000*N+100*A+10*L+D\\
   +100000*G+10000*E+1000*R+100*A+10*L+D\\
   =100000*R+10000*O+1000*B+100*E+10*R+T\\
   \textrm{allDifferent}(A, B, D, E, G, L, N, O, R, T)
   \end{aligned}

This representation of the puzzle illustrates the main concepts of
constraint programming. A, B, D, E, G, L, N, O, R and T are variables,
each with initial domain :math:`0\ldots9`. There are two constraints,
one representing the sum and the other representing that the variables
each take a different value. A solution is a function mapping each
variable to a value in its initial domain, such that all constraints are
satisfied. The solution to this puzzle is A=4, B=3, D=5, E=9, G=1, L=8,
N=6, O=2, R=7, T=0.

Constraints are *declarative* — the statement of the problem and the
algorithms used to solve it are separated. This is an attractive feature
of constraints, since it can reduce the human effort required to solve a
problem. Various general purpose and specialized algorithms exist for
solving systems of constraints. A great variety of problems can be
expressed with constraints. The following list of subject areas was
taken from CSPLib :cite:`csplib`:

-  Scheduling (e.g. job shop scheduling :cite:`martin-jobshop-96new`),
-  Design, configuration and diagnosis (e.g. template design
   :cite:`proll-smith-templatedesign`),
-  Bin packing and partitioning (e.g. social golfer problem
   :cite:`harvey01symmetry`),
-  Frequency assignment (e.g. the Golomb ruler problem
   :cite:`smith99golomb`),
-  Combinatorial mathematics (e.g. balanced incomplete block design
   :cite:`frisch-symmetry-implied-04`),
-  Games and puzzles (e.g. maximum density still life
   :cite:`smith-model-life`),
-  Bioinformatics (e.g. discovering protein shapes
   :cite:`protein-structure-problems`).

Solving constraint problems
---------------------------

The classical constraint satisfaction problem (CSP) has a finite set of
variables, each with a finite domain, and a set of constraints over
those variables. A solution to an instance of CSP is an assignment to
each variable, such that all constraints are simultaneously *satisfied*
— that is, they are all true under the assignment. Solvers typically
find one or all solutions, or prove there are no solutions. The decision
problem (‘does there exist a solution?’) is NP-complete
:cite:`apt-constraint-programming`, therefore there is no known
polynomial-time procedure to find a solution.

The most common technique (and the one used by Minion) is to interleave
splitting (also called branching) and propagation. Splitting is the
basic operation of search, and propagation simplifies the CSP instance.
Apt views the solution process as the repeated transformation of the CSP
until a solution state is reached :cite:`apt-constraint-programming`. In
this view, both splitting and propagation are transformations, where
propagation simplifies the CSP by removing values which cannot take part
in any solution. A splitting operation transforms a CSP instance into
two or more simpler CSP instances, and by recursive application of
splitting any CSP can be solved.

Since splitting is an exponential-time solution method, it is important
that splitting is minimized by effective propagation. Much effort has
gone into developing propagation algorithms which are fast and effective
in removing values. Most propagation algorithms are specialized to
particular types of constraint (e.g. a vector of variables take distinct
values in any solution, the AllDifferent constraint). They typically run
in polynomial time.
