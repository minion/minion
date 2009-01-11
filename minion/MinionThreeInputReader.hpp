/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id: MinionThreeInputReader.cpp 615 2007-07-17 12:48:20Z azumanga $
*/

// MinionThreeInputReader.cpp
//
// Subversion Identity $Id: MinionThreeInputReader.cpp 615 2007-07-17 12:48:20Z azumanga $
//
// Plan here is to generate an instance of a problem (or whatever you have)
// and return that.

/** @help input Description

Minion expects to be provided with the name of an input file as an
argument. This file contains a specification of the CSP to be solved
as well as settings that the search process should use. The format is

Minion3Input::= MINION 3
                <InputSection>+
                **EOF**

InputSection::= <VariablesSection> 
              | <SearchSection>
              | <ConstraintsSection> 
              | <TuplelistSection>

i.e. 'MINION 3' followed by any number of variable, search,
constraints and tuplelists sections (can repeat) followed by
'**EOF**', the end of file marker.

All text from a '#' character to the end of the line is ignored.

See the associated help entries below for information on each section.
*/

/** @help input Notes 
You can give an input file via standard input by specifying '--' as the file
name, this might help when minion is being used as a tool in a shell script or
for compressed input, e.g.,

   gunzip -c myinput.minion.gz | minion
*/

/** @help input;variables Description
The variables section consists of any number of variable declarations
on separate lines.

VariablesSection::= **VARIABLES**
                    <VarDeclaration>*
*/

/** @help input;variables Example
 **VARIABLES**

BOOL bool                          #boolean var
BOUND b {1..3}                     #bounds var
SPARSEBOUND myvar {1,3,4,6,7,9,11} #sparse bounds var
DISCRETE d[3] {1..3}               #array of discrete vars
*/

/** @help input;variables References
See the help section

   help variables

for detailed information on variable declarations.
*/

/** @help input;constraints Description 

The constraints section consists of any number of constraint
declarations on separate lines.

ConstraintsSection::= **CONSTRAINTS**
                      <ConstraintDeclaration>*
*/

/** @help input;constraints Example
**CONSTRAINTS**
eq(bool,0)
alldiff(d)
*/

/** @help input;constraints References
See help entries for individual constraints under

   help constraints

for details on constraint declarations.
*/

/** @help input;tuplelist Description
In a tuplelist section lists of allowed tuples for table constraints
can be specified. This technique is preferable to specifying the
tuples in the constraint declaration, since the tuplelists can be
shared between constraints and named for readability.

The required format is

TuplelistSection::= **TUPLELIST**
                    <Tuplelist>*

Tuplelist::= <name> <num_tuples> <tuple_length> <numbers>+
*/

/** @help input;tuplelist Example
**TUPLELIST**
AtMostOne 4 3
0 0 0
0 0 1
0 1 0
1 0 0
*/

/** @help input;tuplelist References
help constraints table
*/

/** @help input;search Description 

Inside the search section one can specify

- variable orderings, 
- value orderings,
- optimisation function, and
- details of how to print out solutions.

SearchSection::= <VariableOrdering>?
                 <ValueOrdering>?
                 <OptimisationFn>?
                 <PrintFormat>?

In the variable ordering a fixed ordering can be specified on any
subset of variables. These are the search variables that will be
instantiated in every solution. If none is specified some other fixed
ordering of all the variables will be used.

   VariableOrdering::= VARORDER[ <varname>+ ]

The value ordering allows the user to specify an instantiation order
for the variables involved in the variable order, either ascending (a)
or descending (d) for each. When no value ordering is specified, the
default is to use ascending order for every search variable.

   ValueOrdering::= VALORDER[ (a|d)+ ]

To model an optimisation problem the user can specify to minimise
or maximise a variable's value.

   OptimisationFn::= MAXIMISING <varname>
                   | MINIMISING <varname>

Finally, the user can control some aspects of the way solutions are
printed. By default (no PrintFormat specified) all the variables are
printed in declaration order. Alternatively a custom vector, or ALL
variables, or no (NONE) variables can be printed. If a matrix or, more
generally, a tensor is given instead of a vector, it is automatically
flattened into a vector as described in 'help variables vectors'.

   PrintFormat::= PRINT <vector>
                | PRINT ALL
                | PRINT NONE
*/

/** @help input;example Example
Below is a complete minion input file with commentary, as an example.

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
# The input is: <name> <num_of_tuples> <tuple_length> <numbers...>
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
*/

#include <string>
#include "system/system.h"

#include "CSPSpec.h"

#include "MinionInputReader.h"

template<typename FileReader>
void MinionThreeInputReader<FileReader>::parser_info(string s)
{
  if(parser_verbose)
    cout << s << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// read
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
template<typename FileReader>
void MinionThreeInputReader<FileReader>::read(FileReader* infile) {  
  string s = infile->get_asciistring();
  parser_info("Read: '" + s + "'");

  string eof, wrong_eof;

  if(isGadgetReader())
  {
    eof = "**GADGET_END**";
    wrong_eof = "**EOF**";
  }
  else
  {
    eof = "**EOF**";
    wrong_eof = "**GADGET_END**";
  }

  while(s != eof)
  {
    if(s == "**VARIABLES**")
      readVars(infile);
    else if(s == "**SEARCH**")
      readSearch(infile);
    else if(s == "**TUPLELIST**")
      readTuples(infile);
    else if(s =="**CONSTRAINTS**")
    {
      while(infile->peek_char() != '*')
        instance.constraints.push_back(readConstraint(infile, false));
    }
    else if(s == "**GADGET**")
      { readGadget(infile); }
    else if(s == wrong_eof)
    { 
      throw parse_exception("Section terminated with " + wrong_eof + 
        " instead of " + eof);
    }
    else
      throw parse_exception("Don't understand '" + s + "' as a section header");
    s = infile->get_asciistring();
    parser_info("Read: '" + s + "'");
  }

  parser_info("Reached end of CSP");

  if(isGadgetReader() && instance.constructionSite.empty())
    throw parse_exception("Gadgets need a construction site!");

  // Fill in any missing defaults
  if(instance.search_order.empty())
  {
    parser_info("No order generated, auto-generating complete order");
    instance.search_order.push_back(instance.vars.get_all_vars());
  }

  for(int i = 0; i < instance.search_order.size(); ++i)
    instance.search_order[i].setupValueOrder();

  // This has to be delayed unless not all variables are defined where 'PRINT ALL' occurs.
  if(print_all_vars)
    instance.print_matrix = instance.all_vars_list;
    
  if(instance.sym_order.empty())
    instance.sym_order = instance.vars.get_all_vars();
    
  if(instance.sym_order.size() != instance.vars.get_all_vars().size())
    throw parse_exception("SYMORDER must contain every variable");
    
  if(instance.sym_order.size() != set<Var>(instance.sym_order.begin(), instance.sym_order.end()).size())
    throw parse_exception("SYMORDER cannot contain any variable more than once");
}

template<typename FileReader>
void MinionThreeInputReader<FileReader>::readGadget(FileReader* infile)
{
  parser_info("Entering gadget parsing");
  if(isGadgetReader())
    throw parse_exception("Gadgets can't have gadgets!");

  infile->check_string("NAME");
  string name = infile->get_string();
  parser_info("Gadget name:" + name);

  MinionThreeInputReader gadget(parser_verbose);
  gadget.setGadgetReader();
  gadget.read(infile);

  // Take the CSPInstance out of the Minion3InputReader, and make a copy of it.
  instance.addGadgetSymbol(name, shared_ptr<CSPInstance>(new CSPInstance(MOVE(gadget.instance))));
  parser_info("Exiting gadget parsing");
}


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// readConstraint
// Recognise constraint by its name, read past name and leading '('
// Return false if eof or unknown ct. Else true.
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
template<typename FileReader>
ConstraintBlob MinionThreeInputReader<FileReader>::readConstraint(FileReader* infile, BOOL reified) {
  string id = infile->getline('(');

  int constraint_num = -1;
  for(int i = 0; i < num_of_constraints; ++i)
  {
    if(constraint_list[i].name == id)
    {
      constraint_num = i;
      break;
    }
  }

  if(constraint_num == -1) 
  {
    if (infile->eof()) 
    {
      throw parse_exception(string("Bad Constraint Name or reached end of file: '") + id + "'");
    }
    else
      { throw parse_exception(string("Unknown Constraint:") + id); }
  }
  ConstraintDef* constraint = constraint_list + constraint_num;

  if( constraint->trig_type == DYNAMIC_CT )
  {
#ifndef WATCHEDLITERALS
    cerr << "This version of Minion was not complied with -WATCHEDLITERALS" << endl;
    cerr << "So there is not support for the " << constraint.name << "." << endl;
    exit(1);
#else
#ifdef CT_REIFY_ABC
    if(reified && constraint->type == CT_REIFY)
    {
      FAIL_EXIT("Cannot reify a watched constraint!");
    }
#endif
#endif
  }

  switch(constraint->type)
  {
#ifdef CT_WATCHED_OR_ABC
    case CT_WATCHED_OR:
    return readConstraintOr(infile, get_constraint(CT_WATCHED_OR));
    break;
#endif

#ifdef CT_GADGET_ABC
    case CT_GADGET:
    return readConstraintGadget(infile);
    break;
#endif

    default:
    return readGeneralConstraint(infile, constraint);
  }
  // g++ seems to think compilation can get here. I disagree, but putting a catch doesn't hurt.
  throw parse_exception("Fatal error in parsing constraints");
}


template<typename FileReader>
ConstraintBlob MinionThreeInputReader<FileReader>::readGeneralConstraint(FileReader* infile, ConstraintDef* def)
{
  // This slightly strange code is to save copying the ConstraintBlob as much as possible.
  ConstraintBlob con(def);
  vector<vector<Var> >& varsblob = con.vars;
  vector<vector<int> >& constblob = con.constants;

  for(int i = 0; i < def->number_of_params; ++i)
  {
    switch(def->read_types[i])
    {
      case read_list:
      varsblob.push_back(readLiteralVector(infile));
      break;
      case read_var:
      varsblob.push_back(make_vec(readIdentifier(infile)));
      break;
      case read_bool_var:
      varsblob.push_back(make_vec(readIdentifier(infile)));
      if(varsblob.back().back().type() != VAR_BOOL)
        throw parse_exception("Expected Boolean variable!");
      break;
      case read_2_vars:
      {
        vector<Var> vars(2);
        vars[0] = readIdentifier(infile);
        infile->check_sym(',');
        vars[1] = readIdentifier(infile);
        varsblob.push_back(MOVE(vars));
      }
      break;
      case read_constant:
      constblob.push_back(make_vec(infile->read_num()));
      break;
      case read_constant_list:
      {
        vector<Var> vectorOfConst = readLiteralVector(infile);
        vector<int> vals;
    		for(unsigned int loop = 0; loop < vectorOfConst.size(); ++loop)
    		{
    		  if(vectorOfConst[loop].type() != VAR_CONSTANT)
    			  throw parse_exception("Vector must only contain constants.");
    			else
            vals.push_back(vectorOfConst[loop].pos());
    		}
    		constblob.push_back(MOVE(vals));
      }
      break;  
      case read_constraint:
      con.internal_constraints.push_back(readConstraint(infile, false));
      break;
      case read_constraint_list:
      con.internal_constraints = readConstraintList(infile);
      break;
      case read_tuples:
      con.tuples = readConstraintTupleList(infile);
      break;
      default:
      D_FATAL_ERROR("Internal Error!");
    }
    if(i != def->number_of_params - 1)
      infile->check_sym(',');
  }
  infile->check_sym(')');
  
  return con;
}

template<typename FileReader>
TupleList* MinionThreeInputReader<FileReader>::readConstraintTupleList(FileReader* infile)
{
  TupleList* tuplelist;
  
  if(infile->peek_char() != '{')
  {
    string name = infile->get_string();
    tuplelist = instance.getTableSymbol(name);
  }
  else
  {
    vector<vector<int> > tuples ;
    infile->check_sym('{');
    char delim = infile->peek_char();
    
    int tupleSize = 0;
    
    while (delim != '}') 
    {
      infile->check_sym('<');
      vector<int> tuple;
      // Optimisation
      tuple.reserve(tupleSize);
      
      char next_char = ',';
      while(next_char == ',')
      {
        tuple.push_back(infile->read_num());
        next_char = infile->get_char();
      }
      if(next_char != '>')
        throw parse_exception("Expected ',' or '>'");
      
      if(tupleSize == 0)
        tupleSize = tuple.size();
      if(tupleSize != tuple.size())
        throw parse_exception("All tuples in each constraint must be the same size!");
      tuples.push_back(MOVE(tuple)) ;

      delim = infile->get_char();                          // ',' or '}'
      if(delim != ',' && delim!= '}')
        throw parse_exception("Expected ',' or '}'");
    }
    tuplelist = instance.tupleListContainer->getNewTupleList(tuples);
    instance.addUnnamedTableSymbol(tuplelist);
  }
  
  return tuplelist;
}

/*
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// readConstraintTable
// table(<vectorOfVars>, {<tuple> [, <tuple>]})
// Tuples represented as a vector of int arrays.
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
template<typename FileReader>
ConstraintBlob MinionThreeInputReader<FileReader>::readConstraintTable(FileReader* infile, ConstraintDef* def) 
{
  parser_info( "reading a table ct (unreifiable)" ) ;

  vector<Var> vectorOfVars = readLiteralVector(infile) ;
  int tupleSize = vectorOfVars.size() ;

  infile->check_sym(',');

  TupleList* tuplelist = readConstraintTupleList(infile);

  infile->check_sym(')');
  ConstraintBlob tableCon(def, vectorOfVars);
  tableCon.tuples = tuplelist;
  return tableCon;
}*/

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// readConstraintTable
// table(<vectorOfVars>, {<tuple> [, <tuple>]})
// Tuples represented as a vector of int arrays.
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#ifdef CT_GADGET_ABC
template<typename FileReader>
ConstraintBlob MinionThreeInputReader<FileReader>::readConstraintGadget(FileReader* infile) 
{
  parser_info( "Reading a gadget constraint" ) ;

  vector<Var> vectorOfVars = readLiteralVector(infile) ;

  infile->check_sym(',');

  string s = infile->get_string();

  parser_info( "Gadget name: '" + s + "'");
  shared_ptr<CSPInstance> in_gadget = instance.getGadgetSymbol(s);
  ConstraintBlob gadgetCon( get_constraint(CT_GADGET) , vectorOfVars);
  gadgetCon.gadget = in_gadget;
  infile->check_sym(',');
  gadgetCon.gadget_prop_type = GetPropMethodFromString(infile->get_string());
  infile->check_sym(')');
  parser_info("End gadget reading");
  return gadgetCon;
}
#endif

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// readConstraintOr
// or(<vectorOfVars>)
// SAT clauses represented as literals and negated literals
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
template<typename FileReader>
ConstraintBlob MinionThreeInputReader<FileReader>::readConstraintOr(FileReader* infile, 
  ConstraintDef* ct)
{
  parser_info("Reading a SAT clause");
  infile->check_sym('[');
  vector<int> negs;
  vector<Var> clause_vars;
  if(infile->peek_char() != ']')
    clause_vars.push_back(readIdentifier(infile));
  while(infile->peek_char() != ']') {
    infile->check_sym(',');
    clause_vars.push_back(readIdentifier(infile));
  }
  infile->check_sym(']');
  infile->check_sym(')');
  for(int i = 0; i < clause_vars.size(); i++) {
    if(clause_vars[i].type() == VAR_NOTBOOL) {
      negs.push_back(0);
      clause_vars[i].setType(VAR_BOOL);
    } else {
      negs.push_back(1);
    }
  }
  ConstraintBlob cb(ct, clause_vars);
  cb.negs = negs;
  return cb;
}

/// Reads an identifier which represents a single variable or constant.
template<typename FileReader>
Var MinionThreeInputReader<FileReader>::readIdentifier(FileReader* infile) {
  char idChar = infile->peek_char();

  if ((('0' <= idChar) && ('9' >= idChar)) || idChar == '-') {
    int i = infile->read_num();
    return Var(VAR_CONSTANT, i);
  }
  bool negVar = false;
  // Check to see if this is a negated Boolean
  if(infile->peek_char() == '!')
  {
    negVar = true;
    infile->get_char();
  }

  string name = infile->get_string();
  Var var = instance.vars.getSymbol(name);
  if(var.type() == VAR_MATRIX)
  {
    vector<int> params = readConstantVector(infile);
    vector<int> max_index = instance.vars.getMatrixSymbol(name);
    if(params.size() != max_index.size())
      throw parse_exception("Can't index a " + to_string(max_index.size()) + 
      "-d matrix with " + to_string(params.size()) +
      " indices.");
    for(int i = 0; i < params.size(); ++i)
    {
      if(params[i] < 0 || params[i] >= max_index[i])
        throw parse_exception(to_string(i) + string("th index is out of bounds,") + 
        to_string(params[i]) + " is not between 0 and " +
        to_string(max_index[i] - 1));
    }
    name += to_var_name(params);
    var = instance.vars.getSymbol(name);
  }

  if(negVar)
  {
    if(var.type() != VAR_BOOL)
      parser_info("Only Booleans can be negated!");
    else
      var.setType(VAR_NOTBOOL);
  }

  parser_info("Read variable '" + name + "', internally: " + to_string(var));
  return var;
}

// This function reads an identifier which might be a single variable,
// which includes a fully derefenced matrix, or might be a partially or
// not-at-all dereferenced matrix. It could also just be a number!
// The code shares a lot with readIdentifier, and at some point the two
// should probably merge
template<typename FileReader>
vector<Var> MinionThreeInputReader<FileReader>::readPossibleMatrixIdentifier(FileReader* infile, bool mustBeMatrix) {
  char idChar = infile->peek_char();

  vector<Var> returnVec;

  if ((('0' <= idChar) && ('9' >= idChar)) || idChar == '-') {
    if(mustBeMatrix)
      throw parse_exception("Must be matrix here, not constant");
    int i = infile->read_num();
    returnVec.push_back(Var(VAR_CONSTANT, i));
    return returnVec;
  }

  bool negVar = false;
  // Check to see if this is a negated Boolean
  if(infile->peek_char() == '!')
  {
    negVar = true;
  // Swallow the '!'
    infile->get_char();
  }

  // Get name of variable.
  string name = infile->get_string();

  Var var = instance.vars.getSymbol(name);

  if(var.type() == VAR_MATRIX)
  {
    if(negVar)
      throw parse_exception("Sorry, can't negate a matrix");
    vector<int> params;
    if(infile->peek_char() == '[')
      params = readConstantVector(infile,'[',']',true);
    else
      { // build a vector of all 'nulls'
      vector<int> maxterms = instance.vars.getMatrixSymbol(name);
    params = vector<int>(maxterms.size(), -999);
  }
  returnVec = instance.vars.buildVarList(name, params);
  parser_info("Got matrix:" + to_string(returnVec));
}
else
{ 
  if(mustBeMatrix)
    throw parse_exception("Must give matrix here, not single variable!");
  if(negVar)
  {
    if(var.type() != VAR_BOOL)
      parser_info("Only Booleans can be negated!");
    else
      var.setType(VAR_NOTBOOL);
  }
  returnVec.push_back(var);
}
parser_info("Read variable '" + name + "', internally: " + to_string(var));
return returnVec;  
}

template<typename FileReader>
vector<ConstraintBlob> MinionThreeInputReader<FileReader>::readConstraintList(FileReader* infile) {
  vector<ConstraintBlob> conlist;
  
  infile->check_sym('{');
  conlist.push_back(readConstraint(infile));
  
  char delim = infile->get_char();
  while(delim != '}')
  {
    if(delim != ',')
      throw parse_exception(string("Expected '}' or ',' , confused by '") + delim + string("'"));
    conlist.push_back(readConstraint(infile));
    delim = infile->get_char();
  }
  return conlist;
}

/// Reads a vector of variables (which can include constants).
/// Accepts:
/// M (for matrix identifer M)
/// [ M,B,.. ] (for matrix identifers M and variables B)
template<typename FileReader>
vector<Var> MinionThreeInputReader<FileReader>::readLiteralVector(FileReader* infile) {
  vector<Var> newVector;

  if(infile->peek_char() != '[')
  { // Must just be a matrix identifier
    return readPossibleMatrixIdentifier(infile, true);
  }

  infile->check_sym('[');

  // Delim here might end up being "x" or something similar. The reason
  // that we peek it is in case whis is an empty vector.

  char delim = infile->peek_char();

  if(delim == ']')
  {
  // Eat the ']'
    infile->get_char();
    parser_info("Read empty vector.");
  }
  else
  {
    while (delim != ']') {
      vector<Var> v = readPossibleMatrixIdentifier(infile);
      newVector.insert(newVector.end(), v.begin(), v.end());
  //newVector.push_back(readIdentifier(infile)) ;
      delim = infile->get_char();
      if(delim != ',' && delim != ']')
      {
  // replace X with the character we got.
        string s = "Expected ',' or ']'. Got 'X'.";
        s[s.size() - 3] = delim;
        throw parse_exception(s);
      }
    }
  }
  return newVector;
}


template<typename FileReader>
vector<vector<Var> > MinionThreeInputReader<FileReader>::read2DMatrix(FileReader* infile)
{
  vector<vector<Var> > return_vals; 

  if(infile->peek_char() != '[')
    return read2DMatrixVariable(infile);

  infile->check_sym('[');

  while(infile->peek_char() != ']')
  {
    parser_info("Continuing reading matrix, peeked at " + to_string(']'));
    // See if there is an array, or just a variable.
    if(infile->peek_char() == '[') 
      return_vals.push_back(readLiteralVector(infile));
    else
    {
      vector<vector<Var> > vars = read2DMatrixVariable(infile);
      for(int i = 0; i < vars.size(); ++i)
        return_vals.push_back(vars[i]);
    }
    // Eat a comma if there is one there.
    if(infile->peek_char() == ',')
      infile->check_sym(',');
  }

  infile->check_sym(']');
  return return_vals;
}

// This function reads the next identifier, which should be a 1D or 2D matrix,
// and returns it (if it was 1D, it returns it as a 1 row 2D matrix.
template<typename FileReader>
vector<vector<Var> > MinionThreeInputReader<FileReader>::read2DMatrixVariable(FileReader* infile) {
  string name = infile->get_string();
  Var var = instance.vars.getSymbol(name);
  // Check it is a matrix
  if(var.type() != VAR_MATRIX)
    throw parse_exception("Expected matrix");
  // Get dimension of matrix.
  vector<int> indices = instance.vars.getMatrixSymbol(name);
  // Make sure the matrix doesn't have an index after it. This is to produce better error messages.
  if(infile->peek_char() != ',' && infile->peek_char() != ']')
    throw parse_exception("Only accept raw matrix names here, expected ',' next.");

  if(indices.size() == 1)
  {
    vector<int> terms;
    terms.push_back(-999);
    // Use the existing code to flatten a matrix.
    // make_vec takes a T and turns it into a 1 element vector<T>.
    return make_vec(instance.vars.buildVarList(name, terms));
  }
  else
  {
    return instance.vars.flattenTo2DMatrix(name);
  }
}

// Note: allowNulls maps '_' to -999 (a horrible hack I know).
// That last parameter defaults to false.
// The start and end default to '[' and ']'
template<typename FileReader>
vector<int> MinionThreeInputReader<FileReader>::readConstantVector
  (FileReader* infile, char start, char end, bool allowNulls) 
{
  vector<int> newVector;
  infile->check_sym(start);

  // The reason we peek here is in case this is an empty vector
  char delim = infile->peek_char();

  if(delim == end)
  {
    // Eat the ']'
    infile->get_char();
    parser_info("Read empty vector.");
  }
  else
  {
    while (delim != end) 
    {
      if(allowNulls && infile->peek_char() == '_')
      {
        infile->get_char();
        newVector.push_back(-999);
      }
      else
        newVector.push_back(infile->read_num()) ;
      delim = infile->get_char();
      if(delim != ',' && delim != end)
        throw parse_exception(string("Expect ',' or ") + end + string("'. Got '") +
        delim + string("'"));
    }
  }
  return newVector;
}

/// Read an expression of the type ' {<num>..<num>} '
template<typename FileReader>
vector<int> MinionThreeInputReader<FileReader>::readRange(FileReader* infile) 
{
  vector<int> newVector;
  infile->check_sym('{');

  newVector.push_back(infile->read_num());
  infile->check_sym('.');
  infile->check_sym('.');

  newVector.push_back(infile->read_num());

  infile->check_sym('}');
  return newVector;
}


/// Read a list of tuples
template<typename FileReader>
void MinionThreeInputReader<FileReader>::readTuples(FileReader* infile)
{
  while(infile->peek_char() != '*')
  {
    string name = infile->get_string();
    int num_of_tuples = infile->read_num();
    int tuple_length = infile->read_num();
    parser_info("Reading tuplelist '" + name + "', length " + to_string(num_of_tuples) +
      ", arity " + to_string(tuple_length) );
    TupleList* tuplelist = instance.tupleListContainer->getNewTupleList(num_of_tuples, tuple_length);
    int* tuple_ptr = tuplelist->getPointer();
    for(int i = 0; i < num_of_tuples; ++i)
      for(int j = 0; j < tuple_length; ++j)
    {
      tuple_ptr[i * tuple_length + j] = infile->read_num();
    }
    tuplelist->finalise_tuples();
    instance.addTableSymbol(name, tuplelist);
  }

}

template<typename FileReader>
void MinionThreeInputReader<FileReader>::readSearch(FileReader* infile) {  
  while(infile->peek_char() != '*')
  {
    string var_type = infile->get_string();

    if(var_type == "VARORDER")
    {
      VarOrderEnum vo = ORDER_ORIGINAL;
      bool find_one_sol = false;
      
      if(infile->peek_char() == 'A')
      {
        string s = infile->get_string();
        if(s != "AUX")
          throw parse_exception("I do not understand " + s);
        find_one_sol = true;
      }
        
      if(infile->peek_char() != '[')
      {
        string s = infile->get_string();
#define Z(x) if(s == #x) { vo = ORDER_##x; goto found; }
Z(STATIC) Z(SDF) Z(SRF) Z(LDF) Z(ORIGINAL) Z(WDEG) Z(CONFLICT) Z(DOMOVERWDEG)
#undef Z        
throw parse_exception("Don't understand '" + s + "'");
found: ;
      }
      
      instance.search_order.push_back(SearchOrder(readLiteralVector(infile), vo, find_one_sol));
      parser_info("Read var order, length " +
        to_string(instance.search_order.back().var_order.size()));
    }
    else if(var_type == "PERMUTATION")
    {
      if(!instance.permutation.empty())
        throw parse_exception("Can't have two PERMUTATIONs!");
      instance.permutation = readLiteralVector(infile);
      parser_info("Read permutation, length " +
        to_string(instance.permutation.size()));      
    }
    else if(var_type == "SYMORDER")
    {
      if(!instance.sym_order.empty())
        throw parse_exception("Can't have two SYMORDERs!");
      instance.sym_order = readLiteralVector(infile);
      parser_info("Read Symmetry Ordering, length " +
        to_string(instance.permutation.size()));      
    }
    else if(var_type == "VALORDER")
    {
      if(instance.search_order.empty())
        throw parse_exception("Must declare VARORDER first");
      if(!instance.search_order.back().val_order.empty())
        throw parse_exception("Can't have two VALORDERs for a VARORDER");
      vector<char> valOrder ;

      infile->check_sym('[');

      char delim = infile->peek_char();

      while (delim != ']') {
        char valOrderIdentifier = infile->get_char();
        if(valOrderIdentifier != 'a' && valOrderIdentifier != 'd')
          throw parse_exception("Expected 'a' or 'd'");
        valOrder.push_back(valOrderIdentifier == 'a');
        delim = infile->get_char();                                 // , or ]
      }
      instance.search_order.back().val_order = valOrder;

      parser_info("Read val order, length " +
          to_string(instance.search_order.back().val_order.size()));
    }
    else if(var_type == "MAXIMISING" || var_type == "MAXIMIZING")
    {
      if(instance.is_optimisation_problem == true)
        throw parse_exception("Can only have one min / max per problem!");

      Var var = readIdentifier(infile);
      parser_info("Maximising " + to_string(var));
      instance.set_optimise(false, var);
    }
    else if(var_type == "MINIMISING" || var_type == "MINIMIZING")
    {
      if(instance.is_optimisation_problem == true)
        throw parse_exception("Can only have one min / max per problem!");

      Var var = readIdentifier(infile);
      parser_info("Minimising " + to_string(var));
      instance.set_optimise(true, var);
    }
    else if(var_type == "PRINT")
    {
      if(infile->peek_char() == 'A')
      {
        string in = infile->get_string();
        if(in != "ALL")
          throw parse_exception("Don't understand '"+in+"'. Do you mean 'ALL'?");
        print_all_vars = true;
      }
      else if(infile->peek_char() == 'N')
      {
        string in = infile->get_string();
        if(in != "NONE")
          throw parse_exception("Don't understand '"+in+"'. Do you mean 'NONE'?");
        print_all_vars = false;
      }
      else
      {
        print_all_vars = false;
        vector<vector<Var> > new_matrix = read2DMatrix(infile);
        for(int i = 0; i < new_matrix.size(); ++i)
          instance.print_matrix.push_back(new_matrix[i]);
      }
    }
    else if(var_type == "CONSTRUCTION")
    {
      if(!isGadgetReader())
        throw parse_exception("Only have construction sites on gadgets!");

      instance.constructionSite = readLiteralVector(infile);
      parser_info("Read construction site, size " + to_string(instance.constructionSite.size()));
    }
    else
      {  throw parse_exception("Don't understand '" + var_type + "' as a variable type."); }
  }
}


template<typename FileReader>
void MinionThreeInputReader<FileReader>::readAliasMatrix(FileReader* infile, const vector<int>& max_indices, vector<int> indices, string name)
{
  if(infile->peek_char() == '[')
  {
    infile->check_sym('[');
    // Have another level of reading to do..
    indices.push_back(0);
    readAliasMatrix(infile, max_indices, indices, name);
    infile->check_sym(']');
    while(infile->peek_char() == ',')
    {
      infile->check_sym(',');
      indices.back()++;
      infile->check_sym('[');
      readAliasMatrix(infile, max_indices, indices, name);
      infile->check_sym(']');
    }
    if(indices.back() + 1 != max_indices[indices.size() - 1])
      throw parse_exception("Incorrectly sized matrix!, expected index " +
      to_string(indices.size() - 1) + " to have " + to_string(max_indices[indices.size() - 1]) +
      " terms, got " + to_string(indices.back() + 1));
  }
  else
  {
    // Have reached the bottom level!
    indices.push_back(0);
    Var v = readIdentifier(infile);
    instance.vars.addSymbol(name + to_var_name(indices), v);
    while(infile->peek_char() == ',')
    {
      infile->check_sym(',');
      indices.back()++;
      Var v = readIdentifier(infile);
      instance.vars.addSymbol(name + to_var_name(indices), v);
    }
    if(indices.back() + 1 != max_indices[indices.size() - 1])
      throw parse_exception("Incorrectly sized matrix!, expected index " +
      to_string(indices.size() - 1) + " to have " + to_string(max_indices[indices.size() - 1]) +
      " terms, got " + to_string(indices.back() + 1));
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
template<typename FileReader>
void MinionThreeInputReader<FileReader>::readVars(FileReader* infile) {
  while(infile->peek_char() != '*')
  {
    parser_info("Begin reading variables");
    string var_type = infile->get_string();

    if(var_type != "BOOL" && var_type != "BOUND" && var_type != "SPARSEBOUND"
      && var_type != "DISCRETE" && var_type != "ALIAS")
      throw parse_exception(string("Unknown variable type: '") + var_type + "'");

    string varname = infile->get_string();
    parser_info("Name:" + varname);

    bool isArray = false;
    vector<int> indices;

    if(infile->peek_char() == '[')
    {
      parser_info("Is array!");
      isArray = true;
      indices = readConstantVector(infile);
      parser_info("Found " + to_string(indices.size()) + " indices");
    }

    VariableType variable_type = VAR_INVALID;
    vector<int> domain;

    if(var_type == "ALIAS")
    {
      if(isArray == false)
      {
        infile->check_sym('='); // XYZ
        Var v = readIdentifier(infile);
        instance.vars.addSymbol(varname, v);
      }
      else
      {
        instance.vars.addMatrixSymbol(varname, indices);
        infile->check_sym('=');
        infile->check_sym('[');
        readAliasMatrix(infile, indices, vector<int>(), varname);
        infile->check_sym(']');
      }
    }
    else if(var_type == "BOOL")
    {
      variable_type = VAR_BOOL;
    }
    else if(var_type == "BOUND")
    {
      variable_type = VAR_BOUND;
      domain = readRange(infile);
      if(domain.size() != 2)
        throw parse_exception("Ranges contain 2 numbers!");
    }
    else if(var_type == "DISCRETE")
    {
      variable_type = VAR_DISCRETE;
      domain = readRange(infile);
      if(domain.size() != 2)
        throw parse_exception("Ranges contain 2 numbers!");
    }
    else if(var_type == "SPARSEBOUND")
    {
      variable_type = VAR_SPARSEBOUND;
      domain = readConstantVector(infile, '{', '}');
      if(domain.size() < 1)
        throw parse_exception("Don't accept empty domains!");
    }
    else
      throw parse_exception("I don't know about var_type '" + var_type + "'");

    if(var_type != "ALIAS")
    {
      if(isArray)
      {
        // If any index is 0, don't add any variables.
        if(find(indices.begin(), indices.end(), 0) != indices.end())
        {
          instance.vars.addMatrixSymbol(varname, indices);
          vector<int> current_index(indices.size(), 0);
          parser_info("New Var: " + varname + to_var_name(current_index));
          instance.vars.addSymbol(varname + to_var_name(current_index),
                                  instance.vars.getNewVar(variable_type, domain));
          while(increment_vector(current_index, indices))
          {
            parser_info("New Var: " + varname + to_var_name(current_index));
            instance.vars.addSymbol(varname + to_var_name(current_index),
                                    instance.vars.getNewVar(variable_type, domain));
          }

          vector<vector<Var> > matrix_list = instance.vars.flattenTo2DMatrix(varname);
          for(int i = 0; i < matrix_list.size(); ++i)
            instance.all_vars_list.push_back(matrix_list[i]);
        }
      }
      else
      {
        Var v = instance.vars.getNewVar(variable_type, domain);
        instance.vars.addSymbol(varname, v);
        instance.all_vars_list.push_back(make_vec(v));
      }
    }
  }

}
