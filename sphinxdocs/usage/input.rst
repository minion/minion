Minion expects to be provided with the name of an input file as an
argument. This file contains a specification of the CSP to be solved as
well as settings that the search process should use. The format is

Minion3Input::= MINION 3
   <InputSection>+ **EOF**

InputSection::= <VariablesSection>
   | <SearchSection>
   | <ConstraintsSection>
   | <TuplelistSection>
   | <ShortTuplelistSection>

i.e. 'MINION 3' followed by any number of variable, search, constraints
and tuplelists sections (can repeat) followed by '**EOF**', the end of
file marker.

All text from a '#' character to the end of the line is ignored.

See the associated help entries below for information on each section.

You can give an input file via standard input by specifying '--' as the
file name, this can be used when minion is being used as a tool in a
shell script or for compressed input, e.g.,

   gunzip -c myinput.minion.gz \| minion

Variables
=========

The variables section consists of any number of variable declarations on
separate lines.

VariablesSection::= **VARIABLES**
   <VarDeclaration>\*

Example
-------

.. code:: 

   **VARIABLES**

   BOOL bool                          #boolean var
   BOUND b {1..3}                     #bounds var
   SPARSEBOUND myvar {1,3,4,6,7,9,11} #sparse bounds var
   DISCRETE d[3] {1..3}               #array of discrete vars

Minion supports 4 different variable types, namely

-  0/1 variables,
-  bounds variables,
-  sparse bounds variables, and
-  discrete variables.

Sub-dividing the variable types in this manner affords the greatest
opportunity for optimisation. In general, we recommend thinking of the
variable types as a hierarchy, where 1 (0/1 variables) is the most
efficient type, and 4 (Discrete variables) is the least. The user should
use the variable which is the highest in the hierarchy, yet encompasses
enough information to provide a full model for the problem they are
attempting to solve.

Minion also supports use of constants in place of variables, and
constant vectors in place of vectors of variables. Using constants will
be at least as efficient as using variables when the variable has a
singleton domain.

Constants
~~~~~~~~~

Minion supports the use of constants anywhere where a variable can be
used. For example, in a constraint as a replacement for a single
variable, or a vector of constants as a replacement for a vector of
variables.

Examples
--------

Use of a constant:

::

   eq(x,1)

Use of a constant vector:

::

   element([10,9,8,7,6,5,4,3,2,1],idx,e)

Vectors
~~~~~~~

Vectors, matrices and tensors can be declared in minion input. Matrices
and tensors are for convenience, as constraints do not take these as
input; they must first undergo a flattening process to convert them to a
vector before use. Additional commas at the end of vectors are ignored
(see example below).

.. _examples-1:

Examples
--------

A vector of 0/1 variables:

::

   BOOL myvec[5]

A matrix of discrete variables:

::

   DISCRETE sudoku[9,9] {1..9}

A 3D tensor of 0/1s:

::

   BOOL mycube[3,3,2]

One can create a vector from scalars and elements of vectors, etc.:

::

   alldiff([x,y,myvec[1],mymatrix[3,4]])

When a matrix or tensor is constrained, it is treated as a vector whose
entries have been strung out into a vector in index order with the
rightmost index changing most quickly, e.g.:

::

   alldiff(sudoku)

is equivalent to

   alldiff([sudoku[0,0],...,sudoku[0,8],...,sudoku[8,0],...,sudoku[8,8]]):

Furthermore, with indices filled selectively and the remainder filled
with underscores (_) the flattening applies only to the underscore
indices:

::

   alldiff(sudoku[4,_])

is equivalent to:

::

   alldiff([sudoku[4,0],...,sudoku[4,8]])

Lastly, one can optionally add square brackets ([]) around an expression
to be flattened to make it look more like a vector:

::

   alldiff([sudoku[4,_]])

is equivalent to:

::

   alldiff(sudoku[4,_])

Additional hanging commas at the end of array are ignored, e.g.:

::

   lexleq([A,B,C,],[D,E,F,])

is equivalent to

   lexleq([A,B,C],[D,E,F]):

Aliases
~~~~~~~

Specifying an alias is a way to give a variable another name. Aliases
appear in the **VARIABLES** section of an input file. It is best
described using some examples:

::

   ALIAS c = a

   ALIAS c[2,2] = [[myvar,b[2]],[b[1],anothervar]]

Booleans
~~~~~~~~

Booleans, or 01 variablesm are used very commonly for logical
expressions, and for encoding the characteristic functions of sets and
relations. Note that wherever a 01 variable can appear, the negation of
that variable can also appear. A boolean variable x's negation is
identified by !x.

.. _examples-2:

Examples
--------

Declaration of a 01 variable called bool in input file:

::

   BOOL bool

Use of this variable in a constraint:

::

   eq(bool, 0) #variable bool equals 0

Discrete
~~~~~~~~

In discrete variables, the domain ranges between the specified lower and
upper bounds, but during search any domain value may be pruned, i.e.,
propagation and search may punch arbitrary holes in the domain.

.. _examples-3:

Examples
--------

Declaration of a discrete variable x with domain {1,2,3,4} in input
file:

::

   DISCRETE x {1..4}

Use of this variable in a constraint:

::

   eq(x, 2) #variable x equals 2

Sparse Bound -----------

In sparse bounds variables the domain is composed of discrete values
(e.g. {1, 5, 36, 92}), but only the upper and lower bounds of the domain
may be updated during search. Although the domain of these variables is
not a continuous range, any holes in the domains must be there at time
of specification, as they can not be added during the solving process.

.. _examples-4:

Examples
--------

Declaration of a sparse bounds variable called myvar containing values
{1,3,4,6,7,9,11} in input file:

::

   SPARSEBOUND myvar {1,3,4,6,7,9,11}

Use of this variable in a constraint:

::

   eq(myvar, 3) #myvar equals 3

Bounds
~~~~~~

Bounds variables, where only the upper and lower bounds of the domain
are maintained. These domains must be continuous ranges of integers i.e.
holes cannot be put in the domains of the variables.

.. _examples-5:

Examples
--------

Declaration of a bound variable called myvar with domain between 1 and 7
in input file:

::

   BOUND myvar {1..7}

Use of this variable in a constraint:

::

   eq(myvar, 4) #variable myvar equals 4

Constraints
===========

The constraints section consists of any number of constraint
declarations on separate lines.:

::

   ConstraintsSection::= **CONSTRAINTS**
                         <ConstraintDeclaration>*

Tuple Lists
~~~~~~~~~~~

A tuplelist section lists of allowed tuples for table constraints can be
specified. This technique is preferable to specifying the tuples in the
constraint declaration, since the tuplelists can be shared between
constraints and named for readability.

The required format is

TuplelistSection::= **TUPLELIST**
   <Tuplelist>\*

Tuplelist::= <name> <num_tuples> <tupleLength> <numbers>+

.. _example-1:

Example
-------

**TUPLELIST** AtMostOne 4 3 0 0 0 0 0 1 0 1 0 1 0 0

Short Tuple Lists
~~~~~~~~~~~~~~~~~

A shorttuplelist section lists of allowed tuples for haggisgac,
shortstr2, shortctuplestr2 and other constraints which accept short
tuple lists.

The required format is:

::

   TuplelistSection::= **TUPLELIST**
                       <Tuplelist>*

   Tuplelist::= <name> <num_tuples> <shortTuple>+

   shortTuple ::= [ <literal>*, ]

   literal ::= (<num>, <num>)


Example
-------

An example is given below:

::

   **SHORTTUPLELIST**
   mycon 4
   [(0,0),(3,0)]
   [(1,0),(3,0)]
   [(2,0),(3,0)]
   [(0,1),(1,1),(2,1),(3,1)]

Which represents the same constraint as:

::

   **TUPLELIST**
   mycon 8 4
   0 0 0 0
   0 0 1 0
   0 1 0 0
   0 1 1 0
   1 0 0 0
   1 0 1 0
   1 1 0 0
   1 1 1 1

Short tuples give us a way of shrinking some constraints. Short tuples
consist of pairs (x,y), where x is a varible position, and y is a value
for that variable. For example, `[(0,0),(3,0)]` represents "If the variable
at index 0 is 0, and the variable at index 3 is 0, then the constraint is true".

Some constraints (currently just shortctuplestr2) allow more than one
literal per variable for example, `[(0,0),(0,1),(3,0)]`
represents 'if the variable at index 0 is 0 or 1, and the variable at
index 3 is 0, then the constraint is true'.

Note that some tuples are double-represented in the example 'mycon'. The
first 3 short tuples all allow the assignment '0 0 0 0'. This is fine.
The important thing for efficency is to try to give a small list of
short tuples.

Related Sections
----------------

haggisgac, haggisgac-stable, tuplelist

Search
======

Inside the search section one can specify

-  variable orderings,
-  value orderings,
-  optimisation function, and
-  details of how to print out solutions.:

   ::

      SearchSection::= <VarValOrdering>*
                       <OptimisationFn>?
                       <PrintFormat>?

If no varval ordering is given then the variables are assigned in
instantiation order and the values tried in ascending order.

If a variable order is given as a command line argument it will override
anything specified in the input file.

Multiple variable orders can be given, each with an optional value
ordering:

::

   VarValOrdering::= <VarOrder>
                    <ValOrder>?

In each VarOrder an instantiation order is specified for a subset of
variables. Variables can optionally be "auxiliary variables" (add "AUX"
to the varorder) meaning that if there are several solutions to the
problem differing only in the auxiliary variables, only one is reported
by minion.:

::

   VarOrder::= VARORDER AUX? <ORDER>? [ <varname>+ ]

where:

::

   <ORDER>::= STATIC | SDF | SRF | LDF | ORIGINAL | WDEG | CONFLICT |

DOMOVERWDEG

The value ordering allows the user to specify an instantiation order for
the variables involved in the variable order, either ascending (a) or
descending (d) for each. When no value ordering is specified, the
default is to use ascending order for every search variable.:

::

   ValOrder::= VALORDER[ (a|d)+ ]

To model an optimisation problem the user can specify to minimise or
maximise a variable's value, or list of variables (under lexicographic
ordering):

::

   OptimisationFn::= MAXIMISING <varname> or <varlist>
                   | MINIMISING <varname> or <varlist>

Finally, the user can control some aspects of the way solutions are
printed. By default (no PrintFormat specified) all the variables are
printed in declaration order. Alternatively a custom vector, or ALL
variables, or no (NONE) variables can be printed. If a matrix or, more
generally, a tensor is given instead of a vector, it is automatically
flattened into a vector as described in 'help variables vectors'.:

::

   PrintFormat::= PRINT <vector>
                | PRINT ALL
                | PRINT NONE

Example
-------

Below is a complete minion input file with commentary, as an example.:

::

   MINION 3

   # While the variable section doesn't have to come first, you can't
   # really do anything until
   # You have one...
   **VARIABLES**

   # There are 4 type of variables
   BOOL bool         # Boolean don't need a domain
   BOUND b {1..3}    # Bound vars need a domain given as a range
   DISCRETE d {1..3} # So do discrete vars

   #Note: Names are case sensitive!

   # Internally, Bound variables are stored only as a lower and upper bound
   # Whereas discrete variables allow any sub-domain

   SPARSEBOUND s {1,3,6,7} # Sparse bound variables take a sorted list of values

   # We can also declare matrices of variables!

   DISCRETE q[3] {0..5} # This is a matrix with 3 variables: q[0],q[1] and q[2]
   BOOL bm[2,2] # A 2d matrix, variables bm[0,0], bm[0,1], bm[1,0], bm[1,1]
   BOOL bn[2,2,2,2] # You can have as many indices as you like!

   #The search section is entirely optional
   **SEARCH**

   # Note that everything in SEARCH is optional, and can only be given at
   # most once!

   # If you don't give an explicit variable ordering, one is generated.
   # These can take matrices in interesting ways like constraints, see below.
   VARORDER [bool,b,d]

   # If you don't give a value ordering, 'ascending' is used
   #VALORDER [a,a,a,a]

   # You can have one objective function, or none at all.
   MAXIMISING bool
   # MINIMISING x3

   # both (MAX/MIN)IMISING and (MAX/MIN)IMIZING are accepted...


   # Print statement takes a vector of things to print

   PRINT [bool, q]

   # You can also give:
   # PRINT ALL (the default)
   # PRINT NONE


   # Declare constraints in this section!
   **CONSTRAINTS**

   # Constraints are defined in exactly the same way as in MINION input
   formats 1 & 2
   eq(bool, 0)
   eq(b,d)

   # To get a single variable from a matrix, just index it
   eq(q[1],0)
   eq(bn[0,1,1,1], bm[1,1])

   # It's easy to get a row or column from a matrix. Just use _ in the
   # indices you want
   # to vary. Just giving a matrix gives all the variables in that matrix.

   #The following shows how flattening occurs...

   # [bm] == [ bm[_,_] ] == [ bm[0,0], bm[0,1], bm[1,0], bm[1,1] ]
   # [ bm[_,1] ] = [ bm[0,1], bm[1,1] ]
   # [ bn[1,_,0,_] = [ bn[1,0,0,0], b[1,0,0,1], b[1,1,0,0], b[1,1,0,1] ]

   # You can string together a list of such expressions!

   lexleq( [bn[1,_,0,_], bool, q[0]] , [b, bm, d] )

   # One minor problem.. you must always put [ ] around any matrix expression, so
   # lexleq(bm, bm) is invalid

   lexleq( [bm], [bm] ) # This is OK!

   # Can give tuplelists, which can have names!
   # The input is: <name> <numOf_tuples> <tupleLength> <numbers...>
   # The formatting can be about anything..

   **TUPLELIST**

   Fred 3 3
   0 2 3
   2 0 3
   3 1 3

   Bob 2 2 1 2 3 4

   #No need to put everything in one section! All sections can be reopened..
   **VARIABLES**

   # You can even have empty sections.. if you want

   **CONSTRAINTS**

   #Specify tables by their names..

   table([q], Fred)

   # Can still list tuples explicitally in the constraint if you want at
   # the moment.
   # On the other hand, I might remove this altogether, as it's worse than giving
   # Tuplelists

   table([q],{ <0,2,3>,<2,0,3>,<3,1,3> })

   #Must end with the **EOF** marker!

   **EOF**

   Any text down here is ignored, so you can write whatever you like (or
   nothing at all...)
