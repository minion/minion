#include <iostream>
#include <string>
using namespace std;
#define NEWLINE '\n'
void help(string request)
{
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
if("variables discrete" == request) {
cout << "Help entry: " << "variables discrete" << NEWLINE << NEWLINE;
cout << "Description" << "---------------------------------------------------------------------" << NEWLINE;
cout << "In discrete variables the domain ranges from the lower bound to the" << NEWLINE
<< "upper bound specified, but the deletion of any domain element in this" << NEWLINE
<< "range is permitted. This means that holes can be put in the domain of" << NEWLINE
<< "these variables." << NEWLINE << NEWLINE << NEWLINE;
cout << "Example" << "-------------------------------------------------------------------------" << NEWLINE;
cout << "Declaration of a discrete variable called myvar containing values" << NEWLINE
<< "1 through to 10 inclusive in input file:" << NEWLINE
<< "" << NEWLINE
<< "DISCRETE myvar {1..10}" << NEWLINE
<< "" << NEWLINE
<< "Use of this variable in a constraint:" << NEWLINE
<< "eq(bool, 3)" << NEWLINE << NEWLINE << NEWLINE;
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
cout << "In discrete variables the domain ranges from the lower bound to the" << NEWLINE
<< "upper bound specified, but the deletion of any domain element in this" << NEWLINE
<< "range is permitted. This means that holes can be put in the domain of" << NEWLINE
<< "these variables." << NEWLINE << NEWLINE << NEWLINE;
cout << "Example" << "-------------------------------------------------------------------------" << NEWLINE;
cout << "Declaration of a discrete variable called myvar containing values" << NEWLINE
<< "1 through to 10 inclusive in input file:" << NEWLINE
<< "" << NEWLINE
<< "DISCRETE myvar {1..10}" << NEWLINE
<< "" << NEWLINE
<< "Use of this variable in a constraint:" << NEWLINE
<< "eq(bool, 3)" << NEWLINE << NEWLINE << NEWLINE;
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
if("variables" == request) {
cout << "Help entry: " << "variables" << NEWLINE << NEWLINE;
cout << "General" << "-------------------------------------------------------------------------" << NEWLINE;
cout << "Minion nominally supports 5 different variable types, namely" << NEWLINE
<< "" << NEWLINE
<< "- 0/1 variables," << NEWLINE
<< "- bounds variables," << NEWLINE
<< "- sparse bounds variables," << NEWLINE
<< "- discrete variables, and" << NEWLINE
<< "- discrete sparse variables." << NEWLINE
<< "" << NEWLINE
<< "Sub-dividing the variable types in this manner affords the greatest" << NEWLINE
<< "opportunity for optimisation. In general, we recommend thinking of the" << NEWLINE
<< "variable types as a hierarchy, where 1 (0/1 variables) is the most" << NEWLINE
<< "efficient type, and 5 (Discrete Sparse Variables) is the least. The" << NEWLINE
<< "user should use the variable which is the highest in the hierarchy," << NEWLINE
<< "yet encompasses enough information to provide a full model for the" << NEWLINE
<< "problem they are attempting to solve." << NEWLINE << NEWLINE << NEWLINE;
cout << "Notes" << "---------------------------------------------------------------------------" << NEWLINE;
cout << "Discrete sparse variables are not yet implemented, and some of the" << NEWLINE
<< "others only work with certain constraints (particularly bounds" << NEWLINE
<< "variables). See entries for individual constraints for any problematic" << NEWLINE
<< "variable types." << NEWLINE << NEWLINE << NEWLINE;
cout << "General" << "-------------------------------------------------------------------------" << NEWLINE;
cout << "Minion nominally supports 5 different variable types, namely" << NEWLINE
<< "" << NEWLINE
<< "- 0/1 variables," << NEWLINE
<< "- bounds variables," << NEWLINE
<< "- sparse bounds variables," << NEWLINE
<< "- discrete variables, and" << NEWLINE
<< "- discrete sparse variables." << NEWLINE
<< "" << NEWLINE
<< "Sub-dividing the variable types in this manner affords the greatest" << NEWLINE
<< "opportunity for optimisation. In general, we recommend thinking of the" << NEWLINE
<< "variable types as a hierarchy, where 1 (0/1 variables) is the most" << NEWLINE
<< "efficient type, and 5 (Discrete Sparse Variables) is the least. The" << NEWLINE
<< "user should use the variable which is the highest in the hierarchy," << NEWLINE
<< "yet encompasses enough information to provide a full model for the" << NEWLINE
<< "problem they are attempting to solve." << NEWLINE << NEWLINE << NEWLINE;
cout << "Notes" << "---------------------------------------------------------------------------" << NEWLINE;
cout << "Discrete sparse variables are not yet implemented, and some of the" << NEWLINE
<< "others only work with certain constraints (particularly bounds" << NEWLINE
<< "variables). See entries for individual constraints for any problematic" << NEWLINE
<< "variable types." << NEWLINE << NEWLINE << NEWLINE;
} else
cout << "Unknown entry, please try again." << NEWLINE;
if("" == request) {
cout << "Available subentries:" << NEWLINE;
cout << "variables" << NEWLINE;
} else
if("variables" == request) {
cout << "Available subentries:" << NEWLINE;
cout << "variables 01" << NEWLINE;
cout << "variables bounds" << NEWLINE;
cout << "variables discrete" << NEWLINE;
cout << "variables sparsebounds" << NEWLINE;
} else
;
}
