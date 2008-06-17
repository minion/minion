#include <iostream>
#include <string>
using namespace std;
void help(string request)
{
if("" == request) {
cout << "To use this help feature run the minion executable followed by help followed by the" << endl;
cout << "entry you wish to see. For example to see documentation on variables you should type:" << endl << endl;
cout << "   minion help variables" << endl << endl;
cout << "You can find out what other entries are available, if any, by looking at the 'subentries'" << endl;
cout << "section at the end of an entry." << endl << endl;
} else
if("switches" == request) {
cout << "Help entry: " << "switches" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "Minion supports a number of switches to augment default behaviour. To" << endl
<< "see more information on any switch, use the help system. The list" << endl
<< "below contains all available switches. For example to see help on" << endl
<< "-quiet type something similar to" << endl
<< "" << endl
<< " minion help switches -quiet" << endl
<< "" << endl
<< "replacing 'minion' by the name of the executable you're using." << endl << endl << endl;
} else
if("switches -findallsols" == request) {
cout << "Help entry: " << "switches -findallsols" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "Find all solutions and count them. This option is ignored if the" << endl
<< "problem contains any minimising or maximising objective." << endl << endl << endl;
} else
if("switches -quiet" == request) {
cout << "Help entry: " << "switches -quiet" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "Do not print parser progress." << endl << endl << endl;
cout << "References" << "----------------------------------------------------------------------" << endl;
cout << "help switches -verbose" << endl << endl << endl;
} else
if("switches -verbose" == request) {
cout << "Help entry: " << "switches -verbose" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "Print parser progress." << endl << endl << endl;
cout << "References" << "----------------------------------------------------------------------" << endl;
cout << "help switches -quiet" << endl << endl << endl;
} else
if("switches -printsols" == request) {
cout << "Help entry: " << "switches -printsols" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "Print solutions." << endl << endl << endl;
} else
if("switches -noprintsols" == request) {
cout << "Help entry: " << "switches -noprintsols" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "Do not print solutions." << endl << endl << endl;
} else
if("switches -printsolsonly" == request) {
cout << "Help entry: " << "switches -printsolsonly" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "Print only solutions and a summary at the end." << endl << endl << endl;
} else
if("switches -preprocess" == request) {
cout << "Help entry: " << "switches -preprocess" << endl << endl;
cout << "" << "--------------------------------------------------------------------------------" << endl;
cout << "" << endl
<< "This switch allows the user to choose what level of preprocess is" << endl
<< "applied to their model before search commences." << endl
<< "" << endl
<< "The choices are:" << endl
<< "" << endl
<< "- GAC " << endl
<< "- generalised arc consistency (default)" << endl
<< "- all propagators are run to a fixed point" << endl
<< "- if some propagators enforce less than GAC then the model will" << endl
<< "not necessarily be fully GAC at the outset" << endl
<< "" << endl
<< "- SACBounds " << endl
<< "- singleton arc consistency on the bounds of each variable" << endl
<< "- AC can be achieved when any variable lower or upper bound is a " << endl
<< "singleton in its own domain" << endl
<< "" << endl
<< "- SAC " << endl
<< "- singleton arc consistency" << endl
<< "- AC can be achieved in the model if any value is a singleton in" << endl
<< "its own domain" << endl
<< "" << endl
<< "- SSACBounds" << endl
<< "- singleton singleton bounds arc consistency" << endl
<< "- SAC can be achieved in the model when domains are replaced by either" << endl
<< "the singleton containing their upper bound, or the singleton containing" << endl
<< "their lower bound" << endl
<< "" << endl
<< "- SSAC " << endl
<< "- singleton singleton arc consistency" << endl
<< "- SAC can be achieved when any value is a singleton in its own domain" << endl
<< "" << endl
<< "These are listed in order of roughly how long they take to" << endl
<< "achieve. Preprocessing is a one off cost at the start of search. The" << endl
<< "success of higher levels of preprocessing is problem specific; SAC" << endl
<< "preprocesses may take a long time to complete, but may reduce search" << endl
<< "time enough to justify the cost." << endl << endl << endl;
cout << "Example" << "-------------------------------------------------------------------------" << endl;
cout << "To enforce SAC before search:" << endl
<< "" << endl
<< " minion -preprocess SAC myinputfile.minion" << endl << endl << endl;
cout << "References" << "----------------------------------------------------------------------" << endl;
cout << "help switches -X-prop-node" << endl << endl << endl;
} else
if("switches -X-prop-node" == request) {
cout << "Help entry: " << "switches -X-prop-node" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "Allows the user to choose the level of consistency to be enforced" << endl
<< "during search." << endl
<< "" << endl
<< "See entry 'help switches -preprocess' for details of the available" << endl
<< "levels of consistency." << endl << endl << endl;
cout << "Example" << "-------------------------------------------------------------------------" << endl;
cout << "To enforce SSAC during search:" << endl
<< "" << endl
<< " minion -X-prop-node SSAC input.minion" << endl << endl << endl;
cout << "References" << "----------------------------------------------------------------------" << endl;
cout << "help switches -preprocess" << endl << endl << endl;
} else
if("switches -dumptree" == request) {
cout << "Help entry: " << "switches -dumptree" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "Print out the branching decisions and variable states at each node." << endl << endl << endl;
} else
if("switches -fullprop" == request) {
cout << "Help entry: " << "switches -fullprop" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "Disable incremental propagation." << endl << endl << endl;
cout << "Notes" << "---------------------------------------------------------------------------" << endl;
cout << "This should always slow down search while producing exactly the same" << endl
<< "search tree." << endl
<< "" << endl
<< "Only available in a DEBUG executable." << endl << endl << endl;
} else
if("switches -nocheck" == request) {
cout << "Help entry: " << "switches -nocheck" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "Do not check solutions for correctness before printing them out." << endl << endl << endl;
cout << "Notes" << "---------------------------------------------------------------------------" << endl;
cout << "This option is the default on non-DEBUG executables." << endl << endl << endl;
} else
if("switches -check" == request) {
cout << "Help entry: " << "switches -check" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "Check solutions for correctness before printing them out." << endl << endl << endl;
cout << "Notes" << "---------------------------------------------------------------------------" << endl;
cout << "This option is the default for DEBUG executables." << endl << endl << endl;
} else
if("switches -nodelimit" == request) {
cout << "Help entry: " << "switches -nodelimit" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "To stop search after N nodes, do" << endl
<< "" << endl
<< " minion -nodelimit N myinput.minion" << endl << endl << endl;
cout << "References" << "----------------------------------------------------------------------" << endl;
cout << "help switches -timelimit" << endl
<< "help switches -sollimit" << endl << endl << endl;
} else
if("switches -timelimit" == request) {
cout << "Help entry: " << "switches -timelimit" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "To stop search after N seconds, do" << endl
<< "" << endl
<< " minion -timelimit N myinput.minion" << endl << endl << endl;
cout << "References" << "----------------------------------------------------------------------" << endl;
cout << "help switches -nodelimit" << endl
<< "help switches -sollimit" << endl << endl << endl;
} else
if("switches -sollimit" == request) {
cout << "Help entry: " << "switches -sollimit" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "To stop search after N solutions have been found, do" << endl
<< "" << endl
<< " minion -sollimit N myinput.minion" << endl << endl << endl;
cout << "References" << "----------------------------------------------------------------------" << endl;
cout << "help switches -nodelimit" << endl
<< "help switches -timelimit" << endl << endl << endl;
} else
if("switches -varorder" == request) {
cout << "Help entry: " << "switches -varorder" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "" << endl
<< "Enable a particular variable ordering for the search process. This" << endl
<< "flag is experimental and minion's default ordering might be faster." << endl
<< "" << endl
<< "The available orders are:" << endl
<< "" << endl
<< "- sdf - smallest domain first, break ties lexicographically" << endl
<< "" << endl
<< "- sdf-random - sdf, but break ties randomly" << endl
<< "" << endl
<< "- srf - smallest ratio first, chooses unassigned variable with smallest" << endl
<< " percentage of its initial values remaining, break ties lexicographically" << endl
<< "" << endl
<< "- srf-random - srf, but break ties randomly" << endl
<< "" << endl
<< "- ldf - largest domain first, break ties lexicographically" << endl
<< "" << endl
<< "- ldf-random - ldf, but break ties randomly" << endl
<< "" << endl
<< "- random - random variable ordering" << endl
<< "" << endl
<< "- static - lexicographical ordering" << endl << endl << endl;
} else
if("switches -randomseed" == request) {
cout << "Help entry: " << "switches -randomseed" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "Set the pseudorandom seed to N. This allows 'random' behaviour to be" << endl
<< "repeated in different runs of minion." << endl << endl << endl;
} else
if("switches -tableout" == request) {
cout << "Help entry: " << "switches -tableout" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "Append a line of data about the current run of minion to a named file." << endl
<< "This data includes minion version information, arguments to the" << endl
<< "executable, build and solve time statistics, etc. See the file itself" << endl
<< "for a precise schema of the supplied information." << endl << endl << endl;
cout << "Example" << "-------------------------------------------------------------------------" << endl;
cout << "To add statistics about solving myproblem.minion to mystats.txt do" << endl
<< "" << endl
<< " minion -tableout mystats.txt myproblem.minion" << endl << endl << endl;
} else
if("switches -solsout" == request) {
cout << "Help entry: " << "switches -solsout" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "Append all solutionsto a named file." << endl
<< "Each solution is placed on a line, with no extra formatting." << endl << endl << endl;
cout << "Example" << "-------------------------------------------------------------------------" << endl;
cout << "To add the solutions of myproblem.minion to mysols.txt do" << endl
<< "" << endl
<< " minion -solsout mysols.txt myproblem.minion" << endl << endl << endl;
} else
if("switches -randomiseorder" == request) {
cout << "Help entry: " << "switches -randomiseorder" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "Randomises the ordering of the decision variables. If the input file" << endl
<< "specifies as ordering it will randomly permute this. If no ordering is" << endl
<< "specified a random permutation of all the variables is used." << endl << endl << endl;
} else
if("constraints element_one" == request) {
cout << "Help entry: " << "constraints element_one" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "The constraint element one is identical to element, except that the" << endl
<< "vector is indexed from 1 rather than from 0." << endl << endl << endl;
cout << "References" << "----------------------------------------------------------------------" << endl;
cout << "See" << endl
<< "" << endl
<< " help constraints element" << endl
<< "" << endl
<< "for details of the element constraint which is almost identical to this" << endl
<< "one." << endl << endl << endl;
} else
if("constraints element" == request) {
cout << "Help entry: " << "constraints element" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "The constraint " << endl
<< "" << endl
<< " element(vec, i, e)" << endl
<< "" << endl
<< "specifies that, in any solution, vec[i] = e." << endl << endl << endl;
cout << "Reifiability" << "--------------------------------------------------------------------" << endl;
cout << "This constraint is not reifiable." << endl << endl << endl;
cout << "Notes" << "---------------------------------------------------------------------------" << endl;
cout << "" << endl
<< "Warning: This constraint is not confluent. Depending on the order the" << endl
<< "propagators are called in Minion, the number of search nodes may vary when" << endl
<< "using element. To avoid this problem, use watchelement instead. More details" << endl
<< "below." << endl
<< "" << endl
<< "The level of propagation enforced by this constraint is not named, however it" << endl
<< "works as follows. For constraint vec[i]=e:" << endl
<< "" << endl
<< "- After i is assigned, ensures that min(vec[i]) = min(e) and " << endl
<< " max(vec[i]) = max(e)." << endl
<< "" << endl
<< "- When e is assigned, removes idx from the domain of i whenever e is not an" << endl
<< " element of the domain of vec[idx]." << endl
<< "" << endl
<< "- When m[idx] is assigned, removes idx from i when m[idx] is not in the domain" << endl
<< " of e." << endl
<< "" << endl
<< "This level of consistency is designed to avoid the propagator having to scan" << endl
<< "through vec, except when e is assigned. It does a quantity of cheap propagation" << endl
<< "and may work well in practise on certain problems." << endl
<< "" << endl
<< "Element is not confluent, which may cause the number of search nodes to vary" << endl
<< "depending on the order in which constraints are listed in the input file, or " << endl
<< "the order they are called in Minion. For example, the following input causes" << endl
<< "Minion to search 41 nodes." << endl
<< "" << endl
<< "MINION 3" << endl
<< "**VARIABLES**" << endl
<< "DISCRETE x[5] {1..5}" << endl
<< "**CONSTRAINTS**" << endl
<< "element([x[0],x[1],x[2]], x[3], x[4])" << endl
<< "alldiff([x])" << endl
<< "**EOF**" << endl
<< "" << endl
<< "However if the two constraints are swapped over, Minion explores 29 nodes." << endl
<< "As a rule of thumb, to get a lower node count, move element constraints" << endl
<< "to the end of the list." << endl << endl << endl;
cout << "References" << "----------------------------------------------------------------------" << endl;
cout << "See the entry " << endl
<< "" << endl
<< " constraints watchelement" << endl
<< "" << endl
<< "for details of an identical constraint that enforces generalised arc" << endl
<< "consistency." << endl << endl << endl;
} else
if("constraints pow" == request) {
cout << "Help entry: " << "constraints pow" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "The constraint" << endl
<< " " << endl
<< " pow([x,y],z)" << endl
<< "" << endl
<< "ensures that x^y=z." << endl << endl << endl;
cout << "Notes" << "---------------------------------------------------------------------------" << endl;
cout << "This constraint is only available for positive domains x, y and z." << endl << endl << endl;
cout << "Reifiability" << "--------------------------------------------------------------------" << endl;
cout << "Not reifiable." << endl << endl << endl;
} else
if("constraints product" == request) {
cout << "Help entry: " << "constraints product" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "The constraint" << endl
<< "" << endl
<< " product(x,y,z)" << endl
<< "" << endl
<< "ensures that z=xy in any solution." << endl << endl << endl;
cout << "Notes" << "---------------------------------------------------------------------------" << endl;
cout << "This constraint can be used for (and, in fact, has a specialised" << endl
<< "implementation for) achieving boolean AND, i.e. x & y=z can be modelled" << endl
<< "as" << endl
<< "" << endl
<< " product(x,y,z)" << endl
<< "" << endl
<< "The general constraint achieves bounds generalised arc consistency for" << endl
<< "positive numbers." << endl << endl << endl;
cout << "Reifiability" << "--------------------------------------------------------------------" << endl;
cout << "This constraint is not reifiable." << endl << endl << endl;
} else
if("constraints div" == request) {
cout << "Help entry: " << "constraints div" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "The constraint" << endl
<< " " << endl
<< " div(x,y,z)" << endl
<< "" << endl
<< "ensures that floor(x/y)=z." << endl << endl << endl;
cout << "Notes" << "---------------------------------------------------------------------------" << endl;
cout << "This constraint is only available for positive domains x, y and z." << endl << endl << endl;
cout << "Reifiability" << "--------------------------------------------------------------------" << endl;
cout << "Not reifiable." << endl << endl << endl;
cout << "References" << "----------------------------------------------------------------------" << endl;
cout << "help constraints modulo" << endl << endl << endl;
} else
if("constraints eq" == request) {
cout << "Help entry: " << "constraints eq" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "Constrain two variables to take equal values." << endl << endl << endl;
cout << "Example" << "-------------------------------------------------------------------------" << endl;
cout << "eq(x0,x1)" << endl << endl << endl;
cout << "Notes" << "---------------------------------------------------------------------------" << endl;
cout << "Achieves bounds consistency." << endl << endl << endl;
cout << "Reifiability" << "--------------------------------------------------------------------" << endl;
cout << "This constraint is reifiable." << endl << endl << endl;
cout << "Reference" << "-----------------------------------------------------------------------" << endl;
cout << "help constraints minuseq" << endl << endl << endl;
} else
if("constraints minuseq" == request) {
cout << "Help entry: " << "constraints minuseq" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "Constraint" << endl
<< "" << endl
<< " minuseq(x,y)" << endl
<< "" << endl
<< "ensures that x=-y." << endl << endl << endl;
cout << "Reifiability" << "--------------------------------------------------------------------" << endl;
cout << "This constraint is reifiable." << endl << endl << endl;
cout << "Reference" << "-----------------------------------------------------------------------" << endl;
cout << "help constraints eq" << endl << endl << endl;
} else
if("constraints diseq" == request) {
cout << "Help entry: " << "constraints diseq" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "Constrain two variables to take different values." << endl << endl << endl;
cout << "Notes" << "---------------------------------------------------------------------------" << endl;
cout << "Achieves arc consistency." << endl << endl << endl;
cout << "Example" << "-------------------------------------------------------------------------" << endl;
cout << "diseq(v0,v1)" << endl << endl << endl;
cout << "Reifiability" << "--------------------------------------------------------------------" << endl;
cout << "This constraint is reifiable." << endl << endl << endl;
} else
if("constraints table" == request) {
cout << "Help entry: " << "constraints table" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "An extensional constraint that enforces GAC. The constraint is" << endl
<< "specified via a list of tuples." << endl << endl << endl;
cout << "Example" << "-------------------------------------------------------------------------" << endl;
cout << "To specify a constraint over 3 variables that allows assignments" << endl
<< "(0,0,0), (1,0,0), (0,1,0) or (0,0,1) do the following." << endl
<< "" << endl
<< "1) Add a tuplelist to the **TUPLELIST** section, e.g.:" << endl
<< "" << endl
<< "**TUPLELIST**" << endl
<< "myext 4 3" << endl
<< "0 0 0" << endl
<< "1 0 0" << endl
<< "0 1 0" << endl
<< "0 0 1" << endl
<< "" << endl
<< "N.B. the number 4 is the number of tuples in the constraint, the " << endl
<< "number 3 is the -arity." << endl
<< "" << endl
<< "2) Add a table constraint to the **CONSTRAINTS** section, e.g.:" << endl
<< "" << endl
<< "**CONSTRAINTS**" << endl
<< "table(myvec, myext)" << endl
<< "" << endl
<< "and now the variables of myvec will satisfy the constraint myext." << endl << endl << endl;
cout << "Example" << "-------------------------------------------------------------------------" << endl;
cout << "The constraints extension can also be specified in the constraint" << endl
<< "definition, e.g.:" << endl
<< "" << endl
<< "table(myvec, {<0,0,0>,<1,0,0>,<0,1,0>,<0,0,1>})" << endl << endl << endl;
cout << "References" << "----------------------------------------------------------------------" << endl;
cout << "help input tuplelist" << endl << endl << endl;
} else
if("constraints occurrence" == request) {
cout << "Help entry: " << "constraints occurrence" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "The constraint" << endl
<< "" << endl
<< " occurrence(vec, elem, count)" << endl
<< "" << endl
<< "ensures that there are count occurrences of the value elem in the" << endl
<< "vector vec." << endl << endl << endl;
cout << "Notes" << "---------------------------------------------------------------------------" << endl;
cout << "elem must be a constant, not a variable." << endl << endl << endl;
cout << "Reifiability" << "--------------------------------------------------------------------" << endl;
cout << "This constraint is not reifiable." << endl << endl << endl;
cout << "References" << "----------------------------------------------------------------------" << endl;
cout << "help constraints occurrenceleq" << endl
<< "help constraints occurrencegeq" << endl << endl << endl;
} else
if("constraints occurrenceleq" == request) {
cout << "Help entry: " << "constraints occurrenceleq" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "The constraint" << endl
<< "" << endl
<< " occurrenceleq(vec, elem, count)" << endl
<< "" << endl
<< "ensures that there are AT MOST count occurrences of the value elem in" << endl
<< "the vector vec." << endl << endl << endl;
cout << "Notes" << "---------------------------------------------------------------------------" << endl;
cout << "elem must be a constant" << endl << endl << endl;
cout << "Reifiability" << "--------------------------------------------------------------------" << endl;
cout << "This constraint is not reifiable." << endl << endl << endl;
cout << "References" << "----------------------------------------------------------------------" << endl;
cout << "help constraints occurrence" << endl
<< "help constraints occurrencegeq" << endl << endl << endl;
} else
if("constraints occurrencegeq" == request) {
cout << "Help entry: " << "constraints occurrencegeq" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "The constraint" << endl
<< "" << endl
<< " occurrencegeq(vec, elem, count)" << endl
<< "" << endl
<< "ensures that there are AT LEAST count occurrences of the value elem in" << endl
<< "the vector vec." << endl << endl << endl;
cout << "Notes" << "---------------------------------------------------------------------------" << endl;
cout << "elem must be a constant" << endl << endl << endl;
cout << "Reifiability" << "--------------------------------------------------------------------" << endl;
cout << "This constraint is not reifiable." << endl << endl << endl;
cout << "References" << "----------------------------------------------------------------------" << endl;
cout << "help constraints occurrence" << endl
<< "help constraints occurrenceleq" << endl << endl << endl;
} else
if("constraints alldiffgacslow" == request) {
cout << "Help entry: " << "constraints alldiffgacslow" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "Forces the input vector of variables to take distinct values." << endl << endl << endl;
cout << "Example" << "-------------------------------------------------------------------------" << endl;
cout << "Suppose the input file had the following vector of variables defined:" << endl
<< "" << endl
<< "DISCRETE myVec[9] {1..9}" << endl
<< "" << endl
<< "To ensure that each variable takes a different value include the" << endl
<< "following constraint:" << endl
<< "" << endl
<< "alldiffgacslow(myVec)" << endl << endl << endl;
cout << "Reifiability" << "--------------------------------------------------------------------" << endl;
cout << "Not reifiable." << endl << endl << endl;
cout << "Notes" << "---------------------------------------------------------------------------" << endl;
cout << "This constraint enforces generalised arc consistency." << endl << endl << endl;
} else
if("constraints" == request) {
cout << "Help entry: " << "constraints" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "Minion supports many constraints and these are regularly being" << endl
<< "improved and added to. In some cases multiple implementations of the" << endl
<< "same constraints are provided and we would appreciate additional" << endl
<< "feedback on their relative merits in your problem." << endl
<< "" << endl
<< "Minion does not support nesting of constraints, however this can be" << endl
<< "achieved by auxiliary variables and reification." << endl
<< "" << endl
<< "Variables can be replaced by constants. You can find out more on" << endl
<< "expressions for variables, vectors, etc. in the section on variables." << endl << endl << endl;
cout << "References" << "----------------------------------------------------------------------" << endl;
cout << "help variables" << endl << endl << endl;
} else
if("constraints weightedsumleq" == request) {
cout << "Help entry: " << "constraints weightedsumleq" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "The constraint" << endl
<< "" << endl
<< " weightedsumleq(constantVec, varVec, total)" << endl
<< "" << endl
<< "ensures that constantVec.varVec >= total, where constantVec.varVec is" << endl
<< "the scalar dot product of constantVec and varVec." << endl << endl << endl;
cout << "References" << "----------------------------------------------------------------------" << endl;
cout << "help constraints weightedsumgeq" << endl
<< "help constraints sumleq" << endl
<< "help constraints sumgeq" << endl << endl << endl;
} else
if("constraints weightedsumgeq" == request) {
cout << "Help entry: " << "constraints weightedsumgeq" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "The constraint" << endl
<< "" << endl
<< " weightedsumgeq(constantVec, varVec, total)" << endl
<< "" << endl
<< "ensures that constantVec.varVec <= total, where constantVec.varVec is" << endl
<< "the scalar dot product of constantVec and varVec." << endl << endl << endl;
cout << "References" << "----------------------------------------------------------------------" << endl;
cout << "help constraints weightedsumleq" << endl
<< "help constraints sumleq" << endl
<< "help constraints sumgeq" << endl << endl << endl;
} else
if("constraints reify" == request) {
cout << "Help entry: " << "constraints reify" << endl << endl;
cout << "References" << "----------------------------------------------------------------------" << endl;
cout << "See" << endl
<< " help constraints reification" << endl << endl << endl;
} else
if("constraints reifyimply" == request) {
cout << "Help entry: " << "constraints reifyimply" << endl << endl;
cout << "References" << "----------------------------------------------------------------------" << endl;
cout << "See" << endl
<< " help constraints reification" << endl << endl << endl;
} else
if("constraints reification" == request) {
cout << "Help entry: " << "constraints reification" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "Reification is provided in two forms: reify and reifyimply." << endl
<< "" << endl
<< " reify(constraint, r) where r is a 0/1 var" << endl
<< "" << endl
<< "ensures that r is set to 1 if and only if constraint is satisfied. That is, if r" << endl
<< "is 0 the constraint must NOT be satisfied; and if r is 1 it must be satisfied as" << endl
<< "normal. Conversely, if the constraint is satisfied then r must be 1, and if not" << endl
<< "then r must be 0." << endl
<< "" << endl
<< " reifyimply(constraint, r)" << endl
<< "" << endl
<< "only checks that if r is set to 1 then constraint must be satisfied. If r is not" << endl
<< "1, constraint may be either satisfied or unsatisfied. Furthermore r is never set" << endl
<< "by propagation, only by search; that is, satisfaction of constraint does not" << endl
<< "affect the value of r." << endl << endl << endl;
cout << "Notes" << "---------------------------------------------------------------------------" << endl;
cout << "Not all constraints are reifiable. Entries for individual constraints give" << endl
<< "more information." << endl << endl << endl;
} else
if("constraints modulo" == request) {
cout << "Help entry: " << "constraints modulo" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "The constraint" << endl
<< " " << endl
<< " modulo([x,y],z)" << endl
<< "" << endl
<< "ensures that x%y=z i.e. z is the remainder of dividing x by y." << endl << endl << endl;
cout << "Notes" << "---------------------------------------------------------------------------" << endl;
cout << "This constraint is only available for positive domains x, y and z." << endl << endl << endl;
cout << "Reifiability" << "--------------------------------------------------------------------" << endl;
cout << "Not reifiable." << endl << endl << endl;
cout << "References" << "----------------------------------------------------------------------" << endl;
cout << "help constraints div" << endl << endl << endl;
} else
if("constraints lexless" == request) {
cout << "Help entry: " << "constraints lexless" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "The constraint" << endl
<< "" << endl
<< " lexless(vec0, vec1)" << endl
<< "" << endl
<< "takes two vectors vec0 and vec1 of the same length and ensures that" << endl
<< "vec0 is lexicographically less than vec1 in any solution." << endl << endl << endl;
cout << "Notes" << "---------------------------------------------------------------------------" << endl;
cout << "This constraint maintains GAC." << endl << endl << endl;
cout << "Reifiability" << "--------------------------------------------------------------------" << endl;
cout << "This constraint is reifiable." << endl << endl << endl;
cout << "References" << "----------------------------------------------------------------------" << endl;
cout << "See also" << endl
<< "" << endl
<< " help constraints lexleq" << endl
<< "" << endl
<< "for a similar constraint with non-strict lexicographic inequality." << endl << endl << endl;
} else
if("constraints lexleq" == request) {
cout << "Help entry: " << "constraints lexleq" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "The constraint" << endl
<< "" << endl
<< " lexleq(vec0, vec1)" << endl
<< "" << endl
<< "takes two vectors vec0 and vec1 of the same length and ensures that" << endl
<< "vec0 is lexicographically less than or equal to vec1 in any solution." << endl << endl << endl;
cout << "Notes" << "---------------------------------------------------------------------------" << endl;
cout << "This constraints achieves GAC." << endl << endl << endl;
cout << "Reifiability" << "--------------------------------------------------------------------" << endl;
cout << "This constraint is reifiable." << endl << endl << endl;
cout << "References" << "----------------------------------------------------------------------" << endl;
cout << "See also" << endl
<< "" << endl
<< " help constraints lexless" << endl
<< "" << endl
<< "for a similar constraint with strict lexicographic inequality." << endl << endl << endl;
} else
if("constraints sumleq" == request) {
cout << "Help entry: " << "constraints sumleq" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "The constraint" << endl
<< "" << endl
<< " sumleq(vec, c)" << endl
<< "" << endl
<< "ensures that sum(vec) <= c." << endl << endl << endl;
cout << "Reifiability" << "--------------------------------------------------------------------" << endl;
cout << "This constrait is reifiable." << endl << endl << endl;
} else
if("constraints sumgeq" == request) {
cout << "Help entry: " << "constraints sumgeq" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "The constraint" << endl
<< "" << endl
<< " sumgeq(vec, c)" << endl
<< "" << endl
<< "ensures that sum(vec) >= c." << endl << endl << endl;
} else
if("constraints ineq" == request) {
cout << "Help entry: " << "constraints ineq" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "The constraint" << endl
<< "" << endl
<< " ineq(x, y, k)" << endl
<< "" << endl
<< "ensures that " << endl
<< "" << endl
<< " x <= y + k " << endl
<< "" << endl
<< "in any solution." << endl << endl << endl;
cout << "Notes" << "---------------------------------------------------------------------------" << endl;
cout << "Minion has no strict inequality (<) constraints. However x < y can be" << endl
<< "achieved by" << endl
<< "" << endl
<< " ineq(x, y, -1)" << endl << endl << endl;
cout << "Reifiability" << "--------------------------------------------------------------------" << endl;
cout << "This constraint is reifiable." << endl << endl << endl;
} else
if("constraints alldiff" == request) {
cout << "Help entry: " << "constraints alldiff" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "Forces the input vector of variables to take distinct values." << endl << endl << endl;
cout << "Example" << "-------------------------------------------------------------------------" << endl;
cout << "Suppose the input file had the following vector of variables defined:" << endl
<< "" << endl
<< "DISCRETE myVec[9] {1..9}" << endl
<< "" << endl
<< "To ensure that each variable takes a different value include the" << endl
<< "following constraint:" << endl
<< "" << endl
<< "alldiff(myVec)" << endl << endl << endl;
cout << "Notes" << "---------------------------------------------------------------------------" << endl;
cout << "Enforces the same level of consistency as a clique of not equals " << endl
<< "constraints." << endl << endl << endl;
cout << "Reifiability" << "--------------------------------------------------------------------" << endl;
cout << "Not reifiable." << endl << endl << endl;
cout << "References" << "----------------------------------------------------------------------" << endl;
cout << "See" << endl
<< "" << endl
<< " help constraints alldiffgacslow" << endl
<< "" << endl
<< "for the same constraint that enforces GAC." << endl << endl << endl;
} else
if("constraints max" == request) {
cout << "Help entry: " << "constraints max" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "The constraint" << endl
<< "" << endl
<< " max(vec, x)" << endl
<< "" << endl
<< "ensures that x is equal to the maximum value of any variable in vec." << endl << endl << endl;
cout << "Reifiability" << "--------------------------------------------------------------------" << endl;
cout << "This constraint is not reifiable." << endl << endl << endl;
cout << "References" << "----------------------------------------------------------------------" << endl;
cout << "See" << endl
<< "" << endl
<< " help constraints min" << endl
<< "" << endl
<< "for the opposite constraint." << endl << endl << endl;
} else
if("constraints min" == request) {
cout << "Help entry: " << "constraints min" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "The constraint" << endl
<< "" << endl
<< " min(vec, x)" << endl
<< "" << endl
<< "ensures that x is equal to the minimum value of any variable in vec." << endl << endl << endl;
cout << "Reifiability" << "--------------------------------------------------------------------" << endl;
cout << "This constraint is not reifiable." << endl << endl << endl;
cout << "References" << "----------------------------------------------------------------------" << endl;
cout << "See" << endl
<< "" << endl
<< " help constraints max" << endl
<< "" << endl
<< "for the opposite constraint." << endl << endl << endl;
} else
if("constraints difference" == request) {
cout << "Help entry: " << "constraints difference" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "The constraint" << endl
<< "" << endl
<< " difference(x,y,z)" << endl
<< "" << endl
<< "ensures that z=|x-y| in any solution." << endl << endl << endl;
} else
if("constraints product" == request) {
cout << "Help entry: " << "constraints product" << endl << endl;
cout << "Notes" << "---------------------------------------------------------------------------" << endl;
cout << "This constraint can be expressed in a much longer form, this form both avoids requiring an extra" << endl
<< "variable, and also gets better propagation. It gets bounds consistency." << endl << endl << endl;
cout << "Reifiability" << "--------------------------------------------------------------------" << endl;
cout << "This constraint is not reifiable." << endl << endl << endl;
} else
if("input" == request) {
cout << "Help entry: " << "input" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "" << endl
<< "Minion expects to be provided with the name of an input file as an" << endl
<< "argument. This file contains a specification of the CSP to be solved" << endl
<< "as well as settings that the search process should use. The format is" << endl
<< "" << endl
<< "Minion3Input::= MINION 3" << endl
<< " <InputSection>+" << endl
<< " **EOF**" << endl
<< "" << endl
<< "InputSection::= <VariablesSection> " << endl
<< " | <SearchSection>" << endl
<< " | <ConstraintsSection> " << endl
<< " | <TuplelistSection>" << endl
<< "" << endl
<< "i.e. 'MINION 3' followed by any number of variable, search," << endl
<< "constraints and tuplelists sections (can repeat) followed by" << endl
<< "'**EOF**', the end of file marker." << endl
<< "" << endl
<< "All text from a '#' character to the end of the line is ignored." << endl
<< "" << endl
<< "See the associated help entries below for information on each section." << endl << endl << endl;
cout << "Notes" << "---------------------------------------------------------------------------" << endl;
cout << "You can give an input file via standard input by specifying '--' as the file" << endl
<< "name, this might help when minion is being used as a tool in a shell script or" << endl
<< "for compressed input, e.g.," << endl
<< "" << endl
<< " gunzip -c myinput.minion.gz | minion" << endl << endl << endl;
} else
if("input variables" == request) {
cout << "Help entry: " << "input variables" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "The variables section consists of any number of variable declarations" << endl
<< "on separate lines." << endl
<< "" << endl
<< "VariablesSection::= **VARIABLES**" << endl
<< " <VarDeclaration>*" << endl << endl << endl;
cout << "Example" << "-------------------------------------------------------------------------" << endl;
cout << "**VARIABLES**" << endl
<< "" << endl
<< "BOOL bool #boolean var" << endl
<< "BOUND b {1..3} #bounds var" << endl
<< "SPARSEBOUND myvar {1,3,4,6,7,9,11} #sparse bounds var" << endl
<< "DISCRETE d[3] {1..3} #array of discrete vars" << endl << endl << endl;
cout << "References" << "----------------------------------------------------------------------" << endl;
cout << "See the help section" << endl
<< "" << endl
<< " help variables" << endl
<< "" << endl
<< "for detailed information on variable declarations." << endl << endl << endl;
} else
if("input constraints" == request) {
cout << "Help entry: " << "input constraints" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "" << endl
<< "The constraints section consists of any number of constraint" << endl
<< "declarations on separate lines." << endl
<< "" << endl
<< "ConstraintsSection::= **CONSTRAINTS**" << endl
<< " <ConstraintDeclaration>*" << endl << endl << endl;
cout << "Example" << "-------------------------------------------------------------------------" << endl;
cout << "**CONSTRAINTS**" << endl
<< "eq(bool,0)" << endl
<< "alldiff(d)" << endl << endl << endl;
cout << "References" << "----------------------------------------------------------------------" << endl;
cout << "See help entries for individual constraints under" << endl
<< "" << endl
<< " help constraints" << endl
<< "" << endl
<< "for details on constraint declarations." << endl << endl << endl;
} else
if("input tuplelist" == request) {
cout << "Help entry: " << "input tuplelist" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "In a tuplelist section lists of allowed tuples for table constraints" << endl
<< "can be specified. This technique is preferable to specifying the" << endl
<< "tuples in the constraint declaration, since the tuplelists can be" << endl
<< "shared between constraints and named for readability." << endl
<< "" << endl
<< "The required format is" << endl
<< "" << endl
<< "TuplelistSection::= **TUPLELIST**" << endl
<< " <Tuplelist>*" << endl
<< "" << endl
<< "Tuplelist::= <name> <num_tuples> <tuple_length> <numbers>+" << endl << endl << endl;
cout << "Example" << "-------------------------------------------------------------------------" << endl;
cout << "**TUPLELIST**" << endl
<< "AtMostOne 4 3" << endl
<< "0 0 0" << endl
<< "0 0 1" << endl
<< "0 1 0" << endl
<< "1 0 0" << endl << endl << endl;
cout << "References" << "----------------------------------------------------------------------" << endl;
cout << "help constraints table" << endl << endl << endl;
} else
if("input search" == request) {
cout << "Help entry: " << "input search" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "" << endl
<< "Inside the search section one can specify" << endl
<< "" << endl
<< "- variable orderings, " << endl
<< "- value orderings," << endl
<< "- optimisation function, and" << endl
<< "- details of how to print out solutions." << endl
<< "" << endl
<< "SearchSection::= <VariableOrdering>?" << endl
<< " <ValueOrdering>?" << endl
<< " <OptimisationFn>?" << endl
<< " <PrintFormat>?" << endl
<< "" << endl
<< "In the variable ordering a fixed ordering can be specified on any" << endl
<< "subset of variables. These are the search variables that will be" << endl
<< "instantiated in every solution. If none is specified some other fixed" << endl
<< "ordering of all the variables will be used." << endl
<< "" << endl
<< " VariableOrdering::= VARORDER[ <varname>+ ]" << endl
<< "" << endl
<< "The value ordering allows the user to specify an instantiation order" << endl
<< "for the variables involved in the variable order, either ascending (a)" << endl
<< "or descending (d) for each. When no value ordering is specified, the" << endl
<< "default is to use ascending order for every search variable." << endl
<< "" << endl
<< " ValueOrdering::= VALORDER[ (a|d)+ ]" << endl
<< "" << endl
<< "To model an optimisation problem the user can specify to minimise" << endl
<< "or maximise a variable's value." << endl
<< "" << endl
<< " OptimisationFn::= MAXIMISING <varname>" << endl
<< " | MINIMISING <varname>" << endl
<< "" << endl
<< "Finally, the user can control some aspects of the way solutions are" << endl
<< "printed. By default (no PrintFormat specified) all the variables are" << endl
<< "printed in declaration order. Alternatively a custom vector, or ALL" << endl
<< "variables, or no (NONE) variables can be printed. If a matrix or, more" << endl
<< "generally, a tensor is given instead of a vector, it is automatically" << endl
<< "flattened into a vector as described in 'help variables vectors'." << endl
<< "" << endl
<< " PrintFormat::= PRINT <vector>" << endl
<< " | PRINT ALL" << endl
<< " | PRINT NONE" << endl << endl << endl;
} else
if("input example" == request) {
cout << "Help entry: " << "input example" << endl << endl;
cout << "Example" << "-------------------------------------------------------------------------" << endl;
cout << "Below is a complete minion input file with commentary, as an example." << endl
<< "" << endl
<< "MINION 3" << endl
<< "" << endl
<< "# While the variable section doesn't have to come first, you can't " << endl
<< "# really do anything until" << endl
<< "# You have one..." << endl
<< "**VARIABLES**" << endl
<< "" << endl
<< "# There are 4 type of variables" << endl
<< "BOOL bool # Boolean don't need a domain" << endl
<< "BOUND b {1..3} # Bound vars need a domain given as a range" << endl
<< "DISCRETE d {1..3} # So do discrete vars" << endl
<< "" << endl
<< "#Note: Names are case sensitive!" << endl
<< "" << endl
<< "# Internally, Bound variables are stored only as a lower and upper bound" << endl
<< "# Whereas discrete variables allow any sub-domain" << endl
<< "" << endl
<< "SPARSEBOUND s {1,3,6,7} # Sparse bound variables take a sorted list of values" << endl
<< "" << endl
<< "# We can also declare matrices of variables!" << endl
<< "" << endl
<< "DISCRETE q[3] {0..5} # This is a matrix with 3 variables: q[0],q[1] and q[2]" << endl
<< "BOOL bm[2,2] # A 2d matrix, variables bm[0,0], bm[0,1], bm[1,0], bm[1,1]" << endl
<< "BOOL bn[2,2,2,2] # You can have as many indices as you like!" << endl
<< "" << endl
<< "#The search section is entirely optional" << endl
<< "**SEARCH**" << endl
<< "" << endl
<< "# Note that everything in SEARCH is optional, and can only be given at" << endl
<< "# most once!" << endl
<< "" << endl
<< "# If you don't give an explicit variable ordering, one is generated." << endl
<< "# These can take matrices in interesting ways like constraints, see below." << endl
<< "VARORDER [bool,b,d]" << endl
<< "" << endl
<< "# If you don't give a value ordering, 'ascending' is used" << endl
<< "#VALORDER [a,a,a,a]" << endl
<< "" << endl
<< "# You can have one objective function, or none at all." << endl
<< "MAXIMISING bool" << endl
<< "# MINIMISING x3" << endl
<< "" << endl
<< "# both (MAX/MIN)IMISING and (MAX/MIN)IMIZING are accepted..." << endl
<< "" << endl
<< "" << endl
<< "# Print statement takes a vector of things to print" << endl
<< "" << endl
<< "PRINT [bool, q]" << endl
<< "" << endl
<< "# You can also give:" << endl
<< "# PRINT ALL (the default)" << endl
<< "# PRINT NONE" << endl
<< "" << endl
<< "" << endl
<< "# Declare constraints in this section!" << endl
<< "**CONSTRAINTS**" << endl
<< "" << endl
<< "# Constraints are defined in exactly the same way as in MINION input" << endl
<< "formats 1 & 2" << endl
<< "eq(bool, 0)" << endl
<< "eq(b,d)" << endl
<< "" << endl
<< "# To get a single variable from a matrix, just index it" << endl
<< "eq(q[1],0)" << endl
<< "eq(bn[0,1,1,1], bm[1,1])" << endl
<< "" << endl
<< "# It's easy to get a row or column from a matrix. Just use _ in the" << endl
<< "# indices you want" << endl
<< "# to vary. Just giving a matrix gives all the variables in that matrix." << endl
<< "" << endl
<< "#The following shows how flattening occurs..." << endl
<< "" << endl
<< "# [bm] == [ bm[_,_] ] == [ bm[0,0], bm[0,1], bm[1,0], bm[1,1] ]" << endl
<< "# [ bm[_,1] ] = [ bm[0,1], bm[1,1] ]" << endl
<< "# [ bn[1,_,0,_] = [ bn[1,0,0,0], b[1,0,0,1], b[1,1,0,0], b[1,1,0,1] ]" << endl
<< "" << endl
<< "# You can string together a list of such expressions!" << endl
<< "" << endl
<< "lexleq( [bn[1,_,0,_], bool, q[0]] , [b, bm, d] )" << endl
<< "" << endl
<< "# One minor problem.. you must always put [ ] around any matrix expression, so" << endl
<< "# lexleq(bm, bm) is invalid" << endl
<< "" << endl
<< "lexleq( [bm], [bm] ) # This is OK!" << endl
<< "" << endl
<< "# Can give tuplelists, which can have names!" << endl
<< "# The input is: <name> <num_of_tuples> <tuple_length> <numbers...>" << endl
<< "# The formatting can be about anything.." << endl
<< "" << endl
<< "**TUPLELIST**" << endl
<< "" << endl
<< "Fred 3 3" << endl
<< "0 2 3" << endl
<< "2 0 3" << endl
<< "3 1 3" << endl
<< "" << endl
<< "Bob 2 2 1 2 3 4" << endl
<< "" << endl
<< "#No need to put everything in one section! All sections can be reopened.." << endl
<< "**VARIABLES**" << endl
<< "" << endl
<< "# You can even have empty sections.. if you want" << endl
<< "" << endl
<< "**CONSTRAINTS**" << endl
<< "" << endl
<< "#Specify tables by their names.." << endl
<< "" << endl
<< "table([q], Fred)" << endl
<< "" << endl
<< "# Can still list tuples explicitally in the constraint if you want at" << endl
<< "# the moment." << endl
<< "# On the other hand, I might remove this altogether, as it's worse than giving" << endl
<< "# Tuplelists" << endl
<< "" << endl
<< "table([q],{ <0,2,3>,<2,0,3>,<3,1,3> })" << endl
<< "" << endl
<< "#Must end with the **EOF** marker!" << endl
<< "" << endl
<< "**EOF**" << endl
<< "" << endl
<< "Any text down here is ignored, so you can write whatever you like (or" << endl
<< "nothing at all...)" << endl << endl << endl;
} else
if("constraints hamming" == request) {
cout << "Help entry: " << "constraints hamming" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "The constraint" << endl
<< "" << endl
<< " hamming(X,Y,c)" << endl
<< "" << endl
<< "ensures that the hamming distance between X and Y is c. That is, that" << endl
<< "c is the size of the set {i | X[i] != y[i]}" << endl << endl << endl;
cout << "Reifiability" << "--------------------------------------------------------------------" << endl;
cout << "This constraint is not reifiable." << endl << endl << endl;
} else
if("constraints watchvecneq" == request) {
cout << "Help entry: " << "constraints watchvecneq" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "The constraint" << endl
<< "" << endl
<< " watchvecneq(A, B)" << endl
<< "" << endl
<< "ensures that A and B are not the same vector, i.e., there exists some index i" << endl
<< "such that A[i] != B[i]." << endl << endl << endl;
cout << "Reifiability" << "--------------------------------------------------------------------" << endl;
cout << "This constraint is not reifiable." << endl << endl << endl;
} else
if("constraints watchvecexists_less" == request) {
cout << "Help entry: " << "constraints watchvecexists_less" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "The constraint" << endl
<< "" << endl
<< " watchvecexists_less(A, B)" << endl
<< "" << endl
<< "ensures that there exists some index i such that A[i] < B[i]." << endl << endl << endl;
cout << "Reifiability" << "--------------------------------------------------------------------" << endl;
cout << "This constraint is not reifiable." << endl << endl << endl;
} else
if("constraints watchvecexists_and" == request) {
cout << "Help entry: " << "constraints watchvecexists_and" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "The constraint" << endl
<< "" << endl
<< " watchvecexists_less(A, B)" << endl
<< "" << endl
<< "ensures that there exists some index i such that A[i] > 0 and B[i] > 0." << endl
<< "" << endl
<< "For booleans this is the same as 'exists i s.t. A[i] && B[i]'." << endl << endl << endl;
cout << "Reifiability" << "--------------------------------------------------------------------" << endl;
cout << "This constraint is not reifiable." << endl << endl << endl;
} else
if("constraints watchelement_one" == request) {
cout << "Help entry: " << "constraints watchelement_one" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "This constraint is identical to watchelement, except the vector" << endl
<< "is indexed from 1 rather than from 0." << endl << endl << endl;
cout << "References" << "----------------------------------------------------------------------" << endl;
cout << "See entry" << endl
<< "" << endl
<< " help constraints watchelement" << endl
<< "" << endl
<< "for details of watchelement which watchelement_one is based on." << endl << endl << endl;
} else
if("constraints watchelement" == request) {
cout << "Help entry: " << "constraints watchelement" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "The constraint " << endl
<< "" << endl
<< " watchelement(vec, i, e)" << endl
<< "" << endl
<< "specifies that, in any solution, vec[i] = e." << endl << endl << endl;
cout << "Reifiability" << "--------------------------------------------------------------------" << endl;
cout << "This constraint is NOT reifiable." << endl << endl << endl;
cout << "Notes" << "---------------------------------------------------------------------------" << endl;
cout << "Enforces generalised arc consistency." << endl << endl << endl;
cout << "References" << "----------------------------------------------------------------------" << endl;
cout << "See entry" << endl
<< "" << endl
<< " help constraints element" << endl
<< "" << endl
<< "for details of an identical constraint that enforces a lower level of" << endl
<< "consistency." << endl << endl << endl;
} else
if("constraints litsumgeq" == request) {
cout << "Help entry: " << "constraints litsumgeq" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "The constraint litsumgeq(vec1, vec2, c) ensures that there exists at least c" << endl
<< "distinct indices i such that vec1[i] = vec2[i]." << endl << endl << endl;
cout << "Notes" << "---------------------------------------------------------------------------" << endl;
cout << "A SAT clause {x,y,z} can be created using:" << endl
<< "" << endl
<< " litsumgeq([x,y,z],[1,1,1],1)" << endl
<< "" << endl
<< "Note also that this constraint is more efficient for smaller values of c. For" << endl
<< "large values consider using watchsumleq." << endl << endl << endl;
cout << "Reifiability" << "--------------------------------------------------------------------" << endl;
cout << "This constraint is NOT reifiable." << endl << endl << endl;
cout << "References" << "----------------------------------------------------------------------" << endl;
cout << "See also" << endl
<< "" << endl
<< " help constraints watchsumleq" << endl
<< " help constraints watchsumgeq" << endl << endl << endl;
} else
if("constraints watchsumgeq" == request) {
cout << "Help entry: " << "constraints watchsumgeq" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "The constraint watchsumgeq(vec, c) ensures that sum(vec) >= c." << endl << endl << endl;
cout << "Notes" << "---------------------------------------------------------------------------" << endl;
cout << "Equivalent to litsumgeq(vec, [1,...,1], c), but faster." << endl << endl << endl;
cout << "Notes" << "---------------------------------------------------------------------------" << endl;
cout << "This constraint works on 0/1 variables only." << endl << endl << endl;
cout << "Reifiablity" << "---------------------------------------------------------------------" << endl;
cout << "This constraint is NOT reifiable." << endl << endl << endl;
cout << "References" << "----------------------------------------------------------------------" << endl;
cout << "See also" << endl
<< "" << endl
<< " help constraints watchsumleq " << endl
<< " help constraints litsumgeq" << endl << endl << endl;
} else
if("constraints watchsumleq" == request) {
cout << "Help entry: " << "constraints watchsumleq" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "The constraint watchsumleq(vec, c) ensures that sum(vec) <= c." << endl << endl << endl;
cout << "Notes" << "---------------------------------------------------------------------------" << endl;
cout << "For this constraint, small values of c are more efficient." << endl << endl << endl;
cout << "Notes" << "---------------------------------------------------------------------------" << endl;
cout << "Equivelent to litsumgeq([vec1,...,vecn], [0,...,0], n-c) but faster." << endl << endl << endl;
cout << "Notes" << "---------------------------------------------------------------------------" << endl;
cout << "This constraint works on binary variables only." << endl << endl << endl;
cout << "Notes" << "---------------------------------------------------------------------------" << endl;
cout << "For this constraint, large values of c are more efficient." << endl << endl << endl;
cout << "Reifiablity" << "---------------------------------------------------------------------" << endl;
cout << "This constraint is NOT reifiable." << endl << endl << endl;
cout << "References" << "----------------------------------------------------------------------" << endl;
cout << "See also" << endl
<< "" << endl
<< " help constraints watchsumgeq " << endl
<< " help constraints litsumgeq" << endl << endl << endl;
} else
if("variables 01" == request) {
cout << "Help entry: " << "variables 01" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "01 variables are used very commonly for logical expressions, and for" << endl
<< "encoding the characteristic functions of sets and relations. Note that" << endl
<< "wherever a 01 variable can appear, the negation of that variable can" << endl
<< "also appear. A boolean variable x's negation is identified by !x." << endl << endl << endl;
cout << "Example" << "-------------------------------------------------------------------------" << endl;
cout << "Declaration of a 01 variable called bool in input file:" << endl
<< "" << endl
<< "BOOL bool" << endl
<< "" << endl
<< "Use of this variable in a constraint:" << endl
<< "" << endl
<< "eq(bool, 0) #variable bool equals 0" << endl << endl << endl;
} else
if("variables bounds" == request) {
cout << "Help entry: " << "variables bounds" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "Bounds variables, where only the upper and lower bounds of the domain" << endl
<< "are maintained. These domains must be continuous ranges of integers" << endl
<< "i.e. holes cannot be put in the domains of the variables." << endl << endl << endl;
cout << "Example" << "-------------------------------------------------------------------------" << endl;
cout << "" << endl
<< "Declaration of a bound variable called myvar with domain between 1" << endl
<< "and 7 in input file:" << endl
<< "" << endl
<< "BOUND myvar {1..7}" << endl
<< "" << endl
<< "Use of this variable in a constraint:" << endl
<< "" << endl
<< "eq(myvar, 4) #variable myvar equals 4" << endl << endl << endl;
} else
if("variables sparsebounds" == request) {
cout << "Help entry: " << "variables sparsebounds" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "In sparse bounds variables the domain is composed of discrete values" << endl
<< "(e.g. {1, 5, 36, 92}), but only the upper and lower bounds of the" << endl
<< "domain may be updated during search. Although the domain of these" << endl
<< "variables is not a continuous range, any holes in the domains must be" << endl
<< "there at time of specification, as they can not be added during the" << endl
<< "solving process." << endl << endl << endl;
cout << "Notes" << "---------------------------------------------------------------------------" << endl;
cout << "Declaration of a sparse bounds variable called myvar containing values" << endl
<< "{1,3,4,6,7,9,11} in input file:" << endl
<< "" << endl
<< "SPARSEBOUND myvar {1,3,4,6,7,9,11}" << endl
<< "" << endl
<< "Use of this variable in a constraint:" << endl
<< "eq(myvar, 3) #myvar equals 3" << endl << endl << endl;
} else
if("variables discrete" == request) {
cout << "Help entry: " << "variables discrete" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "In discrete variables, the domain ranges between the specified lower and upper" << endl
<< "bounds, but during search any domain value may be pruned, i.e., propagation and" << endl
<< "search may punch arbitrary holes in the domain." << endl << endl << endl;
cout << "Example" << "-------------------------------------------------------------------------" << endl;
cout << "Declaration of a discrete variable x with domain {1,2,3,4} in input file:" << endl
<< "" << endl
<< "DISCRETE x {1..4}" << endl
<< "" << endl
<< "Use of this variable in a constraint:" << endl
<< "" << endl
<< "eq(x, 2) #variable x equals 2" << endl << endl << endl;
} else
if("variables" == request) {
cout << "Help entry: " << "variables" << endl << endl;
cout << "General" << "-------------------------------------------------------------------------" << endl;
cout << "Minion supports 4 different variable types, namely" << endl
<< "" << endl
<< "- 0/1 variables," << endl
<< "- bounds variables," << endl
<< "- sparse bounds variables, and" << endl
<< "- discrete variables." << endl
<< "" << endl
<< "Sub-dividing the variable types in this manner affords the greatest" << endl
<< "opportunity for optimisation. In general, we recommend thinking of the" << endl
<< "variable types as a hierarchy, where 1 (0/1 variables) is the most" << endl
<< "efficient type, and 4 (Discrete variables) is the least. The" << endl
<< "user should use the variable which is the highest in the hierarchy," << endl
<< "yet encompasses enough information to provide a full model for the" << endl
<< "problem they are attempting to solve." << endl
<< "" << endl
<< "Minion also supports use of constants in place of variables, and constant" << endl
<< "vectors in place of vectors of variables. Using constants will be at least" << endl
<< "as efficient as using variables when the variable has a singleton domain." << endl
<< "" << endl
<< "See the entry on vectors for information on how vectors, matrices and," << endl
<< "more generally, tensors are handled in minion input. See also the" << endl
<< "alias entry for information on how to multiply name variables for" << endl
<< "convenience." << endl << endl << endl;
} else
if("variables constants" == request) {
cout << "Help entry: " << "variables constants" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "Minion supports the use of constants anywhere where a variable can be used. For" << endl
<< "example, in a constraint as a replacement for a single variable, or a vector of" << endl
<< "constants as a replacement for a vector of variables." << endl << endl << endl;
cout << "Examples" << "------------------------------------------------------------------------" << endl;
cout << "Use of a constant:" << endl
<< "" << endl
<< " eq(x,1)" << endl
<< "" << endl
<< "Use of a constant vector:" << endl
<< "" << endl
<< " element([10,9,8,7,6,5,4,3,2,1],idx,e)" << endl << endl << endl;
} else
if("variables vectors" == request) {
cout << "Help entry: " << "variables vectors" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "Vectors, matrices and tensors can be declared in minion" << endl
<< "input. Matrices and tensors are for convenience, as constraints do not" << endl
<< "take these as input; they must first undergo a flattening process to" << endl
<< "convert them to a vector before use." << endl << endl << endl;
cout << "Examples" << "------------------------------------------------------------------------" << endl;
cout << "A vector of 0/1 variables:" << endl
<< "" << endl
<< "BOOL myvec[5]" << endl
<< "" << endl
<< "A matrix of discrete variables:" << endl
<< "" << endl
<< "DISCRETE sudoku[9,9] {1..9}" << endl
<< "" << endl
<< "A 3D tensor of 0/1s:" << endl
<< "" << endl
<< "BOOL mycube[3,3,2]" << endl
<< "" << endl
<< "One can create a vector from scalars and elements of vectors, etc.:" << endl
<< "" << endl
<< "alldiff([x,y,myvec[1],mymatrix[3,4]])" << endl
<< "" << endl
<< "When a matrix or tensor is constrained, it is treated as a vector" << endl
<< "whose entries have been strung out into a vector in index order with" << endl
<< "the rightmost index changing most quickly, e.g." << endl
<< "" << endl
<< "alldiff(sudoku)" << endl
<< "" << endl
<< "is equivalent to" << endl
<< "" << endl
<< "alldiff([sudoku[0,0],...,sudoku[0,8],...,sudoku[8,0],...,sudoku[8,8]])" << endl
<< "" << endl
<< "Furthermore, with indices filled selectively and the remainder filled" << endl
<< "with underscores (_) the flattening applies only to the underscore" << endl
<< "indices:" << endl
<< "" << endl
<< "alldiff(sudoku[4,_])" << endl
<< "" << endl
<< "is equivalent to" << endl
<< "" << endl
<< "alldiff([sudoku[4,0],...,sudoku[4,8]])" << endl
<< "" << endl
<< "Lastly, one can optionally add square brackets ([]) around an" << endl
<< "expression to be flattened to make it look more like a vector:" << endl
<< "" << endl
<< "alldiff([sudoku[4,_]])" << endl
<< "" << endl
<< "is equivalent to" << endl
<< "" << endl
<< "alldiff(sudoku[4,_])" << endl << endl << endl;
} else
if("variables alias" == request) {
cout << "Help entry: " << "variables alias" << endl << endl;
cout << "Description" << "---------------------------------------------------------------------" << endl;
cout << "Specifying an alias is a way to give a variable another name. Aliases" << endl
<< "appear in the **VARIABLES** section of an input file. It is best" << endl
<< "described using some examples:" << endl
<< "" << endl
<< "ALIAS c = a" << endl
<< "" << endl
<< "ALIAS c[2,2] = [[myvar,b[2]],[b[1],anothervar]]" << endl << endl << endl;
} else
cout << "Unknown entry, please try again." << endl;
if("" == request) {
cout << "Available subentries:" << endl;
cout << "help constraints" << endl;
cout << "help input" << endl;
cout << "help switches" << endl;
cout << "help variables" << endl;
} else
if("constraints" == request) {
cout << "Available subentries:" << endl;
cout << "help constraints alldiff" << endl;
cout << "help constraints alldiffgacslow" << endl;
cout << "help constraints difference" << endl;
cout << "help constraints diseq" << endl;
cout << "help constraints div" << endl;
cout << "help constraints element" << endl;
cout << "help constraints element_one" << endl;
cout << "help constraints eq" << endl;
cout << "help constraints hamming" << endl;
cout << "help constraints ineq" << endl;
cout << "help constraints lexleq" << endl;
cout << "help constraints lexless" << endl;
cout << "help constraints litsumgeq" << endl;
cout << "help constraints max" << endl;
cout << "help constraints min" << endl;
cout << "help constraints minuseq" << endl;
cout << "help constraints modulo" << endl;
cout << "help constraints occurrence" << endl;
cout << "help constraints occurrencegeq" << endl;
cout << "help constraints occurrenceleq" << endl;
cout << "help constraints pow" << endl;
cout << "help constraints product" << endl;
cout << "help constraints reification" << endl;
cout << "help constraints reify" << endl;
cout << "help constraints reifyimply" << endl;
cout << "help constraints sumgeq" << endl;
cout << "help constraints sumleq" << endl;
cout << "help constraints table" << endl;
cout << "help constraints watchelement" << endl;
cout << "help constraints watchelement_one" << endl;
cout << "help constraints watchsumgeq" << endl;
cout << "help constraints watchsumleq" << endl;
cout << "help constraints watchvecexists_and" << endl;
cout << "help constraints watchvecexists_less" << endl;
cout << "help constraints watchvecneq" << endl;
cout << "help constraints weightedsumgeq" << endl;
cout << "help constraints weightedsumleq" << endl;
} else
if("input" == request) {
cout << "Available subentries:" << endl;
cout << "help input constraints" << endl;
cout << "help input example" << endl;
cout << "help input search" << endl;
cout << "help input tuplelist" << endl;
cout << "help input variables" << endl;
} else
if("switches" == request) {
cout << "Available subentries:" << endl;
cout << "help switches -check" << endl;
cout << "help switches -dumptree" << endl;
cout << "help switches -findallsols" << endl;
cout << "help switches -fullprop" << endl;
cout << "help switches -nocheck" << endl;
cout << "help switches -nodelimit" << endl;
cout << "help switches -noprintsols" << endl;
cout << "help switches -preprocess" << endl;
cout << "help switches -printsols" << endl;
cout << "help switches -printsolsonly" << endl;
cout << "help switches -quiet" << endl;
cout << "help switches -randomiseorder" << endl;
cout << "help switches -randomseed" << endl;
cout << "help switches -sollimit" << endl;
cout << "help switches -solsout" << endl;
cout << "help switches -tableout" << endl;
cout << "help switches -timelimit" << endl;
cout << "help switches -varorder" << endl;
cout << "help switches -verbose" << endl;
cout << "help switches -X-prop-node" << endl;
} else
if("variables" == request) {
cout << "Available subentries:" << endl;
cout << "help variables 01" << endl;
cout << "help variables alias" << endl;
cout << "help variables bounds" << endl;
cout << "help variables constants" << endl;
cout << "help variables discrete" << endl;
cout << "help variables sparsebounds" << endl;
cout << "help variables vectors" << endl;
} else
;
}
