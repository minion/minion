#include "CSPSpec.h"

#include <sstream>

namespace ProbSpec
{
  
void print_instance(ostringstream& oss, const int& i, const CSPInstance& csp)
{ oss << i; }
  
void print_instance(ostringstream& oss, const Var& var, const CSPInstance& csp)
{ 
  if(var.type() == VAR_CONSTANT)
    print_instance(oss, var.pos(), csp);
  else
    oss << csp.vars.getName(var); 
}



template<typename T>
void print_instance(ostringstream& oss, const vector<T>& vars, const CSPInstance& csp, char start = '[', char end = ']')
{
  oss << start;
  if(!vars.empty())
  {
    print_instance(oss, vars[0], csp);
    for(int i = 1; i < vars.size(); ++i)
    {
      oss << ",";
      print_instance(oss, vars[i], csp);
    }
  }
  oss << end;
}

void print_instance(ostringstream& oss, const ConstraintBlob& blob, const CSPInstance& csp)
{
    
  if(blob.reified)
  {
    oss << "reify( ";
  }
  
  if(blob.implied_reified)
  {
    oss << "reifyimply( ";
  }
  
  oss << blob.constraint->name;
  oss << "(";
  
  int var_pos = 0;
  int const_pos = 0;
  
  for(int i = 0; i < blob.constraint->number_of_params; i++)
  {
    if(i != 0)
  	  oss << ", ";
  	  
    switch(blob.constraint->read_types[i])
  	{
  	  case read_list:
        print_instance(oss, blob.vars[var_pos++], csp);
  		break;
  	  case read_var:
        print_instance(oss, blob.vars[var_pos++][0], csp);
  		break;
  	  case read_2_vars:
  	  {
        print_instance(oss, blob.vars[var_pos][0], csp);
        oss << ",";
        print_instance(oss, blob.vars[var_pos++][1], csp);
  	  }
  		break;
  	  case read_constant:
        print_instance(oss, blob.constants[const_pos++][0], csp);
  		break;
  	  case read_constant_list:
        print_instance(oss, blob.constants[const_pos++], csp);
  		break;  
      case read_tuples:
        oss << csp.getTableName(blob.tuples);
      break;
  	  default:
      oss << "???";
//  	    D_FATAL_ERROR("Internal Error!");
  	}
  }
  
  oss << ")";
  
  if(blob.reified || blob.implied_reified)
  {
    oss << ", ";
    print_instance(oss, blob.reify_var, csp);
    oss << " )";
  }
  oss << endl;  
}
  
void print_instance(ostringstream& oss, const VarContainer& vars, const CSPInstance& csp)
{ 
  for(int i = 0; i < vars.BOOLs; ++i)
  {  
    oss << "BOOL ";
    print_instance(oss, Var(VAR_BOOL, i), csp);
    oss << endl;
  }
    
  // Bounds.
  int bound_sum = 0;
  for(int x = 0; x < vars.bound.size(); ++x)
  {
    for(int i = 0; i < vars.bound[x].first; ++i)
    {
      oss << "BOUND ";
      print_instance(oss, Var(VAR_BOUND, i + bound_sum), csp);
      oss << "{" << vars.bound[x].second.lower_bound << ".." << vars.bound[x].second.upper_bound << "}" << endl;
    }
    bound_sum += vars.bound[x].first;
  }
  
  // Sparse Bounds.
  
  int sparse_bound_sum = 0;
  for(int x = 0; x < vars.sparse_bound.size(); ++x)
  {
    for(int i = 0; i < vars.sparse_bound[x].first; ++i)
    {
      oss << "SPARSEBOUND "; 
      print_instance(oss, Var(VAR_BOUND, i + sparse_bound_sum), csp);
      oss << " ";
      print_instance(oss, vars.sparse_bound[x].second, csp, '{', '}');
      oss << endl;
    }
    sparse_bound_sum += vars.sparse_bound[x].first;
  }
  
  // Bounds.
  int discrete_sum = 0;
  for(int x = 0; x < vars.discrete.size(); ++x)
  {
    for(int i = 0; i < vars.discrete[x].first; ++i)
    {
      oss << "DISCRETE ";
      print_instance(oss, Var(VAR_DISCRETE, i + discrete_sum), csp);
      oss << "{" << vars.discrete[x].second.lower_bound << ".." << vars.discrete[x].second.upper_bound << "}" << endl;
    }
    discrete_sum += vars.discrete[x].first;
  }
  
}

void print_tuples(ostringstream& oss, CSPInstance& csp)
{
  typedef map<string, TupleList*>::const_iterator it_type;
  
  for(it_type it = csp.table_symboltable.begin(); it != csp.table_symboltable.end(); ++it)
  {
    oss << it->first << " ";
    int tuple_size = it->second->tuple_size();
    int num_tuples = it->second->size();
    int* tuple_ptr = it->second->getPointer();
    oss << num_tuples << " " << tuple_size << endl;
    for(int i = 0; i < num_tuples; ++i)
    {
      for(int j = 0; j < tuple_size; ++j)
        oss << *(tuple_ptr + (i * tuple_size) + j) << " ";
      oss << endl;
    }
    oss << endl;
  }
   
  
}

void print_search_info(ostringstream& oss, CSPInstance& csp)
{
  if(csp.is_optimisation_problem)
  {
    if(csp.optimise_minimising)
      oss << "MINIMISING ";
    else
      oss << "MAXIMISING ";
    print_instance(oss, csp.optimise_variable, csp);
    oss << endl; 
  } 
  
  if(!csp.var_order.empty())
  {
    oss << "VARORDER ";
    print_instance(oss, csp.var_order, csp);
    oss << endl;
  }
  
  if(!csp.val_order.empty())
  {
    oss << "VALORDER ";
    print_instance(oss, csp.val_order, csp);
    oss << endl;
  }
  
  if(!csp.permutation.empty())
  {
    oss << "PERMUTATION ";
    print_instance(oss, csp.permutation, csp);
    oss << endl;
  }
  
}
  
void print_instance(ostringstream& oss, CSPInstance& csp)
{   
    oss << "MINION 3" << endl;
  if(csp.vars.symbol_table.empty())
  {
    oss << "# This instance was format MINION 1 or 2, so filling in variable names" << endl;
    // This was a MINION 1 or MINION 2 input file. Let's fix it!
    vector<Var> all_vars = csp.vars.get_all_vars();
    
    for(int i = 0; i < all_vars.size(); ++i)
    {
      csp.vars.addSymbol("x" + to_string(i), all_vars[i]);  
    }  
    
  }
  

  oss << "**VARIABLES**" << endl;
  print_instance(oss, csp.vars, csp);
  
  oss << "**TUPLELIST**" << endl;
  print_tuples(oss, csp);
  
  oss << "**CONSTRAINTS**" << endl;
  for(list<ConstraintBlob>::const_iterator it = csp.constraints.begin(); 
      it != csp.constraints.end(); ++it)
  {
    print_instance(oss, *it, csp);
  }
  oss << "**EOF**" << endl;
}

}