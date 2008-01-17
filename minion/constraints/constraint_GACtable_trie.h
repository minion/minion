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

#include "tries.h"

template<typename VarArray>
struct GACTableConstraint : public DynamicConstraint
{
  virtual string constraint_name()
  { return "TableTrie"; }
  
  typedef typename VarArray::value_type VarRef;
  VarArray vars;
  
  TupleTrieArray* tupleTrieArrayptr;
  
  //Following is setup globally in constraint to be passed by reference & recycled
  int* recyclableTuple;
  
  /// For each literal, the number of the tuple that supports it.
  //   renamed off from current_support in case both run in parallel
  vector<TrieObj**> trie_current_support;
  
  /// Check if all allowed values in a given tuple are still in the domains of the variables.
  BOOL check_tuple(const vector<int>& v)
  {
	for(unsigned i = 0; i < v.size(); ++i)
	{
	  if(!vars[i].inDomain(v[i]))
		return false;
	}
	return true;
  }
    
  TupleList* tuples;
  
  GACTableConstraint(const VarArray& _vars, TupleList* _tuples) :
	vars(_vars), tuples(_tuples)
  { 
    tupleTrieArrayptr = tuples->getTries();
	int arity = tuples->tuple_size();	  
	D_ASSERT(_vars.size() == arity);
	  
	trie_current_support.resize(tuples->literal_num); 
	for(int i = 0; i < tuples->literal_num; ++i)
	{
	  trie_current_support[i] = new TrieObj*[arity];
	  for(int j = 0; j < arity; j++)
		trie_current_support[i][j] = NULL;
	}
	// initialise supportting tuple for recycle
	recyclableTuple = new int[arity] ;
  }
  
  int dynamic_trigger_count()
  { return tuples->literal_num * ( vars.size() - 1) ; }
  
  BOOL find_new_support(int literal)
  {
     pair<int,int> varval = tuples->get_varval_from_literal(literal);
	 int varIndex = varval.first;
	 int val = varval.second;
         int new_support = 
           tupleTrieArrayptr->getTrie(varIndex).
                                nextSupportingTuple(val, vars, trie_current_support[literal]);
         if (new_support < 0)
         { // cout << "find_new_support failed literal: " << literal << " var: " << varIndex << " val: " << get_val_from_literal(literal) << endl ;
             return false;
         }
         // cout << "find_new_support sup= "<< new_support << " literal: " << literal << " var: " << varIndex << " val: " << get_val_from_literal(literal) << endl;
         //trie_current_support[literal] = new_support; 
         return true;
  }
  
  DYNAMIC_PROPAGATE_FUNCTION(DynamicTrigger* propogated_trig)
  {
	PROP_INFO_ADDONE(DynGACTable);
	D_INFO(1, DI_TABLECON, "Propogation Triggered: " + to_string(propogated_trig));
	DynamicTrigger* dt = dynamic_trigger_start();
	int trigger_pos = propogated_trig - dt;
	int propogated_literal = trigger_pos / (vars.size() - 1);
	
	BOOL is_new_support = find_new_support(propogated_literal);

    pair<int,int> varval = tuples->get_varval_from_literal(propogated_literal);
	int varIndex = varval.first;
	int val = varval.second;
	
	if(is_new_support)
	{
	  D_INFO(1, DI_TABLECON, "Found new support!");
	  setup_watches(varIndex, propogated_literal);
	}
	else
	{
	  D_INFO(1, DI_TABLECON, "Failed to find new support");
	  vars[varIndex].removeFromDomain(val);
	}
  }
  
  void setup_watches(int var, int lit)
  {
    // cout << "setup_watches lit= "<< lit << endl ; cout << "calling reconstructTuple from setup_watches" << endl ; 
	tupleTrieArrayptr->getTrie(var).reconstructTuple(recyclableTuple,trie_current_support[lit]);
    // cout << "  " << var << ", literal" << lit << ":";
    // for(int z = 0; z < vars.size(); ++z) cout << recyclableTuple[z] << " "; cout << endl;
    
	DynamicTrigger* dt = dynamic_trigger_start();
	
	int vars_size = vars.size();
	dt += lit * (vars_size - 1);
	for(int v = 0; v < vars_size; ++v)
	{
	  if(v != var)
	  {
		D_ASSERT(vars[v].inDomain(recyclableTuple[v]));
		vars[v].addDynamicTrigger(dt, DomainRemoval, recyclableTuple[v]);
		++dt;
	  }
	}
  }
  
  virtual void full_propogate()
  {
      D_INFO(2, DI_TABLECON, "Full prop");
      if(tuples->size()==0)
      {   // it seems to work without this explicit check, but I put it in anyway.
          Controller::fail();
          return;
      }
      for(int varIndex = 0; varIndex < vars.size(); ++varIndex) 
      {
	    vars[varIndex].setMin((tuples->dom_smallest)[varIndex]);
	    vars[varIndex].setMax((tuples->dom_smallest)[varIndex] + (tuples->dom_size)[varIndex]);
		
		if(Controller::failed) return;
		
        DomainInt max = vars[varIndex].getMax();
        for(DomainInt i = vars[varIndex].getMin(); i <= max; ++i) 
        { 
		    int literal = tuples->get_literal(varIndex, i);
            int sup = tupleTrieArrayptr->getTrie(varIndex).       
                            nextSupportingTuple(i, vars, trie_current_support[literal]);
            //trie_current_support[literal] = sup;
            // cout << "    var " << varIndex << " val: " << i << " sup " << sup << " " << endl;
            if(sup < 0)
            {
			  D_INFO(2, DI_TABLECON, "No valid support for " + to_string(i) + " in var " + to_string(varIndex));
			  vars[varIndex].removeFromDomain(i);
            }
            else
            {
			  setup_watches(varIndex, literal);
            }
        }
      }
      // cout << endl; cout << "  fp: finished finding supports: " << endl ;
  }
  
  virtual BOOL check_assignment(vector<DomainInt> v)
  {
    for(unsigned i = 0; i < tuples->size(); ++i)
	{
	  if( std::equal(v.begin(), v.end(), (*tuples)[i]) )
	    return true;
	}
	return false;
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> anyvars;
    for(unsigned i = 0; i < vars.size(); ++i)
	  anyvars.push_back(vars[i]);
	return anyvars;
  }
  
  
};


template<typename VarArray>
DynamicConstraint*
GACTableCon(const VarArray& vars, TupleList* tuples)
{ return new GACTableConstraint<VarArray>(vars, tuples); }

inline TupleTrieArray* TupleList::getTries()
{
  if(triearray == NULL)
    triearray = new TupleTrieArray(this);
  return triearray;
}

