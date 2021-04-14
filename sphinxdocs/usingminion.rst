Minion in Practice
==================

This chapter discusses 7 minion example files which are clearly
commented so that the user can see what a minion file looks like in
practice. Comments in minion start with a :math:`\sharp`, however for
reasons of ease of reading all lines of actual code be it Minion or
Essence’ are shown in typewriter text and comments are inserted in
normal text. The first file is a modified version of the one that all
the minion developers turn to when modelling a new problem in minion. It
shows exactly what a minion file can include and what the syntax is for
all the possible sections. If you are modelling a problem as minion than
we recommend you take a copy of this file and edit it appropriately, as
this will help to guide you through the modelling process. These
examples can be used as the bases to implement any similar problems.

Minion Example File
-------------------

This file does not really relate to any English problem description,
although it does parse and run, it is an example which clearly shows all
of the possible Minion input file constructs. If you are modelling a
problem as minion than we recommend you take a copy of this file and
edit it appropriately, as this will help to guide you through the
modelling process. It can be found in the

::

   summer_school

directory and is called

::

   format_example.minion

we have added comments to explain the different sections to the novice
user.

::

   MINION 3

This file includes an example of all the different inputs you can give
to Minion. It is a very good place to start from when modelling problem
in the Minion specification.

The first section is where all the variables are declared.

::

   **VARIABLES**

There are 4 type of variables. Booleans don’t need a domain and are
formatted as follows:

::

   BOOL bo

Internally, Bound variables are stored only as a lower and upper bound
whereas discrete variables allow any sub-domain. Bound variables need a
domain given as a range as follows:

::

   BOUND b {1..3}

Discrete variables also need a domain given as a range as follows:

::

   DISCRETE d {1..3}

Sparse bound variables take a sorted list of values as follows:

::

   SPARSEBOUND s {1,3,6,7}

We can also declare matrices of variables. The first example is a matrix
with 3 variables: q[0],q[1] and q[2].

::

   DISCRETE q[3] {0..5}

The second example is of a 2d matrix, where the variables are bm[0,0],
bm[0,1], bm[1,0], bm[1,1].

::

   BOOL bm[2,2]

The third example shows how to declare a matrix with more indices. You
can have as many indices as you like!

::

   BOOL bn[2,2,2,2]

In this next section, which is optional, you can define tuplelists.
Tuplelists provide a method of defining sets of tuples which can then be
used in ``table`` and ``negativetable`` constraints. Defining these in a
``**TUPLELIST**`` does not change the search, but can save memory by
reusing the same list of tuples in multiple constraints.The input is:
:math:`\langle`\ name\ :math:`\rangle`
:math:`\langle`\ num\ :math:`\_`\ of\ :math:`\_`\ tuples\ :math:`\rangle`
:math:`\langle`\ tuple\ :math:`\_`\ length\ :math:`\rangle`
:math:`\langle`\ numbers\ :math:`\ldots \rangle`.

::

   **TUPLELIST**
   Fred 3 3
   0 2 3
   2 0 3
   3 1 3

The next thing to declare are the constraints which go in this section.

::

   **CONSTRAINTS**

Constraints are defined in the same way as functions are in most
programming paradigms! A complete list of constraints can be found at
the end of the manual. The two following constraints very simply set
bo=0 and b=d.

::

   eq(bo, 0)
   eq(b,d)

Note that except in special cases (the ``reify`` and ``reifyimply``
constraints), Minion constraints cannot be nested. For example
``eq(eq(bo,0), d)`` is not valid. Such constraints must be written by
manually adding extra variables.

To get a single variable from a matrix, you index it with square
brackets using commas to separate the dimensions of the matrix. The
first example following is a 1D matrix, the second in 4D.

::

   eq(q[1],0)
   eq(bn[0,1,1,1], bm[1,1])

| It’s easy to get a row or column from a matrix. You use :math:`\_` in
  the indices you want to vary. Giving a matrix without an index simply
  gives all the variables in that matrix. The following shows how
  flattening occurs...
| :math:`[bm] == [ bm[\_,\_] ] == [ bm[0,0], bm[0,1], bm[1,0], bm[1,1] ]`
| :math:`[ bm[\_,1] ] = [ bm[0,1], bm[1,1] ]`
| :math:`[ bn[1,\_,0,\_] = [ bn[1,0,0,0], b[1,0,0,1], b[1,1,0,0], b[1,1,0,1] ]`
| You can string together a list of such expressions as in the following
  example:

::

   lexleq( [bn[1,_,0,_], bo, q[0]] , [b, bm, d] )

So the parser can recognise them you must always put [ ] around any
matrix expression, so lexleq(bm, bm) is invalid, but the following is
valid:

::

   lexleq( [bm], [bm] )

An example of a constraint which uses tuples

::

   table([q], Fred)

You do not have to pre-declare tuples, you can write them explicitly if
you wish. The above constraint for example is equivalent to:

::

   table([q],{ <0,2,3>,<2,0,3>,<3,1,3> })

The last section is the search section. This section is optional, and
allows some limited control over the way minion searches for a solution.
Note that everything in this section can be given at most once.

::

   **SEARCH**

You give the variable ordering by listing each of the variables in the
order you wish them to be searched. You can either list each of the
variables in a matrix individually by giving the index of each variable,
or you can just state the matrix in which case it goes through each of
the variables in turn. If you miss any of the variables out than these
variables are not branched on. Note that this can lead to Minion
reporting invalid solutions, so use with care! If you don’t give an
explicit variable ordering, than one is generated based on the order the
variables are declared. If you give a ``-varorder`` on the command line,
that will only consider the variable given in the ``VARORDER``.

::

   VARORDER [bo,b,d,q[_]]

You give the value order for each variable as either ``a`` for ascending
or ``d`` for descending. The value orderings are given in the same order
as the variable ordering. For example, to make the variable b by
searched in descending order you make the second term into a ``d`` as
the above variable ordering shows it to be the second variable to be
searched. The default variable order is ascending order for all
variables.

::

   VALORDER [a,a,d,a]

You can have one objective function which can be either to maximise or
minimise any single variable. To minimise a constraint, you should
assign it equal to a new variable.

::

   MAXIMISING bo
   # MINIMISING x3

The print statement takes a 2D matrix of things to print. The following
example prints both the variables bo and q, putting these in double
square brackets turns them into a 2D matrix so they are acceptable
input. You can also give: PRINT ALL (the default) which prints all
variables and PRINT NONE which turns printing off completely.

::

   PRINT [ [bo, q] ]

The file must end with the \*\ *EOF*\ \* marker! Any text under that is
ignored, so you can write whatever you like (or nothing at all...)

::

   **EOF**

The only remaining part of Minion’s input language are its many
constraints. These are listed in the Appendix.

The Farmers Problem
-------------------

The Farmers Problem is a very simple problem which makes a very good
example to be the first CP that you model. The problem is as follows: A
farmer has 7 animals on his farm: pigs and hens. They all together have
22 legs. How many pigs (4 legs) and how many hens(2 legs) does the
farmer have? These files can be found in ``/summer_school/examples``.
The Essence’ file is named ``FarmersProblem.eprime`` and the Minion file
is ``FarmersProblem.minion``

The Essence’ specification of this (which was explained in detail in the
Tailor section is as follows:

::

   find pigs, hens: int(0..7)

   such that

   pigs + hens = 7,
   pigs * 4 + hens * 2 = 22

The Minion input file for this is:

::

   MINION 3

There are two variables pigs and hens both have domain 0..7

::

   **VARIABLES**
   DISCRETE pigs {0..7}
   DISCRETE hens {0..7}

Both variables pigs and hens should be printed and the variable ordering
is search pigs than hens.

::

   **SEARCH**

   PRINT [[pigs],[hens]]

   VARORDER [pigs,hens]


   **CONSTRAINTS**

The following two constraints relate to the following
:math:`(pigs \times 4) + (hens \times 2) = 22`. There is no weighted sum
constraint in Minion so you should use the weighted sum less than and
equal to constraint and the weighted sum greater than and equal to
constraint. You read this as
:math:`(hens \times 2) + (pigs \times 4)) \leq 22` and
:math:`(hens \times 2) + (pigs \times 4)  \geq 22`.

::

   weightedsumgeq([2,4], [hens,pigs], 22)
   weightedsumleq([2,4], [hens,pigs], 22)

The following two constraints relate to the following
:math:`pigs + hens = 7`. There is no sum constraint in Minion so you
should use the sum less than and equal to constraint and the sum greater
than and equal to constraint. You read this as
:math:`hens + pigs \leq 7` and :math:`hens + pigs \geq 7`.

::

   sumleq([hens,pigs], 7)
   sumgeq([hens,pigs], 7)
   **EOF**

Cryptarithmetic
---------------

The second problem outlined is a very famous Cryptarithmetic puzzle:
SEND + MORE = MONEY. These files can be found in
``/summer_school/examples`` the Essence’ file is
``SENDMOREMONEY.eprime`` and the Minion file is
``SENDMOREMONEY.minion``. The Essence’ specification is as follows:

::

   find S,E,N,D,M,O,R,Y : int(0..9)

   such that

   1000*S + 100*E + 10*N + D +
   1000*M + 100*O + 10*R + E =
   10000*M + 1000*O + 100*N + 10*E + Y,

   alldiff([S,E,N,D,M,O,R,Y])

The Minion model is then:

::

   MINION 3

There are 8 variables: S,E,N,D,M,O,R,Y all with domains 0 to 9.

::

   **VARIABLES**
   DISCRETE S {0..9}
   DISCRETE E {0..9}
   DISCRETE N {0..9}
   DISCRETE D {0..9}
   DISCRETE M {0..9}
   DISCRETE O {0..9}
   DISCRETE R {0..9}
   DISCRETE Y {0..9}

Search the variables in the order S, E, N, D, M, O, R, Y and print the
same variable in this order.

::

   **SEARCH**

   PRINT [[S],[E],[N],[D],[M],[O],[R],[Y]]

   VARORDER [S,E,N,D,M,O,R,Y]

The first constraint is an all different which is across all variables
this is an implicit constraint in the problem, as all the letters
represent different numbers.

::

   **CONSTRAINTS**

   alldiff([ S, E, N, D, M, O, R, Y])

The second constraint
represents:\ :math:`(1000 \times S) + (100 \times E) + (10 \times N) + D +  (1000 \times M) + (100 \times O )+ (10 \times R) + E = (10000 \times M) + (1000 \times O) + (100 \times N) + (10 \times E) + Y`.
The first thing the model does is rewrite this expression to make it
equal to a number, in this case 0. So this expression becomes:
:math:`(10000 \times M) + (1000 \times O) + (100 \times N) + (10 \times E) + Y - (1000 \times S)  - (100 \times E) - (10 \times N) - D - (1000 \times M) - (100 \times O) - (10 \times R) - E = 0`.
The terms are then rearranged so the same weights are together and the
positive numbers are first this then becomes:
:math:`Y + (10 \times E) + (100 \times N) + (1000 \times O) + (10000 \times M) -D - E - (10 \times N) - (10 \times R) - (100 \times E) - (100 \times O)  - (1000 \times M) - (1000 \times S) = 0`.
Minion does not have a weighted sum equals constraint, so this is
represented as one weighted sum less than or equal to and one weighted
sum greater than or equal to. The two constraints are then:
:math:`Y + (10 \times E) + (100 \times N) + (1000 \times O) + (10000 \times M) -D - E - (10 \times N) - (10 \times R) - (100 \times E) - (100 \times O)  - (1000 \times M) - (1000 \times S) \leq 0`
and
:math:`Y + (10 \times E) + (100 \times N) + (1000 \times O) + (10000 \times M) -D - E - (10 \times N) - (10 \times R) - (100 \times E) - (100 \times O)  - (1000 \times M) - (1000 \times S) \geq 0`.

::

   weightedsumgeq(
       [1,10,100,1000,10000,-1,-1,-10,-10,-100,-100,-1000,-1000],
       [Y,E,N,O,M,D,E,N,R,E,O,M,S], 0)
   weightedsumleq(
       [1,10,100,1000,10000,-1,-1,-10,-10,-100,-100,-1000,-1000],
       [Y,E,N,O,M,D,E,N,R,E,O,M,S], 0)

   **EOF**

The Eight Number Puzzle
-----------------------

The eight number puzzle asks you to label the nodes of the graph shown
in Figure \ `3.1 <#fig:8puzzle>`__ with the values 1 to 8 such that no
two connected nodes have consecutive values. These files can be found in
/summer_school/examples the Essence’ file is EightPuzzleDiagram.eprime
and the Minion file is EightPuzzleDiagram.minion. The Essence’
specification is as follows:

.. figure:: EightPuzzleDiagram.pdf
   :alt: Graph which represents The Eight Number Puzzle

   Graph which represents The Eight Number Puzzle

::

   find circles: matrix indexed by [int(1..8)] of int(1..8)

   such that

   alldiff(circles),
   | circles[1] - circles[2] | > 1,
   | circles[1] - circles[3] | > 1,
   | circles[1] - circles[4] | > 1,
   | circles[2] - circles[3] | > 1,
   | circles[3] - circles[4] | > 1,
   | circles[2] - circles[5] | > 1,
   | circles[2] - circles[6] | > 1,
   | circles[3] - circles[5] | > 1,
   | circles[3] - circles[6] | > 1,
   | circles[3] - circles[7] | > 1,
   | circles[4] - circles[6] | > 1,
   | circles[4] - circles[7] | > 1,
   | circles[5] - circles[6] | > 1,
   | circles[6] - circles[7] | > 1,
   | circles[5] - circles[8] | > 1,
   | circles[6] - circles[8] | > 1,
   | circles[7] - circles[8] | > 1

The Minion model is then:

::

   MINION 3

There is a 1d matrix of size 8 with domain {1,..,8} to represent the 8
circles which numbers can be allocated to. There are also 34 auxiliary
variables, 2 to represent each constraint.

::

   **VARIABLES**
   DISCRETE circles[8] {1..8}

   # auxiliary variables
   DISCRETE aux0 {-7..7}
   DISCRETE aux1 {0..7}
   DISCRETE aux2 {-7..7}
   DISCRETE aux3 {0..7}
   DISCRETE aux4 {-7..7}
   DISCRETE aux5 {0..7}
   DISCRETE aux6 {-7..7}
   DISCRETE aux7 {0..7}
   DISCRETE aux8 {-7..7}
   DISCRETE aux9 {0..7}
   DISCRETE aux10 {-7..7}
   DISCRETE aux11 {0..7}
   DISCRETE aux12 {-7..7}
   DISCRETE aux13 {0..7}
   DISCRETE aux14 {-7..7}
   DISCRETE aux15 {0..7}
   DISCRETE aux16 {-7..7}
   DISCRETE aux17 {0..7}
   DISCRETE aux18 {-7..7}
   DISCRETE aux19 {0..7}
   DISCRETE aux20 {-7..7}
   DISCRETE aux21 {0..7}
   DISCRETE aux22 {-7..7}
   DISCRETE aux23 {0..7}
   DISCRETE aux24 {-7..7}
   DISCRETE aux25 {0..7}
   DISCRETE aux26 {-7..7}
   DISCRETE aux27 {0..7}
   DISCRETE aux28 {-7..7}
   DISCRETE aux29 {0..7}
   DISCRETE aux30 {-7..7}
   DISCRETE aux31 {0..7}
   DISCRETE aux32 {-7..7}
   DISCRETE aux33 {0..7}

The variable ordering branches on all the circle variables before each
of the aux variables. Only the circle variables are printed.

::

   **SEARCH**

   PRINT [circles]

   VARORDER [circles,
   aux0,aux1,aux2,aux3,aux4,aux5,aux6,aux7,
   aux8,aux9,aux10,aux11,aux12,aux13,aux14,aux15,
   aux16,aux17,aux18,aux19,aux20,aux21,aux22,aux23,
   aux24,aux25,aux26,aux27,aux28,aux29,aux30,aux31,
   aux32,aux33]

The all different constraint on the circle variables are explicit in the
problem, this is the first constraint in the collection. The other
constraints are all of the type :math:`|circles[a] - circles[b]| > 1`.
The first of these such constraints is
:math:`|circles[1] - circles[2]| > 1` this type of constraint is
represented by a series of 4 constraints in Minion. The constraints are
reversed in the Minion specification so that the last 4 constraints
represent this first expression. The constraints are indexed from 1 in
Essence’ and 1 in Minion, so the above constraint becomes
:math:`|circles[0] - circles[1]| > 1`. Then
:math:`|circles[0] - circles[1]| > 1` is decomposed to
:math:`circles[1] - circles[2] = aux0` and :math:`|aux0| = aux1` and
:math:`1 \leq aux1-1`. As Minion has no weighted sum equals to
constraint a weighted sum greater than or equals to constraint and a
weighted sum less than or equals to, so
:math:`circles[1] - circles[2] = aux0` is
:math:`circles[1] - circles[2] \leq aux0` and
:math:`circles[1] - circles[2] \geq aux0`. The other constraints all
form the same pattern.

::

   **CONSTRAINTS**

   alldiff([circles])
   weightedsumgeq([1,-1], [circles[6],circles[7]], aux32)
   weightedsumleq([1,-1], [circles[6],circles[7]], aux32)
   abs(aux33,aux32)
   ineq(1,aux33,-1)
   weightedsumgeq([1,-1], [circles[5],circles[7]], aux30)
   weightedsumleq([1,-1], [circles[5],circles[7]], aux30)
   abs(aux31,aux30)
   ineq(1,aux31,-1)
   weightedsumgeq([1,-1], [circles[4],circles[7]], aux28)
   weightedsumleq([1,-1], [circles[4],circles[7]], aux28)
   abs(aux29,aux28)
   ineq(1,aux29,-1)
   weightedsumgeq([1,-1], [circles[5],circles[6]], aux26)
   weightedsumleq([1,-1], [circles[5],circles[6]], aux26)
   abs(aux27,aux26)
   ineq(1,aux27,-1)
   weightedsumgeq([1,-1], [circles[4],circles[5]], aux24)
   weightedsumleq([1,-1], [circles[4],circles[5]], aux24)
   abs(aux25,aux24)
   ineq(1,aux25,-1)
   weightedsumgeq([1,-1], [circles[3],circles[6]], aux22)
   weightedsumleq([1,-1], [circles[3],circles[6]], aux22)
   abs(aux23,aux22)
   ineq(1,aux23,-1)
   weightedsumgeq([1,-1], [circles[3],circles[5]], aux20)
   weightedsumleq([1,-1], [circles[3],circles[5]], aux20)
   abs(aux21,aux20)
   ineq(1,aux21,-1)
   weightedsumgeq([1,-1], [circles[2],circles[6]], aux18)
   weightedsumleq([1,-1], [circles[2],circles[6]], aux18)
   abs(aux19,aux18)
   ineq(1,aux19,-1)
   weightedsumgeq([1,-1], [circles[2],circles[5]], aux16)
   weightedsumleq([1,-1], [circles[2],circles[5]], aux16)
   abs(aux17,aux16)
   ineq(1,aux17,-1)
   weightedsumgeq([1,-1], [circles[2],circles[4]], aux14)
   weightedsumleq([1,-1], [circles[2],circles[4]], aux14)
   abs(aux15,aux14)
   ineq(1,aux15,-1)
   weightedsumgeq([1,-1], [circles[1],circles[5]], aux12)
   weightedsumleq([1,-1], [circles[1],circles[5]], aux12)
   abs(aux13,aux12)
   ineq(1,aux13,-1)
   weightedsumgeq([1,-1], [circles[1],circles[4]], aux10)
   weightedsumleq([1,-1], [circles[1],circles[4]], aux10)
   abs(aux11,aux10)
   ineq(1,aux11,-1)
   weightedsumgeq([1,-1], [circles[2],circles[3]], aux8)
   weightedsumleq([1,-1], [circles[2],circles[3]], aux8)
   abs(aux9,aux8)
   ineq(1,aux9,-1)
   weightedsumgeq([1,-1], [circles[1],circles[2]], aux6)
   weightedsumleq([1,-1], [circles[1],circles[2]], aux6)
   abs(aux7,aux6)
   ineq(1,aux7,-1)
   weightedsumgeq([1,-1], [circles[0],circles[3]], aux4)
   weightedsumleq([1,-1], [circles[0],circles[3]], aux4)
   abs(aux5,aux4)
   ineq(1,aux5,-1)
   weightedsumgeq([1,-1], [circles[0],circles[2]], aux2)
   weightedsumleq([1,-1], [circles[0],circles[2]], aux2)
   abs(aux3,aux2)
   ineq(1,aux3,-1)
   weightedsumgeq([1,-1], [circles[0],circles[1]], aux0)
   weightedsumleq([1,-1], [circles[0],circles[1]], aux0)
   abs(aux1,aux0)
   ineq(1,aux1,-1)

   **EOF**

A :math:`K_4 \times P_2` Graceful Graph
---------------------------------------

This problem is stated as follows. A labelling :math:`f` of the nodes of
a graph with :math:`q` edges is graceful if :math:`f` assigns each node
a unique label from :math:`0,1,..., q` and when each edge :math:`xy` is
labelled with :math:`|f(x) - f(y)|`, the edge labels are all different.
(Hence, the edge labels are a permutation of :math:`1, 2, ..., q`.) Does
the :math:`K_4 \times P_2` graph shown in Figure \ `3.2 <#fig:k4xp2>`__
have a graceful library. These files can be found in
``/summer_school/examples``, the Essence’ file is called
``K4P2GracefulGraph.eprime`` and the Minion file is
``K4P2GracefulGraph.minion``. The Essence’ specification is as follows:

.. figure:: k4xp2.pdf
   :alt: A :math:`K_4 \times P_2` Graph

   A :math:`K_4 \times P_2` Graph

::

   find nodes : matrix indexed by [int(1..8)] of int(0..16),
          edges: matrix indexed by [int(1..16)] of int(1..16)

   such that

   |nodes[1] - nodes[2]| = edges[1],
   |nodes[1] - nodes[3]| = edges[2],
   |nodes[1] - nodes[4]| = edges[3],
   |nodes[2] - nodes[3]| = edges[4],
   |nodes[2] - nodes[4]| = edges[5],
   |nodes[3] - nodes[4]| = edges[6],

   |nodes[5] - nodes[6]| = edges[7],
   |nodes[5] - nodes[7]| = edges[8],
   |nodes[5] - nodes[8]| = edges[9],
   |nodes[6] - nodes[7]| = edges[10],
   |nodes[6] - nodes[8]| = edges[11],
   |nodes[7] - nodes[8]| = edges[12],

   |nodes[1] - nodes[5]| = edges[13],
   |nodes[2] - nodes[6]| = edges[14],
   |nodes[3] - nodes[7]| = edges[15],
   |nodes[4] - nodes[8]| = edges[16],

   alldiff(edges),
   alldiff(nodes)

The Minion model is then:

::

   MINION 3

There are two 1d arrays of variables one representing all the node
variables and one representing all the edge variables. The 8 node
variables have domain 0 to 16 and the edge variables have domain 1 to
16. There are also 16 auxiliary variables introduced called aux0 to
aux15 there is one of these for each constraint and there is one
constraint to represent each edge.

::

   **VARIABLES**
   DISCRETE nodes[8] {0..16}
   DISCRETE edges[16] {1..16}

   # auxiliary variables
   DISCRETE aux0 {-16..16}
   DISCRETE aux1 {-16..16}
   DISCRETE aux2 {-16..16}
   DISCRETE aux3 {-16..16}
   DISCRETE aux4 {-16..16}
   DISCRETE aux5 {-16..16}
   DISCRETE aux6 {-16..16}
   DISCRETE aux7 {-16..16}
   DISCRETE aux8 {-16..16}
   DISCRETE aux9 {-16..16}
   DISCRETE aux10 {-16..16}
   DISCRETE aux11 {-16..16}
   DISCRETE aux12 {-16..16}
   DISCRETE aux13 {-16..16}
   DISCRETE aux14 {-16..16}
   DISCRETE aux15 {-16..16}

The variable order is to branch on the nodes then on the edges then the
auxiliary variables. Only the node and the edge variables are printed.

::

   **SEARCH**

   PRINT [nodes,edges]

   VARORDER [nodes,edges,
   aux0,aux1,aux2,aux3,aux4,aux5,aux6,aux7,
   aux8,aux9,aux10,aux11,aux12,aux13,aux14,aux15]

Implicit in the problem is an all different constraint on both the node
and edge variables. The other constraints are all of the form \|nodes[a]
- nodes[b]\| = edges[a], the first of these constraints from the
Essence’ specification is :math:`|nodes[1] - nodes[2]| = edges[1]` this
corresponds to the last three constraints in the minion file as the
order of constraints are reversed. Minion starts indexing matrices from
0, whereas Essence’ started numbering from 1 so the above constraint
becomes :math:`|nodes[0] - nodes[1]| = edges[0]`. This is broken into
:math:`nodes[0] - nodes[1] = aux0` and :math:`|edges[0]| = aux0`. As
minion has no weighted sum equals this is broken into a weighted sum
less than or equals to and weighted sum greater than or equals to. So
this full constraint is represented as
:math:`nodes[0] - nodes[1]  \leq aux0` and
:math:`nodes[0] - nodes[1]  \geq aux0` and :math:`|edges[0]| = aux0`.

::

   **CONSTRAINTS**

   alldiff([nodes])
   alldiff([edges])
   weightedsumgeq([1,-1], [nodes[3],nodes[7]], aux15)
   weightedsumleq([1,-1], [nodes[3],nodes[7]], aux15)
   abs(edges[15],aux15)
   weightedsumgeq([1,-1], [nodes[2],nodes[6]], aux14)
   weightedsumleq([1,-1], [nodes[2],nodes[6]], aux14)
   abs(edges[14],aux14)
   weightedsumgeq([1,-1], [nodes[1],nodes[5]], aux13)
   weightedsumleq([1,-1], [nodes[1],nodes[5]], aux13)
   abs(edges[13],aux13)
   weightedsumgeq([1,-1], [nodes[0],nodes[4]], aux12)
   weightedsumleq([1,-1], [nodes[0],nodes[4]], aux12)
   abs(edges[12],aux12)
   weightedsumgeq([1,-1], [nodes[6],nodes[7]], aux11)
   weightedsumleq([1,-1], [nodes[6],nodes[7]], aux11)
   abs(edges[11],aux11)
   weightedsumgeq([1,-1], [nodes[5],nodes[7]], aux10)
   weightedsumleq([1,-1], [nodes[5],nodes[7]], aux10)
   abs(edges[10],aux10)
   weightedsumgeq([1,-1], [nodes[5],nodes[6]], aux9)
   weightedsumleq([1,-1], [nodes[5],nodes[6]], aux9)
   abs(edges[9],aux9)
   weightedsumgeq([1,-1], [nodes[4],nodes[7]], aux8)
   weightedsumleq([1,-1], [nodes[4],nodes[7]], aux8)
   abs(edges[8],aux8)
   weightedsumgeq([1,-1], [nodes[4],nodes[6]], aux7)
   weightedsumleq([1,-1], [nodes[4],nodes[6]], aux7)
   abs(edges[7],aux7)
   weightedsumgeq([1,-1], [nodes[4],nodes[5]], aux6)
   weightedsumleq([1,-1], [nodes[4],nodes[5]], aux6)
   abs(edges[6],aux6)
   weightedsumgeq([1,-1], [nodes[2],nodes[3]], aux5)
   weightedsumleq([1,-1], [nodes[2],nodes[3]], aux5)
   abs(edges[5],aux5)
   weightedsumgeq([1,-1], [nodes[1],nodes[3]], aux4)
   weightedsumleq([1,-1], [nodes[1],nodes[3]], aux4)
   abs(edges[4],aux4)
   weightedsumgeq([1,-1], [nodes[1],nodes[2]], aux3)
   weightedsumleq([1,-1], [nodes[1],nodes[2]], aux3)
   abs(edges[3],aux3)
   weightedsumgeq([1,-1], [nodes[0],nodes[3]], aux2)
   weightedsumleq([1,-1], [nodes[0],nodes[3]], aux2)
   abs(edges[2],aux2)
   weightedsumgeq([1,-1], [nodes[0],nodes[2]], aux1)
   weightedsumleq([1,-1], [nodes[0],nodes[2]], aux1)
   abs(edges[1],aux1)
   weightedsumgeq([1,-1], [nodes[0],nodes[1]], aux0)
   weightedsumleq([1,-1], [nodes[0],nodes[1]], aux0)
   abs(edges[0],aux0)

   **EOF**

The Zebra Puzzle
----------------

The Zebra Puzzle is a very famous logic puzzle. There are many different
versions, but the version we will answer is as follows:

#. There are five houses.
#. The Englishman lives in the red house.
#. The Spaniard owns the dog.
#. Coffee is drunk in the green house.
#. The Ukrainian drinks tea.
#. The green house is immediately to the right of the ivory house.
#. The Old Gold smoker owns snails.
#. Kools are smoked in the yellow house.
#. Milk is drunk in the middle house.
#. The Norwegian lives in the first house.
#. The man who smokes Chesterfields lives in the house next to the man
   with the fox.
#. Kools are smoked in the house next to the house where the horse is
   kept.
#. The Lucky Strike smoker drinks orange juice.
#. The Japanese smokes Parliaments.
#. The Norwegian lives next to the blue house.

Now, who drinks water? Who owns the zebra? In the interest of clarity,
it must be added that each of the five houses is painted a different
colour, and their inhabitants are of different national extractions, own
different pets, drink different beverages and smoke different brands of
American cigarettes. These files can be found in /summer_school/examples
the Essence’ file is zebra.eprime and the Minion file is zebra.minion.
The Essence’ specification is as follows:

::

   language ESSENCE' 1.b.a

   $red = colour[1]
   $green = colour[2]
   $ivory = colour[3]
   $yellow = colour[4]
   $blue = colour[5]
   $Englishman = nationality[1]
   $Spaniard = nationality[2]
   $Ukranian = nationality[3]
   $Norwegian = nationality[4]
   $Japanese = nationality[5]
   $coffee = drink[1]
   $tea = drink[2]
   $milk = drink[3]
   $orange juice = drink[4]
   $Old Gold = smoke[1]
   $Kools = smoke[2]
   $Chesterfields = smoke[3]
   $Lucky Strike = smoke[4]
   $Parliaments = smoke[5]
   $dog = pets[1]
   $snails = pets[2]
   $fox = pets[3]
   $horse = pets[4]


   find colour: matrix indexed by [int(1..5)] of int(1..5),
          nationality: matrix indexed by [int(1..5)] of int(1..5),
          drink: matrix indexed by [int(1..5)] of int(1..5),
          smoke: matrix indexed by [int(1..5)] of int(1..5),
          pets: matrix indexed by [int(1..5)] of int(1..5)

   such that

   $constraints needed as this is a logical problem where
   $the value allocated to each position of the matrix
   $represents positon of house
   alldiff(colour),
   alldiff(nationality),
   alldiff(drink),
   alldiff(smoke),
   alldiff(pets),

   $There are five houses.
   $No constraint covered by domain specification

   $The Englishman lives in the red house
   nationality[1] = colour[1],

   $The Spaniard owns the dog.
   nationality[2] = pets[1],

   $Coffee is drunk in the green house.
   drink[1] = colour[2],

   $The Ukranian drinks tea.
   nationality[3] = drink[2],

   $The green house is immediately to the
   $right of the ivory house.
   colour[2] + 1 = colour[3],

   $The Old Gold smoker owns snails.
   smoke[1] = pets[2],

   $Kools are smoked in the yellow house.
   smoke[2] = colour[4],

   $Milk is drunk in the middle house.
   drink[3] = 3,

   $The Norwegian lives in the first house
   nationality[4] = 1,

   $The man who smokes Chesterfields lives in
   $the house next to the man with the fox.
   |smoke[3] - pets[3]| = 1,

   $Kools are smoked in the house next
   $ to the house where the horse is kept.
   |smoke[2] - pets[4]| = 1,

   $The Lucky Strike smoker drinks orange juice.
   smoke[4] = drink[4],

   $The Japanese smokes Parliaments.
   nationality[5] = smoke[5],

   $The Norwegian lives next to the blue house.
   |nationality[4] - colour[5]| = 1

The Minion model is then:

::

   MINION 3

There are matrices named colour, nationality, drink, smoke and pets to
represent each of the objects discussed in the puzzle. They have domain
:math:`\{1,\ldots ,5\}` which represents where in the row of five houses
this object is held. There are also three auxiliary variables introduced
which are necessary for the most difficult constraints, these all have
domains :math:`\{-4,\ldots ,4\}`.

::

   **VARIABLES**
   DISCRETE colour[5] {1..5}
   DISCRETE nationality[5] {1..5}
   DISCRETE drink[5] {1..5}
   DISCRETE smoke[5] {1..5}
   DISCRETE pets[5] {1..5}

   # auxiliary variables
   DISCRETE aux0 {-4..4}
   DISCRETE aux1 {-4..4}
   DISCRETE aux2 {-4..4}

The variable order branches on each of the matrices in turn then on the
auxiliary variables. Only the matrices of variables are printed.

::

   **SEARCH**

   PRINT [colour,nationality,drink,smoke,pets]

   VARORDER [colour,nationality,drink,smoke,pets,aux0,aux1,aux2]

We will go through each constraint in turn. As usual the constraints in
Minion are in the reverse order of the Essence’ specification and the
minion matrices are indexed from 0 whereas

::

   **CONSTRAINTS**

:math:`|nationality[4] - colour[5]| = 1` becomes by counting indices
from zero: :math:`|nationality[3] - colour[4]| = 1`. This is then
decomposed as :math:`nationality[3] - colour[4] \geq aux2`,
:math:`nationality[3] - colour[4] \leq aux2` and :math:`|aux2|=1`.

::

   weightedsumgeq([1,-1], [nationality[3],colour[4]], aux2)
   weightedsumleq([1,-1], [nationality[3],colour[4]], aux2)
   abs(1,aux2)

:math:`nationality[5] = smoke[5]` becomes by counting indices from zero:
:math:`nationality[4] = smoke[4]`.

::

   eq(nationality[4], smoke[4])

:math:`drink[4] = smoke[4]` becomes by counting indices from zero:
:math:`drink[3] = smoke[3]`.

::

   eq(drink[3], smoke[3])

:math:`|smoke[2] - pets[4]| = 1` becomes by counting indices from zero:
:math:`|smoke[1] - pets[3]| = 1`. This is then decomposed as
:math:`smoke[1] - pets[3] \leq aux1`,
:math:`smoke[1] - pets[3] \geq aux1` and :math:`|aux1|=1`.

::

   weightedsumgeq([1,-1], [smoke[1],pets[3]], aux1)
   weightedsumleq([1,-1], [smoke[1],pets[3]], aux1)
   abs(1,aux1)

:math:`|smoke[3] - pets[3]| = 1` becomes by counting indices from zero:
:math:`|smoke[2] - pets[2]| = 1`. This is then decomposed as
:math:`smoke[2] - pets[2] \leq aux0`,
:math:`smoke[2] - pets[2] \geq aux0` and :math:`|aux0|=1`.

::

   weightedsumgeq([1,-1], [smoke[2],pets[2]], aux0)
   weightedsumleq([1,-1], [smoke[2],pets[2]], aux0)
   abs(1,aux0)

:math:`nationality[4] = 1` becomes by counting indices from zero:
:math:`nationality[3] = 1`.

::

   eq(1, nationality[3])

:math:`drink[3] = 3` becomes by counting indices from zero:
:math:`drink[2] = 3`.

::

   eq(3, drink[2])

:math:`smoke[2] = colour[4]` becomes by counting indices from zero:
:math:`smoke[1] = colour[3]`

::

   eq(colour[3], smoke[1])

:math:`smoke[1] = pets[2]` becomes by counting indices from zero:
:math:`smoke[0] = pets[1]`

::

   eq(pets[1], smoke[0])

:math:`colour[2] + 1 = colour[3]` becomes by counting indices from zero:
:math:`colour[1] + 1 = colour[2]`. This is decomposed as
:math:`colour[1] + 1 \leq colour[2]` and
:math:`colour[1] + 1 \geq colour[2]`.

::

   sumleq([1,colour[1]], colour[2])
   sumgeq([1,colour[1]], colour[2])

:math:`nationality[3] = drink[2]` becomes by counting indices from zero:
:math:`nationality[2] = drink[1]`

::

   eq(drink[1], nationality[2])

:math:`drink[1] = colour[2]` becomes by counting indices from zero:
:math:`drink[0] = colour[1]`

::

   eq(colour[1], drink[0])

:math:`nationality[2] = pets[1]` becomes by counting indices from zero:
:math:`nationality[1] = pets[0]`

::

   eq(nationality[1], pets[0])

:math:`nationality[1] = colour[1]` becomes by counting indices from
zero: :math:`nationality[0] = colour[0]`

::

   eq(colour[0], nationality[0])

There is an implicit all different in the problem which is placed over
all the matrices of variables.

::

   alldiff([pets])
   alldiff([smoke])
   alldiff([drink])
   alldiff([nationality])
   alldiff([colour])

   **EOF**

N-Queens
--------

N-Queens is perhaps the most famous problem in CP. It is often used to
demonstrate systems. It is stated as the problem of putting :math:`n`
chess queens on an :math:`n \times n` chessboard such that none of them
is able to capture any other using the standard chess queen’s moves. The
model we will discuss here is the column model, where there is one
variable of domain 1, .. n for each row, which is the easiest model to
describe. We will look at the version where :math:`n=4` as this has a
reasonably small number of constraints to These files can be found in
/summer_school/examples the Essence’ file is NQueensColumn.eprime and
the Minion file is NQueensColumn.minion. The Essence’ specification is
as follows:

::

   given n: int
   find queens: matrix indexed by [int(1..n)] of int(1..n)

   such that

   forall i : int(1..n). forall j : int(i+1..n).
    |queens[i] - queens[j]| != |i - j|,
    alldiff(queens),

   letting n be 4

The Minion model is then:

::

   MINION 3

There are 4 variables, each of which represents a column of the chess
board. This instance is of a :math:`4 \times 4` chessboard so there are
4 variables stored in a matrix called queens with domain
:math:`\{1,\ldots ,4\}`. There are two auxiliary variables for each of
the 6 diagonal constraints, one with domain :math:`\{-3, \ldots ,3\}`
and one with domain :math:`\{0, \ldots ,3\}`.

::

   **VARIABLES**
   DISCRETE queens[4] {1..4}

   # auxiliary variables
   DISCRETE aux0 {-3..3}
   DISCRETE aux1 {0..3}
   DISCRETE aux2 {-3..3}
   DISCRETE aux3 {0..3}
   DISCRETE aux4 {-3..3}
   DISCRETE aux5 {0..3}
   DISCRETE aux6 {-3..3}
   DISCRETE aux7 {0..3}
   DISCRETE aux8 {-3..3}
   DISCRETE aux9 {0..3}
   DISCRETE aux10 {-3..3}
   DISCRETE aux11 {0..3}

The variable order branches on each of the matrix variables in turn then
on the auxiliary variables. Only the matrix of variables is printed.

::

   **SEARCH**

   PRINT [queens]

   VARORDER [queens,
   aux0,aux1,aux2,aux3,aux4,aux5,aux6,aux7,
   aux8,aux9,aux10,aux11]

There is an all different constraint on the queens variables. This
ensures that two queens cannot be put in the same row. The other
constraints stop two queens being placed on a diagonal. These diagonal
constraints are all of the form
:math:`|queens[i] - queens[j]| \ne |i - j|`. This is decomposed into the
following: :math:`queens[i] - queens[j] = auxa`, :math:`|auxa| = auxb`
and :math:`auxb \ne constant`. As minion has no weighted sum equals the
constraint is broken into a weighted sum less than or equals to and
weighted sum greater than or equals to. So this full constraint
:math:`queens[i] - queens[j] = auxa` is represented as
:math:`queens[i] - queens[j] \leq auxa` and
:math:`queens[i] - queens[j] \geq auxa`.

::

   **CONSTRAINTS**

   weightedsumgeq([1,-1], [queens[2],queens[3]], aux0)
   weightedsumleq([1,-1], [queens[2],queens[3]], aux0)
   abs(aux1,aux0)
   weightedsumgeq([1,-1], [queens[1],queens[3]], aux2)
   weightedsumleq([1,-1], [queens[1],queens[3]], aux2)
   abs(aux3,aux2)
   weightedsumgeq([1,-1], [queens[1],queens[2]], aux4)
   weightedsumleq([1,-1], [queens[1],queens[2]], aux4)
   abs(aux5,aux4)
   diseq(2, aux3)
   weightedsumgeq([1,-1], [queens[0],queens[3]], aux6)
   weightedsumleq([1,-1], [queens[0],queens[3]], aux6)
   abs(aux7,aux6)
   weightedsumgeq([1,-1], [queens[0],queens[2]], aux8)
   weightedsumleq([1,-1], [queens[0],queens[2]], aux8)
   abs(aux9,aux8)
   weightedsumgeq([1,-1], [queens[0],queens[1]], aux10)
   weightedsumleq([1,-1], [queens[0],queens[1]], aux10)
   abs(aux11,aux10)
   diseq(3, aux7)
   diseq(2, aux9)
   diseq(1, aux1)
   diseq(1, aux5)
   diseq(1, aux11)
   alldiff([queens])

   **EOF**
