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

#ifndef CSPSPEC_H
#define CSPSPEC_H

#include <vector>
#include <list>
#include <utility>
#include <map>

using namespace std;

#include "../system/system.h"
#include "../constants.h"

#include "InputVariableDef.h"

#include "../tuple_container.h"


/// The currently accepted types of Constraints.

#include "../build_constraints/ConstraintEnum.h"

inline string to_var_name(const vector<int>& params)
{
  ostringstream s;
  s << "_";
  for(int i = 0; i < params.size(); ++i)
    s << to_string(params[i]) << "_";
  s << "_";
  return s.str();
}

enum ReadTypes
{
  read_list,
  read_var,
  read_2_vars,
  read_constant,
  read_constant_list,
  read_tuples,
  read_constraint,
  read_constraint_list,
  read_bool_var,
  read_nothing
};

enum VarOrderEnum
{
  ORDER_NONE,

  ORDER_STATIC,
  ORDER_SDF,
  ORDER_SRF,
  ORDER_LDF,
  ORDER_ORIGINAL,
  ORDER_WDEG,
  ORDER_DOMOVERWDEG,
  ORDER_CONFLICT
};

struct ConstraintDef
{
  std::string name;
  ConstraintType type;
  int number_of_params;
  array<ReadTypes,4> read_types;
};

extern ConstraintDef constraint_list[];



namespace ProbSpec
{
struct CSPInstance;


/// Constructed by the parser. Suitable for holding any kind of constraint.
struct ConstraintBlob
{
  /// The type of constraint.
  ConstraintDef* constraint;
  /// The variables of the problem.
  vector<vector<Var> > vars;
  /// Pointer to list of tuples. Only used in Table Constraints.
  TupleList* tuples;
  /// A vector of signs. Only used for SAT clause "or" constraint.
  vector<int> negs;

  /// A vector of constants, for any constraint which reads a constant array
  vector<vector<int> > constants;

  /// For use in Gadget constraints, lists the propagation level to be achieved.
  PropagationLevel gadget_prop_type;

  /// For use in Gadget constraints, gives the actual gadget.
  shared_ptr<CSPInstance> gadget;

  /// For use in nested constraints.
  vector<ConstraintBlob> internal_constraints;

  ConstraintBlob(ConstraintDef* _con) : constraint(_con)
  {}

  ConstraintBlob(ConstraintDef* _con, const vector<vector<Var> >& _vars) : constraint(_con), vars(_vars)
  {}

#ifdef USE_CXX0X
  ConstraintBlob(ConstraintBlob&& b) :
  CXXMOVE(constraint, b), CXXMOVE(vars, b), CXXMOVE(tuples, b), CXXMOVE(negs, b), CXXMOVE(constants, b),
  CXXMOVE(gadget_prop_type, b), CXXMOVE(gadget, b), CXXMOVE(internal_constraints, b)
  { }
#endif

  /// A helper constructor for when only a SingleVar is passed.
  ConstraintBlob(ConstraintDef* _con, vector<Var>& _var) : constraint(_con)
  { vars.push_back(_var); }

  set<Var> get_all_vars() const
  {
    set<Var> return_vars;
    for(vector<vector<Var> >::const_iterator it = vars.begin(); it != vars.end(); ++it)
    {
      for(vector<Var>::const_iterator it2 = (*it).begin(); it2 != (*it).end(); ++it2)
      {
        if(it2->type() != VAR_CONSTANT)
        {
          if(it2->type() == VAR_NOTBOOL)
            return_vars.insert(Var(VAR_BOOL, it2->pos()));
          else
            return_vars.insert(*it2);
        }
      }
    }

    for(vector<ConstraintBlob>::const_iterator it = internal_constraints.begin(); it != internal_constraints.end(); ++it)
    {
      set<Var> newvars = it->get_all_vars();
      return_vars.insert(newvars.begin(), newvars.end());
    }

    return return_vars;
  }
};


/// Contains all the variables in a CSP instance.
  struct VarContainer
{
  int BOOLs;
  INPUT_MAP_TYPE<string, Var> symbol_table;
  INPUT_MAP_TYPE<Var, string> name_table;
  vector<pair<int, Bounds> > bound;
  vector<pair<int, vector<int> > > sparse_bound;
  vector<pair<int, Bounds> > discrete;
  vector<pair<int, vector<int> > > sparse_discrete;
  INPUT_MAP_TYPE<string, vector<int> > matrix_table;
  VarContainer() : BOOLs(0)
  {}

#ifdef USE_CXX0X
  VarContainer(VarContainer&& v) :
  CXXMOVE(BOOLs, v), CXXMOVE(symbol_table, v), CXXMOVE(name_table, v), CXXMOVE(bound, v), CXXMOVE(sparse_bound, v),
  CXXMOVE(discrete, v), CXXMOVE(sparse_discrete, v), CXXMOVE(matrix_table, v)
  { }
#endif


  /// Given a matrix variable and a parameter list, returns a slice of the matrix.
  /// Params can either be wildcards (denoted -999), or a value for the matrix.
  vector<Var> buildVarList(const string& name, const vector<int>& params)
  {
     vector<Var> return_list;

     vector<int> max_index = getMatrixSymbol(name);
     if(params.size() != max_index.size())
      throw parse_exception("Can't index a " + to_string(max_index.size()) +
                            "-d matrix with " + to_string(params.size()) +
                            " indices.");
    for(int i = 0; i < params.size(); ++i)
    {
      // Horrible hack: -999 means it was an _
      if(params[i] != -999 && (params[i] < 0 || params[i] >= max_index[i]))
        throw parse_exception(to_string(i) + string("th index is invalid"));
    }

    // Set all fixed indices to 1, so they won't move.
    // Set all variable indices to their max value.
    vector<int> modified_max(params.size());
    for(int i = 0; i < max_index.size(); i++)
    {
      if(max_index[i]==0)
      {   // matrix is empty, all slices are empty.
          return return_list;
      }
      
      if(params[i] == -999)
        modified_max[i] = max_index[i];
      else
        modified_max[i] = 1;
    }
    
    // Iterates through the variable indices
    vector<int> current_index(params.size());
    
    // Vector which actually contains the output
    vector<int> output(params);
    do
    {
      for(int i = 0; i < max_index.size(); i++)
        if(params[i] == -999)
          output[i] = current_index[i];
      return_list.push_back(getSymbol(name + to_var_name(output)));
    }
    while(increment_vector(current_index, modified_max));
    
    return return_list;
  }

  vector<vector<Var> > flattenTo2DMatrix(const string& name)
  {
    // The following code looks a bit weird, but aims to maximise code reuse.
    // The idea is that we use buildVarList with all but the last parameter wild to get
    // out 'slices' of the matrix for each value to the last index. Then glue these together
    // and for any sized matrix, we get a list of vectors, with all but the last dimension flattened!
    vector<int> indices = getMatrixSymbol(name);
    vector<int> loop_indices(indices.size());
    for(int i = 0; i < indices.size() - 1; ++i)
      loop_indices[i] = -999;

    vector<vector<Var> > terms;

    for(int i = 0; i < indices.back(); ++i)
    {
      loop_indices.back() = i;
      vector<Var> slice = buildVarList(name, loop_indices);
      // This line should only do something first pass through the loop.
      terms.resize(slice.size());
      for(int i = 0; i < slice.size(); ++i)
        terms[i].push_back(slice[i]);
    }
    return terms;

  }


  void addSymbol(const string& name, Var variable)
  {
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

  void addMatrixSymbol(const string& name, const vector<int>& indices)
  {
    Var var(VAR_MATRIX, matrix_table.size());
    addSymbol(name, var);
    matrix_table[name] = indices;
  }

  Var getSymbol(const string& name) const
  {
    INPUT_MAP_TYPE<string, Var>::const_iterator it = symbol_table.find(name);
    if(it == symbol_table.end())
      throw parse_exception("Undefined name: '" + name + "'");
    return it->second;
  }

  string getName(const Var& var) const
  {
    if(var.type() == VAR_CONSTANT) {
      return to_string(var.pos()); //special case for constants
    }
    INPUT_MAP_TYPE<Var, string>::const_iterator it = name_table.find(var);
    if(it == name_table.end())
      throw parse_exception("Undefined Var");
    return it->second;
  }

  vector<int> getMatrixSymbol(const string& name)
  {
    INPUT_MAP_TYPE<string, vector<int> >::iterator it = matrix_table.find(name);
    if(it == matrix_table.end())
      throw parse_exception("Undefined matrix: '" + name + "'");
    return it->second;
  }

  // Returns a bool, where the first element is if these are bounds.
  pair<BoundType, vector<DomainInt> > get_domain(Var v) const
  {
    vector<DomainInt> dom;
    switch(v.type())
    {
    case VAR_CONSTANT:
      dom.push_back(v.pos());
      return make_pair(Bound_No, dom);
    case VAR_BOOL:
      dom.push_back(0);
      dom.push_back(1);
      return make_pair(Bound_No, dom);
    case VAR_BOUND:
      {
        int bound_size = 0;
        for(unsigned int x = 0; x < bound.size(); ++x)
        {
          bound_size += bound[x].first;
          if(v.pos() < bound_size)
          {
            dom.push_back(bound[x].second.lower_bound);
            dom.push_back(bound[x].second.upper_bound);
            return make_pair(Bound_Yes, dom);
          }
        }
        throw parse_exception("Internal Error - Bound OverFlow");
      }
    case VAR_SPARSEBOUND:
      {
        int sparse_bound_size = 0;
        for(unsigned int x=0;x<sparse_bound.size();++x)
        {
          sparse_bound_size += sparse_bound[x].first;
          if(v.pos() < sparse_bound_size)
            return make_pair(Bound_No, sparse_bound[x].second);
        }
        throw parse_exception("Internal Error - SparseBound OverFlow");
      }
    case VAR_DISCRETE:
    {
      int discrete_size = 0;
      for(unsigned int x = 0; x < discrete.size(); ++x)
      {
        discrete_size += discrete[x].first;
        if(v.pos() < discrete_size)
        {
          dom.push_back(discrete[x].second.lower_bound);
          dom.push_back(discrete[x].second.upper_bound);
          return make_pair(Bound_Yes, dom);
        }
      }
      throw parse_exception("Internal Error - Discrete OverFlow");
    }
    case VAR_SPARSEDISCRETE:
    {
      int sparse_discrete_size = 0;
      for(unsigned int x = 0; x < sparse_discrete.size(); ++x)
      {
        sparse_discrete_size += sparse_discrete[x].first;
        if(v.pos() < sparse_discrete_size)
          return make_pair(Bound_No, sparse_discrete[x].second);
      }
      throw parse_exception("Internal Error - SparseDiscrete OverFlow");
    }
    default:
      throw parse_exception("Internal Error - Unknown Variable Type");
    }
  }


  Bounds get_bounds(Var v) const
  {
    switch(v.type())
    {
    case VAR_CONSTANT:
      return Bounds(v.pos(), v.pos());
    case VAR_BOOL:
      return Bounds(0,1);
    case VAR_BOUND:
      {
        int bound_size = 0;
        for(unsigned int x = 0; x < bound.size(); ++x)
        {
          bound_size += bound[x].first;
          if(v.pos() < bound_size)
            return bound[x].second;
        }
        throw parse_exception("Internal Error - Bound OverFlow");
      }

    case VAR_SPARSEBOUND:
      {
        int sparse_bound_size = 0;
        for(unsigned int x=0;x<sparse_bound.size();++x)
        {
          sparse_bound_size += sparse_bound[x].first;
          if(v.pos() < sparse_bound_size)
            return Bounds(sparse_bound[x].second.front(), sparse_bound[x].second.back());
        }
        throw parse_exception("Internal Error - SparseBound OverFlow");
      }
    case VAR_DISCRETE:
    {
      int discrete_size = 0;
      for(unsigned int x = 0; x < discrete.size(); ++x)
      {
        discrete_size += discrete[x].first;
        if(v.pos() < discrete_size)
          return discrete[x].second;
      }
      throw parse_exception("Internal Error - Discrete OverFlow");
    }
    case VAR_SPARSEDISCRETE:
    {
      int sparse_discrete_size = 0;
      for(unsigned int x = 0; x < sparse_discrete.size(); ++x)
      {
        sparse_discrete_size += sparse_discrete[x].first;
        if(v.pos() < sparse_discrete_size)
          return Bounds(sparse_discrete[x].second.front(), sparse_discrete[x].second.back());
      }
      throw parse_exception("Internal Error - SparseDiscrete OverFlow");
    }
    default:
      throw parse_exception("Internal Error - Unknown Variable Type");
    }
  }

  Var get_var(char, int i) const
  {
    if(i < BOOLs)
      return Var(VAR_BOOL, i);
    i -= BOOLs;
    {
      int bound_size = 0;
      for(unsigned int x = 0; x < bound.size(); ++x)
        bound_size += bound[x].first;
      if(i < bound_size)
        return Var(VAR_BOUND, i);
      i -= bound_size;
    }
    {
      int sparse_bound_size = 0;
      for(unsigned int x=0;x<sparse_bound.size();++x)
        sparse_bound_size += sparse_bound[x].first;
      if(i < sparse_bound_size)
        return Var(VAR_SPARSEBOUND, i);
      i -= sparse_bound_size;
    }

    {
      int discrete_size = 0;
      for(unsigned int x=0;x<discrete.size();++x)
        discrete_size += discrete[x].first;
      if(i < discrete_size)
        return Var(VAR_DISCRETE, i);
      i -= discrete_size;
    }
    {
      int sparse_discrete_size = 0;
      for(unsigned int x=0;x<sparse_discrete.size();++x)
        sparse_discrete_size += sparse_discrete[x].first;
      if(i < sparse_discrete_size)
        return Var(VAR_SPARSEDISCRETE, i);
      i -= sparse_discrete_size;
    }
    throw parse_exception("Var Out of Range!");
  }


  Var getNewVar(VariableType type, vector<int> bounds)
  {
    switch(type)
    {
      case VAR_BOOL:
        return getNewBoolVar();
      case VAR_BOUND:
        return getNewBoundVar(bounds[0], bounds[1]);
      case VAR_SPARSEBOUND:
        return getNewSparseBoundVar(bounds);
      case VAR_DISCRETE:
        return getNewDiscreteVar(bounds[0], bounds[1]);
      default:
        D_FATAL_ERROR("Internal error");
    }
  }

  Var getNewBoolVar()
  {
    Var newBool(VAR_BOOL, BOOLs);
    BOOLs++;
    return newBool;
  }

  Var getNewBoundVar(int lower, int upper)
  {
     bound.push_back(make_pair(1, Bounds(lower, upper)));
     return Var(VAR_BOUND, bound.size() - 1);
  }

  Var getNewSparseBoundVar(const vector<int>& vals)
  {
    sparse_bound.push_back(make_pair(1, vals));
    return Var(VAR_SPARSEBOUND, sparse_bound.size() - 1);
  }

  Var getNewDiscreteVar(int lower, int upper)
  {
    discrete.push_back(make_pair(1, Bounds(lower, upper)));
    return Var(VAR_DISCRETE, discrete.size() - 1);
  }

  vector<Var> get_all_vars() const
  {
    int total_var_count = 0;
    total_var_count += BOOLs;

    for(unsigned int x = 0; x < bound.size(); ++x)
      total_var_count += bound[x].first;
    for(unsigned int x=0;x<sparse_bound.size();++x)
      total_var_count += sparse_bound[x].first;
    for(unsigned int x=0;x<discrete.size();++x)
      total_var_count += discrete[x].first;
    for(unsigned int x=0;x<sparse_discrete.size();++x)
      total_var_count += sparse_discrete[x].first;
    vector<Var> all_vars(total_var_count);
    for(int i = 0; i < total_var_count; ++i)
      all_vars[i] = get_var('x',i);
    return all_vars;
  }
};

struct SearchOrder
{
  vector<Var> var_order;
  vector<char> val_order;
  VarOrderEnum order;
  bool find_one_assignment;

  SearchOrder() : order(ORDER_ORIGINAL), find_one_assignment(false)
  { }

  SearchOrder(const vector<Var>& _var_order) :
  var_order(_var_order), order(ORDER_ORIGINAL), find_one_assignment(false)
  { }

  SearchOrder(const vector<Var>& _var_order, VarOrderEnum _order, bool _find_one_assignment = false) :
  var_order(_var_order), order(_order), find_one_assignment(_find_one_assignment)
  { }

  void setupValueOrder()
  {
    if(val_order.empty())
      val_order.resize(var_order.size(), 'a');
  }
};

struct CSPInstance
{
  VarContainer vars;
  list<ConstraintBlob> constraints;
  shared_ptr<TupleListContainer> tupleListContainer;

  vector<SearchOrder> search_order;
  //vector<Var> var_order;
  //vector<char> val_order;
  vector<Var> permutation;
  vector<Var> sym_order;

  /// Only used for gadgets.
  vector<Var> constructionSite;

  bool is_optimisation_problem;
  bool optimise_minimising;
  Var optimise_variable;

  vector<vector<Var> > print_matrix;

  /// A complete list of variables in the order they are defined.
  vector<vector<Var> > all_vars_list;

  map<string, TupleList*> table_symboltable;
  map<TupleList*, string> table_nametable;

  /// We make these shared_ptrs so they automatically clear up after themselves.
  map<string, shared_ptr<CSPInstance> > gadgetMap;


  CSPInstance() : tupleListContainer(new TupleListContainer), is_optimisation_problem(false)
  {}

private:
    CSPInstance(const CSPInstance&);
public:

#ifdef USE_CXX0X
  CSPInstance(CSPInstance&& i) :
  CXXMOVE(vars, i), CXXMOVE(constraints, i), CXXMOVE(tupleListContainer, i), CXXMOVE(search_order, i),
  CXXMOVE(permutation, i), CXXMOVE(sym_order, i), CXXMOVE(constructionSite, i), CXXMOVE(is_optimisation_problem, i),
  CXXMOVE(optimise_minimising, i), CXXMOVE(optimise_variable, i), CXXMOVE(print_matrix, i), CXXMOVE(all_vars_list, i),
  CXXMOVE(table_symboltable, i), CXXMOVE(table_nametable, i), CXXMOVE(gadgetMap, i)
  { }

#endif

  void set_optimise(BOOL _minimising, Var var)
  {
    is_optimisation_problem = true;
    optimise_minimising = _minimising;
    optimise_variable = var;
  }

  void add_constraint(const ConstraintBlob& b)
  { constraints.push_back(b); }

  // Perform a simple check to ensure the constraint will not cause integer overflow.
  bool bounds_check_last_constraint()
  {
    const ConstraintBlob& con = constraints.back();
    switch(con.constraint->type)
    {
#ifdef CT_PRODUCT2_ABC
      case CT_PRODUCT2:
        return DOMAIN_CHECK(checked_cast<BigInt>(vars.get_bounds(con.vars[0][0]).lower_bound)*
                     checked_cast<BigInt>(vars.get_bounds(con.vars[0][0]).lower_bound))
        &&
        DOMAIN_CHECK(checked_cast<BigInt>(vars.get_bounds(con.vars[0][0]).upper_bound)*
                     checked_cast<BigInt>(vars.get_bounds(con.vars[0][0]).upper_bound));
#endif
#ifdef CT_POW_ABC
      case CT_POW:
      {
        BigInt a = checked_cast<BigInt>(vars.get_bounds(con.vars[0][0]).upper_bound);
        BigInt b = checked_cast<BigInt>(vars.get_bounds(con.vars[0][1]).upper_bound);
        BigInt out = 1;
        for(int i = 0; i < b; ++i)
        {
          out *= a;
          if(!DOMAIN_CHECK(out))
            return false;
        }
        return true;
      }
#endif

      default:
        return true;
    }
    // This should be unreachable.
    throw parse_exception("Internal Error - Constraint has not had a check implemented to ensure\n"
                          "The values given will not cause integer overflow.");

  }

  void addUnnamedTableSymbol(TupleList* tuplelist)
  {
    if(table_nametable.count(tuplelist) != 0)
      throw parse_exception("Unnamed Tuplelist double registered!");
    int pos = table_symboltable.size();
    while( table_symboltable.count("_Unnamed__" + pos) != 0)
      pos++;

    table_symboltable["_Unnamed__" + to_string(pos) + "_"] = tuplelist;
    table_nametable[tuplelist] = "_Unnamed__" + to_string(pos) + "_";
  }

  void addTableSymbol(string name, TupleList* tuplelist)
  {
    if(table_symboltable.count(name) != 0)
      throw parse_exception("Tuplename '"+name+"' already in use");
    if(table_nametable.count(tuplelist) != 0)
      throw parse_exception("Named tuplelist double registered!");
    table_symboltable[name] = tuplelist;
    table_nametable[tuplelist] = name;
  }

  TupleList* getTableSymbol(string name) const
  {
    map<string, TupleList*>::const_iterator it = table_symboltable.find(name);
    if(it == table_symboltable.end())
      throw parse_exception("Undefined tuplelist: '" + name + "'");
    return it->second;
  }

  string getTableName(TupleList* tuples) const
  {
    map<TupleList*, string>::const_iterator it = table_nametable.find(tuples);
    if(it == table_nametable.end())
      throw parse_exception("Undefined tuplelist: '" + to_string(size_t(tuples)) + "'");
    return it->second;
  }

  void addGadgetSymbol(string name, shared_ptr<CSPInstance> gadget)
  {
    if(gadgetMap.count(name) != 0)
      throw parse_exception("Gadget name "+ name + " already in use.");
    gadgetMap[name] = gadget;
  }

  shared_ptr<CSPInstance> getGadgetSymbol(string name)
  {
    map<string, shared_ptr<CSPInstance> >::iterator it = gadgetMap.find(name);
    if(it == gadgetMap.end())
      throw parse_exception("Undefined gadget name '" + name + "'");
    return it->second;
  }

  void add_variable_names()
  {
    if(vars.symbol_table.empty())
    {
      // This was a MINION 1 or MINION 2 input file. Let's fix it!
      vector<Var> all_vars = vars.get_all_vars();

      for(int i = 0; i < all_vars.size(); ++i)
        vars.addSymbol("x" + to_string(i), all_vars[i]);
    }

    if(sym_order.empty())
      sym_order = vars.get_all_vars();
  }
};

}

extern ConstraintDef constraint_list[];  // why is this here twice?
extern int num_of_constraints;

inline ConstraintDef* get_constraint(ConstraintType t)
{
  for(int i = 0; i < num_of_constraints; ++i)
  {
    if(constraint_list[i].type == t)
      return constraint_list + i;
  }

  D_FATAL_ERROR("Constraint not found");
}

using namespace ProbSpec;

#endif

