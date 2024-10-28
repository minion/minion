// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0













/** @help input;constraints Example
**CONSTRAINTS**
eq(bool,0)
alldiff(d)
*/





/** @help input;tuplelist Example
**TUPLELIST**
AtMostOne 4 3
0 0 0
0 0 1
0 1 0
1 0 0
*/





/** @help input;shorttuplelist Example
**SHORTTUPLELIST**
mycon 4
[(0,0),(3,0)]
[(1,0),(3,0)]
[(2,0),(3,0)]
[(0,1),(1,1),(2,1),(3,1)]

Represents the same constraint as:

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

Short tuples give us a way of shrinking this list. Short tuples consist
of pairs (x,y), where x is a varible position, and y is a value for that
variable. For example:

[(0,0),(3,0)]

Represents 'If the variable at index 0 is 0, and the variable at index
3 is 0, then the constraint is true'.

Some constraints (currently just shortctuplestr2) allow more than
one literal per variable for example:

[(0,0),(0,1),(3,0)]

Represents 'If the variable at index 0 is 0 or 1, and the variable at index
3 is 0, then the constraint is true'.

Note that some tuples are double-represented in the example 'mycon'.
The first 3 short tuples all allow the assignment '0 0 0 0'. This is fine.
The important thing for efficency is to try to give a small list of
short tuples.
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
*/

// Plan here is to generate an instance of a problem (or whatever you have)
// and return that.

#include <string>

#include "MinionInputReader.h"

#define MAYBE_PARSER_INFO(X)                                                                       \
  {                                                                                                \
    if(this->parserVerbose) {                                                                     \
      this->parser_info(X);                                                                        \
    }                                                                                              \
  }

template <typename FileReader>
void MinionThreeInputReader<FileReader>::parser_info(string s) {
  if(parserVerbose)
    cout << s << endl;
}

// This function is called to finalise reading an instance that may have
// consisted of two input files. If the first input file was a "MINION 1" then
// this function should be safe for it also.
template <typename FileReader>
void MinionThreeInputReader<FileReader>::finalise() {
  if(isGadgetReader() && instance->constructionSite.empty())
    throw parse_exception("Gadgets need a construction site!");

  // Fill in any missing defaults
  if(instance->searchOrder.empty()) {
    MAYBE_PARSER_INFO("No order generated, auto-generating complete order");
    instance->searchOrder.push_back(instance->vars.getAllVars());
  }

  vector<Var> allVars = instance->vars.getAllVars();
  set<Var> unusedVars(allVars.begin(), allVars.end());
  for(SysInt i = 0; i < (SysInt)instance->searchOrder.size(); ++i) {
    const vector<Var>& vars_ref = instance->searchOrder[i].varOrder;
    for(vector<Var>::const_iterator it = vars_ref.begin(); it != vars_ref.end(); ++it) {
      unusedVars.erase(*it);
    }
  }

  if(!unusedVars.empty() && ensureBranchOnAllVars) {
    vector<Var> unusedVec(unusedVars.begin(), unusedVars.end());
    if(instance->searchOrder.size() > 1 &&
       instance->searchOrder.back().findOneAssignment == true) {
      instance->searchOrder.back().varOrder.insert(instance->searchOrder.back().varOrder.end(),
                                                     unusedVec.begin(), unusedVec.end());
    } else {
      instance->searchOrder.push_back(unusedVec);
      instance->searchOrder.back().findOneAssignment = true;
    }
  }

  for(SysInt i = 0; i < (SysInt)instance->searchOrder.size(); ++i)
    instance->searchOrder[i].setupValueOrder();

  // This has to be delayed unless not all variables are defined where 'PRINT
  // ALL' occurs.
  if(print_allVars)
    instance->print_matrix = instance->allVars_list;

  if(instance->symOrder.empty())
    instance->symOrder = instance->vars.getAllVars();

  if(instance->symOrder.size() != instance->vars.getAllVars().size()) {
    MAYBE_PARSER_INFO("Extending symmetry order with auxillery variables");
    vector<Var> allVars = instance->vars.getAllVars();
    for(typename vector<Var>::iterator i = allVars.begin(); i != allVars.end(); ++i) {
      if(find(instance->symOrder.begin(), instance->symOrder.end(), *i) ==
         instance->symOrder.end())
        instance->symOrder.push_back(*i);
    }
  }

  if(instance->symOrder.size() !=
     set<Var>(instance->symOrder.begin(), instance->symOrder.end()).size())
    throw parse_exception("SYMORDER cannot contain any variable more than once");

  if(instance->symOrder.size() != instance->vars.getAllVars().size())
    throw parse_exception("SYMORDER must contain every variable");
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// read
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
template <typename FileReader>
void MinionThreeInputReader<FileReader>::read(FileReader* infile) {
  string s = infile->getAsciiString();
  MAYBE_PARSER_INFO("Read: '" + s + "'");

  string eof, wrong_eof;

  if(isGadgetReader()) {
    eof = "**GADGET_END**";
    wrong_eof = "**EOF**";
  } else {
    eof = "**EOF**";
    wrong_eof = "**GADGET_END**";
  }

  while(s != eof) {
    if(s == "**VARIABLES**")
      readVars(infile);
    else if(s == "**SEARCH**")
      readSearch(infile);
    else if(s == "**TUPLELIST**")
      readTuples(infile);
    else if(s == "**SHORTTUPLELIST**")
      readShortTuples(infile);
    else if(s == "**CONSTRAINTS**") {
      while(infile->peekChar() != '*')
        instance->constraints.push_back(readConstraint(infile, false));
    } else if(s == "**GADGET**") {
      readGadget(infile);
    } else if(s == "**MUTEXDETECT**") {
      readMutexDetect(infile);
    } else if(s == "**MUTEXDETECT2**") {
      readMutexDetect2(infile);
    } else if(s == wrong_eof) {
      throw parse_exception("Section terminated with " + wrong_eof + " instead of " + eof);
    } else
      throw parse_exception("Don't understand '" + s + "' as a section header");
    s = infile->getAsciiString();
    MAYBE_PARSER_INFO("Read: '" + s + "'");
  }

  MAYBE_PARSER_INFO("Reached end of CSP");
}


template <typename FileReader>
void MinionThreeInputReader<FileReader>::readGadget(FileReader* infile) {
  MAYBE_PARSER_INFO("Entering gadget parsing");
  if(isGadgetReader())
    throw parse_exception("Gadgets can't have gadgets!");

  infile->checkString("NAME");
  string name = infile->getString();
  MAYBE_PARSER_INFO("Gadget name:" + name);

  MinionThreeInputReader gadget(parserVerbose, map_long_short_mode, ensureBranchOnAllVars);
  CSPInstance* new_instance = new CSPInstance;
  gadget.instance = new_instance;
  gadget.setGadgetReader();
  gadget.read(infile);

  // Take the CSPInstance out of the Minion3InputReader, and make a copy of it.
  instance->addGadgetSymbol(name, shared_ptr<CSPInstance>(new_instance));
  MAYBE_PARSER_INFO("Exiting gadget parsing");
}

template <typename FileReader>
void MinionThreeInputReader<FileReader>::readMutexDetect(FileReader* infile) {
  MAYBE_PARSER_INFO("Entering mutex detect list parsing");
  //  Just read an ordinary vector
  instance->mutexDetectList = readLiteralVector(infile, true);
  MAYBE_PARSER_INFO("Exiting mutex detect list parsing");
}

template <typename FileReader>
void MinionThreeInputReader<FileReader>::readMutexDetect2(FileReader* infile) {
  MAYBE_PARSER_INFO("Entering mutex detect list parsing");
  //  Just read an ordinary  matrix with 0,0 as the separator between scopes. 
  instance->mutexDetectList2 = readLiteralVector(infile);
  MAYBE_PARSER_INFO("Exiting mutex detect list parsing");
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// readConstraint
// Recognise constraint by its name, read past name and leading '('
// Return false if eof or unknown ct. Else true.
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
template <typename FileReader>
ConstraintBlob MinionThreeInputReader<FileReader>::readConstraint(FileReader* infile,
                                                                  BOOL reified) {
  string id = infile->getline('(');

  SysInt constraintNum = -1;
  for(SysInt i = 0; i < numOfConstraints; ++i) {
    if(constraint_list[i].name == id) {
      constraintNum = i;
      break;
    }
  }

  if(constraintNum == -1) {
    if(infile->eof()) {
      throw parse_exception(string("Bad Constraint Name or reached end of file: '") + id + "'");
    } else {
      throw parse_exception(string("Unknown Constraint: '") + id + string("'"));
    }
  }
  ConstraintDef* constraint = constraint_list + constraintNum;

  switch(constraint->type) {
  default:
    if(constraint->numberOfParams == 2 &&
       (constraint->read_types[1] == read_tuples || constraint->read_types[1] == read_short_tuples))
      return readConstraintTable(infile, constraint);
    else
      return readGeneralConstraint(infile, constraint);
  }
  // g++ seems to think compilation can get here. I disagree, but putting a
  // catch doesn't hurt.
  throw parse_exception("Fatal error in parsing constraints");
}

template <typename FileReader>
ConstraintBlob MinionThreeInputReader<FileReader>::readConstraintTable(FileReader* infile,
                                                                       ConstraintDef* def) {
  ConstraintBlob con(def);

  con.vars.push_back(readLiteralVector(infile));
  infile->checkSym(',');

  if(def->read_types[1] == read_tuples)
    con.tuples = readConstraintTupleList(infile);
  else if(def->read_types[1] == read_short_tuples)
    con.shortTuples = readConstraintShortTupleList(infile);
  else
    assert(0);

  infile->checkSym(')');

  if(def->read_types[1] == read_tuples && con.tuples->size() == 0) {
    return ConstraintBlob(get_constraint(CT_FALSE));
  }

  if(def->read_types[1] == read_short_tuples && con.shortTuples->size() == 0) {
    return ConstraintBlob(get_constraint(CT_FALSE));
  }

  if(def->read_types[1] == read_tuples) {
    if((SysInt)con.vars[0].size() != con.tuples->tupleSize()) {
      throw parse_exception("Tuple constraint with " + tostring(con.vars[0].size()) +
                            " variables cannot have tuples of length " +
                            tostring(con.tuples->tupleSize()));
    }
  }

  // We already know there is at least one tuple
  if(con.vars[0].size() == 0) {
    if(def->read_types[1] == read_short_tuples && !(*con.shortTuples->tuplePtr())[0].empty())
      throw parse_exception("Not a valid list of short tuples for a "
                            "constraint with no variables!");
    return ConstraintBlob(get_constraint(CT_TRUE));
  }

  return con;
}

template <typename FileReader>
ConstraintBlob MinionThreeInputReader<FileReader>::readGeneralConstraint(FileReader* infile,
                                                                         ConstraintDef* def) {
  // This slightly strange code is to save copying the ConstraintBlob as much as
  // possible.
  ConstraintBlob con(def);
  vector<vector<Var>>& varsblob = con.vars;
  vector<vector<DomainInt>>& constblob = con.constants;

  for(SysInt i = 0; i < def->numberOfParams; ++i) {
    switch(def->read_types[i]) {
    case read_list: varsblob.push_back(readLiteralVector(infile)); break;
    case read_var: varsblob.push_back(makeVec(readIdentifier(infile))); break;
    case read_2_vars: {
      vector<Var> vars(2);
      vars[0] = readIdentifier(infile);
      infile->checkSym(',');
      vars[1] = readIdentifier(infile);
      varsblob.push_back(std::move(vars));
    } break;
    case read_constant: constblob.push_back(makeVec(infile->readNum())); break;
    case read_constant_list: {
      vector<Var> vectorOfConst = readLiteralVector(infile);
      vector<DomainInt> vals;
      for(UnsignedSysInt loop = 0; loop < vectorOfConst.size(); ++loop) {
        if(vectorOfConst[loop].type() != VAR_CONSTANT)
          throw parse_exception("Vector must only contain constants.");
        else
          vals.push_back(vectorOfConst[loop].pos());
      }
      constblob.push_back(std::move(vals));
    } break;
    case read_constraint: con.internal_constraints.push_back(readConstraint(infile, false)); break;
    case read_constraint_list: con.internal_constraints = readConstraintList(infile); break;
    case read_tuples:
      if(con.tuples == NULL)
        con.tuples = readConstraintTupleList(infile);
      else if(con.tuples2 == NULL)
        con.tuples2 = readConstraintTupleList(infile);
      else
        throw parse_exception("Too many tuplelists");
      break;
    default: D_FATAL_ERROR("Internal Error!");
    }
    if(i != def->numberOfParams - 1)
      infile->checkSym(',');
  }
  infile->checkSym(')');

  return con;
}

template <typename FileReader>
std::shared_ptr<ShortTupleList>
MinionThreeInputReader<FileReader>::readConstraintShortTupleList(FileReader* infile) {

  string name = infile->getString();
  return instance->getShortTableSymbol(name);
}

template <typename FileReader>
std::shared_ptr<TupleList> MinionThreeInputReader<FileReader>::readConstraintTupleList(FileReader* infile) {
  std::shared_ptr<TupleList> tuplelist;

  if(infile->peekChar() != '{') {
    string name = infile->getString();
    tuplelist = instance->getTableSymbol(name);
  } else {
    vector<vector<DomainInt>> tuples;
    infile->checkSym('{');
    char delim = infile->peekChar();

    SysInt tupleSize = 0;

    while(delim != '}') {
      infile->checkSym('<');
      vector<DomainInt> tuple;
      // Optimisation
      tuple.reserve(tupleSize);

      char nextChar = ',';
      while(nextChar == ',') {
        tuple.push_back(infile->readNum());
        nextChar = infile->getChar();
      }
      if(nextChar != '>')
        throw parse_exception("Expected ',' or '>'");

      if(tupleSize == 0)
        tupleSize = tuple.size();
      if(tupleSize != (SysInt)tuple.size())
        throw parse_exception("All tuples in each constraint must be the same size!");
      tuples.push_back(std::move(tuple));

      delim = infile->getChar(); // ',' or '}'
      if(delim != ',' && delim != '}')
        throw parse_exception("Expected ',' or '}'");
    }
    tuplelist = instance->tupleListContainer->getNewTupleList(tuples);
    instance->addUnnamedTableSymbol(tuplelist);
  }

  return tuplelist;
}

/// Reads an identifier which represents a single variable or constant.
template <typename FileReader>
Var MinionThreeInputReader<FileReader>::readIdentifier(FileReader* infile) {
  char idChar = infile->peekChar();

  if((('0' <= idChar) && ('9' >= idChar)) || idChar == '-') {
    DomainInt i = infile->readNum();
    return Var(VAR_CONSTANT, i);
  }
  bool negVar = false;
  // Check to see if this is a negated Boolean
  if(infile->peekChar() == '!') {
    negVar = true;
    infile->getChar();
  }

  string name = infile->getString();
  Var var = instance->vars.getSymbol(name);
  if(var.type() == VAR_MATRIX) {
    vector<DomainInt> params = readConstantVector(infile);
    vector<DomainInt> maxIndex = instance->vars.getMatrixSymbol(name);
    if(params.size() != maxIndex.size())
      throw parse_exception("Can't index a " + tostring(maxIndex.size()) + "-d matrix with " +
                            tostring(params.size()) + " indices.");
    for(SysInt i = 0; i < (SysInt)params.size(); ++i) {
      if(params[i] < 0 || params[i] >= maxIndex[i])
        throw parse_exception(tostring(i) + string("th index is out of bounds,") +
                              tostring(params[i]) + " is not between 0 and " +
                              tostring(maxIndex[i] - 1));
    }
    name += to_var_name(params);
    var = instance->vars.getSymbol(name);
  }

  if(negVar) {
    if(var.type() != VAR_BOOL) {
      throw parse_exception("Only booleans can be negated!");
    } else
      var.setType(VAR_NOTBOOL);
  }

  MAYBE_PARSER_INFO("Read variable '" + name + "', internally: " + tostring(var));
  return var;
}

// This function reads an identifier which might be a single variable,
// which includes a fully derefenced matrix, or might be a partially or
// not-at-all dereferenced matrix. It could also just be a number!
// The code shares a lot with readIdentifier, and at some point the two
// should probably merge
template <typename FileReader>
vector<Var> MinionThreeInputReader<FileReader>::readPossibleMatrixIdentifier(FileReader* infile,
                                                                             bool mustBeMatrix) {
  char idChar = infile->peekChar();

  vector<Var> returnVec;

  if((('0' <= idChar) && ('9' >= idChar)) || idChar == '-') {
    if(mustBeMatrix)
      throw parse_exception("Must be matrix here, not constant");
    DomainInt i = infile->readNum();
    returnVec.push_back(Var(VAR_CONSTANT, i));
    return returnVec;
  }

  bool negVar = false;
  // Check to see if this is a negated Boolean
  if(infile->peekChar() == '!') {
    negVar = true;
    // Swallow the '!'
    infile->getChar();
  }

  // Get name of variable.
  string name = infile->getString();

  Var var = instance->vars.getSymbol(name);

  if(var.type() == VAR_MATRIX) {
    vector<DomainInt> params;
    if(infile->peekChar() == '[') {
      params = readConstantVector(infile, '[', ']', true);
      returnVec = instance->vars.buildVarList(name, params);
      if(negVar) {
        if(returnVec.size() != 1) {
          throw parse_exception("Sorry, can't negate a matrix");
        }
        if(returnVec[0].type() != VAR_BOOL) {
          throw parse_exception("Sorry, can't negate a non-Boolean variable");
        }
        returnVec[0].setType(VAR_NOTBOOL);
        // The clever thing to do would be to change type of all of returnVec at
        // the end of the function, if necessary.
      }
    } else { // build a vector of all 'nulls'
      if(negVar) {
        throw parse_exception("Sorry, can't negate a matrix");
      }

      vector<DomainInt> maxterms = instance->vars.getMatrixSymbol(name);
      params = vector<DomainInt>(maxterms.size(), -999);
      returnVec = instance->vars.buildVarList(name, params);
    }
    MAYBE_PARSER_INFO("Got matrix:" + tostring(returnVec));
  } else {
    if(mustBeMatrix)
      throw parse_exception("Must give matrix here, not single variable!");
    if(negVar) {
      if(var.type() != VAR_BOOL) {
        throw parse_exception("Sorry, can't negate a non-Boolean variable");
      } else
        var.setType(VAR_NOTBOOL);
    }
    returnVec.push_back(var);
  }
  MAYBE_PARSER_INFO("Read variable '" + name + "', internally: " + tostring(var));
  return returnVec;
}

template <typename FileReader>
vector<ConstraintBlob> MinionThreeInputReader<FileReader>::readConstraintList(FileReader* infile) {
  vector<ConstraintBlob> conlist;

  infile->checkSym('{');
  conlist.push_back(readConstraint(infile));

  char delim = infile->getChar();
  while(delim != '}') {
    if(delim != ',')
      throw parse_exception(string("Expected '}' or ',' , confused by '") + delim + string("'"));
    conlist.push_back(readConstraint(infile));
    delim = infile->getChar();
  }
  return conlist;
}

/// Reads a vector of variables (which can include constants).
/// Accepts:
/// M (for matrix identifer M)
/// [ M,B,.. ] (for matrix identifers M and variables B)
template <typename FileReader>
vector<Var> MinionThreeInputReader<FileReader>::readLiteralVector(FileReader* infile, bool mustBeVector) {
  vector<Var> newVector;

  if(infile->peekChar() != '[') { // Must just be a matrix identifier
    return readPossibleMatrixIdentifier(infile, mustBeVector);
  }

  infile->checkSym('[');

  // Delim here might end up being "x" or something similar. The reason
  // that we peek it is in case whis is an empty vector.

  char delim = infile->peekChar();

  if(delim == ']') {
    // Eat the ']'
    infile->getChar();
    MAYBE_PARSER_INFO("Read empty vector.");
  } else {
    while(delim != ']') {
      vector<Var> v = readPossibleMatrixIdentifier(infile);
      newVector.insert(newVector.end(), v.begin(), v.end());
      delim = infile->getChar();
      if(delim != ',' && delim != ']') {
        // replace X with the character we got.
        string s = "Expected ',' or ']'. Got 'X'.";
        s[s.size() - 3] = delim;
        throw parse_exception(s);
      }

      if(delim == ',' && infile->peekChar() == ']')
        delim = infile->getChar();
    }
  }
  return newVector;
}

template <typename FileReader>
vector<vector<Var>> MinionThreeInputReader<FileReader>::read2DMatrix(FileReader* infile) {
  vector<vector<Var>> returnVals;

  if(infile->peekChar() != '[')
    return read2DMatrixVariable(infile);

  infile->checkSym('[');

  while(infile->peekChar() != ']') {
    MAYBE_PARSER_INFO("Continuing reading matrix, peeked at " + tostring(']'));
    // See if there is an array, or just a variable.
    if(infile->peekChar() == '[')
      returnVals.push_back(readLiteralVector(infile));
    else {
      vector<vector<Var>> vars = read2DMatrixVariable(infile);
      for(SysInt i = 0; i < (SysInt)vars.size(); ++i)
        returnVals.push_back(vars[i]);
    }
    // Eat a comma if there is one there.
    if(infile->peekChar() == ',')
      infile->checkSym(',');
  }

  infile->checkSym(']');
  return returnVals;
}

// This function reads the next identifier, which should be a 1D or 2D matrix,
// and returns it (if it was 1D, it returns it as a 1 row 2D matrix.
template <typename FileReader>
vector<vector<Var>> MinionThreeInputReader<FileReader>::read2DMatrixVariable(FileReader* infile) {
  string name = infile->getString();
  Var var = instance->vars.getSymbol(name);
  // Check it is a matrix
  if(var.type() != VAR_MATRIX)
    throw parse_exception("Expected matrix");
  // Get dimension of matrix.
  vector<DomainInt> indices = instance->vars.getMatrixSymbol(name);
  // Make sure the matrix doesn't have an index after it. This is to produce
  // better error messages.
  if(infile->peekChar() != ',' && infile->peekChar() != ']')
    throw parse_exception("Only accept raw matrix names here, expected ',' next.");

  if(indices.size() == 1) {
    vector<DomainInt> terms;
    terms.push_back(-999);
    // Use the existing code to flatten a matrix.
    // makeVec takes a T and turns it into a 1 element vector<T>.
    return makeVec(instance->vars.buildVarList(name, terms));
  } else {
    return instance->vars.flattenTo2DMatrix(name);
  }
}

// Note: allowNulls maps '_' to -999 (a horrible hack I know).
// That last parameter defaults to false.
// The start and end default to '[' and ']'
template <typename FileReader>
vector<DomainInt> MinionThreeInputReader<FileReader>::readConstantVector(FileReader* infile,
                                                                         char start, char end,
                                                                         bool allowNulls) {
  vector<DomainInt> newVector;
  infile->checkSym(start);

  // The reason we peek here is in case this is an empty vector
  char delim = infile->peekChar();

  if(delim == end) {
    // Eat the ']'
    infile->getChar();
    MAYBE_PARSER_INFO("Read empty vector.");
  } else {
    while(delim != end) {
      if(allowNulls && infile->peekChar() == '_') {
        infile->getChar();
        newVector.push_back(-999);
      } else
        newVector.push_back(infile->readNum());
      delim = infile->getChar();
      if(delim != ',' && delim != end)
        throw parse_exception(string("Expect ',' or ") + end + string("'. Got '") + delim +
                              string("'"));
    }
  }
  return newVector;
}

// Note: allowNulls maps '_' to -999 (a horrible hack I know).
// That last parameter defaults to false.
// The start and end default to '[' and ']'
template <typename FileReader>
vector<pair<SysInt, DomainInt>>
MinionThreeInputReader<FileReader>::readShortTuple(FileReader* infile) {
  vector<pair<SysInt, DomainInt>> newVector;
  infile->checkSym('[');

  while(infile->peekChar() == '(') {
    infile->checkSym('(');
    SysInt var = checked_cast<SysInt>(infile->readNum());
    infile->checkSym(',');
    DomainInt val = infile->readNum();
    infile->checkSym(')');
    newVector.push_back(make_pair(var, val));
    if(infile->peekChar() == ',')
      infile->checkSym(',');
  }
  infile->checkSym(']');

  return newVector;
}

/// Read an expression of the type ' {<num>..<num>} '
template <typename FileReader>
vector<DomainInt> MinionThreeInputReader<FileReader>::readRange(FileReader* infile) {
  vector<DomainInt> newVector;
  infile->checkSym('{');

  newVector.push_back(infile->readNum());
  infile->checkSym('.');
  infile->checkSym('.');

  newVector.push_back(infile->readNum());

  infile->checkSym('}');
  return newVector;
}

/// Read a list of tuples
template <typename FileReader>
void MinionThreeInputReader<FileReader>::readShortTuples(FileReader* infile) {
  while(infile->peekChar() != '*') {
    string name = infile->getString();
    DomainInt numOf_shortTuples = infile->readNum();
    vector<vector<pair<SysInt, DomainInt>>> tups;

    for(DomainInt i = 0; i < numOf_shortTuples; ++i)
      tups.push_back(readShortTuple(infile));

    std::shared_ptr<ShortTupleList> stl = instance->shortTupleListContainer->getNewShortTupleList(tups);
    instance->addShortTableSymbol(name, stl);
  }
}

template <typename FileReader>
void MinionThreeInputReader<FileReader>::readTuples(FileReader* infile) {
  while(infile->peekChar() != '*') {
    string name = infile->getString();
    DomainInt numOf_tuples = infile->readNum();
    DomainInt tupleLength = infile->readNum();
    MAYBE_PARSER_INFO("Reading tuplelist '" + name + "', length " + tostring(numOf_tuples) +
                      ", arity " + tostring(tupleLength));
    std::shared_ptr<TupleList> tuplelist =
        instance->tupleListContainer->getNewTupleList(numOf_tuples, tupleLength);
    DomainInt* tuplePtr = tuplelist->getPointer();
    for(DomainInt i = 0; i < numOf_tuples; ++i)
      for(DomainInt j = 0; j < tupleLength; ++j) {
        tuplePtr[checked_cast<SysInt>(i * tupleLength + j)] = infile->readNum();
      }
    tuplelist->finalise_tuples();
    instance->addTableSymbol(name, tuplelist);

    if(map_long_short_mode != MLTTS_NoMap) {
      std::shared_ptr<ShortTupleList> stl =
          instance->shortTupleListContainer->getNewShortTupleList(tuplelist, map_long_short_mode);
      instance->addShortTableSymbol(name, stl);
    }
  }
}

template <typename FileReader>
void MinionThreeInputReader<FileReader>::readSearch(FileReader* infile) {
  while(infile->peekChar() != '*') {
    string varType = infile->getString();

    if(varType == "VARORDER") {
      VarOrderEnum vo = ORDER_ORIGINAL;
      bool find_one_sol = false;

      if(infile->peekChar() == 'A') {
        string s = infile->getString();
        if(s != "AUX")
          throw parse_exception("I do not understand " + s);
        find_one_sol = true;
      }

      if(infile->peekChar() != '[') {
        string s = infile->getString();
#define Z(x)                                                                                       \
  if(s == #x) {                                                                                    \
    vo = ORDER_##x;                                                                                \
    goto found;                                                                                    \
  }
        Z(STATIC)
        Z(SDF)
        Z(SRF)
        Z(LDF)
        Z(ORIGINAL)
        Z(WDEG)
        Z(CONFLICT)
        Z(DOMOVERWDEG)
#undef Z
        if(vo == ORDER_WDEG || vo == ORDER_DOMOVERWDEG) {
#ifndef WDEG
          USER_ERROR("This minion was not compiled with support for WDEG or "
                     "DOMOVERWDEG orderings (add -WDEG to build options)");
#endif
        }
        throw parse_exception("Don't understand '" + s + "'");
      found:;
      }

      instance->searchOrder.push_back(SearchOrder(readLiteralVector(infile), vo, find_one_sol));
      MAYBE_PARSER_INFO("Read var order, length " +
                        tostring(instance->searchOrder.back().varOrder.size()));
    } else if(varType == "PERMUTATION") {
      if(!instance->permutation.empty())
        throw parse_exception("Can't have two PERMUTATIONs!");
      instance->permutation = readLiteralVector(infile);
      MAYBE_PARSER_INFO("Read permutation, length " + tostring(instance->permutation.size()));
    } else if(varType == "SYMORDER") {
      if(!instance->symOrder.empty())
        throw parse_exception("Can't have two SYMORDERs!");
      instance->symOrder = readLiteralVector(infile);
      MAYBE_PARSER_INFO("Read Symmetry Ordering, length " + tostring(instance->permutation.size()));
    } else if(varType == "VALORDER") {
      if(instance->searchOrder.empty())
        throw parse_exception("Must declare VARORDER first");
      if(!instance->searchOrder.back().valOrder.empty())
        throw parse_exception("Can't have two VALORDERs for a VARORDER");
      vector<ValOrder> valOrder;

      infile->checkSym('[');

      char delim = infile->peekChar();

      while(delim != ']') {
        char valOrderIdentifier = infile->getChar();
        switch(valOrderIdentifier) {
        case 'a': valOrder.push_back(VALORDER_ASCEND); break;
        case 'd': valOrder.push_back(VALORDER_DESCEND); break;
        case 'r': valOrder.push_back(VALORDER_RANDOM); break;

        default: throw parse_exception("Expected 'a' or 'd' or 'r'");
        }
        delim = infile->getChar(); // , or ]
      }
      instance->searchOrder.back().valOrder = valOrder;

      MAYBE_PARSER_INFO("Read val order, length " +
                        tostring(instance->searchOrder.back().valOrder.size()));
    } else if(varType == "MAXIMISING" || varType == "MAXIMIZING") {
      if(instance->is_optimisation_problem == true)
        throw parse_exception("Can only have one min / max per problem!");

      vector<Var> vars = readLiteralVector(infile, false);
      MAYBE_PARSER_INFO("Maximising " + tostring(vars));
      instance->set_optimise(false, vars);
    } else if(varType == "MINIMISING" || varType == "MINIMIZING") {
      if(instance->is_optimisation_problem == true)
        throw parse_exception("Can only have one min / max per problem!");

      vector<Var> vars = readLiteralVector(infile, false);
      MAYBE_PARSER_INFO("Minimising " + tostring(vars));
      instance->set_optimise(true, vars);
    } else if(varType == "PRINT") {
      if(infile->peekChar() == 'A') {
        string in = infile->getString();
        if(in != "ALL")
          throw parse_exception("Don't understand '" + in + "'. Do you mean 'ALL'?");
        print_allVars = true;
      } else if(infile->peekChar() == 'N') {
        string in = infile->getString();
        if(in != "NONE")
          throw parse_exception("Don't understand '" + in + "'. Do you mean 'NONE'?");
        print_allVars = false;
      } else {
        print_allVars = false;
        vector<vector<Var>> new_matrix = read2DMatrix(infile);
        for(SysInt i = 0; i < (SysInt)new_matrix.size(); ++i)
          instance->print_matrix.push_back(new_matrix[i]);
      }
    } else if(varType == "CONSTRUCTION") {
      if(!isGadgetReader())
        throw parse_exception("Only have construction sites on gadgets!");

      instance->constructionSite = readLiteralVector(infile);
      MAYBE_PARSER_INFO("Read construction site, size " +
                        tostring(instance->constructionSite.size()));
    } else {
      throw parse_exception("Don't understand '" + varType + "' as a variable type.");
    }
  }
}

template <typename FileReader>
void MinionThreeInputReader<FileReader>::readAliasMatrix(FileReader* infile,
                                                         const vector<DomainInt>& max_indices,
                                                         vector<DomainInt> indices, string name) {
  if(infile->peekChar() == '[') {
    infile->checkSym('[');
    // Have another level of reading to do..
    indices.push_back(0);
    readAliasMatrix(infile, max_indices, indices, name);
    infile->checkSym(']');
    while(infile->peekChar() == ',') {
      infile->checkSym(',');
      ++indices.back();
      infile->checkSym('[');
      readAliasMatrix(infile, max_indices, indices, name);
      infile->checkSym(']');
    }
    if(indices.back() + 1 != max_indices[indices.size() - 1])
      throw parse_exception("Incorrectly sized matrix!, expected index " +
                            tostring(indices.size() - 1) + " to have " +
                            tostring(max_indices[indices.size() - 1]) + " terms, got " +
                            tostring(indices.back() + 1));
  } else {
    // Have reached the bottom level!
    indices.push_back(0);
    Var v = readIdentifier(infile);
    instance->vars.addSymbol(name + to_var_name(indices), v);
    while(infile->peekChar() == ',') {
      infile->checkSym(',');
      ++indices.back();
      Var v = readIdentifier(infile);
      instance->vars.addSymbol(name + to_var_name(indices), v);
    }
    if(indices.back() + 1 != max_indices[indices.size() - 1])
      throw parse_exception("Incorrectly sized matrix!, expected index " +
                            tostring(indices.size() - 1) + " to have " +
                            tostring(max_indices[indices.size() - 1]) + " terms, got " +
                            tostring(indices.back() + 1));
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
template <typename FileReader>
void MinionThreeInputReader<FileReader>::readVars(FileReader* infile) {
  while(infile->peekChar() != '*') {
    MAYBE_PARSER_INFO("Begin reading variables");
    string varType = infile->getString();

    if(varType != "BOOL" && varType != "BOUND" && varType != "SPARSEBOUND" &&
       varType != "DISCRETE" && varType != "ALIAS")
      throw parse_exception(string("Unknown variable type: '") + varType + "'");

    string varname = infile->getString();
    MAYBE_PARSER_INFO("Name:" + varname);

    bool isArray = false;
    vector<DomainInt> indices;

    if(infile->peekChar() == '[') {
      MAYBE_PARSER_INFO("Is array!");
      isArray = true;
      indices = readConstantVector(infile);
      if (indices.size() < 1) 
        throw parse_exception("Matrix " + varname + " has no indicies");

      for(UnsignedSysInt i = 0; i < indices.size(); ++i)
        if(indices[i] < 0)
          throw parse_exception("Matrix " + varname + " has a negative size for index " +
                                tostring(i));
      MAYBE_PARSER_INFO("Found " + tostring(indices.size()) + " indices");
    }

    VariableType variable_type = VAR_INVALID;
    vector<DomainInt> domain;

    if(varType == "ALIAS") {
      if(isArray == false) {
        infile->checkSym('='); // XYZ
        Var v = readIdentifier(infile);
        instance->vars.addSymbol(varname, v);
      } else {
        instance->vars.addMatrixSymbol(varname, indices);
        infile->checkSym('=');
        infile->checkSym('[');
        readAliasMatrix(infile, indices, vector<DomainInt>(), varname);
        infile->checkSym(']');
      }
    } else if(varType == "BOOL") {
      variable_type = VAR_BOOL;
    } else if(varType == "BOUND") {
      variable_type = VAR_BOUND;
      domain = readRange(infile);
      if(domain[0] > domain[1])
        throw parse_exception("Range in decreasing order e.g. 1..0  in "
                              "declaration of BOUND variable.");
      if(domain.size() != 2)
        throw parse_exception("Ranges contain 2 numbers!");
    } else if(varType == "DISCRETE") {
      variable_type = VAR_DISCRETE;
      domain = readRange(infile);
      if(domain[0] > domain[1])
        throw parse_exception("Range in decreasing order e.g. 1..0  in "
                              "declaration of BOUND variable.");
      if(domain.size() != 2)
        throw parse_exception("Ranges contain 2 numbers!");
    } else if(varType == "SPARSEBOUND") {
      variable_type = VAR_SPARSEBOUND;
      domain = readConstantVector(infile, '{', '}');

      for(unsigned int i = 0; i < (SysInt)domain.size() - 1; i++) {
        if(domain[i] > domain[i + 1]) {
          throw parse_exception("Values out of order in SPARSEBOUND domain.");
        }
        if(domain[i] == domain[i + 1]) {
          throw parse_exception("Repeated values in SPARSEBOUND domain.");
        }
      }
      if(domain.size() < 1)
        throw parse_exception("Don't accept empty domains!");
    } else
      throw parse_exception("I don't know about varType '" + varType + "'");

    if(varType != "ALIAS") {
      if(isArray) {
        instance->vars.addMatrixSymbol(varname, indices);
        // If any index is 0, don't add any variables.
        if(find(indices.begin(), indices.end(), 0) == indices.end()) {
          vector<DomainInt> currentIndex(indices.size(), 0);
          MAYBE_PARSER_INFO("New Var: " + varname + to_var_name(currentIndex));
          instance->vars.addSymbol(varname + to_var_name(currentIndex),
                                   instance->vars.getNewVar(variable_type, domain));
          while(incrementVector(currentIndex, indices)) {
            MAYBE_PARSER_INFO("New Var: " + varname + to_var_name(currentIndex));
            instance->vars.addSymbol(varname + to_var_name(currentIndex),
                                     instance->vars.getNewVar(variable_type, domain));
          }

          vector<vector<Var>> matrix_list = instance->vars.flattenTo2DMatrix(varname);
          for(SysInt i = 0; i < (SysInt)matrix_list.size(); ++i)
            instance->allVars_list.push_back(matrix_list[i]);
        }
      } else {
        Var v = instance->vars.getNewVar(variable_type, domain);
        instance->vars.addSymbol(varname, v);
        instance->allVars_list.push_back(makeVec(v));
      }
    }
  }
}
