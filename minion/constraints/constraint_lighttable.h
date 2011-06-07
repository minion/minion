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

#include "tries.h"

/** @help constraints;lighttable Description
An extensional constraint that enforces GAC. The constraint is
specified via a list of tuples. lighttable is a variant of the 
table constraint that is stateless and potentially faster
for small constraints.

For full documentation, see the help for the table constraint.
*/

#ifdef P
#undef P
#endif

//#define P(x) cout << x << endl
#define P(x)

struct Literal
{
  int var;
  DomainInt val;
  Literal(int _var, DomainInt _val) : var(_var), val(_val) { }
};


class BaseTableData
{
protected:
  TupleList* tuple_data;

public:
  int getVarCount()
    { return tuple_data->tuple_size(); }

  int getNumOfTuples()
    { return tuple_data->size(); }

  int getLiteralPos(Literal l)
    { return tuple_data->get_literal(l.var, l.val); }

  int* getPointer()
    { return tuple_data->getPointer(); }

  int getLiteralCount()
  { return tuple_data->literal_num; }

  Literal getLiteralFromPos(int pos)
  {
    pair<int,int> lit = tuple_data->get_varval_from_literal(pos);
    return Literal(lit.first, lit.second);
  }

  pair<DomainInt,DomainInt> getDomainBounds(int var)
  {
    return make_pair(tuple_data->dom_smallest[var],
      tuple_data->dom_smallest[var] + tuple_data->dom_size[var]);
  }

  BaseTableData(TupleList* _tuple_data) : tuple_data(_tuple_data) { }
};

class TrieData : public BaseTableData
{

public:
    TupleTrieArray* tupleTrieArrayptr;

  TrieData(TupleList* _tuple_data) :
  BaseTableData(_tuple_data), tupleTrieArrayptr(_tuple_data->getTries())
  { }

  // TODO: Optimise possibly?
  bool checkTuple(DomainInt* tuple, int tuple_size)
   {
     D_ASSERT(tuple_size == getVarCount());
     for(int i = 0; i < getNumOfTuples(); ++i)
     {
       if(std::equal(tuple, tuple + tuple_size, tuple_data->get_tupleptr(i)))
         return true;
     }
     return false;
   }
};


// Altered from NewTableConstraint in file new_table.h

template<typename VarArray, typename TableDataType = TrieData>
struct LightTableConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "LightTable"; }

  typedef typename VarArray::value_type VarRef;
  VarArray vars;

  TableDataType* data;   // Assuming this is a TrieData for the time being.
  // Can this be the thing instead of a *??
  
  LightTableConstraint(StateObj* stateObj, const VarArray& _vars, TupleList* _tuples) :
  AbstractConstraint(stateObj), vars(_vars), data(new TableDataType(_tuples))
  {
      CheckNotBound(vars, "table constraints","");
      if(_tuples->tuple_size()!=_vars.size())
      {
          cout << "Table constraint: Number of variables "
            << _vars.size() << " does not match length of tuples "
            << _tuples->tuple_size() << "." << endl;
          FAIL_EXIT();
      }
  }
  
  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    for(int i=0; i<vars.size(); i++)
    {
        t.push_back(make_trigger(vars[i], Trigger(this, i), DomainChanged));
    }
    return t;
  }
  
  virtual void propagate(int changed_var, DomainDelta)
  {
      // Propagate to all vars except the one that changed.
      for(int i=0; i<vars.size(); i++)
      {
          if(i!=changed_var)
          {
              propagate_var(i);
          }
      }
  }
  
  void propagate_var(int varidx)
  {
      VarRef var=vars[varidx];
      
      for(DomainInt val=var.getMin(); val<=var.getMax(); val++)
      {
          if(var.inDomain(val))
          {
              // find the right trie first.
              
              TupleTrie& trie =data->tupleTrieArrayptr->getTrie(varidx);
              
              bool support=trie.search_trie_nostate(val, vars);
              
              if(!support)
              {
                  var.removeFromDomain(val);
              }
          }
      }
  }
  
  virtual void full_propagate()
  {
      for(int i=0; i<vars.size(); i++)
      {
          propagate_var(i);
      }
  }
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    return data->checkTuple(v, v_size);
  }

  virtual vector<AnyVarRef> get_vars()
  {
    vector<AnyVarRef> anyvars;
    for(unsigned i = 0; i < vars.size(); ++i)
      anyvars.push_back(vars[i]);
    return anyvars;
  }

};

inline TupleTrieArray* TupleList::getTries()
{
  if(triearray == NULL)
    triearray = new TupleTrieArray(this);
  return triearray;
}

template<typename VarArray>
AbstractConstraint*
  GACLightTableCon(StateObj* stateObj, const VarArray& vars, TupleList* tuples)
  { return new LightTableConstraint<VarArray>(stateObj, vars, tuples); }
