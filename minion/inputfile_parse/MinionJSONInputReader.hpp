/*
* Minion http://minion.sourceforge.net
* Copyright (C) 2006-09
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/


#include <string>

#include "MinionInputReader.h"
#include "../minlib/gason/gason.h"

#define MAYBE_PARSER_INFO(X) { if(this->parser_verbose) { this->parser_info(X); } }


void MinionJSONInputReader::parser_info(string s)
{
  if(parser_verbose)
    cout << s << endl;
}

//This function is called to finalise reading an instance that may have
//consisted of two input files. If the first input file was a "MINION 1" then
//this function should be safe for it also.

void MinionJSONInputReader::finalise() {
  // Fill in any missing defaults
  if(instance->search_order.empty())
  {
    MAYBE_PARSER_INFO("No order generated, auto-generating complete order");
    instance->search_order.push_back(instance->vars.get_all_vars());
  }

  vector<Var> all_vars = instance->vars.get_all_vars();
  set<Var> unused_vars(all_vars.begin(), all_vars.end());
  for(SysInt i = 0; i < instance->search_order.size(); ++i)
  {
    const vector<Var>& vars_ref = instance->search_order[i].var_order;
    for(vector<Var>::const_iterator it = vars_ref.begin(); it != vars_ref.end(); ++it)
    {
      unused_vars.erase(*it);
    }
  }

  if(!unused_vars.empty())
  {
    vector<Var> unused_vec(unused_vars.begin(), unused_vars.end());
    if(instance->search_order.size() > 1 && instance->search_order.back().find_one_assignment == true)
    {
      instance->search_order.back().var_order.insert(
        instance->search_order.back().var_order.end(), unused_vec.begin(), unused_vec.end());
    }
    else
    {
      instance->search_order.push_back(unused_vec);
      instance->search_order.back().find_one_assignment=true;
    }
  }

  for(SysInt i = 0; i < instance->search_order.size(); ++i)
    instance->search_order[i].setupValueOrder();

  // This has to be delayed unless not all variables are defined where 'PRINT ALL' occurs.
  if(print_all_vars)
    instance->print_matrix = instance->all_vars_list;

  if(instance->sym_order.empty())
    instance->sym_order = instance->vars.get_all_vars();

  if(instance->sym_order.size() != instance->vars.get_all_vars().size())
  {
    MAYBE_PARSER_INFO("Extending symmetry order with auxillery variables");
    vector<Var> all_vars = instance->vars.get_all_vars();
    for(typename vector<Var>::iterator i = all_vars.begin(); i != all_vars.end(); ++i)
    {
      if(find(instance->sym_order.begin(), instance->sym_order.end(), *i) == instance->sym_order.end() )
        instance->sym_order.push_back(*i);
    }
  }

  if(instance->sym_order.size() != set<Var>(instance->sym_order.begin(), instance->sym_order.end()).size())
     throw parse_exception("SYMORDER cannot contain any variable more than once");

   if(instance->sym_order.size() != instance->vars.get_all_vars().size())
     throw parse_exception("SYMORDER must contain every variable");
}

void MinionJSONInputReader::check_tag(JsonValue json, JsonTag tag, std::string place)
{
    if(json.getTag() != tag)
    {
        std::string error_str = "Expected a " + std::string(jsonTagToString(tag)) +
        " but found a " + jsonTagToString(json.getTag()) + " when " + place;
        throw parse_exception(error_str);
    }
}

void MinionJSONInputReader::check_keys(JsonValue json,
                                       const std::set<std::string>& keys)
{
    for(auto k : json)
    {
        if(keys.count(k->key) == 0)
        {
            throw parse_exception("Do not understand '" +std::string(k->key) + "'");
        }
    }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// read
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void MinionJSONInputReader::read(JsonValue infile) {

  check_tag(infile, JSON_TAG_OBJECT, "parsing top level");
  for(auto i : infile)
  {
      if(i->key == std::string("variables"))
          readVars(i->value);
      else if(i->key == std::string("search"))
          readSearch(i->value);
      else if(i->key == std::string("tuplelist"))
          readTuples(i->value);
      else if(i->key == std::string("shorttuplelist"))
          readShortTuples(i->value);
      else if(i->key == std::string("constraints"))
      {
          check_tag(i->value, JSON_TAG_ARRAY, "parsing constraints");
          for(auto c : i->value)
          {
              instance->constraints.push_back(readConstraint(c->value, false));
          }
      }
      else
        throw parse_exception("Don't understand '" + std::string(i->key) + "' as a section header");
      }
  MAYBE_PARSER_INFO("Reached end of CSP");
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// readConstraint
// Recognise constraint by its name, read past name and leading '('
// Return false if eof or unknown ct. Else true.
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

ConstraintBlob MinionJSONInputReader::readConstraint(JsonValue infile, BOOL reified) {
  check_tag(infile, JSON_TAG_OBJECT, "reading a constraint");
  check_keys(infile, std::set<std::string>{"name","args"});
  string id;

  for(auto i : infile)
  {
    if(i->key == std::string("name"))
    {
      check_tag(i->value, JSON_TAG_STRING, "reading a constraint name");
      id = i->value.toString();
    }
  }

  if(id.empty())
  {
    throw parse_exception("Constraint has no name");
  }

  SysInt constraint_num = -1;
  for(SysInt i = 0; i < num_of_constraints; ++i)
  {
    if(constraint_list[i].name == id)
    {
      constraint_num = i;
      break;
    }
  }

  if(constraint_num == -1)
  { throw parse_exception(string("Unknown Constraint: '") + id + string("'")); }
  ConstraintDef* constraint = constraint_list + constraint_num;

  JsonValue res;

  for(auto i : infile)
  {
    if(i->key == std::string("args"))
      res = i->value;
  }

  if(res.getTag() == JSON_TAG_NULL)
  { throw parse_exception("Constraint has no args"); }


  switch(constraint->type)
  {
#ifdef CT_WATCHED_OR_ABC
    case CT_WATCHED_OR:
    return readConstraintOr(res, get_constraint(CT_WATCHED_OR));
    break;
#endif

    default:
    if(constraint->number_of_params == 2 &&
         (constraint->read_types[1] == read_tuples || constraint->read_types[1] == read_short_tuples) )
      return readConstraintTable(res, constraint);
    else
      return readGeneralConstraint(res, constraint);
  }
  // g++ seems to think compilation can get here. I disagree, but putting a catch doesn't hurt.
  throw parse_exception("Fatal error in parsing constraints");
}


ConstraintBlob MinionJSONInputReader::readConstraintTable(JsonValue infile, ConstraintDef* def)
{
  assert(0);
  ConstraintBlob con(def);
/*
  con.vars.push_back(readLiteralVector(infile));
  infile->check_sym(',');

  if(def->read_types[1] == read_tuples)
    con.tuples = readConstraintTupleList(infile);
  else if(def->read_types[1] == read_short_tuples)
    con.short_tuples = readConstraintShortTupleList(infile);
  else
    assert(0);

  infile->check_sym(')');

  if(def->read_types[1] == read_tuples)
  {
    if(con.vars[0].size() != con.tuples->tuple_size())
    {
      throw parse_exception("Tuple constraint with " + tostring(con.vars[0].size()) +
                            " variables cannot have tuples of length " + tostring(con.tuples->tuple_size()));
    }
  }


  if(con.vars[0].size() == 0)
  {
    if(def->read_types[1] == read_tuples)
    {
      // Either trivially true, or trivially false, depending on how many tuples there are.
      if(con.tuples->size() != 0)
        return ConstraintBlob(get_constraint(CT_TRUE));
      else
        return ConstraintBlob(get_constraint(CT_FALSE));
    }
    else
    {
      if(con.short_tuples->size() == 0)
        return ConstraintBlob(get_constraint(CT_FALSE));
      else if((*con.short_tuples->tuplePtr())[0].empty())
          return ConstraintBlob(get_constraint(CT_TRUE));
      else throw parse_exception("Not a valid list of short tuples for a constraint with no variables!");
    }
  }
*/
  return con;
}


ConstraintBlob MinionJSONInputReader::readGeneralConstraint(JsonValue infile, ConstraintDef* def)
{
  assert(0);
  // This slightly strange code is to save copying the ConstraintBlob as much as possible.
  ConstraintBlob con(def);

  vector<vector<Var> >& varsblob = con.vars;
  vector<vector<DomainInt> >& constblob = con.constants;

/*
  for(SysInt i = 0; i < def->number_of_params; ++i)
  {
    switch(def->read_types[i])
    {
      case read_list:
      varsblob.push_back(readLiteralVector(infile));
      break;
      case read_var:
      varsblob.push_back(make_vec(readIdentifier(infile)));
      break;
      case read_2_vars:
      {
        vector<Var> vars(2);
        vars[0] = readIdentifier(infile);
        infile->check_sym(',');
        vars[1] = readIdentifier(infile);
        varsblob.push_back(std::move(vars));
      }
      break;
      case read_constant:
      constblob.push_back(make_vec(infile->read_num()));
      break;
      case read_constant_list:
      {
        vector<Var> vectorOfConst = readLiteralVector(infile);
        vector<DomainInt> vals;
          for(UnsignedSysInt loop = 0; loop < vectorOfConst.size(); ++loop)
          {
            if(vectorOfConst[loop].type() != VAR_CONSTANT)
                throw parse_exception("Vector must only contain constants.");
              else
          vals.push_back(vectorOfConst[loop].pos());
          }
          constblob.push_back(std::move(vals));
      }
      break;
      case read_constraint:
      con.internal_constraints.push_back(readConstraint(infile, false));
      break;
      case read_constraint_list:
      con.internal_constraints = readConstraintList(infile);
      break;
      case read_tuples:
      if(con.tuples == NULL)
        con.tuples = readConstraintTupleList(infile);
      else if(con.tuples2 == NULL)
        con.tuples2 = readConstraintTupleList(infile);
      else throw parse_exception("Too many tuplelists");
      break;
      default:
      D_FATAL_ERROR("Internal Error!");
    }
    if(i != def->number_of_params - 1)
      infile->check_sym(',');
  }
  infile->check_sym(')');
*/
  return con;
}


ShortTupleList* MinionJSONInputReader::readConstraintShortTupleList(JsonValue infile)
{
  assert(0);
  /*
  string name = infile->get_string();
  return instance->getShortTableSymbol(name);
  */
}


TupleList* MinionJSONInputReader::readConstraintTupleList(JsonValue infile)
{
  assert(0);
  TupleList* tuplelist;
/*
  if(infile->peek_char() != '{')
  {
    string name = infile->get_string();
    tuplelist = instance->getTableSymbol(name);
  }
  else
  {
    vector<vector<DomainInt> > tuples ;
    infile->check_sym('{');
    char delim = infile->peek_char();

    SysInt tupleSize = 0;

    while (delim != '}')
    {
      infile->check_sym('<');
      vector<DomainInt> tuple;
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
      tuples.push_back(std::move(tuple)) ;

      delim = infile->get_char();                          // ',' or '}'
      if(delim != ',' && delim!= '}')
        throw parse_exception("Expected ',' or '}'");
    }
    tuplelist = instance->tupleListContainer->getNewTupleList(tuples);
    instance->addUnnamedTableSymbol(tuplelist);
  }
*/
  return tuplelist;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// readConstraintOr
// or(<vectorOfVars>)
// SAT clauses represented as literals and negated literals
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

ConstraintBlob MinionJSONInputReader::readConstraintOr(JsonValue infile,
  ConstraintDef* ct)
{
  assert(0);
  /*
  MAYBE_PARSER_INFO("Reading a SAT clause");
  infile->check_sym('[');
  vector<DomainInt> negs;
  vector<Var> clause_vars;
  if(infile->peek_char() != ']')
    clause_vars.push_back(readIdentifier(infile));
  while(infile->peek_char() != ']') {
    infile->check_sym(',');
    clause_vars.push_back(readIdentifier(infile));
  }
  infile->check_sym(']');
  infile->check_sym(')');
  for(SysInt i = 0; i < clause_vars.size(); i++) {
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
  */
}


/// Reads an identifier which represents a single variable or constant.

Var MinionJSONInputReader::readIdentifier(JsonValue infile) {

    switch(infile.getTag())
    {
      case JSON_TAG_NUMBER:
        return Var(VAR_CONSTANT, infile.toNumber());
      case JSON_TAG_STRING:
        {
          std::string name = infile.toString();
          bool negVar = false;
          // Check to see if this is a negated Boolean
          if(name.size() > 0 && name[0] == '!')
          {
            negVar = true;
            name.erase(name.begin());
          }

          Var var = instance->vars.getSymbol(name);

          if(negVar)
          {
            if(var.type() != VAR_BOOL)
            {  throw parse_exception("Only booleans can be negated!"); }
            else
              var.setType(VAR_NOTBOOL);
          }
        }
      default:
        throw parse_exception("Invalid variable");
    }
}


vector<ConstraintBlob> MinionJSONInputReader::readConstraintList(JsonValue infile) {
  vector<ConstraintBlob> conlist;

  check_tag(infile, JSON_TAG_ARRAY, "constraints must be a list");

  for(auto con : infile)
  {
    conlist.push_back(readConstraint(con->value));
  }
  return conlist;
}

/// Reads a vector of variables (which can include constants).
/// Accepts:
/// M (for matrix identifer M)
/// [ M,B,.. ] (for matrix identifers M and variables B)


vector<Var> MinionJSONInputReader::readLiteralVector(JsonValue infile) {
  assert(0);
  vector<Var> newVector;
/*
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
    MAYBE_PARSER_INFO("Read empty vector.");
  }
  else
  {
    while (delim != ']') {
      vector<Var> v = readPossibleMatrixIdentifier(infile);
      newVector.insert(newVector.end(), v.begin(), v.end());
      delim = infile->get_char();
      if(delim != ',' && delim != ']')
      {
  // replace X with the character we got.
        string s = "Expected ',' or ']'. Got 'X'.";
        s[s.size() - 3] = delim;
        throw parse_exception(s);
      }

      if(delim == ',' && infile->peek_char() == ']')
        delim = infile->get_char();
    }
  }
  */
  return newVector;
}



vector<vector<Var> > MinionJSONInputReader::read2DMatrix(JsonValue infile)
{
  assert(0);
  vector<vector<Var> > return_vals;
/*
  if(infile->peek_char() != '[')
    return read2DMatrixVariable(infile);

  infile->check_sym('[');

  while(infile->peek_char() != ']')
  {
    MAYBE_PARSER_INFO("Continuing reading matrix, peeked at " + tostring(']'));
    // See if there is an array, or just a variable.
    if(infile->peek_char() == '[')
      return_vals.push_back(readLiteralVector(infile));
    else
    {
      vector<vector<Var> > vars = read2DMatrixVariable(infile);
      for(SysInt i = 0; i < vars.size(); ++i)
        return_vals.push_back(vars[i]);
    }
    // Eat a comma if there is one there.
    if(infile->peek_char() == ',')
      infile->check_sym(',');
  }

  infile->check_sym(']');
*/
  return return_vals;
}

// This function reads the next identifier, which should be a 1D or 2D matrix,
// and returns it (if it was 1D, it returns it as a 1 row 2D matrix.

vector<vector<Var> > MinionJSONInputReader::read2DMatrixVariable(JsonValue infile) {
  assert(0);
  string name; /*= infile->get_string();
/*
  Var var = instance->vars.getSymbol(name);
  // Check it is a matrix
  if(var.type() != VAR_MATRIX)
    throw parse_exception("Expected matrix");
  // Get dimension of matrix.
  vector<DomainInt> indices = instance->vars.getMatrixSymbol(name);
  // Make sure the matrix doesn't have an index after it. This is to produce better error messages.
  if(infile->peek_char() != ',' && infile->peek_char() != ']')
    throw parse_exception("Only accept raw matrix names here, expected ',' next.");

  if(indices.size() == 1)
  {
    vector<DomainInt> terms;
    terms.push_back(-999);
    // Use the existing code to flatten a matrix.
    // make_vec takes a T and turns it into a 1 element vector<T>.
    return make_vec(instance->vars.buildVarList(name, terms));
  }
  else
  {
    return instance->vars.flattenTo2DMatrix(name);
  }
*/
}

// Note: allowNulls maps '_' to -999 (a horrible hack I know).
// That last parameter defaults to false.
// The start and end default to '[' and ']'

vector<DomainInt> MinionJSONInputReader::readConstantVector
  (JsonValue infile, char start, char end, bool allowNulls)
{
  assert(0);
  vector<DomainInt> newVector;
  /*
  infile->check_sym(start);

  // The reason we peek here is in case this is an empty vector
  char delim = infile->peek_char();

  if(delim == end)
  {
    // Eat the ']'
    infile->get_char();
    MAYBE_PARSER_INFO("Read empty vector.");
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
  */
  return newVector;
}

// Note: allowNulls maps '_' to -999 (a horrible hack I know).
// That last parameter defaults to false.
// The start and end default to '[' and ']'

vector<pair<SysInt,DomainInt> > MinionJSONInputReader::readShortTuple(JsonValue infile)
{
  assert(0);
  vector<pair<SysInt, DomainInt> > newVector;
/*
  infile->check_sym('[');

  while(infile->peek_char() == '(')
  {
    infile->check_sym('(');
    SysInt var = checked_cast<SysInt>(infile->read_num());
    infile->check_sym(',');
    DomainInt val = infile->read_num();
    infile->check_sym(')');
    newVector.push_back(make_pair(var, val));
    if(infile->peek_char() == ',')
      infile->check_sym(',');
  }
  infile->check_sym(']');
*/
  return newVector;
}

/// Read an expression of the type ' {<num>..<num>} '

vector<DomainInt> MinionJSONInputReader::readRange(JsonValue infile)
{
  assert(0);
  vector<DomainInt> newVector;
/*
  infile->check_sym('{');

  newVector.push_back(infile->read_num());
  infile->check_sym('.');
  infile->check_sym('.');

  newVector.push_back(infile->read_num());

  infile->check_sym('}');
*/
  return newVector;
}


/// Read a list of tuples

void MinionJSONInputReader::readShortTuples(JsonValue infile)
{
  assert(0);
  /*
  while(infile->peek_char() != '*')
  {
    string name = infile->get_string();
    DomainInt num_of_short_tuples = infile->read_num();
    vector<vector<pair<SysInt, DomainInt> > > tups;

    for(DomainInt i = 0; i < num_of_short_tuples; ++i)
      tups.push_back(readShortTuple(infile));

    ShortTupleList* stl = instance->shortTupleListContainer->getNewShortTupleList(tups);
    instance->addShortTableSymbol(name, stl);
  }
  */
}


void MinionJSONInputReader::readTuples(JsonValue infile)
{
  assert(0);
  /*
  while(infile->peek_char() != '*')
  {
    string name = infile->get_string();
    DomainInt num_of_tuples = infile->read_num();
    DomainInt tuple_length = infile->read_num();
    MAYBE_PARSER_INFO("Reading tuplelist '" + name + "', length " + tostring(num_of_tuples) +
      ", arity " + tostring(tuple_length) );
    TupleList* tuplelist = instance->tupleListContainer->getNewTupleList(num_of_tuples, tuple_length);
    DomainInt* tuple_ptr = tuplelist->getPointer();
    for(DomainInt i = 0; i < num_of_tuples; ++i)
      for(DomainInt j = 0; j < tuple_length; ++j)
    {
      tuple_ptr[checked_cast<SysInt>(i * tuple_length + j)] = infile->read_num();
    }
    tuplelist->finalise_tuples();
    instance->addTableSymbol(name, tuplelist);

    if(map_long_short_mode != MLTTS_NoMap)
    {
      ShortTupleList* stl = instance->shortTupleListContainer->getNewShortTupleList(tuplelist, map_long_short_mode);
      instance->addShortTableSymbol(name, stl);
    }
  }
  */
}


void MinionJSONInputReader::readSearch(JsonValue infile) {
  assert(0);
  /*
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
        if(vo == ORDER_WDEG || vo == ORDER_DOMOVERWDEG)
        {
#ifndef WDEG
          USER_ERROR("This minion was not compiled with support for WDEG or DOMOVERWDEG orderings (add -WDEG to build options)");
#endif
        }
throw parse_exception("Don't understand '" + s + "'");
found: ;
      }

      instance->search_order.push_back(SearchOrder(readLiteralVector(infile), vo, find_one_sol));
      MAYBE_PARSER_INFO("Read var order, length " +
        tostring(instance->search_order.back().var_order.size()));
    }
    else if(var_type == "PERMUTATION")
    {
      if(!instance->permutation.empty())
        throw parse_exception("Can't have two PERMUTATIONs!");
      instance->permutation = readLiteralVector(infile);
      MAYBE_PARSER_INFO("Read permutation, length " +
        tostring(instance->permutation.size()));
    }
    else if(var_type == "SYMORDER")
    {
      if(!instance->sym_order.empty())
        throw parse_exception("Can't have two SYMORDERs!");
      instance->sym_order = readLiteralVector(infile);
      MAYBE_PARSER_INFO("Read Symmetry Ordering, length " +
        tostring(instance->permutation.size()));
    }
    else if(var_type == "VALORDER")
    {
      if(instance->search_order.empty())
        throw parse_exception("Must declare VARORDER first");
      if(!instance->search_order.back().val_order.empty())
        throw parse_exception("Can't have two VALORDERs for a VARORDER");
      vector<ValOrderEnum> valOrder ;

      infile->check_sym('[');

      char delim = infile->peek_char();

      while (delim != ']') {
        char valOrderIdentifier = infile->get_char();
        switch(valOrderIdentifier)
        {
          case 'a':
            valOrder.push_back(VALORDER_ASCEND);
            break;
          case 'd':
            valOrder.push_back(VALORDER_DESCEND);
            break;
          case 'r':
            valOrder.push_back(VALORDER_RANDOM);
            break;

          default:
            throw parse_exception("Expected 'a' or 'd' or 'r'");
        }
        delim = infile->get_char();                                 // , or ]
      }
      instance->search_order.back().val_order = valOrder;

      MAYBE_PARSER_INFO("Read val order, length " +
          tostring(instance->search_order.back().val_order.size()));
    }
    else if(var_type == "MAXIMISING" || var_type == "MAXIMIZING")
    {
      if(instance->is_optimisation_problem == true)
        throw parse_exception("Can only have one min / max per problem!");

      Var var = readIdentifier(infile);
      MAYBE_PARSER_INFO("Maximising " + tostring(var));
      instance->set_optimise(false, var);
    }
    else if(var_type == "MINIMISING" || var_type == "MINIMIZING")
    {
      if(instance->is_optimisation_problem == true)
        throw parse_exception("Can only have one min / max per problem!");

      Var var = readIdentifier(infile);
      MAYBE_PARSER_INFO("Minimising " + tostring(var));
      instance->set_optimise(true, var);
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
        for(SysInt i = 0; i < new_matrix.size(); ++i)
          instance->print_matrix.push_back(new_matrix[i]);
      }
    }
    else if(var_type == "CONSTRUCTION")
    {
      if(!isGadgetReader())
        throw parse_exception("Only have construction sites on gadgets!");

      instance->constructionSite = readLiteralVector(infile);
      MAYBE_PARSER_INFO("Read construction site, size " + tostring(instance->constructionSite.size()));
    }
    else
      {  throw parse_exception("Don't understand '" + var_type + "' as a variable type."); }
  }
  */
}



void MinionJSONInputReader::readAliasMatrix(JsonValue infile, const vector<DomainInt>& max_indices, vector<DomainInt> indices, string name)
{
  assert(0);
  /*
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
      ++indices.back();
      infile->check_sym('[');
      readAliasMatrix(infile, max_indices, indices, name);
      infile->check_sym(']');
    }
    if(indices.back() + 1 != max_indices[indices.size() - 1])
      throw parse_exception("Incorrectly sized matrix!, expected index " +
      tostring(indices.size() - 1) + " to have " + tostring(max_indices[indices.size() - 1]) +
      " terms, got " + tostring(indices.back() + 1));
  }
  else
  {
    // Have reached the bottom level!
    indices.push_back(0);
    Var v = readIdentifier(infile);
    instance->vars.addSymbol(name + to_var_name(indices), v);
    while(infile->peek_char() == ',')
    {
      infile->check_sym(',');
      ++indices.back();
      Var v = readIdentifier(infile);
      instance->vars.addSymbol(name + to_var_name(indices), v);
    }
    if(indices.back() + 1 != max_indices[indices.size() - 1])
      throw parse_exception("Incorrectly sized matrix!, expected index " +
      tostring(indices.size() - 1) + " to have " + tostring(max_indices[indices.size() - 1]) +
      " terms, got " + tostring(indices.back() + 1));
  }
  */
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void MinionJSONInputReader::readVars(JsonValue infile) {
  assert(0);
  /*
  while(infile->peek_char() != '*')
  {
    MAYBE_PARSER_INFO("Begin reading variables");
    string var_type = infile->get_string();

    if(var_type != "BOOL" && var_type != "BOUND" && var_type != "SPARSEBOUND"
      && var_type != "DISCRETE" && var_type != "ALIAS")
      throw parse_exception(string("Unknown variable type: '") + var_type + "'");

    string varname = infile->get_string();
    MAYBE_PARSER_INFO("Name:" + varname);

    bool isArray = false;
    vector<DomainInt> indices;

    if(infile->peek_char() == '[')
    {
      MAYBE_PARSER_INFO("Is array!");
      isArray = true;
      indices = readConstantVector(infile);
      for(UnsignedSysInt i = 0; i < indices.size(); ++i)
        if(indices[i] < 0)
          throw parse_exception("Matrix " + varname + " has a negative size for index " + tostring(i));
      MAYBE_PARSER_INFO("Found " + tostring(indices.size()) + " indices");
    }

    VariableType variable_type = VAR_INVALID;
    vector<DomainInt> domain;

    if(var_type == "ALIAS")
    {
      if(isArray == false)
      {
        infile->check_sym('='); // XYZ
        Var v = readIdentifier(infile);
        instance->vars.addSymbol(varname, v);
      }
      else
      {
        instance->vars.addMatrixSymbol(varname, indices);
        infile->check_sym('=');
        infile->check_sym('[');
        readAliasMatrix(infile, indices, vector<DomainInt>(), varname);
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
      if(domain[0]>domain[1])
        throw parse_exception("Range in decreasing order e.g. 1..0  in declaration of BOUND variable.");
      if(domain.size() != 2)
        throw parse_exception("Ranges contain 2 numbers!");
    }
    else if(var_type == "DISCRETE")
    {
      variable_type = VAR_DISCRETE;
      domain = readRange(infile);
      if(domain[0]>domain[1])
        throw parse_exception("Range in decreasing order e.g. 1..0  in declaration of BOUND variable.");
      if(domain.size() != 2)
        throw parse_exception("Ranges contain 2 numbers!");
    }
    else if(var_type == "SPARSEBOUND")
    {
      variable_type = VAR_SPARSEBOUND;
      domain = readConstantVector(infile, '{', '}');

      for(unsigned int i=0; i<domain.size()-1; i++) {
          if(domain[i]>domain[i+1]) {
              throw parse_exception("Values out of order in SPARSEBOUND domain.");
          }
          if(domain[i]==domain[i+1]) {
              throw parse_exception("Repeated values in SPARSEBOUND domain.");
          }
      }
      if(domain.size() < 1)
        throw parse_exception("Don't accept empty domains!");
    }
    else
      throw parse_exception("I don't know about var_type '" + var_type + "'");

    if(var_type != "ALIAS")
    {
      if(isArray)
      {
        instance->vars.addMatrixSymbol(varname, indices);
        // If any index is 0, don't add any variables.
        if(find(indices.begin(), indices.end(), 0) == indices.end())
        {
          vector<DomainInt> current_index(indices.size(), 0);
          MAYBE_PARSER_INFO("New Var: " + varname + to_var_name(current_index));
          instance->vars.addSymbol(varname + to_var_name(current_index),
            instance->vars.getNewVar(variable_type, domain));
          while(increment_vector(current_index, indices))
          {
            MAYBE_PARSER_INFO("New Var: " + varname + to_var_name(current_index));
            instance->vars.addSymbol(varname + to_var_name(current_index),
              instance->vars.getNewVar(variable_type, domain));
          }

          vector<vector<Var> > matrix_list = instance->vars.flattenTo2DMatrix(varname);
          for(SysInt i = 0; i < matrix_list.size(); ++i)
            instance->all_vars_list.push_back(matrix_list[i]);
        }
      }
      else
      {
        Var v = instance->vars.getNewVar(variable_type, domain);
        instance->vars.addSymbol(varname, v);
        instance->all_vars_list.push_back(make_vec(v));
      }
    }
  }
*/
}
