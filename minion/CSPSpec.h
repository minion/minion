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
using namespace std;

/// The currently accepted types of Constraints.
enum ConstraintType {
  CT_ALLDIFF ,
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
  VAR_DISCRETE,
  VAR_LONG_DISCRETE,
  VAR_SPARSEDISCRETE,
  VAR_CONSTANT,
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
  
  operator string()
  {
   ostringstream s;
   s << "Var. Type:" << type << " Pos:" << pos << ".";
   return s.str();
   }
   
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
  
  BOOL is_dynamic()
  { return constraint.trig_type == DYNAMIC_CT; }
  
};


/// Contains all the variables in a CSP instance.
  struct VarContainer
{
  int total_var_count;

  int BOOLs;
  vector<pair<int, Bounds> > bound;
  vector<pair<int, vector<int> > > sparse_bound;
  vector<pair<int, Bounds> > discrete;
  vector<pair<int, vector<int> > > sparse_discrete;
  VarContainer()
  {}
  
  
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
	
	// Need to handle both discrete and long_discrete variables!
    {
      int short_discrete_size = 0;
	  int long_discrete_size = 0;
      for(unsigned int x=0;x<discrete.size();++x)
	  {
		int var_length = discrete[x].first;
		
		if(rangevar_container.valid_range(discrete[x].second.lower_bound, discrete[x].second.upper_bound))
		{
		  short_discrete_size += var_length;
		  if(short_discrete_size + long_discrete_size > i)
		    return Var(VAR_DISCRETE, i - long_discrete_size);
		}
		else
		{
		  long_discrete_size += var_length;
		  if(short_discrete_size + long_discrete_size > i)
		    return Var(VAR_LONG_DISCRETE, i - short_discrete_size);
		}
	  }
      i -= (short_discrete_size + long_discrete_size);
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
	vector<Var> all_vars(total_var_count);
	for(int i = 0; i < total_var_count; ++i)
	  all_vars[i] = get_var('x',i);
	return all_vars;
  }
};


  struct CSPInstance
{
  VarContainer vars;
  list<ConstraintBlob> constraints;
  vector<Var> var_order;
  vector<char> val_order;
  
  BOOL is_optimisation_problem;
  BOOL optimise_minimising;
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
  //VarArrayBlob make_var_blob(vector<int>&);
};

}

#endif

