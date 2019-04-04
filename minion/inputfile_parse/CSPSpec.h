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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

#ifndef CSPSPEC_H
#define CSPSPEC_H

#include <list>
#include <map>
#include <utility>
#include <vector>

using namespace std;

#include "../system/system.h"

#include "../constants.h"

#include "InputVariableDef.h"

#include "../tuple_container.h"

/// The currently accepted types of Constraints.

#include "ConstraintEnum.h"

inline string to_var_name(const vector<DomainInt>& params) {
  ostringstream s;
  s << "_";
  for(SysInt i = 0; i < (SysInt)params.size(); ++i)
    s << tostring(params[i]) << "_";
  s << "_";
  return s.str();
}

enum ReadTypes {
  read_list,
  read_var,
  read_2_vars,
  read_constant,
  read_constant_list,
  read_tuples,
  read_short_tuples,
  read_constraint,
  read_constraint_list,
  read_nothing
};

enum VarOrderEnum {
  ORDER_NONE,

  ORDER_STATIC,
  ORDER_SDF,
  ORDER_SRF,
  ORDER_LDF,
  ORDER_ORIGINAL,
  ORDER_WDEG,
  ORDER_DOMOVERWDEG,
  ORDER_CONFLICT,
  ORDER_STATIC_LIMITED,
};

inline std::ostream& operator<<(std::ostream& o, VarOrderEnum voe) {
  switch(voe) {
  case ORDER_NONE: return o << "NONE";
  case ORDER_STATIC: return o << "STATIC";
  case ORDER_SDF: return o << "SDF";
  case ORDER_SRF: return o << "SRF";
  case ORDER_LDF: return o << "LDF";
  case ORDER_ORIGINAL: return o << "ORIGINAL";
  case ORDER_STATIC_LIMITED: return o << "STATIC_LIMITED";
  case ORDER_WDEG: return o << "WDEG";
  case ORDER_DOMOVERWDEG: return o << "DOMOVERWDEG";
  case ORDER_CONFLICT: return o << "CONFLICT";
  }
  abort();
}

enum ValOrderEnum { VALORDER_NONE, VALORDER_ASCEND, VALORDER_DESCEND, VALORDER_RANDOM };

struct ValOrder {
  ValOrderEnum type;
  int bias;

  ValOrder(ValOrderEnum t, int b = 0) : type(t), bias(b) {}

  friend bool operator==(const ValOrder& lhs, const ValOrder& rhs) {
    return std::make_tuple(lhs.type, lhs.bias) == std::make_tuple(rhs.type, rhs.bias);
  }

  friend bool operator!=(const ValOrder& lhs, const ValOrder& rhs) {
    return !(lhs == rhs);
  }
};

struct ConstraintDef {
  std::string name;
  ConstraintType type;
  SysInt numberOfParams;
  std::array<ReadTypes, 5> read_types;
};

extern ConstraintDef constraint_list[];

namespace ProbSpec {
struct CSPInstance;

/// Constructed by the parser. Suitable for holding any kind of constraint.
struct ConstraintBlob {
  /// The type of constraint.
  ConstraintDef* constraint;
  /// The variables of the problem.
  vector<vector<Var>> vars;
  /// Pointer to list of tuples. Only used in Table Constraints.
  TupleList* tuples;

  /// Pointer to a list of short tuples. Only used in Short Table constraints.
  ShortTupleList* shortTuples;

  TupleList* tuples2;

  /// A vector of signs. Only used for SAT clause "or" constraint.
  vector<DomainInt> negs;

  /// A vector of constants, for any constraint which reads a constant array
  vector<vector<DomainInt>> constants;

  /// For use in Gadget constraints, lists the propagation level to be achieved.
  PropagationLevel gadget_prop_type;

  /// For use in Gadget constraints, gives the actual gadget.
  shared_ptr<CSPInstance> gadget;

  /// For use in nested constraints.
  vector<ConstraintBlob> internal_constraints;

  ConstraintBlob() {
    assert(0);
  }

  ConstraintBlob(ConstraintDef* _con) : constraint(_con), tuples(0), tuples2(0) {}

  ConstraintBlob(ConstraintDef* _con, const vector<vector<Var>>& _vars)
      : constraint(_con), vars(_vars), tuples(0), shortTuples(0), tuples2(0) {}

  /// A helper constructor for when only a SingleVar is passed.
  ConstraintBlob(ConstraintDef* _con, vector<Var>& _var) : constraint(_con), tuples(0), tuples2(0) {
    vars.push_back(_var);
  }

  set<Var> getAllVars() const {
    set<Var> return_vars;
    for(vector<vector<Var>>::const_iterator it = vars.begin(); it != vars.end(); ++it) {
      for(vector<Var>::const_iterator it2 = (*it).begin(); it2 != (*it).end(); ++it2) {
        if(it2->type() != VAR_CONSTANT) {
          if(it2->type() == VAR_NOTBOOL)
            return_vars.insert(Var(VAR_BOOL, it2->pos()));
          else
            return_vars.insert(*it2);
        }
      }
    }

    for(vector<ConstraintBlob>::const_iterator it = internal_constraints.begin();
        it != internal_constraints.end(); ++it) {
      set<Var> newvars = it->getAllVars();
      return_vars.insert(newvars.begin(), newvars.end());
    }

    return return_vars;
  }
};

/// Contains all the variables in a CSP instance.
struct VarContainer {
  SysInt BOOLs;
  INPUT_MAP_TYPE<string, Var> symbol_table;
  INPUT_MAP_TYPE<Var, string> name_table;
  vector<Bounds> bound;
  vector<vector<DomainInt>> sparseBound;
  vector<Bounds> discrete;
  vector<vector<DomainInt>> sparseDiscrete;
  INPUT_MAP_TYPE<string, vector<DomainInt>> matrix_table;
  VarContainer() : BOOLs(0) {}

  /// Given a matrix variable and a parameter list, returns a slice of the
  /// matrix.
  /// Params can either be wildcards (denoted -999), or a value for the matrix.
  vector<Var> buildVarList(const string& name, const vector<DomainInt>& params) {
    vector<Var> return_list;

    vector<DomainInt> maxIndex = getMatrixSymbol(name);
    if(params.size() != maxIndex.size())
      throw parse_exception("Can't index a " + tostring(maxIndex.size()) + "-d matrix with " +
                            tostring(params.size()) + " indices.");
    for(SysInt i = 0; i < (SysInt)params.size(); ++i) {
      // Horrible hack: -999 means it was an _
      if(params[i] != -999 && (params[i] < 0 || params[i] >= maxIndex[i]))
        throw parse_exception(tostring(i) + string("th index is invalid"));
    }

    // Set all fixed indices to 1, so they won't move.
    // Set all variable indices to their max value.
    vector<DomainInt> modifiedMax(params.size());
    for(SysInt i = 0; i < (SysInt)maxIndex.size(); i++) {
      if(maxIndex[i] == 0) { // matrix is empty, all slices are empty.
        return return_list;
      }

      if(params[i] == -999)
        modifiedMax[i] = maxIndex[i];
      else
        modifiedMax[i] = 1;
    }

    // Iterates through the variable indices
    vector<DomainInt> currentIndex(params.size());

    // Vector which actually contains the output
    vector<DomainInt> output(params);
    do {
      for(SysInt i = 0; i < (SysInt)maxIndex.size(); i++)
        if(params[i] == -999)
          output[i] = currentIndex[i];
      return_list.push_back(getSymbol(name + to_var_name(output)));
    } while(incrementVector(currentIndex, modifiedMax));

    return return_list;
  }

  vector<vector<Var>> flattenTo2DMatrix(const string& name) {
    // The following code looks a bit weird, but aims to maximise code reuse.
    // The idea is that we use buildVarList with all but the last parameter wild
    // to get
    // out 'slices' of the matrix for each value to the last index. Then glue
    // these together
    // and for any sized matrix, we get a list of vectors, with all but the last
    // dimension flattened!
    vector<DomainInt> indices = getMatrixSymbol(name);
    vector<DomainInt> loop_indices(indices.size());
    for(SysInt i = 0; i < (SysInt)indices.size() - 1; ++i)
      loop_indices[i] = -999;

    vector<vector<Var>> terms;

    for(SysInt i = 0; i < indices.back(); ++i) {
      loop_indices.back() = i;
      vector<Var> slice = buildVarList(name, loop_indices);
      // This line should only do something first pass through the loop.
      terms.resize(slice.size());
      for(SysInt i = 0; i < (SysInt)slice.size(); ++i)
        terms[i].push_back(slice[i]);
    }
    return terms;
  }

  void addSymbol(const string& name, Var variable) {
    if(name == "")
      throw parse_exception("Cannot have an empty name!");
    if(name[0] >= 0 && name[0] <= 9)
      throw parse_exception("Names cannot start with a number!:" + name);
    if(symbol_table.count(name) != 0)
      throw parse_exception("Name already in table:" + name);
    symbol_table[name] = variable;

    if(name_table.find(variable) == name_table.end())
      name_table[variable] = name;
  }

  void addMatrixSymbol(const string& name, const vector<DomainInt>& indices) {
    Var var(VAR_MATRIX, matrix_table.size());
    addSymbol(name, var);
    matrix_table[name] = indices;
  }

  Var getSymbol(const string& name) const {
    INPUT_MAP_TYPE<string, Var>::const_iterator it = symbol_table.find(name);
    if(it == symbol_table.end())
      throw parse_exception("Undefined name: '" + name + "'");
    return it->second;
  }

  string getName(const Var& var) const {
    if(var.type() == VAR_CONSTANT) {
      return tostring(var.pos()); // special case for constants
    }
    INPUT_MAP_TYPE<Var, string>::const_iterator it = name_table.find(var);
    if(it == name_table.end())
      throw parse_exception("Undefined Var");
    return it->second;
  }

  vector<DomainInt> getMatrixSymbol(const string& name) {
    INPUT_MAP_TYPE<string, vector<DomainInt>>::iterator it = matrix_table.find(name);
    if(it == matrix_table.end())
      throw parse_exception("Undefined matrix: '" + name + "'");
    return it->second;
  }

  // Returns a bool, where the first element is if these are bounds.
  pair<BoundType, vector<DomainInt>> getDomain(Var v) const {
    vector<DomainInt> dom;
    switch(v.type()) {
    case VAR_CONSTANT: dom.push_back(v.pos()); return make_pair(Bound_No, dom);
    case VAR_BOOL:
      dom.push_back(0);
      dom.push_back(1);
      return make_pair(Bound_No, dom);
    case VAR_BOUND: {
      D_ASSERT(v.pos() < bound.size());
      dom.push_back(bound[v.pos()].lowerBound);
      dom.push_back(bound[v.pos()].upperBound);
      return make_pair(Bound_Yes, dom);
    }
    case VAR_SPARSEBOUND: {
      D_ASSERT(v.pos() < sparseBound.size());
      return make_pair(Bound_No, sparseBound[v.pos()]);
    }
    case VAR_DISCRETE: {
      D_ASSERT(v.pos() < discrete.size());
      dom.push_back(discrete[v.pos()].lowerBound);
      dom.push_back(discrete[v.pos()].upperBound);
      return make_pair(Bound_Yes, dom);
    }
    case VAR_SPARSEDISCRETE: {
      D_ASSERT(v.pos() < sparseDiscrete.size());
      return make_pair(Bound_No, sparseDiscrete[v.pos()]);
    }
    default: throw parse_exception("Internal Error - Unknown Variable Type");
    }
  }

  Bounds getBounds(Var v) const {
    switch(v.type()) {
    case VAR_CONSTANT: return Bounds(v.pos(), v.pos());
    case VAR_BOOL: return Bounds(0, 1);
    case VAR_BOUND: {
      D_ASSERT(v.pos() < bound.size());
      return bound[v.pos()];
    }

    case VAR_SPARSEBOUND: {
      D_ASSERT(v.pos() < sparseBound.size());
      return Bounds(sparseBound[v.pos()].front(), sparseBound[v.pos()].back());
    }

    case VAR_DISCRETE: {
      D_ASSERT(v.pos() < discrete.size());
      return discrete[v.pos()];
    }
    case VAR_SPARSEDISCRETE: {
      D_ASSERT(v.pos() < sparseDiscrete.size());
      return Bounds(sparseDiscrete[v.pos()].front(), sparseDiscrete[v.pos()].back());
    }
    default: throw parse_exception("Internal Error - Unknown Variable Type");
    }
  }

  Var get_var(char, DomainInt in) const {
    SysInt i = checked_cast<SysInt>(in);
    if(i < BOOLs)
      return Var(VAR_BOOL, i);
    i -= BOOLs;

    if(i < bound.size())
      return Var(VAR_BOUND, i);
    i -= bound.size();

    if(i < sparseBound.size())
      return Var(VAR_SPARSEBOUND, i);
    i -= sparseBound.size();

    if(i < discrete.size())
      return Var(VAR_DISCRETE, i);
    i -= discrete.size();

    if(i < sparseDiscrete.size())
      return Var(VAR_SPARSEDISCRETE, i);
    i -= sparseDiscrete.size();

    throw parse_exception("Var Out of Range!");
  }

  vector<Var> allVars;

  Var getNewVar(VariableType type, vector<DomainInt> bounds) {
    Var v;
    switch(type) {
    case VAR_BOOL: v = _getNewBoolVar(); break;
    case VAR_BOUND: v = _getNewBoundVar(bounds[0], bounds[1]); break;
    case VAR_SPARSEBOUND: v = _getNewSparseBoundVar(bounds); break;
    case VAR_DISCRETE: v = _getNewDiscreteVar(bounds[0], bounds[1]); break;
    default: D_FATAL_ERROR("Internal error");
    }
    allVars.push_back(v);
    return v;
  }

private:
  Var _getNewBoolVar() {
    Var newBool(VAR_BOOL, BOOLs);
    BOOLs++;
    return newBool;
  }

  Var _getNewBoundVar(DomainInt lower, DomainInt upper) {
    bound.push_back(Bounds(lower, upper));
    return Var(VAR_BOUND, (SysInt)bound.size() - 1);
  }

  Var _getNewSparseBoundVar(const vector<DomainInt>& vals) {
    sparseBound.push_back(vals);
    return Var(VAR_SPARSEBOUND, (SysInt)sparseBound.size() - 1);
  }

  Var _getNewDiscreteVar(DomainInt lower, DomainInt upper) {
    discrete.push_back(Bounds(lower, upper));
    return Var(VAR_DISCRETE, (SysInt)discrete.size() - 1);
  }

public:
  vector<Var> getAllVars() const {
    return allVars;
  }
};

struct SearchOrder {
  vector<Var> varOrder;
  vector<ValOrder> valOrder;
  VarOrderEnum order;
  unsigned int limit;
  bool find_one_assignment;

  SearchOrder() : order(ORDER_ORIGINAL), find_one_assignment(false) {}

  SearchOrder(const vector<Var>& _varOrder)
      : varOrder(_varOrder), order(ORDER_ORIGINAL), find_one_assignment(false) {}

  SearchOrder(const vector<Var>& _varOrder, VarOrderEnum _order, bool _find_one_assignment = false)
      : varOrder(_varOrder), order(_order), find_one_assignment(_find_one_assignment) {}

  void setupValueOrder() {
    if(valOrder.empty())
      valOrder.resize(varOrder.size(), VALORDER_ASCEND);
    while(valOrder.size() < varOrder.size())
      valOrder.push_back(valOrder.back());
    D_ASSERT(valOrder.size() == varOrder.size());
  }
};

struct CSPInstance {
  VarContainer vars;
  list<ConstraintBlob> constraints;
  shared_ptr<TupleListContainer> tupleListContainer;
  shared_ptr<ShortTupleListContainer> shortTupleListContainer;

  vector<SearchOrder> searchOrder;
  vector<Var> permutation;
  vector<Var> symOrder;

  /// Only used for gadgets.
  vector<Var> constructionSite;

  bool is_optimisation_problem;
  bool optimise_minimising;
  Var optimiseVariable;

  vector<vector<Var>> print_matrix;

  /// A complete list of variables in the order they are defined.
  vector<vector<Var>> allVars_list;

  map<string, TupleList*> table_symboltable;
  map<TupleList*, string> table_nametable;

  map<string, ShortTupleList*> shorttable_symboltable;
  map<ShortTupleList*, string> shorttable_nametable;

  /// We make these shared_ptrs so they automatically clear up after themselves.
  map<string, shared_ptr<CSPInstance>> gadgetMap;

  CSPInstance()
      : tupleListContainer(new TupleListContainer),
        shortTupleListContainer(new ShortTupleListContainer),
        is_optimisation_problem(false) {}

private:
  CSPInstance(const CSPInstance&);

public:
  void set_optimise(BOOL _minimising, Var var) {
    is_optimisation_problem = true;
    optimise_minimising = _minimising;
    optimiseVariable = var;
  }

  void add_constraint(const ConstraintBlob& b) {
    constraints.push_back(b);
  }

  void addUnnamedTableSymbol(TupleList* tuplelist) {
    if(table_nametable.count(tuplelist) != 0)
      return;
    SysInt pos = table_symboltable.size();
    while(table_symboltable.count("_Unnamed__" + tostring(pos)) != 0)
      pos++;

    table_symboltable["_Unnamed__" + tostring(pos) + "_"] = tuplelist;
    table_nametable[tuplelist] = "_Unnamed__" + tostring(pos) + "_";
    tuplelist->setName("_Unnamed__" + tostring(pos) + "_");
  }

  void addTableSymbol(string name, TupleList* tuplelist) {
    if(table_symboltable.count(name) != 0)
      throw parse_exception("Tuplename '" + name + "' already in use");
    if(table_nametable.count(tuplelist) != 0)
      throw parse_exception("Named tuplelist double registered!");
    table_symboltable[name] = tuplelist;
    table_nametable[tuplelist] = name;
    tuplelist->setName(name);
  }

  void addShortTableSymbol(string name, ShortTupleList* tuplelist) {
    if(shorttable_symboltable.count(name) != 0)
      throw parse_exception("ShortTuplename '" + name + "' already in use");
    if(shorttable_nametable.count(tuplelist) != 0)
      throw parse_exception("Named tuplelist double registered!");
    shorttable_symboltable[name] = tuplelist;
    shorttable_nametable[tuplelist] = name;
    tuplelist->setName(name);
  }

  TupleList* getTableSymbol(string name) const {
    map<string, TupleList*>::const_iterator it = table_symboltable.find(name);
    if(it == table_symboltable.end())
      throw parse_exception("Undefined tuplelist: '" + name + "'");
    return it->second;
  }

  string getTableName(TupleList* tuples) const {
    map<TupleList*, string>::const_iterator it = table_nametable.find(tuples);
    if(it == table_nametable.end())
      throw parse_exception("Undefined tuplelist: '" + tostring(size_t(tuples)) + "'");
    return it->second;
  }

  ShortTupleList* getShortTableSymbol(string name) const {
    map<string, ShortTupleList*>::const_iterator it = shorttable_symboltable.find(name);
    if(it == shorttable_symboltable.end())
      throw parse_exception("Undefined shorttuplelist: '" + name + "'");
    return it->second;
  }

  string getShortTableName(ShortTupleList* tuples) const {
    map<ShortTupleList*, string>::const_iterator it = shorttable_nametable.find(tuples);
    if(it == shorttable_nametable.end())
      throw parse_exception("Undefined shorttuplelist: '" + tostring(size_t(tuples)) + "'");
    return it->second;
  }

  void addGadgetSymbol(string name, shared_ptr<CSPInstance> gadget) {
    if(gadgetMap.count(name) != 0)
      throw parse_exception("Gadget name " + name + " already in use.");
    gadgetMap[name] = gadget;
  }

  shared_ptr<CSPInstance> getGadgetSymbol(string name) {
    map<string, shared_ptr<CSPInstance>>::iterator it = gadgetMap.find(name);
    if(it == gadgetMap.end())
      throw parse_exception("Undefined gadget name '" + name + "'");
    return it->second;
  }

  void add_variable_names() {
    if(vars.symbol_table.empty()) {
      // This was a MINION 1 or MINION 2 input file. Let's fix it!
      vector<Var> allVars = vars.getAllVars();

      for(SysInt i = 0; i < (SysInt)allVars.size(); ++i)
        vars.addSymbol("x" + tostring(i), allVars[i]);
    }

    if(symOrder.empty())
      symOrder = vars.getAllVars();
  }
};
} // namespace ProbSpec

extern ConstraintDef constraint_list[]; // why is this here twice?
extern SysInt numOfConstraints;

inline ConstraintDef* get_constraint(ConstraintType t) {
  for(SysInt i = 0; i < numOfConstraints; ++i) {
    if(constraint_list[i].type == t)
      return constraint_list + i;
  }

  D_FATAL_ERROR("Constraint not found");
}

using namespace ProbSpec;

#endif
