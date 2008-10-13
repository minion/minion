#include "CSPSpec.h"

#include <sstream>

namespace ProbSpec
{
  
struct MinionInstancePrinter
{
  ostringstream oss;
  CSPInstance& csp;
  
  MinionInstancePrinter(CSPInstance& _csp) : csp(_csp)
    { print_instance(); }
    
  string getInstance()
  { return oss.str(); }
  
  void print_instance(const int& i)
  { oss << i; }
  
  void print_instance(const string& s)
  { oss << s; }

  void print_instance(const Var& var)
  { 
    if(var.type() == VAR_CONSTANT)
      print_instance( var.pos());
    else
      oss << csp.vars.getName(var); 
  }

template<typename T>
void print_instance( const vector<T>& vars, char start = '[', char end = ']')
{
  oss << start;
  if(!vars.empty())
  {
    print_instance( vars[0]);
    for(int i = 1; i < vars.size(); ++i)
    {
      oss << ",";
      print_instance( vars[i]);
    }
  }
  oss << end;
}

void print_instance(const ConstraintBlob& blob)
{  
  oss << blob.constraint->name;
  oss << "(";
  
  int var_pos = 0;
  int const_pos = 0;
  int constraint_child_pos = 0;
  
  for(int i = 0; i < blob.constraint->number_of_params; i++)
  {
    if(i != 0)
  	  oss << ", ";
  	  
    switch(blob.constraint->read_types[i])
  	{
  	  case read_list:
        print_instance( blob.vars[var_pos++]);
  		break;
  	  case read_var:
      case read_bool_var:
        print_instance( blob.vars[var_pos++][0]);
  		break;
  	  case read_2_vars:
  	  {
        print_instance( blob.vars[var_pos][0]);
        oss << ",";
        print_instance( blob.vars[var_pos++][1]);
  	  }
  		break;
  	  case read_constant:
        print_instance( blob.constants[const_pos++][0]);
  		break;
  	  case read_constant_list:
        print_instance( blob.constants[const_pos++]);
  		break;  
      case read_tuples:
        oss << csp.getTableName(blob.tuples);
      break;
      case read_constraint:
        print_instance(blob.internal_constraints[constraint_child_pos]);
        constraint_child_pos++;
      break;
      case read_constraint_list:
        oss << "{";
        print_instance(blob.internal_constraints[0]);
        for(int j = 1; j < blob.internal_constraints.size(); ++j)
        {
          oss << ", ";
          print_instance(blob.internal_constraints[j]); 
        }
        oss << "}";
        break;
  	  default:
      oss << "???";
//  	    D_FATAL_ERROR("Internal Error!");
  	}
  }
  
  oss << ")";
  oss << endl;  
}
  
void print_instance(const VarContainer& vars)
{ 
  for(int i = 0; i < vars.BOOLs; ++i)
  {  
    oss << "BOOL ";
    print_instance( Var(VAR_BOOL, i));
    oss << endl;
  }
    
  // Bounds.
  int bound_sum = 0;
  for(int x = 0; x < vars.bound.size(); ++x)
  {
    for(int i = 0; i < vars.bound[x].first; ++i)
    {
      oss << "BOUND ";
      print_instance( Var(VAR_BOUND, i + bound_sum));
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
      print_instance( Var(VAR_BOUND, i + sparse_bound_sum));
      oss << " ";
      print_instance( vars.sparse_bound[x].second, '{', '}');
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
      print_instance( Var(VAR_DISCRETE, i + discrete_sum));
      oss << "{" << vars.discrete[x].second.lower_bound << ".." << vars.discrete[x].second.upper_bound << "}" << endl;
    }
    discrete_sum += vars.discrete[x].first;
  }
  
}

void print_tuples( )
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

void print_search_info( )
{
  if(csp.is_optimisation_problem)
  {
    if(csp.optimise_minimising)
      oss << "MINIMISING ";
    else
      oss << "MAXIMISING ";
    print_instance( csp.optimise_variable);
    oss << endl; 
  } 
  
  for(int i = 0; i < csp.search_order.size(); ++i)
  {
    if(!csp.search_order[i].var_order.empty())
    {
      oss << "VARORDER ";
      print_instance( csp.search_order[i].var_order);
      oss << endl;
    }
  
    if(!csp.search_order[i].val_order.empty())
    {
      oss << "VALORDER ";
      vector<string> output_vars;
      for(int j = 0; j < csp.search_order[i].val_order.size(); ++j)
        output_vars.push_back(csp.search_order[i].val_order[j] ? "a" : "d");
      print_instance( output_vars);
      oss << endl;
    }
  }
  if(!csp.permutation.empty())
  {
    oss << "PERMUTATION ";
    print_instance( csp.permutation);
    oss << endl;
  }
  
}

void fix_variables()
{
  if(csp.vars.symbol_table.empty())
  {
    oss << "# This instance was format MINION 1 or 2, so filling in variable names" << endl;
    // This was a MINION 1 or MINION 2 input file. Let's fix it!
    vector<Var> all_vars = csp.vars.get_all_vars();
    
    for(int i = 0; i < all_vars.size(); ++i)
      csp.vars.addSymbol("x" + to_string(i), all_vars[i]);  
  }
}

void print_instance()
{ print_instance(csp.vars); }

void print_instance(const vector<Var>& vars_in)
{
  oss << "MINION 3" << endl;
  
  fix_variables();
    
  oss << "**VARIABLES**" << endl;
  print_instance(vars_in);
  
  oss << "**SEARCH**" << endl;
  print_search_info();
  
  oss << "**TUPLELIST**" << endl;
  print_tuples();
  
  oss << "**CONSTRAINTS**" << endl;
  for(list<ConstraintBlob>::const_iterator it = csp.constraints.begin(); 
      it != csp.constraints.end(); ++it)
  {
    print_instance( *it);
  }
  oss << "**EOF**" << endl;
}

};

}
