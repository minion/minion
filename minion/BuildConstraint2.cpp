/*
 *  BuildConstraint2.cpp
 *  cutecsp
 *
 *  Created by Chris Jefferson on 17/03/2006.
 *  Copyright 2006 __MyCompanyName__. All rights reserved.
 *
 */

#define NO_MAIN

#include "minion.h"
#include "CSPSpec.h"

using namespace ProbSpec;

namespace BuildCon
{  

/// Helper function used in a few places.
AnyVarRef
get_AnyVarRef_from_Var(Var v)
{
  switch(v.type)
		{
		  case VAR_BOOL:
			return AnyVarRef(boolean_container.get_var_num(v.pos));
		  case VAR_NOTBOOL:
		    return AnyVarRef(VarNotRef(boolean_container.get_var_num(v.pos)));
		  case VAR_BOUND:
			return AnyVarRef(boundvar_container.get_var_num(v.pos));
		  case VAR_SPARSEBOUND:
			return AnyVarRef(sparse_boundvar_container.get_var_num(v.pos));
		  case VAR_DISCRETE:
			return AnyVarRef(rangevar_container.get_var_num(v.pos));
		  case VAR_LONG_DISCRETE:
			return AnyVarRef(big_rangevar_container.get_var_num(v.pos));
		  case VAR_SPARSEDISCRETE:	
			D_FATAL_ERROR("Sparse Discrete not supported at present");
		  case VAR_CONSTANT:
			return AnyVarRef(ConstantVar(v.pos));
		  default:
		    D_FATAL_ERROR("Unknown Error.");
		}
}

    /// Build the variable and value ordering used.
	/// The var order is placed, the val order is returned.
	pair<vector<AnyVarRef>, vector<bool> > build_val_and_var_order(ProbSpec::CSPInstance& instance)
  {
	  vector<bool> final_val_order;
	  vector<AnyVarRef> final_var_order;
	  if(instance.var_order.size() != instance.val_order.size())
	  {
	    D_FATAL_ERROR("Variable order and value order must be same size.");
	  }
	  if(!instance.var_order.empty())
	  {
		for(unsigned int i = 0 ;i < instance.var_order.size(); ++i)
		{
		  final_val_order.push_back(instance.val_order[i]);
		  final_var_order.push_back(get_AnyVarRef_from_Var(instance.var_order[i]));
		}
	  }
	  else
	  {
	    int var_count = 0;
		var_count += instance.vars.bools;
		for(unsigned i = 0; i < instance.vars.bound.size(); ++i)
		  var_count += instance.vars.bound[i].first;
		for(unsigned i = 0; i < instance.vars.sparse_bound.size(); ++i)
		  var_count += instance.vars.sparse_bound[i].first;
		for(unsigned i = 0; i < instance.vars.discrete.size(); ++i)
		  var_count += instance.vars.discrete[i].first;
		for(unsigned i = 0; i < instance.vars.sparse_discrete.size(); ++i)
		  var_count += instance.vars.sparse_discrete[i].first;
		
		final_var_order.reserve(var_count);
		final_val_order.reserve(var_count);
		for(int i = 0; i < var_count; ++i)
		{
		  final_var_order.push_back(get_AnyVarRef_from_Var(instance.vars.get_var('x',i)));
		  final_val_order.push_back(true);
		}
	  }
	  return make_pair(final_var_order, final_val_order);
  }	

  /// Create all the variables used in the CSP.
  void build_variables(const ProbSpec::VarContainer& vars)
  {
	for(int i = 0; i < vars.bools; ++i)
	{
	  BoolVarRef b = boolean_container.get_new_var();
	}
	for(unsigned int i = 0; i < vars.bound.size(); ++i)
	{
	  for(int j = 0; j < vars.bound[i].first; ++j)
	  {
		BoundVarRef b = boundvar_container.get_new_var(vars.bound[i].second.lower_bound,
													   vars.bound[i].second.upper_bound);
	  }
	}
	for(unsigned int i = 0; i < vars.sparse_bound.size(); ++i)
	{
	  for(int j = 0; j < vars.sparse_bound[i].first; ++j)
	  {
		SparseBoundVarRef b = 
		sparse_boundvar_container.get_new_var(vars.sparse_bound[i].second);
	  }
	}
	
	for(unsigned int i = 0; i < vars.discrete.size(); ++i)
	{
	  for(int j = 0; j < vars.discrete[i].first; ++j)
	  {
	    if(rangevar_container.valid_range(vars.discrete[i].second.lower_bound, vars.discrete[i].second.upper_bound))
	    LRangeVarRef r = rangevar_container.get_new_var(vars.discrete[i].second.lower_bound,
														vars.discrete[i].second.upper_bound);
		else
		BigRangeVarRef r = big_rangevar_container.get_new_var(vars.discrete[i].second.lower_bound,
															  vars.discrete[i].second.upper_bound);
	  }
    }
	
	for(unsigned int i = 0; i < vars.sparse_discrete.size(); ++i)
	{ D_FATAL_ERROR("Sparse discrete disabled at present due to bugs. Sorry."); }
  }
	
}


