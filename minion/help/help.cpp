#include <iostream>
#include <string>
using namespace std;
#define NEWLINE '\n'
void help(string request)
{
if("" == request) {
cout << "To use this help feature run the minion executable followed by help followed by the" << NEWLINE;
cout << "entry you wish to see. For example to see documentation on variables you should type:" << NEWLINE << NEWLINE;
cout << "   minion help variables" << NEWLINE << NEWLINE;
cout << "You can find out what other entries are available, if any, by looking at the 'subentries'" << NEWLINE;
cout << "section at the end of an entry." << NEWLINE << NEWLINE;
} else
if("switches" == request) {
cout << "Help entry: " << "switches" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "Minion supports a number of switches to augment default behaviour. To" << NEWLINE
<< "see more information on any switch, use the help system. The list" << NEWLINE
<< "below contains all available switches. For example to see help on" << NEWLINE
<< "-quiet type something similar to" << NEWLINE
<< "" << NEWLINE
<< " minion help switches -quiet" << NEWLINE
<< "" << NEWLINE
<< "replacing 'minion' by the name of the executable you're using." << NEWLINE << NEWLINE << NEWLINE;
} else
if("switches -findallsols" == request) {
cout << "Help entry: " << "switches -findallsols" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "Find all solutions and count them. This option is ignored if the" << NEWLINE
<< "problem contains any minimising or maximising objective." << NEWLINE << NEWLINE << NEWLINE;
} else
if("switches -quiet" == request) {
cout << "Help entry: " << "switches -quiet" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "Do not print parser progress." << NEWLINE << NEWLINE << NEWLINE;
cout << "References" << "----------------------------------------------------------------------" << NEWLINE;
cout << "help switches -verbose" << NEWLINE << NEWLINE << NEWLINE;
} else
if("switches -verbose" == request) {
cout << "Help entry: " << "switches -verbose" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "Print parser progress." << NEWLINE << NEWLINE << NEWLINE;
cout << "References" << "----------------------------------------------------------------------" << NEWLINE;
cout << "help switches -quiet" << NEWLINE << NEWLINE << NEWLINE;
} else
if("switches -printsols" == request) {
cout << "Help entry: " << "switches -printsols" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "Print solutions." << NEWLINE << NEWLINE << NEWLINE;
} else
if("switches -noprintsols" == request) {
cout << "Help entry: " << "switches -noprintsols" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "Do not print solutions." << NEWLINE << NEWLINE << NEWLINE;
} else
if("switches -printsolsonly" == request) {
cout << "Help entry: " << "switches -printsolsonly" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "Print only solutions and a summary at the end." << NEWLINE << NEWLINE << NEWLINE;
} else
if("switches -preprocess" == request) {
cout << "Help entry: " << "switches -preprocess" << NEWLINE << NEWLINE;
cout << "" << "--------------------------------------------------------------------------------" << NEWLINE;
cout << "" << NEWLINE
<< "This switch allows the user to choose what level of preprocess is" << NEWLINE
<< "applied to their model before search commences." << NEWLINE
<< "" << NEWLINE
<< "The choices are:" << NEWLINE
<< "" << NEWLINE
<< "- GAC " << NEWLINE
<< "- generalised arc consistency (default)" << NEWLINE
<< "- all propagators are run to a fixed point" << NEWLINE
<< "- if some propagators enforce less than GAC then the model will" << NEWLINE
<< "not necessarily be fully GAC at the outset" << NEWLINE
<< "" << NEWLINE
<< "- SACBounds " << NEWLINE
<< "- singleton arc consistency on the bounds of each variable" << NEWLINE
<< "- AC can be achieved when any variable lower or upper bound is a " << NEWLINE
<< "singleton in its own domain" << NEWLINE
<< "" << NEWLINE
<< "- SAC " << NEWLINE
<< "- singleton arc consistency" << NEWLINE
<< "- AC can be achieved in the model if any value is a singleton in" << NEWLINE
<< "its own domain" << NEWLINE
<< "" << NEWLINE
<< "- SSACBounds" << NEWLINE
<< "- singleton singleton bounds arc consistency" << NEWLINE
<< "- SAC can be achieved in the model when domains are replaced by either" << NEWLINE
<< "the singleton containing their upper bound, or the singleton containing" << NEWLINE
<< "their lower bound" << NEWLINE
<< "" << NEWLINE
<< "- SSAC " << NEWLINE
<< "- singleton singleton arc consistency" << NEWLINE
<< "- SAC can be achieved when any value is a singleton in its own domain" << NEWLINE
<< "" << NEWLINE
<< "These are listed in order of roughly how long they take to" << NEWLINE
<< "achieve. Preprocessing is a one off cost at the start of search. The" << NEWLINE
<< "success of higher levels of preprocessing is problem specific; SAC" << NEWLINE
<< "preprocesses may take a long time to complete, but may reduce search" << NEWLINE
<< "time enough to justify the cost." << NEWLINE << NEWLINE << NEWLINE;
cout << "Example" << "-------------------------------------------------------------------------" << NEWLINE;
cout << "To enforce SAC before search:" << NEWLINE
<< "" << NEWLINE
<< " minion -preprocess SAC myinputfile.minion" << NEWLINE << NEWLINE << NEWLINE;
cout << "References" << "----------------------------------------------------------------------" << NEWLINE;
cout << "help switches -X-prop-node" << NEWLINE << NEWLINE << NEWLINE;
} else
if("switches -X-prop-node" == request) {
cout << "Help entry: " << "switches -X-prop-node" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "Allows the user to choose the level of consistency to be enforced" << NEWLINE
<< "during search." << NEWLINE
<< "" << NEWLINE
<< "See entry 'help switches -preprocess' for details of the available" << NEWLINE
<< "levels of consistency." << NEWLINE << NEWLINE << NEWLINE;
cout << "Example" << "-------------------------------------------------------------------------" << NEWLINE;
cout << "To enforce SSAC during search:" << NEWLINE
<< "" << NEWLINE
<< " minion -X-prop-node SSAC input.minion" << NEWLINE << NEWLINE << NEWLINE;
cout << "References" << "----------------------------------------------------------------------" << NEWLINE;
cout << "help switches -preprocess" << NEWLINE << NEWLINE << NEWLINE;
} else
if("switches -dumptree" == request) {
cout << "Help entry: " << "switches -dumptree" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "Print out the branching decisions and variable states at each node." << NEWLINE << NEWLINE << NEWLINE;
} else
if("switches -fullprop" == request) {
cout << "Help entry: " << "switches -fullprop" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "Disable incremental propagation." << NEWLINE << NEWLINE << NEWLINE;
cout << "Notes" << "---------------------------------------------------------------------------" << NEWLINE;
cout << "This should always slow down search while producing exactly the same" << NEWLINE
<< "search tree." << NEWLINE
<< "" << NEWLINE
<< "Only available in a DEBUG executable." << NEWLINE << NEWLINE << NEWLINE;
} else
if("switches -nocheck" == request) {
cout << "Help entry: " << "switches -nocheck" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "Do not check solutions for correctness before printing them out." << NEWLINE << NEWLINE << NEWLINE;
cout << "Notes" << "---------------------------------------------------------------------------" << NEWLINE;
cout << "This option is the default on non-DEBUG executables." << NEWLINE << NEWLINE << NEWLINE;
} else
if("switches -check" == request) {
cout << "Help entry: " << "switches -check" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "Check solutions for correctness before printing them out." << NEWLINE << NEWLINE << NEWLINE;
cout << "Notes" << "---------------------------------------------------------------------------" << NEWLINE;
cout << "This option is the default for DEBUG executables." << NEWLINE << NEWLINE << NEWLINE;
} else
if("switches -nodelimit" == request) {
cout << "Help entry: " << "switches -nodelimit" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "To stop search after N nodes, do" << NEWLINE
<< "" << NEWLINE
<< " minion -nodelimit N myinput.minion" << NEWLINE << NEWLINE << NEWLINE;
cout << "References" << "----------------------------------------------------------------------" << NEWLINE;
cout << "help switches -timelimit" << NEWLINE
<< "help switches -sollimit" << NEWLINE << NEWLINE << NEWLINE;
} else
if("switches -timelimit" == request) {
cout << "Help entry: " << "switches -timelimit" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "To stop search after N seconds, do" << NEWLINE
<< "" << NEWLINE
<< " minion -timelimit N myinput.minion" << NEWLINE << NEWLINE << NEWLINE;
cout << "References" << "----------------------------------------------------------------------" << NEWLINE;
cout << "help switches -nodelimit" << NEWLINE
<< "help switches -sollimit" << NEWLINE << NEWLINE << NEWLINE;
} else
if("switches -sollimit" == request) {
cout << "Help entry: " << "switches -sollimit" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "To stop search after N solutions have been found, do" << NEWLINE
<< "" << NEWLINE
<< " minion -sollimit N myinput.minion" << NEWLINE << NEWLINE << NEWLINE;
cout << "References" << "----------------------------------------------------------------------" << NEWLINE;
cout << "help switches -nodelimit" << NEWLINE
<< "help switches -timelimit" << NEWLINE << NEWLINE << NEWLINE;
} else
if("switches -varorder" == request) {
cout << "Help entry: " << "switches -varorder" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "" << NEWLINE
<< "Enable a particular variable ordering for the search process. This" << NEWLINE
<< "flag is experimental and minion's default ordering might be faster." << NEWLINE
<< "" << NEWLINE
<< "The available orders are:" << NEWLINE
<< "" << NEWLINE
<< "- sdf - smallest domain first, break ties lexicographically" << NEWLINE
<< "" << NEWLINE
<< "- sdf-random - sdf, but break ties randomly" << NEWLINE
<< "" << NEWLINE
<< "- ldf - largest domain first, break ties lexicographically" << NEWLINE
<< "" << NEWLINE
<< "- ldf-random - ldf, but break ties randomly" << NEWLINE
<< "" << NEWLINE
<< "- random - random variable ordering" << NEWLINE
<< "" << NEWLINE
<< "- static -" << NEWLINE << NEWLINE << NEWLINE;
} else
if("switches -randomseed" == request) {
cout << "Help entry: " << "switches -randomseed" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "Set the pseudorandom seed to N. This allows 'random' behaviour to be" << NEWLINE
<< "repeated in different runs of minion." << NEWLINE << NEWLINE << NEWLINE;
} else
if("switches -tableout" == request) {
cout << "Help entry: " << "switches -tableout" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "Append a line of data about the current run of minion to a named file." << NEWLINE
<< "This data includes minion version information, arguments to the" << NEWLINE
<< "executable, build and solve time statistics, etc. See the file itself" << NEWLINE
<< "for a precise schema of the supplied information." << NEWLINE << NEWLINE << NEWLINE;
cout << "Example" << "-------------------------------------------------------------------------" << NEWLINE;
cout << "To add statistics about solving myproblem.minion to mystats.txt do" << NEWLINE
<< "" << NEWLINE
<< " minion -tableout mystats.txt myproblem.minion" << NEWLINE << NEWLINE << NEWLINE;
} else
if("switches -solsout" == request) {
cout << "Help entry: " << "switches -solsout" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "Append all solutionsto a named file." << NEWLINE
<< "Each solution is placed on a line, with no extra formatting." << NEWLINE << NEWLINE << NEWLINE;
cout << "Example" << "-------------------------------------------------------------------------" << NEWLINE;
cout << "To add the solutions of myproblem.minion to mysols.txt do" << NEWLINE
<< "" << NEWLINE
<< " minion -solsout mysols.txt myproblem.minion" << NEWLINE << NEWLINE << NEWLINE;
} else
if("switches -randomiseorder" == request) {
cout << "Help entry: " << "switches -randomiseorder" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "Randomises the ordering of the decision variables. If the input file" << NEWLINE
<< "specifies as ordering it will randomly permute this. If no ordering is" << NEWLINE
<< "specified a random permutation of all the variables is used." << NEWLINE << NEWLINE << NEWLINE;
} else
if("constraints element" == request) {
cout << "Help entry: " << "constraints element" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "The constraint " << NEWLINE
<< "" << NEWLINE
<< " element(vec, i, e)" << NEWLINE
<< "" << NEWLINE
<< "specifies that, in any solution, vec[i] = e." << NEWLINE << NEWLINE << NEWLINE;
cout << "Reifiability" << "--------------------------------------------------------------------" << NEWLINE;
cout << "This constraint is not reifiable." << NEWLINE << NEWLINE << NEWLINE;
cout << "Notes" << "---------------------------------------------------------------------------" << NEWLINE;
cout << "" << NEWLINE
<< "The level of propagation enforced by this constraint is not named, however it" << NEWLINE
<< "works as follows. For constraint vec[i]=e:" << NEWLINE
<< "" << NEWLINE
<< "- After i is assigned, ensures that min(vec[i]) = min(e) and " << NEWLINE
<< " max(vec[i]) = min(e)." << NEWLINE
<< "" << NEWLINE
<< "- When e is assigned, removes idx from the domain of i whenever e is not an" << NEWLINE
<< " element of the domain of vec[idx]." << NEWLINE
<< "" << NEWLINE
<< "- When m[idx] is assigned, removes idx from i when m[idx] is not in the domain" << NEWLINE
<< " of e." << NEWLINE
<< "" << NEWLINE
<< "This level of constency is designed to avoid the propagator having to scan" << NEWLINE
<< "through vec, except when e is assigned. It does a quantity of cheap propagation" << NEWLINE
<< "and may work well in practise on certain problems." << NEWLINE << NEWLINE << NEWLINE;
cout << "References" << "----------------------------------------------------------------------" << NEWLINE;
cout << "See the entry " << NEWLINE
<< "" << NEWLINE
<< " constraints watchelement" << NEWLINE
<< "" << NEWLINE
<< "for details of an identical constraint that enforces generalised arc" << NEWLINE
<< "consistency." << NEWLINE << NEWLINE << NEWLINE;
} else
if("constraints pow" == request) {
cout << "Help entry: " << "constraints pow" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "The constraint" << NEWLINE
<< " " << NEWLINE
<< " pow([x,y],z)" << NEWLINE
<< "" << NEWLINE
<< "ensures that x^y=z." << NEWLINE << NEWLINE << NEWLINE;
cout << "Notes" << "---------------------------------------------------------------------------" << NEWLINE;
cout << "This constraint is only available for positive domains x, y and z." << NEWLINE << NEWLINE << NEWLINE;
cout << "Reifiability" << "--------------------------------------------------------------------" << NEWLINE;
cout << "Not reifiable." << NEWLINE << NEWLINE << NEWLINE;
} else
if("constraints product" == request) {
cout << "Help entry: " << "constraints product" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "The constraint" << NEWLINE
<< "" << NEWLINE
<< " product(x,y,z)" << NEWLINE
<< "" << NEWLINE
<< "ensures that z=xy in any solution." << NEWLINE << NEWLINE << NEWLINE;
cout << "Notes" << "---------------------------------------------------------------------------" << NEWLINE;
cout << "This constraint can be used for (and, in fact, has a specialised" << NEWLINE
<< "implementation for) achieving boolean AND, i.e. x & y=z can be modelled" << NEWLINE
<< "as" << NEWLINE
<< "" << NEWLINE
<< " product(x,y,z)" << NEWLINE
<< "" << NEWLINE
<< "The general constraint achieves bounds generalised arc consistency for" << NEWLINE
<< "positive numbers." << NEWLINE << NEWLINE << NEWLINE;
cout << "Reifiability" << "--------------------------------------------------------------------" << NEWLINE;
cout << "This constraint is not reifiable." << NEWLINE << NEWLINE << NEWLINE;
} else
if("constraints div" == request) {
cout << "Help entry: " << "constraints div" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "The constraint" << NEWLINE
<< " " << NEWLINE
<< " div(x,y,z)" << NEWLINE
<< "" << NEWLINE
<< "ensures that floor(x/y)=z." << NEWLINE << NEWLINE << NEWLINE;
cout << "Notes" << "---------------------------------------------------------------------------" << NEWLINE;
cout << "This constraint is only available for positive domains x, y and z." << NEWLINE << NEWLINE << NEWLINE;
cout << "Reifiability" << "--------------------------------------------------------------------" << NEWLINE;
cout << "Not reifiable." << NEWLINE << NEWLINE << NEWLINE;
cout << "References" << "----------------------------------------------------------------------" << NEWLINE;
cout << "help constraints modulo" << NEWLINE << NEWLINE << NEWLINE;
} else
if("constraints eq" == request) {
cout << "Help entry: " << "constraints eq" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "Constrain two variables to take equal values." << NEWLINE << NEWLINE << NEWLINE;
cout << "Example" << "-------------------------------------------------------------------------" << NEWLINE;
cout << "eq(x0,x1)" << NEWLINE << NEWLINE << NEWLINE;
cout << "Notes" << "---------------------------------------------------------------------------" << NEWLINE;
cout << "Achieves bounds consistency." << NEWLINE << NEWLINE << NEWLINE;
cout << "Reifiability" << "--------------------------------------------------------------------" << NEWLINE;
cout << "This constraint is reifiable." << NEWLINE << NEWLINE << NEWLINE;
cout << "Reference" << "-----------------------------------------------------------------------" << NEWLINE;
cout << "help constraints minuseq" << NEWLINE << NEWLINE << NEWLINE;
} else
if("constraints minuseq" == request) {
cout << "Help entry: " << "constraints minuseq" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "Constraint" << NEWLINE
<< "" << NEWLINE
<< " minuseq(x,y)" << NEWLINE
<< "" << NEWLINE
<< "ensures that x=-y." << NEWLINE << NEWLINE << NEWLINE;
cout << "Reifiability" << "--------------------------------------------------------------------" << NEWLINE;
cout << "This constraint is reifiable." << NEWLINE << NEWLINE << NEWLINE;
cout << "Reference" << "-----------------------------------------------------------------------" << NEWLINE;
cout << "help constraints eq" << NEWLINE << NEWLINE << NEWLINE;
} else
if("constraints table" == request) {
cout << "Help entry: " << "constraints table" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "An extensional constraint that enforces GAC. The constraint is" << NEWLINE
<< "specified via a list of tuples." << NEWLINE << NEWLINE << NEWLINE;
cout << "Example" << "-------------------------------------------------------------------------" << NEWLINE;
cout << "To specify a constraint over 3 variables that allows assignments" << NEWLINE
<< "(0,0,0), (1,0,0), (0,1,0) or (0,0,1) do the following." << NEWLINE
<< "" << NEWLINE
<< "1) Add a tuplelist to the **TUPLELIST** section, e.g.:" << NEWLINE
<< "" << NEWLINE
<< "**TUPLELIST**" << NEWLINE
<< "myext 4 3" << NEWLINE
<< "0 0 0" << NEWLINE
<< "1 0 0" << NEWLINE
<< "0 1 0" << NEWLINE
<< "0 0 1" << NEWLINE
<< "" << NEWLINE
<< "N.B. the number 4 is the number of tuples in the constraint, the " << NEWLINE
<< "number 3 is the -arity." << NEWLINE
<< "" << NEWLINE
<< "2) Add a table constraint to the **CONSTRAINTS** section, e.g.:" << NEWLINE
<< "" << NEWLINE
<< "**CONSTRAINTS**" << NEWLINE
<< "table(myvec, myext)" << NEWLINE
<< "" << NEWLINE
<< "and now the variables of myvec will satisfy the constraint myext." << NEWLINE << NEWLINE << NEWLINE;
cout << "Example" << "-------------------------------------------------------------------------" << NEWLINE;
cout << "The constraints extension can also be specified in the constraint" << NEWLINE
<< "definition, e.g.:" << NEWLINE
<< "" << NEWLINE
<< "table(myvec, {<0,0,0>,<1,0,0>,<0,1,0>,<0,0,1>})" << NEWLINE << NEWLINE << NEWLINE;
cout << "References" << "----------------------------------------------------------------------" << NEWLINE;
cout << "help input tuplelist" << NEWLINE << NEWLINE << NEWLINE;
} else
if("constraints occurrence" == request) {
cout << "Help entry: " << "constraints occurrence" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "The constraint" << NEWLINE
<< "" << NEWLINE
<< " occurrence(vec, elem, count)" << NEWLINE
<< "" << NEWLINE
<< "ensures that there are count occurrences of the value elem in the" << NEWLINE
<< "vector vec." << NEWLINE << NEWLINE << NEWLINE;
cout << "Reifiability" << "--------------------------------------------------------------------" << NEWLINE;
cout << "This constraint is not reifiable." << NEWLINE << NEWLINE << NEWLINE;
cout << "References" << "----------------------------------------------------------------------" << NEWLINE;
cout << "help constraints occurrenceleq" << NEWLINE
<< "help constraints occurrencegeq" << NEWLINE << NEWLINE << NEWLINE;
} else
if("constraints occurrenceleq" == request) {
cout << "Help entry: " << "constraints occurrenceleq" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "The constraint" << NEWLINE
<< "" << NEWLINE
<< " occurrenceleq(vec, elem, count)" << NEWLINE
<< "" << NEWLINE
<< "ensures that there are AT MOST count occurrences of the value elem in" << NEWLINE
<< "the vector vec." << NEWLINE << NEWLINE << NEWLINE;
cout << "Notes" << "---------------------------------------------------------------------------" << NEWLINE;
cout << "Variable count must be a constant." << NEWLINE << NEWLINE << NEWLINE;
cout << "Reifiability" << "--------------------------------------------------------------------" << NEWLINE;
cout << "This constraint is not reifiable." << NEWLINE << NEWLINE << NEWLINE;
cout << "References" << "----------------------------------------------------------------------" << NEWLINE;
cout << "help constraints occurrence" << NEWLINE
<< "help constraints occurrencegeq" << NEWLINE << NEWLINE << NEWLINE;
} else
if("constraints occurrencegeq" == request) {
cout << "Help entry: " << "constraints occurrencegeq" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "The constraint" << NEWLINE
<< "" << NEWLINE
<< " occurrencegeq(vec, elem, count)" << NEWLINE
<< "" << NEWLINE
<< "ensures that there are AT LEAST count occurrences of the value elem in" << NEWLINE
<< "the vector vec." << NEWLINE << NEWLINE << NEWLINE;
cout << "Notes" << "---------------------------------------------------------------------------" << NEWLINE;
cout << "Variable count must be a constant." << NEWLINE << NEWLINE << NEWLINE;
cout << "Reifiability" << "--------------------------------------------------------------------" << NEWLINE;
cout << "This constraint is not reifiable." << NEWLINE << NEWLINE << NEWLINE;
cout << "References" << "----------------------------------------------------------------------" << NEWLINE;
cout << "help constraints occurrence" << NEWLINE
<< "help constraints occurrenceleq" << NEWLINE << NEWLINE << NEWLINE;
} else
if("constraints alldiffgacslow" == request) {
cout << "Help entry: " << "constraints alldiffgacslow" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "Forces the input vector of variables to take distinct values." << NEWLINE << NEWLINE << NEWLINE;
cout << "Example" << "-------------------------------------------------------------------------" << NEWLINE;
cout << "Suppose the input file had the following vector of variables defined:" << NEWLINE
<< "" << NEWLINE
<< "DISCRETE myVec[9] {1..9}" << NEWLINE
<< "" << NEWLINE
<< "To ensure that each variable takes a different value include the" << NEWLINE
<< "following constraint:" << NEWLINE
<< "" << NEWLINE
<< "alldiffgacslow(myVec)" << NEWLINE << NEWLINE << NEWLINE;
cout << "Reifiability" << "--------------------------------------------------------------------" << NEWLINE;
cout << "Not reifiable." << NEWLINE << NEWLINE << NEWLINE;
cout << "Notes" << "---------------------------------------------------------------------------" << NEWLINE;
cout << "This constraint enforces generalised arc consistency." << NEWLINE << NEWLINE << NEWLINE;
} else
if("constraints" == request) {
cout << "Help entry: " << "constraints" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "Minion supports many constraints and these are regularly being" << NEWLINE
<< "improved and added to. In some cases multiple implementations of the" << NEWLINE
<< "same constraints are provided and we would appreciate additional" << NEWLINE
<< "feedback on their relative merits in your problem." << NEWLINE
<< "" << NEWLINE
<< "Minion does not support nesting of constraints, however this can be" << NEWLINE
<< "achieved by auxiliary variables and reification." << NEWLINE
<< "" << NEWLINE
<< "Variables can be replaced by constants. You can find out more on" << NEWLINE
<< "expressions for variables, vectors, etc. in the section on variables." << NEWLINE << NEWLINE << NEWLINE;
cout << "References" << "----------------------------------------------------------------------" << NEWLINE;
cout << "help variables" << NEWLINE << NEWLINE << NEWLINE;
} else
if("constraints weightedsumleq" == request) {
cout << "Help entry: " << "constraints weightedsumleq" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "The constraint" << NEWLINE
<< "" << NEWLINE
<< " weightedsumleq(constantVec, varVec, total)" << NEWLINE
<< "" << NEWLINE
<< "ensures that constantVec.varVec >= total, where constantVec.varVec is" << NEWLINE
<< "the scalar dot product of constantVec and varVec." << NEWLINE << NEWLINE << NEWLINE;
cout << "References" << "----------------------------------------------------------------------" << NEWLINE;
cout << "help constraints weightedsumgeq" << NEWLINE
<< "help constraints sumleq" << NEWLINE
<< "help constraints sumgeq" << NEWLINE << NEWLINE << NEWLINE;
} else
if("constraints weightedsumgeq" == request) {
cout << "Help entry: " << "constraints weightedsumgeq" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "The constraint" << NEWLINE
<< "" << NEWLINE
<< " weightedsumgeq(constantVec, varVec, total)" << NEWLINE
<< "" << NEWLINE
<< "ensures that constantVec.varVec <= total, where constantVec.varVec is" << NEWLINE
<< "the scalar dot product of constantVec and varVec." << NEWLINE << NEWLINE << NEWLINE;
cout << "References" << "----------------------------------------------------------------------" << NEWLINE;
cout << "help constraints weightedsumleq" << NEWLINE
<< "help constraints sumleq" << NEWLINE
<< "help constraints sumgeq" << NEWLINE << NEWLINE << NEWLINE;
} else
if("constraints reify" == request) {
cout << "Help entry: " << "constraints reify" << NEWLINE << NEWLINE;
cout << "References" << "----------------------------------------------------------------------" << NEWLINE;
cout << "See" << NEWLINE
<< " help constraints reifiable" << NEWLINE << NEWLINE << NEWLINE;
} else
if("constraints reifyimply" == request) {
cout << "Help entry: " << "constraints reifyimply" << NEWLINE << NEWLINE;
cout << "References" << "----------------------------------------------------------------------" << NEWLINE;
cout << "See" << NEWLINE
<< " help constraints reifiable" << NEWLINE << NEWLINE << NEWLINE;
} else
if("constraints reification" == request) {
cout << "Help entry: " << "constraints reification" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "Reification is provided in two forms: reify and reifyimply." << NEWLINE
<< "" << NEWLINE
<< " reify(constraint, r) where r is a 0/1 var" << NEWLINE
<< "" << NEWLINE
<< "ensures that r is set to 1 if and only if constraint is satisfied. That is, if r" << NEWLINE
<< "is 0 the constraint must NOT be satisfied; and if r is 1 it must be satisfied as" << NEWLINE
<< "normal. Conversely, if the constraint is satisfied then r must be 1, and if not" << NEWLINE
<< "then r must be 0." << NEWLINE
<< "" << NEWLINE
<< " reifyimply(constraint, r)" << NEWLINE
<< "" << NEWLINE
<< "only checks that if r is set to 1 then constraint must be satisfied. If r is not" << NEWLINE
<< "1, constraint may be either satisfied or unsatisfied. Furthermore r is never set" << NEWLINE
<< "by propagation, only by search; that is, satisfaction of constraint does not" << NEWLINE
<< "affect the value of r." << NEWLINE << NEWLINE << NEWLINE;
cout << "Notes" << "---------------------------------------------------------------------------" << NEWLINE;
cout << "Not all constraints are reifiable. Entries for individual constraints give" << NEWLINE
<< "more information." << NEWLINE << NEWLINE << NEWLINE;
} else
if("constraints modulo" == request) {
cout << "Help entry: " << "constraints modulo" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "The constraint" << NEWLINE
<< " " << NEWLINE
<< " modulo([x,y],z)" << NEWLINE
<< "" << NEWLINE
<< "ensures that x%y=z i.e. z is the remainder of dividing x by y." << NEWLINE << NEWLINE << NEWLINE;
cout << "Notes" << "---------------------------------------------------------------------------" << NEWLINE;
cout << "This constraint is only available for positive domains x, y and z." << NEWLINE << NEWLINE << NEWLINE;
cout << "Reifiability" << "--------------------------------------------------------------------" << NEWLINE;
cout << "Not reifiable." << NEWLINE << NEWLINE << NEWLINE;
cout << "References" << "----------------------------------------------------------------------" << NEWLINE;
cout << "help constraints div" << NEWLINE << NEWLINE << NEWLINE;
} else
if("constraints lexless" == request) {
cout << "Help entry: " << "constraints lexless" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "The constraint" << NEWLINE
<< "" << NEWLINE
<< " lexless(vec0, vec1)" << NEWLINE
<< "" << NEWLINE
<< "takes two vectors vec0 and vec1 of the same length and ensures that" << NEWLINE
<< "vec0 is lexicographically less than vec1 in any solution." << NEWLINE << NEWLINE << NEWLINE;
cout << "Notes" << "---------------------------------------------------------------------------" << NEWLINE;
cout << "This constraint maintains GAC." << NEWLINE << NEWLINE << NEWLINE;
cout << "Reifiability" << "--------------------------------------------------------------------" << NEWLINE;
cout << "This constraint is reifiable." << NEWLINE << NEWLINE << NEWLINE;
cout << "References" << "----------------------------------------------------------------------" << NEWLINE;
cout << "See also" << NEWLINE
<< "" << NEWLINE
<< " help constraints lexleq" << NEWLINE
<< "" << NEWLINE
<< "for a similar constraint with non-strict lexicographic inequality." << NEWLINE << NEWLINE << NEWLINE;
} else
if("constraints lexleq" == request) {
cout << "Help entry: " << "constraints lexleq" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "The constraint" << NEWLINE
<< "" << NEWLINE
<< " lexleq(vec0, vec1)" << NEWLINE
<< "" << NEWLINE
<< "takes two vectors vec0 and vec1 of the same length and ensures that" << NEWLINE
<< "vec0 is lexicographically less than or equal to vec1 in any solution." << NEWLINE << NEWLINE << NEWLINE;
cout << "Notes" << "---------------------------------------------------------------------------" << NEWLINE;
cout << "This constraints achieves GAC." << NEWLINE << NEWLINE << NEWLINE;
cout << "Reifiability" << "--------------------------------------------------------------------" << NEWLINE;
cout << "This constraint is reifiable." << NEWLINE << NEWLINE << NEWLINE;
cout << "References" << "----------------------------------------------------------------------" << NEWLINE;
cout << "See also" << NEWLINE
<< "" << NEWLINE
<< " help constraints lexless" << NEWLINE
<< "" << NEWLINE
<< "for a similar constraint with strict lexicographic inequality." << NEWLINE << NEWLINE << NEWLINE;
} else
if("constraints sumleq" == request) {
cout << "Help entry: " << "constraints sumleq" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "The constraint" << NEWLINE
<< "" << NEWLINE
<< " sumleq(vec, c)" << NEWLINE
<< "" << NEWLINE
<< "ensures that sum(vec) <= c." << NEWLINE << NEWLINE << NEWLINE;
cout << "Reifiability" << "--------------------------------------------------------------------" << NEWLINE;
cout << "This constrait is reifiable." << NEWLINE << NEWLINE << NEWLINE;
} else
if("constraints sumgeq" == request) {
cout << "Help entry: " << "constraints sumgeq" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "The constraint" << NEWLINE
<< "" << NEWLINE
<< " sumgeq(vec, c)" << NEWLINE
<< "" << NEWLINE
<< "ensures that sum(vec) >= c." << NEWLINE << NEWLINE << NEWLINE;
} else
if("constraints ineq" == request) {
cout << "Help entry: " << "constraints ineq" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "The constraint" << NEWLINE
<< "" << NEWLINE
<< " ineq(x, y, k)" << NEWLINE
<< "" << NEWLINE
<< "ensures that " << NEWLINE
<< "" << NEWLINE
<< " x <= y + k " << NEWLINE
<< "" << NEWLINE
<< "in any solution." << NEWLINE << NEWLINE << NEWLINE;
cout << "Notes" << "---------------------------------------------------------------------------" << NEWLINE;
cout << "Minion has no strict inequality (<) constraints. However x < y can be" << NEWLINE
<< "achieved by" << NEWLINE
<< "" << NEWLINE
<< " ineq(x, y, -1)" << NEWLINE << NEWLINE << NEWLINE;
cout << "Reifiability" << "--------------------------------------------------------------------" << NEWLINE;
cout << "This constraint is reifiable." << NEWLINE << NEWLINE << NEWLINE;
} else
if("constraints diseq" == request) {
cout << "Help entry: " << "constraints diseq" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "Constrain two variables to take different values." << NEWLINE << NEWLINE << NEWLINE;
cout << "Notes" << "---------------------------------------------------------------------------" << NEWLINE;
cout << "Achieves arc consistency." << NEWLINE << NEWLINE << NEWLINE;
cout << "Example" << "-------------------------------------------------------------------------" << NEWLINE;
cout << "diseq(v0,v1)" << NEWLINE << NEWLINE << NEWLINE;
cout << "Reifiability" << "--------------------------------------------------------------------" << NEWLINE;
cout << "This constraint is reifiable." << NEWLINE << NEWLINE << NEWLINE;
} else
if("constraints alldiff" == request) {
cout << "Help entry: " << "constraints alldiff" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "Forces the input vector of variables to take distinct values." << NEWLINE << NEWLINE << NEWLINE;
cout << "Example" << "-------------------------------------------------------------------------" << NEWLINE;
cout << "Suppose the input file had the following vector of variables defined:" << NEWLINE
<< "" << NEWLINE
<< "DISCRETE myVec[9] {1..9}" << NEWLINE
<< "" << NEWLINE
<< "To ensure that each variable takes a different value include the" << NEWLINE
<< "following constraint:" << NEWLINE
<< "" << NEWLINE
<< "alldiff(myVec)" << NEWLINE << NEWLINE << NEWLINE;
cout << "Notes" << "---------------------------------------------------------------------------" << NEWLINE;
cout << "Enforces the same level of consistency as a clique of not equals " << NEWLINE
<< "constraints." << NEWLINE << NEWLINE << NEWLINE;
cout << "Reifiability" << "--------------------------------------------------------------------" << NEWLINE;
cout << "Not reifiable." << NEWLINE << NEWLINE << NEWLINE;
cout << "References" << "----------------------------------------------------------------------" << NEWLINE;
cout << "See" << NEWLINE
<< "" << NEWLINE
<< " help constraints alldiffgacslow" << NEWLINE
<< "" << NEWLINE
<< "for the same constraint that enforces GAC." << NEWLINE << NEWLINE << NEWLINE;
} else
if("constraints max" == request) {
cout << "Help entry: " << "constraints max" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "The constraint" << NEWLINE
<< "" << NEWLINE
<< " max(vec, x)" << NEWLINE
<< "" << NEWLINE
<< "ensures that x is equal to the maximum value of any variable in vec." << NEWLINE << NEWLINE << NEWLINE;
cout << "Reifiability" << "--------------------------------------------------------------------" << NEWLINE;
cout << "This constraint is not reifiable." << NEWLINE << NEWLINE << NEWLINE;
cout << "References" << "----------------------------------------------------------------------" << NEWLINE;
cout << "See" << NEWLINE
<< "" << NEWLINE
<< " help constraints min" << NEWLINE
<< "" << NEWLINE
<< "for the opposite constraint." << NEWLINE << NEWLINE << NEWLINE;
} else
if("constraints min" == request) {
cout << "Help entry: " << "constraints min" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "The constraint" << NEWLINE
<< "" << NEWLINE
<< " min(vec, x)" << NEWLINE
<< "" << NEWLINE
<< "ensures that x is equal to the minimum value of any variable in vec." << NEWLINE << NEWLINE << NEWLINE;
cout << "Reifiability" << "--------------------------------------------------------------------" << NEWLINE;
cout << "This constraint is not reifiable." << NEWLINE << NEWLINE << NEWLINE;
cout << "References" << "----------------------------------------------------------------------" << NEWLINE;
cout << "See" << NEWLINE
<< "" << NEWLINE
<< " help constraints max" << NEWLINE
<< "" << NEWLINE
<< "for the opposite constraint." << NEWLINE << NEWLINE << NEWLINE;
} else
if("input" == request) {
cout << "Help entry: " << "input" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "" << NEWLINE
<< "Minion expects to be provided with the name of an input file as an" << NEWLINE
<< "argument. This file contains a specification of the CSP to be solved" << NEWLINE
<< "as well as settings that the search process should use. The format is" << NEWLINE
<< "" << NEWLINE
<< "Minion3Input::= MINION 3" << NEWLINE
<< " <InputSection>+" << NEWLINE
<< " **EOF**" << NEWLINE
<< "" << NEWLINE
<< "InputSection::= <VariablesSection> " << NEWLINE
<< " | <SearchSection>" << NEWLINE
<< " | <ConstraintsSection> " << NEWLINE
<< " | <TuplelistSection>" << NEWLINE
<< "" << NEWLINE
<< "i.e. 'MINION 3' followed by any number of variable, search," << NEWLINE
<< "constraints and tuplelists sections (can repeat) followed by" << NEWLINE
<< "'**EOF**', the end of file marker." << NEWLINE
<< "" << NEWLINE
<< "All text from a '#' character to the end of the line is ignored." << NEWLINE
<< "" << NEWLINE
<< "See the associated help entries below for information on each section." << NEWLINE << NEWLINE << NEWLINE;
cout << "Notes" << "---------------------------------------------------------------------------" << NEWLINE;
cout << "You can give an input file via standard input by specifying '--' as the file" << NEWLINE
<< "name, this might help when minion is being used as a tool in a shell script or" << NEWLINE
<< "for compressed input, e.g.," << NEWLINE
<< "" << NEWLINE
<< " gunzip -c myinput.minion.gz | minion" << NEWLINE << NEWLINE << NEWLINE;
} else
if("input variables" == request) {
cout << "Help entry: " << "input variables" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "The variables section consists of any number of variable declarations" << NEWLINE
<< "on separate lines." << NEWLINE
<< "" << NEWLINE
<< "VariablesSection::= **VARIABLES**" << NEWLINE
<< " <VarDeclaration>*" << NEWLINE << NEWLINE << NEWLINE;
cout << "Example" << "-------------------------------------------------------------------------" << NEWLINE;
cout << "**VARIABLES**" << NEWLINE
<< "" << NEWLINE
<< "BOOL bool #boolean var" << NEWLINE
<< "BOUND b {1..3} #bounds var" << NEWLINE
<< "SPARSEBOUND myvar {1,3,4,6,7,9,11} #sparse bounds var" << NEWLINE
<< "DISCRETE d[3] {1..3} #array of discrete vars" << NEWLINE << NEWLINE << NEWLINE;
cout << "References" << "----------------------------------------------------------------------" << NEWLINE;
cout << "See the help section" << NEWLINE
<< "" << NEWLINE
<< " help variables" << NEWLINE
<< "" << NEWLINE
<< "for detailed information on variable declarations." << NEWLINE << NEWLINE << NEWLINE;
} else
if("input constraints" == request) {
cout << "Help entry: " << "input constraints" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "" << NEWLINE
<< "The constraints section consists of any number of constraint" << NEWLINE
<< "declarations on separate lines." << NEWLINE
<< "" << NEWLINE
<< "ConstraintsSection::= **CONSTRAINTS**" << NEWLINE
<< " <ConstraintDeclaration>*" << NEWLINE << NEWLINE << NEWLINE;
cout << "Example" << "-------------------------------------------------------------------------" << NEWLINE;
cout << "**CONSTRAINTS**" << NEWLINE
<< "eq(bool,0)" << NEWLINE
<< "alldiff(d)" << NEWLINE << NEWLINE << NEWLINE;
cout << "References" << "----------------------------------------------------------------------" << NEWLINE;
cout << "See help entries for individual constraints under" << NEWLINE
<< "" << NEWLINE
<< " help constraints" << NEWLINE
<< "" << NEWLINE
<< "for details on constraint declarations." << NEWLINE << NEWLINE << NEWLINE;
} else
if("input tuplelist" == request) {
cout << "Help entry: " << "input tuplelist" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "In a tuplelist section lists of allowed tuples for table constraints" << NEWLINE
<< "can be specified. This technique is preferable to specifying the" << NEWLINE
<< "tuples in the constraint declaration, since the tuplelists can be" << NEWLINE
<< "shared between constraints and named for readability." << NEWLINE
<< "" << NEWLINE
<< "The required format is" << NEWLINE
<< "" << NEWLINE
<< "TuplelistSection::= **TUPLELIST**" << NEWLINE
<< " <Tuplelist>*" << NEWLINE
<< "" << NEWLINE
<< "Tuplelist::= <name> <num_tuples> <tuple_length> <numbers>+" << NEWLINE << NEWLINE << NEWLINE;
cout << "Example" << "-------------------------------------------------------------------------" << NEWLINE;
cout << "**TUPLELIST**" << NEWLINE
<< "AtMostOne 4 3" << NEWLINE
<< "0 0 0" << NEWLINE
<< "0 0 1" << NEWLINE
<< "0 1 0" << NEWLINE
<< "1 0 0" << NEWLINE << NEWLINE << NEWLINE;
cout << "References" << "----------------------------------------------------------------------" << NEWLINE;
cout << "help constraints table" << NEWLINE << NEWLINE << NEWLINE;
} else
if("input search" == request) {
cout << "Help entry: " << "input search" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "" << NEWLINE
<< "Inside the search section one can specify" << NEWLINE
<< "" << NEWLINE
<< "- variable orderings, " << NEWLINE
<< "- value orderings," << NEWLINE
<< "- optimisation function, and" << NEWLINE
<< "- details of how to print out solutions." << NEWLINE
<< "" << NEWLINE
<< "SearchSection::= <VariableOrdering>?" << NEWLINE
<< " <ValueOrdering>?" << NEWLINE
<< " <OptimisationFn>?" << NEWLINE
<< " <PrintFormat>?" << NEWLINE
<< "" << NEWLINE
<< "In the variable ordering a fixed ordering can be specified on any" << NEWLINE
<< "subset of variables. These are the search variables that will be" << NEWLINE
<< "instantiated in every solution. If none is specified some other fixed" << NEWLINE
<< "ordering of all the variables will be used." << NEWLINE
<< "" << NEWLINE
<< " VariableOrdering::= VARORDER[ <varname>+ ]" << NEWLINE
<< "" << NEWLINE
<< "The value ordering allows the user to specify an instantiation order" << NEWLINE
<< "for the variables involved in the variable order, either ascending (a)" << NEWLINE
<< "or descending (d) for each. When no value ordering is specified, the" << NEWLINE
<< "default is to use ascending order for every search variable." << NEWLINE
<< "" << NEWLINE
<< " ValueOrdering::= VALORDER[ (a|d)+ ]" << NEWLINE
<< "" << NEWLINE
<< "To model an optimisation problem the user can specify to minimise" << NEWLINE
<< "or maximise a variable's value." << NEWLINE
<< "" << NEWLINE
<< " OptimisationFn::= MAXIMISING <varname>" << NEWLINE
<< " | MINIMISING <varname>" << NEWLINE
<< "" << NEWLINE
<< "Finally, the user can control some aspects of the way solutions are" << NEWLINE
<< "printed. By default (no PrintFormat specified) all the variables are" << NEWLINE
<< "printed in declaration order. Alternatively a custom vector, or ALL" << NEWLINE
<< "variables, or no (NONE) variables can be printed. If a matrix or, more" << NEWLINE
<< "generally, a tensor is given instead of a vector, it is automatically" << NEWLINE
<< "flattened into a vector as described in 'help variables vectors'." << NEWLINE
<< "" << NEWLINE
<< " PrintFormat::= PRINT <vector>" << NEWLINE
<< " | PRINT ALL" << NEWLINE
<< " | PRINT NONE" << NEWLINE << NEWLINE << NEWLINE;
} else
if("input example" == request) {
cout << "Help entry: " << "input example" << NEWLINE << NEWLINE;
cout << "Example" << "-------------------------------------------------------------------------" << NEWLINE;
cout << "Below is a complete minion input file with commentary, as an example." << NEWLINE
<< "" << NEWLINE
<< "MINION 3" << NEWLINE
<< "" << NEWLINE
<< "# While the variable section doesn't have to come first, you can't " << NEWLINE
<< "# really do anything until" << NEWLINE
<< "# You have one..." << NEWLINE
<< "**VARIABLES**" << NEWLINE
<< "" << NEWLINE
<< "# There are 4 type of variables" << NEWLINE
<< "BOOL bool # Boolean don't need a domain" << NEWLINE
<< "BOUND b {1..3} # Bound vars need a domain given as a range" << NEWLINE
<< "DISCRETE d {1..3} # So do discrete vars" << NEWLINE
<< "" << NEWLINE
<< "#Note: Names are case sensitive!" << NEWLINE
<< "" << NEWLINE
<< "# Internally, Bound variables are stored only as a lower and upper bound" << NEWLINE
<< "# Whereas discrete variables allow any sub-domain" << NEWLINE
<< "" << NEWLINE
<< "SPARSEBOUND s {1,3,6,7} # Sparse bound variables take a sorted list of values" << NEWLINE
<< "" << NEWLINE
<< "# We can also declare matrices of variables!" << NEWLINE
<< "" << NEWLINE
<< "DISCRETE q[3] {0..5} # This is a matrix with 3 variables: q[0],q[1] and q[2]" << NEWLINE
<< "BOOL bm[2,2] # A 2d matrix, variables bm[0,0], bm[0,1], bm[1,0], bm[1,1]" << NEWLINE
<< "BOOL bn[2,2,2,2] # You can have as many indices as you like!" << NEWLINE
<< "" << NEWLINE
<< "#The search section is entirely optional" << NEWLINE
<< "**SEARCH**" << NEWLINE
<< "" << NEWLINE
<< "# Note that everything in SEARCH is optional, and can only be given at" << NEWLINE
<< "# most once!" << NEWLINE
<< "" << NEWLINE
<< "# If you don't give an explicit variable ordering, one is generated." << NEWLINE
<< "# These can take matrices in interesting ways like constraints, see below." << NEWLINE
<< "VARORDER [bool,b,d]" << NEWLINE
<< "" << NEWLINE
<< "# If you don't give a value ordering, 'ascending' is used" << NEWLINE
<< "#VALORDER [a,a,a,a]" << NEWLINE
<< "" << NEWLINE
<< "# You can have one objective function, or none at all." << NEWLINE
<< "MAXIMISING bool" << NEWLINE
<< "# MINIMISING x3" << NEWLINE
<< "" << NEWLINE
<< "# both (MAX/MIN)IMISING and (MAX/MIN)IMIZING are accepted..." << NEWLINE
<< "" << NEWLINE
<< "" << NEWLINE
<< "# Print statement takes a vector of things to print" << NEWLINE
<< "" << NEWLINE
<< "PRINT [bool, q]" << NEWLINE
<< "" << NEWLINE
<< "# You can also give:" << NEWLINE
<< "# PRINT ALL (the default)" << NEWLINE
<< "# PRINT NONE" << NEWLINE
<< "" << NEWLINE
<< "" << NEWLINE
<< "# Declare constraints in this section!" << NEWLINE
<< "**CONSTRAINTS**" << NEWLINE
<< "" << NEWLINE
<< "# Constraints are defined in exactly the same way as in MINION input" << NEWLINE
<< "formats 1 & 2" << NEWLINE
<< "eq(bool, 0)" << NEWLINE
<< "eq(b,d)" << NEWLINE
<< "" << NEWLINE
<< "# To get a single variable from a matrix, just index it" << NEWLINE
<< "eq(q[1],0)" << NEWLINE
<< "eq(bn[0,1,1,1], bm[1,1])" << NEWLINE
<< "" << NEWLINE
<< "# It's easy to get a row or column from a matrix. Just use _ in the" << NEWLINE
<< "# indices you want" << NEWLINE
<< "# to vary. Just giving a matrix gives all the variables in that matrix." << NEWLINE
<< "" << NEWLINE
<< "#The following shows how flattening occurs..." << NEWLINE
<< "" << NEWLINE
<< "# [bm] == [ bm[_,_] ] == [ bm[0,0], bm[0,1], bm[1,0], bm[1,1] ]" << NEWLINE
<< "# [ bm[_,1] ] = [ bm[0,1], bm[1,1] ]" << NEWLINE
<< "# [ bn[1,_,0,_] = [ bn[1,0,0,0], b[1,0,0,1], b[1,1,0,0], b[1,1,0,1] ]" << NEWLINE
<< "" << NEWLINE
<< "# You can string together a list of such expressions!" << NEWLINE
<< "" << NEWLINE
<< "lexleq( [bn[1,_,0,_], bool, q[0]] , [b, bm, d] )" << NEWLINE
<< "" << NEWLINE
<< "# One minor problem.. you must always put [ ] around any matrix expression, so" << NEWLINE
<< "# lexleq(bm, bm) is invalid" << NEWLINE
<< "" << NEWLINE
<< "lexleq( [bm], [bm] ) # This is OK!" << NEWLINE
<< "" << NEWLINE
<< "# Can give tuplelists, which can have names!" << NEWLINE
<< "# The input is: <name> <num_of_tuples> <tuple_length> <numbers...>" << NEWLINE
<< "# The formatting can be about anything.." << NEWLINE
<< "" << NEWLINE
<< "**TUPLELIST**" << NEWLINE
<< "" << NEWLINE
<< "Fred 3 3" << NEWLINE
<< "0 2 3" << NEWLINE
<< "2 0 3" << NEWLINE
<< "3 1 3" << NEWLINE
<< "" << NEWLINE
<< "Bob 2 2 1 2 3 4" << NEWLINE
<< "" << NEWLINE
<< "#No need to put everything in one section! All sections can be reopened.." << NEWLINE
<< "**VARIABLES**" << NEWLINE
<< "" << NEWLINE
<< "# You can even have empty sections.. if you want" << NEWLINE
<< "" << NEWLINE
<< "**CONSTRAINTS**" << NEWLINE
<< "" << NEWLINE
<< "#Specify tables by their names.." << NEWLINE
<< "" << NEWLINE
<< "table([q], Fred)" << NEWLINE
<< "" << NEWLINE
<< "# Can still list tuples explicitally in the constraint if you want at" << NEWLINE
<< "# the moment." << NEWLINE
<< "# On the other hand, I might remove this altogether, as it's worse than giving" << NEWLINE
<< "# Tuplelists" << NEWLINE
<< "" << NEWLINE
<< "table([q],{ <0,2,3>,<2,0,3>,<3,1,3> })" << NEWLINE
<< "" << NEWLINE
<< "#Must end with the **EOF** marker!" << NEWLINE
<< "" << NEWLINE
<< "**EOF**" << NEWLINE
<< "" << NEWLINE
<< "Any text down here is ignored, so you can write whatever you like (or" << NEWLINE
<< "nothing at all...)" << NEWLINE << NEWLINE << NEWLINE;
} else
if("constraints hamming" == request) {
cout << "Help entry: " << "constraints hamming" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "The constraint" << NEWLINE
<< "" << NEWLINE
<< " hamming(X,Y,c)" << NEWLINE
<< "" << NEWLINE
<< "ensures that the hamming distance between X and Y is c. That is, that" << NEWLINE
<< "c is the size of the set {i | X[i] != y[i]}" << NEWLINE << NEWLINE << NEWLINE;
cout << "Reifiability" << "--------------------------------------------------------------------" << NEWLINE;
cout << "This constraint is not reifiable." << NEWLINE << NEWLINE << NEWLINE;
} else
if("constraints watchvecneq" == request) {
cout << "Help entry: " << "constraints watchvecneq" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "The constraint" << NEWLINE
<< "" << NEWLINE
<< " watchvecneq(A, B)" << NEWLINE
<< "" << NEWLINE
<< "ensures that A and B are not the same vector, i.e., there exists some index i" << NEWLINE
<< "such that A[i] != B[i]." << NEWLINE << NEWLINE << NEWLINE;
cout << "Reifiability" << "--------------------------------------------------------------------" << NEWLINE;
cout << "This constraint is not reifiable." << NEWLINE << NEWLINE << NEWLINE;
} else
if("constraints watchvecexists_less" == request) {
cout << "Help entry: " << "constraints watchvecexists_less" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "The constraint" << NEWLINE
<< "" << NEWLINE
<< " watchvecexists_less(A, B)" << NEWLINE
<< "" << NEWLINE
<< "ensures that there exists some index i such that A[i] < B[i]." << NEWLINE << NEWLINE << NEWLINE;
cout << "Reifiability" << "--------------------------------------------------------------------" << NEWLINE;
cout << "This constraint is not reifiable." << NEWLINE << NEWLINE << NEWLINE;
} else
if("constraints watchvecexists_and" == request) {
cout << "Help entry: " << "constraints watchvecexists_and" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "The constraint" << NEWLINE
<< "" << NEWLINE
<< " watchvecexists_less(A, B)" << NEWLINE
<< "" << NEWLINE
<< "ensures that there exists some index i such that A[i] > 0 and B[i] > 0." << NEWLINE
<< "" << NEWLINE
<< "For booleans this is the same as 'exists i s.t. A[i] && B[i]'." << NEWLINE << NEWLINE << NEWLINE;
cout << "Reifiability" << "--------------------------------------------------------------------" << NEWLINE;
cout << "This constraint is not reifiable." << NEWLINE << NEWLINE << NEWLINE;
} else
if("constraints watchelement" == request) {
cout << "Help entry: " << "constraints watchelement" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "The constraint " << NEWLINE
<< "" << NEWLINE
<< " watchelement(vec, i, e)" << NEWLINE
<< "" << NEWLINE
<< "specifies that, in any solution, vec[i] = e." << NEWLINE << NEWLINE << NEWLINE;
cout << "Reifiability" << "--------------------------------------------------------------------" << NEWLINE;
cout << "This constraint is NOT reifiable." << NEWLINE << NEWLINE << NEWLINE;
cout << "Notes" << "---------------------------------------------------------------------------" << NEWLINE;
cout << "Enforces generalised arc consistency." << NEWLINE << NEWLINE << NEWLINE;
cout << "References" << "----------------------------------------------------------------------" << NEWLINE;
cout << "See entry" << NEWLINE
<< "" << NEWLINE
<< " help constraints element" << NEWLINE
<< "" << NEWLINE
<< "for details of an identical constraint that enforces a lower level of" << NEWLINE
<< "consistency." << NEWLINE << NEWLINE << NEWLINE;
} else
if("constraints litsumgeq" == request) {
cout << "Help entry: " << "constraints litsumgeq" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "The constraint litsumgeq(vec1, vec2, c) ensures that there exists at least c" << NEWLINE
<< "distinct indices i such that vec1[i] = vec2[i]." << NEWLINE << NEWLINE << NEWLINE;
cout << "Notes" << "---------------------------------------------------------------------------" << NEWLINE;
cout << "A SAT clause {x,y,z} can be created using:" << NEWLINE
<< "" << NEWLINE
<< " litsumgeq([x,y,z],[1,1,1],1)" << NEWLINE
<< "" << NEWLINE
<< "Note also that this constraint is more efficient for smaller values of c. For" << NEWLINE
<< "large values consider using watchsumleq." << NEWLINE << NEWLINE << NEWLINE;
cout << "Reifiability" << "--------------------------------------------------------------------" << NEWLINE;
cout << "This constraint is NOT reifiable." << NEWLINE << NEWLINE << NEWLINE;
cout << "References" << "----------------------------------------------------------------------" << NEWLINE;
cout << "See also" << NEWLINE
<< "" << NEWLINE
<< " help constraints watchsumleq" << NEWLINE
<< " help constraints watchsumgeq" << NEWLINE << NEWLINE << NEWLINE;
} else
if("constraints watchsumgeq" == request) {
cout << "Help entry: " << "constraints watchsumgeq" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "The constraint watchsumgeq(vec, c) ensures that sum(vec) >= c." << NEWLINE << NEWLINE << NEWLINE;
cout << "Notes" << "---------------------------------------------------------------------------" << NEWLINE;
cout << "Equivalent to litsumgeq(vec, [1,...,1], c), but faster." << NEWLINE << NEWLINE << NEWLINE;
cout << "Notes" << "---------------------------------------------------------------------------" << NEWLINE;
cout << "This constraint works on 0/1 variables only." << NEWLINE << NEWLINE << NEWLINE;
cout << "Reifiablity" << "---------------------------------------------------------------------" << NEWLINE;
cout << "This constraint is NOT reifiable." << NEWLINE << NEWLINE << NEWLINE;
cout << "References" << "----------------------------------------------------------------------" << NEWLINE;
cout << "See also" << NEWLINE
<< "" << NEWLINE
<< " help constraints watchsumleq " << NEWLINE
<< " help constraints litsumgeq" << NEWLINE << NEWLINE << NEWLINE;
} else
if("constraints watchsumleq" == request) {
cout << "Help entry: " << "constraints watchsumleq" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "The constraint watchsumleq(vec, c) ensures that sum(vec) <= c." << NEWLINE << NEWLINE << NEWLINE;
cout << "Notes" << "---------------------------------------------------------------------------" << NEWLINE;
cout << "For this constraint, small values of c are more efficient." << NEWLINE << NEWLINE << NEWLINE;
cout << "Notes" << "---------------------------------------------------------------------------" << NEWLINE;
cout << "Equivelent to litsumgeq([vec1,...,vecn], [0,...,0], n-c) but faster." << NEWLINE << NEWLINE << NEWLINE;
cout << "Notes" << "---------------------------------------------------------------------------" << NEWLINE;
cout << "This constraint works on binary variables only." << NEWLINE << NEWLINE << NEWLINE;
cout << "Notes" << "---------------------------------------------------------------------------" << NEWLINE;
cout << "For this constraint, large values of c are more efficient." << NEWLINE << NEWLINE << NEWLINE;
cout << "Reifiablity" << "---------------------------------------------------------------------" << NEWLINE;
cout << "This constraint is NOT reifiable." << NEWLINE << NEWLINE << NEWLINE;
cout << "References" << "----------------------------------------------------------------------" << NEWLINE;
cout << "See also" << NEWLINE
<< "" << NEWLINE
<< " help constraints watchsumgeq " << NEWLINE
<< " help constraints litsumgeq" << NEWLINE << NEWLINE << NEWLINE;
} else
if("variables 01" == request) {
cout << "Help entry: " << "variables 01" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "01 variables are used very commonly for logical expressions, and for" << NEWLINE
<< "encoding the characteristic functions of sets and relations. Note that" << NEWLINE
<< "wherever a 01 variable can appear, the negation of that variable can" << NEWLINE
<< "also appear. A boolean variable x's negation is identified by !x." << NEWLINE << NEWLINE << NEWLINE;
cout << "Example" << "-------------------------------------------------------------------------" << NEWLINE;
cout << "Declaration of a 01 variable called bool in input file:" << NEWLINE
<< "" << NEWLINE
<< "BOOL bool" << NEWLINE
<< "" << NEWLINE
<< "Use of this variable in a constraint:" << NEWLINE
<< "" << NEWLINE
<< "eq(bool, 0) #variable bool equals 0" << NEWLINE << NEWLINE << NEWLINE;
} else
if("variables bounds" == request) {
cout << "Help entry: " << "variables bounds" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "Bounds variables, where only the upper and lower bounds of the domain" << NEWLINE
<< "are maintained. These domains must be continuous ranges of integers" << NEWLINE
<< "i.e. holes cannot be put in the domains of the variables." << NEWLINE << NEWLINE << NEWLINE;
cout << "Example" << "-------------------------------------------------------------------------" << NEWLINE;
cout << "" << NEWLINE
<< "Declaration of a bound variable called myvar with domain between 1" << NEWLINE
<< "and 7 in input file:" << NEWLINE
<< "" << NEWLINE
<< "BOUND myvar {1..7}" << NEWLINE
<< "" << NEWLINE
<< "Use of this variable in a constraint:" << NEWLINE
<< "" << NEWLINE
<< "eq(myvar, 4) #variable myvar equals 4" << NEWLINE << NEWLINE << NEWLINE;
} else
if("variables sparsebounds" == request) {
cout << "Help entry: " << "variables sparsebounds" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "In sparse bounds variables the domain is composed of discrete values" << NEWLINE
<< "(e.g. {1, 5, 36, 92}), but only the upper and lower bounds of the" << NEWLINE
<< "domain may be updated during search. Although the domain of these" << NEWLINE
<< "variables is not a continuous range, any holes in the domains must be" << NEWLINE
<< "there at time of specification, as they can not be added during the" << NEWLINE
<< "solving process." << NEWLINE << NEWLINE << NEWLINE;
cout << "Notes" << "---------------------------------------------------------------------------" << NEWLINE;
cout << "Declaration of a sparse bounds variable called myvar containing values" << NEWLINE
<< "{1,3,4,6,7,9,11} in input file:" << NEWLINE
<< "" << NEWLINE
<< "SPARSEBOUND myvar {1,3,4,6,7,9,11}" << NEWLINE
<< "" << NEWLINE
<< "Use of this variable in a constraint:" << NEWLINE
<< "eq(myvar, 3) #myvar equals 3" << NEWLINE << NEWLINE << NEWLINE;
} else
if("variables discrete" == request) {
cout << "Help entry: " << "variables discrete" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "In discrete variables, the domain ranges between the specified lower and upper" << NEWLINE
<< "bounds, but during search any domain value may be pruned, i.e., propagation and" << NEWLINE
<< "search may punch arbitrary holes in the domain." << NEWLINE << NEWLINE << NEWLINE;
cout << "Example" << "-------------------------------------------------------------------------" << NEWLINE;
cout << "Declaration of a discrete variable x with domain {1,2,3,4} in input file:" << NEWLINE
<< "" << NEWLINE
<< "DISCRETE x {1..4}" << NEWLINE
<< "" << NEWLINE
<< "Use of this variable in a constraint:" << NEWLINE
<< "" << NEWLINE
<< "eq(x, 2) #variable x equals 2" << NEWLINE << NEWLINE << NEWLINE;
} else
if("variables" == request) {
cout << "Help entry: " << "variables" << NEWLINE << NEWLINE;
cout << "General" << "-------------------------------------------------------------------------" << NEWLINE;
cout << "Minion supports 4 different variable types, namely" << NEWLINE
<< "" << NEWLINE
<< "- 0/1 variables," << NEWLINE
<< "- bounds variables," << NEWLINE
<< "- sparse bounds variables, and" << NEWLINE
<< "- discrete variables." << NEWLINE
<< "" << NEWLINE
<< "Sub-dividing the variable types in this manner affords the greatest" << NEWLINE
<< "opportunity for optimisation. In general, we recommend thinking of the" << NEWLINE
<< "variable types as a hierarchy, where 1 (0/1 variables) is the most" << NEWLINE
<< "efficient type, and 4 (Discrete variables) is the least. The" << NEWLINE
<< "user should use the variable which is the highest in the hierarchy," << NEWLINE
<< "yet encompasses enough information to provide a full model for the" << NEWLINE
<< "problem they are attempting to solve." << NEWLINE
<< "" << NEWLINE
<< "See the entry on vectors for information on how vectors, matrices and," << NEWLINE
<< "more generally, tensors are handled in minion input. See also the" << NEWLINE
<< "alias entry for information on how to multiply name variables for" << NEWLINE
<< "convenience." << NEWLINE << NEWLINE << NEWLINE;
} else
if("variables vectors" == request) {
cout << "Help entry: " << "variables vectors" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "Vectors, matrices and tensors can be declared in minion" << NEWLINE
<< "input. Matrices and tensors are for convenience, as constraints do not" << NEWLINE
<< "take these as input; they must first undergo a flattening process to" << NEWLINE
<< "convert them to a vector before use." << NEWLINE << NEWLINE << NEWLINE;
cout << "Examples" << "------------------------------------------------------------------------" << NEWLINE;
cout << "A vector of 0/1 variables:" << NEWLINE
<< "" << NEWLINE
<< "BOOL myvec[5]" << NEWLINE
<< "" << NEWLINE
<< "A matrix of discrete variables:" << NEWLINE
<< "" << NEWLINE
<< "DISCRETE sudoku[9,9] {1..9}" << NEWLINE
<< "" << NEWLINE
<< "A 3D tensor of 0/1s:" << NEWLINE
<< "" << NEWLINE
<< "BOOL mycube[3,3,2]" << NEWLINE
<< "" << NEWLINE
<< "One can create a vector from scalars and elements of vectors, etc.:" << NEWLINE
<< "" << NEWLINE
<< "alldiff([x,y,myvec[1],mymatrix[3,4]])" << NEWLINE
<< "" << NEWLINE
<< "When a matrix or tensor is constrained, it is treated as a vector" << NEWLINE
<< "whose entries have been strung out into a vector in index order with" << NEWLINE
<< "the rightmost index changing most quickly, e.g." << NEWLINE
<< "" << NEWLINE
<< "alldiff(sudoku)" << NEWLINE
<< "" << NEWLINE
<< "is equivalent to" << NEWLINE
<< "" << NEWLINE
<< "alldiff([sudoku[0,0],...,sudoku[0,8],...,sudoku[8,0],...,sudoku[8,8]])" << NEWLINE
<< "" << NEWLINE
<< "Furthermore, with indices filled selectively and the remainder filled" << NEWLINE
<< "with underscores (_) the flattening applies only to the underscore" << NEWLINE
<< "indices:" << NEWLINE
<< "" << NEWLINE
<< "alldiff(sudoku[4,_])" << NEWLINE
<< "" << NEWLINE
<< "is equivalent to" << NEWLINE
<< "" << NEWLINE
<< "alldiff([sudoku[4,0],...,sudoku[4,8]])" << NEWLINE
<< "" << NEWLINE
<< "Lastly, one can optionally add square brackets ([]) around an" << NEWLINE
<< "expression to be flattened to make it look more like a vector:" << NEWLINE
<< "" << NEWLINE
<< "alldiff([sudoku[4,_]])" << NEWLINE
<< "" << NEWLINE
<< "is equivalent to" << NEWLINE
<< "" << NEWLINE
<< "alldiff(sudoku[4,_])" << NEWLINE << NEWLINE << NEWLINE;
} else
if("variables alias" == request) {
cout << "Help entry: " << "variables alias" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "Specifying an alias is a way to give a variable another name. Aliases" << NEWLINE
<< "appear in the **VARIABLES** section of an input file. It is best" << NEWLINE
<< "described using some examples:" << NEWLINE
<< "" << NEWLINE
<< "ALIAS c = a" << NEWLINE
<< "" << NEWLINE
<< "ALIAS c[2,2] = [[myvar,b[2]],[b[1],anothervar]]" << NEWLINE << NEWLINE << NEWLINE;
} else
cout << "Unknown entry, please try again." << NEWLINE;
if("" == request) {
cout << "Available subentries:" << NEWLINE;
cout << "help constraints" << NEWLINE;
cout << "help input" << NEWLINE;
cout << "help switches" << NEWLINE;
cout << "help variables" << NEWLINE;
} else
if("constraints" == request) {
cout << "Available subentries:" << NEWLINE;
cout << "help constraints alldiff" << NEWLINE;
cout << "help constraints alldiffgacslow" << NEWLINE;
cout << "help constraints diseq" << NEWLINE;
cout << "help constraints div" << NEWLINE;
cout << "help constraints element" << NEWLINE;
cout << "help constraints eq" << NEWLINE;
cout << "help constraints hamming" << NEWLINE;
cout << "help constraints ineq" << NEWLINE;
cout << "help constraints lexleq" << NEWLINE;
cout << "help constraints lexless" << NEWLINE;
cout << "help constraints litsumgeq" << NEWLINE;
cout << "help constraints max" << NEWLINE;
cout << "help constraints min" << NEWLINE;
cout << "help constraints minuseq" << NEWLINE;
cout << "help constraints modulo" << NEWLINE;
cout << "help constraints occurrence" << NEWLINE;
cout << "help constraints occurrencegeq" << NEWLINE;
cout << "help constraints occurrenceleq" << NEWLINE;
cout << "help constraints pow" << NEWLINE;
cout << "help constraints product" << NEWLINE;
cout << "help constraints reification" << NEWLINE;
cout << "help constraints reify" << NEWLINE;
cout << "help constraints reifyimply" << NEWLINE;
cout << "help constraints sumgeq" << NEWLINE;
cout << "help constraints sumleq" << NEWLINE;
cout << "help constraints table" << NEWLINE;
cout << "help constraints watchelement" << NEWLINE;
cout << "help constraints watchsumgeq" << NEWLINE;
cout << "help constraints watchsumleq" << NEWLINE;
cout << "help constraints watchvecexists_and" << NEWLINE;
cout << "help constraints watchvecexists_less" << NEWLINE;
cout << "help constraints watchvecneq" << NEWLINE;
cout << "help constraints weightedsumgeq" << NEWLINE;
cout << "help constraints weightedsumleq" << NEWLINE;
} else
if("input" == request) {
cout << "Available subentries:" << NEWLINE;
cout << "help input constraints" << NEWLINE;
cout << "help input example" << NEWLINE;
cout << "help input search" << NEWLINE;
cout << "help input tuplelist" << NEWLINE;
cout << "help input variables" << NEWLINE;
} else
if("switches" == request) {
cout << "Available subentries:" << NEWLINE;
cout << "help switches -check" << NEWLINE;
cout << "help switches -dumptree" << NEWLINE;
cout << "help switches -findallsols" << NEWLINE;
cout << "help switches -fullprop" << NEWLINE;
cout << "help switches -nocheck" << NEWLINE;
cout << "help switches -nodelimit" << NEWLINE;
cout << "help switches -noprintsols" << NEWLINE;
cout << "help switches -preprocess" << NEWLINE;
cout << "help switches -printsols" << NEWLINE;
cout << "help switches -printsolsonly" << NEWLINE;
cout << "help switches -quiet" << NEWLINE;
cout << "help switches -randomiseorder" << NEWLINE;
cout << "help switches -randomseed" << NEWLINE;
cout << "help switches -sollimit" << NEWLINE;
cout << "help switches -solsout" << NEWLINE;
cout << "help switches -tableout" << NEWLINE;
cout << "help switches -timelimit" << NEWLINE;
cout << "help switches -varorder" << NEWLINE;
cout << "help switches -verbose" << NEWLINE;
cout << "help switches -X-prop-node" << NEWLINE;
} else
if("variables" == request) {
cout << "Available subentries:" << NEWLINE;
cout << "help variables 01" << NEWLINE;
cout << "help variables alias" << NEWLINE;
cout << "help variables bounds" << NEWLINE;
cout << "help variables discrete" << NEWLINE;
cout << "help variables sparsebounds" << NEWLINE;
cout << "help variables vectors" << NEWLINE;
} else
;
}
