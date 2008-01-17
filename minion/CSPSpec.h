/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id$
*/

/* Minion
* Copyright (C) 2006
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
#include "tuple_container.h"

using namespace std;

/// The currently accepted types of Constraints.
enum ConstraintType {
  CT_ALLDIFF ,
  CT_ALLDIFF_GACSLOW ,
  CT_DISEQ ,
  CT_INEQ ,
  CT_MAX ,
  CT_MIN ,
  CT_POW ,
  CT_LEQSUM ,
  CT_GEQSUM ,
  CT_PRODUCT2 ,
  CT_WEIGHTLEQSUM,
  CT_WEIGHTGEQSUM,
  CT_ELEMENT ,
  CT_GACELEMENT,
  CT_WATCHED_ELEMENT,
  CT_LEXLEQ ,
  CT_LEXLESS ,
  CT_OCCURRENCE ,
  CT_EQ,
  CT_WATCHED_TABLE,
  CT_WATCHED_LEQSUM,
  CT_WATCHED_GEQSUM,
  CT_WATCHED_VECNEQ,
  CT_WATCHED_LITSUM, 
  CT_MINUSEQ,
  CT_DIV,
  CT_MODULO,
  CT_REIFY,
  CT_REIFYIMPLY
};

enum ReadTypes
{
  read_list,
  read_var,
  read_2_vars,
  read_constant,
  read_constant_list,
  read_tuples,
  read_nothing
};

enum ConstraintTriggerType
{
  DYNAMIC_CT,
  STATIC_CT
};

struct ConstraintDef
{
  std::string name;
  ConstraintType type;
  int number_of_params;
  array<ReadTypes,4> read_types;
  ConstraintTriggerType trig_type;
};

extern ConstraintDef constraint_list[];



/// The currently accepted types of Variables.
enum VariableType
{
  VAR_BOOL,
  VAR_NOTBOOL,
  VAR_BOUND,
  VAR_SPARSEBOUND,
  VAR_DISCRETE_BASE,
  VAR_DISCRETE_SHORT,
  VAR_DISCRETE_LONG,
  VAR_SPARSEDISCRETE,
  VAR_CONSTANT,
  VAR_MATRIX,
  VAR_INVALID = -999
};

namespace ProbSpec
{
  
  
  /// A simple wrapper for a pair of bounds.
  struct Bounds
{
  int lower_bound;
  int upper_bound;
  Bounds(int _lower, int _upper) : lower_bound(_lower), upper_bound(_upper)
  { }
};

struct Var
{
  VariableType type;
  int pos;
  Var(VariableType _type, int _pos) : type(_type), pos(_pos)
  { }
  
  Var(const Var& v) : type(v.type), pos(v.pos)
  {}
  
  Var() : type(VAR_INVALID), pos(-1)
  {}
  
  friend std::ostream& operator<<(std::ostream& o, const Var& v)
  { return o << "Var. Type:" << v.type << " Pos:" << v.pos << "."; }
   
   BOOL operator==(const Var& var)
   { return type == var.type && pos == var.pos; }
};

 
/// Constructed by the parser. Suitable for holding any kind of constraint.
struct ConstraintBlob
{
  /// The type of constraint.
  ConstraintDef constraint;
  /// The variables of the problem.
  vector<vector<Var> > vars;
  ///Pointer to list of tuples. Only used in Table Constraints.
  TupleList* tuples;
  BOOL reified;
  BOOL implied_reified;
  Var reify_var;
  
  ConstraintBlob(ConstraintDef _con) :
	constraint(_con), reified(false), implied_reified(false)
  {}
  
  ConstraintBlob(ConstraintDef _con, const vector<vector<Var> >& _vars) : constraint(_con), vars(_vars), reified(false), implied_reified(false)
  {}
  
  /// A helper constructor for when only a SingleVar is passed.
  ConstraintBlob(ConstraintDef _con, vector<Var>& _var) : constraint(_con), reified(false), implied_reified(false)
  { vars.push_back(_var); }

  void reify(Var _reify_var)
  {
    reified = true;
	reify_var = _reify_var;
  }
  
  void reifyimply(Var _reify_var)
  {
    implied_reified = true;
	reify_var = _reify_var;
  }
  
  bool is_dynamic()
  { return constraint.trig_type == DYNAMIC_CT; }
  
};


/// Contains all the variables in a CSP instance.
  struct VarContainer
{
  int BOOLs;
  map<string, Var> symbol_table;
  vector<pair<int, Bounds> > bound;
  vector<pair<int, vector<int> > > sparse_bound;
  vector<pair<int, Bounds> > discrete;
  vector<pair<int, vector<int> > > sparse_discrete;
  map<string, vector<int> > matrix_table;
  VarContainer() : BOOLs(0)
  {}

  vector<Var> buildVarList(const string& name, vector<int> params)
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
      return_list.push_back(getSymbol(name + to_string(output)));
    }
    while(increment_vector(current_index, modified_max));
          
    return return_list;
  }
  
  void addSymbol(const string& name, Var variable)
  {
    if(name[0] >= 0 && name[0] <= 9)
      throw parse_exception("Names cannot start with a number!:" + name);
    if(symbol_table.count(name) != 0)
      throw parse_exception("Name already in table:" + name);
    
    symbol_table[name] = variable;
  
  }
  
  void addMatrixSymbol(const string& name, const vector<int>& indices)
  {
    Var var(VAR_MATRIX, matrix_table.size());
    addSymbol(name, var);
    matrix_table[name] = indices;
  }
  
  Var getSymbol(const string& name)
  {
    map<string, Var>::iterator it = symbol_table.find(name);
    if(it == symbol_table.end())
      throw parse_exception("Undefined name: '" + name + "'");
    return it->second;
  }
  
  vector<int> getMatrixSymbol(const string& name)
  {
    map<string, vector<int> >::iterator it = matrix_table.find(name);
    if(it == matrix_table.end())
      throw parse_exception("Undefined matrix: '" + name + "'");
    return it->second;
  }
  
  Bounds get_bounds(Var v)
  {
    switch(v.type)
    {
    case VAR_CONSTANT:
      return Bounds(v.pos, v.pos);    
    case VAR_BOOL:
      return Bounds(0,1);
    case VAR_BOUND:  
      {
        int bound_size = 0;
        for(unsigned int x = 0; x < bound.size(); ++x)
        {
          bound_size += bound[x].first;
          if(v.pos < bound_size)
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
          if(v.pos < sparse_bound_size)
            return Bounds(sparse_bound[x].second.front(), sparse_bound[x].second.back());
        }
        throw parse_exception("Internal Error - SparseBound OverFlow");
      }
    case VAR_DISCRETE_BASE: 
    {
      int discrete_size = 0;
      for(unsigned int x = 0; x < discrete.size(); ++x)
      {
        discrete_size += discrete[x].first;
        if(v.pos < discrete_size)
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
        if(v.pos < sparse_discrete_size)
          return Bounds(sparse_discrete[x].second.front(), sparse_discrete[x].second.back());
      }
      throw parse_exception("Internal Error - SparseDiscrete OverFlow");
    }
    }
      throw parse_exception("Internal Error - Unknown Variable Type");
  }
  
  Var get_var(char, int i)
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
		return Var(VAR_DISCRETE_BASE, i);
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
  
  vector<Var> get_all_vars()
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
      case VAR_DISCRETE_BASE:
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
     bound.push_back(make_pair(1, ProbSpec::Bounds(lower, upper)));
     return Var(VAR_BOUND, bound.size() - 1);
  }
  
  Var getNewSparseBoundVar(const vector<int>& vals)
  {
    sparse_bound.push_back(make_pair(1, vals));
    return Var(VAR_SPARSEBOUND, sparse_bound.size() - 1);
  }
  
  Var getNewDiscreteVar(int lower, int upper)
  {
    discrete.push_back(make_pair(1, ProbSpec::Bounds(lower, upper)));
    return Var(VAR_DISCRETE_BASE, discrete.size() - 1);
  }
    
};


  struct CSPInstance
{
  VarContainer vars;
  list<ConstraintBlob> constraints;
  vector<Var> var_order;
  vector<char> val_order;
  bool is_optimisation_problem;
  bool optimise_minimising;
  Var optimise_variable;
  
  vector<vector<Var> > print_matrix;
  
  CSPInstance() : is_optimisation_problem(false)
  {}
  
  void set_optimise(BOOL _minimising, Var var)
  { 
    is_optimisation_problem = true;
    optimise_minimising = _minimising;
    optimise_variable = var;
  }
  
  void add_constraint(const ConstraintBlob& b)
  { constraints.push_back(b); }
  
  void last_constraint_reify(Var reifyVar)
  { constraints.back().reify(reifyVar); }
  
  void last_constraint_reifyimply(Var reifyVar)
  { constraints.back().reifyimply(reifyVar); }
  
  template<typename Map>
  void VarReplace(Var& v, Map& new_map)
  {
	if(v.type == VAR_DISCRETE_BASE)
	{
	  D_ASSERT(new_map.count(v.pos) == 1);
	  v = new_map[v.pos]; 
	}
  }
  
  template<typename Element, typename Map>
  void VarReplace(std::vector<Element>& v, Map& new_map)
  {
    for(int i = 0; i < v.size(); ++i)
	  VarReplace(v[i], new_map);
  }
  
  template<typename SmallBoundCheck>
	void fixDiscrete(SmallBoundCheck check)
  {
	  // Right, this is horrible, but necessary. We need to:
	  // a) Look for VAR_DISCRETEs whose bounds don't satisfy check
	  //    and turn them into VAR_DISCRETE_LONG
	  // b) Go through and fix all the constraints to satisfy the
	  //    new numbering order.
	  
	  // We'll do that in two steps:
	  // 1) Go through variables, and build a map between old and new vars
	  // 2) Go through all the occurrences of variables (including in constraints)
	  //    and change them.
	  
	  // Will map the int part of VAR_DISCRETE vars to whole new vars
	  std::map<int, Var> new_map;
	  
	  vector<pair<int, Bounds> >& discrete = vars.discrete;
	  int short_discrete_size = 0;
	  int long_discrete_size = 0;
	  for(unsigned int x = 0; x < discrete.size(); ++x)
	  {
		int var_length = discrete[x].first;
		if(check(discrete[x].second.lower_bound, discrete
				 [x].second.upper_bound))
		{
		  for(int i = 0; i < var_length; i++)
		  {
			new_map[short_discrete_size + long_discrete_size + i] = 
			Var(VAR_DISCRETE_SHORT, short_discrete_size + i);
		  }
		  short_discrete_size += var_length;
		}
		else
		{
		  for(int i = 0; i < var_length; i++)
		  {
			new_map[short_discrete_size + long_discrete_size + i] = 
			Var(VAR_DISCRETE_LONG, long_discrete_size + i);
		  }
		  long_discrete_size += var_length;
		}
	  }
	  
	  VarReplace(var_order, new_map);
	  VarReplace(print_matrix, new_map);
	  for(list<ConstraintBlob>::iterator it = constraints.begin(); 
		  it != constraints.end(); ++it)
		VarReplace(it->vars, new_map);
  }
  
  // Perform a simple check to ensure the constraint will not cause integer overflow.
  bool bounds_check_last_constraint()
  {
    const ConstraintBlob& con = constraints.back();
    switch(con.constraint.type)
    {
      case CT_REIFY:
      case CT_REIFYIMPLY:
        throw parse_exception("Internal Error - Invalid Constraint in bounds_check_last_constraint.");
      case CT_PRODUCT2:
        return DOMAIN_CHECK(checked_cast<BigInt>(vars.get_bounds(con.vars[0][0]).lower_bound)*
                     checked_cast<BigInt>(vars.get_bounds(con.vars[0][0]).lower_bound))
        &&
        DOMAIN_CHECK(checked_cast<BigInt>(vars.get_bounds(con.vars[0][0]).upper_bound)*
                     checked_cast<BigInt>(vars.get_bounds(con.vars[0][0]).upper_bound));
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
      
        // XXX : Todo : Check these constraints!
      case CT_WEIGHTLEQSUM:
      case CT_WEIGHTGEQSUM:
      case CT_LEQSUM:
      case CT_GEQSUM:
      case CT_WATCHED_GEQSUM:
      case CT_WATCHED_LEQSUM:
      
      default:
        return true;
    }
    // This should be unreachable.
    throw parse_exception("Internal Error - Constraint has not had a check implemented to ensure\n"
                          "The values given will not cause integer overflow.");

  }
  
  map<string, TupleList*> table_symboltable;

  void addTableSymbol(string name, TupleList* tuplelist)
  {
    if(table_symboltable.count(name) != 0)
      throw parse_exception("Tuplename '"+name+"' already in use");
    table_symboltable[name] = tuplelist;
  }
  
  TupleList* getTableSymbol(string name)
  {
    map<string, TupleList*>::iterator it = table_symboltable.find(name);
    if(it == table_symboltable.end())
      throw parse_exception("Undefined tuplelist: '" + name + "'");
    return it->second;
  }
};

}

#endif

