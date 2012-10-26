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

#ifdef P
#undef P
#endif

//#define P(x) cout << x << endl
#define P(x)

  struct Literal
{
  SysInt var;
  DomainInt val;
  Literal(SysInt _var, DomainInt _val) : var(_var), val(_val) { }
};

class BaseTableData
{
protected:
  TupleList* tuple_data;

public:
  DomainInt getVarCount()
    { return tuple_data->tuple_size(); }

  DomainInt getNumOfTuples()
    { return tuple_data->size(); }

  DomainInt getLiteralPos(Literal l)
    { return tuple_data->get_literal(l.var, l.val); }

  DomainInt* getPointer()
    { return tuple_data->getPointer(); }

  DomainInt getLiteralCount()
  { return tuple_data->literal_num; }

  Literal getLiteralFromPos(DomainInt pos)
  {
    pair<SysInt,DomainInt> lit = tuple_data->get_varval_from_literal(pos);
    return Literal(lit.first, lit.second);
  }

  pair<DomainInt,DomainInt> getDomainBounds(DomainInt var_in)
  {
    SysInt var = checked_cast<SysInt>(var_in);
    return make_pair(tuple_data->dom_smallest[var],
      tuple_data->dom_smallest[var] + tuple_data->dom_size[var] - 1);
  }

  BaseTableData(TupleList* _tuple_data) : tuple_data(_tuple_data) { }
};

class TableData : public BaseTableData
{
public:
   TableData(TupleList* _tuple_data) : BaseTableData(_tuple_data) { }

   // TODO : Optimise possibly?
   bool checkTuple(DomainInt* tuple, SysInt tuple_size)
   {
     D_ASSERT(tuple_size == getVarCount());
     for(SysInt i = 0; i < checked_cast<SysInt>(getNumOfTuples()); ++i)
     {
       if(std::equal(tuple, tuple + tuple_size, tuple_data->get_tupleptr(i)))
         return true;
     }
     return false;
   }
};

class TrieData : public BaseTableData
{

public:
    TupleTrieArray* tupleTrieArrayptr;

  TrieData(TupleList* _tuple_data) :
  BaseTableData(_tuple_data), tupleTrieArrayptr(_tuple_data->getTries())
  { }

  // TODO: Optimise possibly?
  bool checkTuple(DomainInt* tuple, SysInt tuple_size)
   {
     D_ASSERT(tuple_size == getVarCount());
     for(SysInt i = 0; i < getNumOfTuples(); ++i)
     {
       if(std::equal(tuple, tuple + tuple_size, tuple_data->get_tupleptr(i)))
         return true;
     }
     return false;
   }
};

class TrieState
{
  TrieData* data;
  vector<TrieObj**> trie_current_support;
  vector<DomainInt> scratch_tuple;
public:
  TrieState(TrieData* _data) : data(_data)
  {
    const SysInt litcount = checked_cast<SysInt>(data->getLiteralCount());
    trie_current_support.resize(litcount);
    for(SysInt i = 0; i < litcount; ++i)
    {
      trie_current_support[i] = new TrieObj*[litcount];
      for(SysInt j = 0; j < data->getVarCount(); ++j)
        trie_current_support[i][j] = NULL;
    }
    scratch_tuple.resize(litcount);
  }

  template<typename VarArray>
  vector<DomainInt>* findSupportingTuple(const VarArray& vars, Literal lit)
  {
    //SysInt tuple_size = data->getVarCount();
    //SysInt length = data->getNumOfTuples();
    //SysInt* tuple_data = data->getPointer();

    SysInt varIndex = lit.var;
    DomainInt val = lit.val;

    DomainInt litnum = data->getLiteralPos(lit);

    DomainInt new_support = data->tupleTrieArrayptr->getTrie(varIndex).
      nextSupportingTuple(val, vars, trie_current_support[checked_cast<SysInt>(litnum)]);

    if(new_support < 0)
      return NULL;
    else
    {
      data->tupleTrieArrayptr->getTrie(varIndex).
      reconstructTuple(&scratch_tuple.front(), trie_current_support[checked_cast<SysInt>(litnum)]);
    return &scratch_tuple;
    }
  }
};

class TableState
{
  TableData* data;

  vector<DomainInt> scratch_tuple;
  /// The constructor of TableState should set up all structures to 'sensible'
  /// default values. It should not look for actual valid supports.
public:
  TableState(TableData* _data) : data(_data)
  { scratch_tuple.resize(checked_cast<SysInt>(data->getVarCount())); }

  /// This function should return a pointer to a valid tuple, if one exists,
  /// and return NULL if none exists. The vector should be stored inside the
  /// state, and need not be thread-safe.
  template<typename VarArray>
  vector<DomainInt>* findSupportingTuple(const VarArray& vars, Literal lit)
  {
    SysInt tuple_size = data->getVarCount();
    SysInt length = data->getNumOfTuples();
    SysInt* tuple_data = data->getPointer();

    for(SysInt i = 0; i < length; ++i)
    {
      SysInt* tuple_start = tuple_data + i*tuple_size;
      bool success = true;
      if(tuple_start[checked_cast<SysInt>(lit.var)] != lit.val)
        success = false;
      for(SysInt j = 0; j < tuple_size && success; ++j)
      {
        if(!vars[j].inDomain(tuple_start[j]))
          success = false;
      }
      if(success)
      {
        std::copy(tuple_start, tuple_start + tuple_size, scratch_tuple.begin());
        return &scratch_tuple;
      }
    }
    return NULL;
  }

};



template<typename VarArray, typename TableDataType = TrieData, typename TableStateType = TrieState>
struct NewTableConstraint : public AbstractConstraint
{
  virtual string constraint_name()
    { return "table"; }

  typedef typename VarArray::value_type VarRef;
  VarArray vars;

  TableDataType* data;

  TableStateType state;
  
  TupleList* tuples;

  NewTableConstraint(StateObj* stateObj, const VarArray& _vars, TupleList* _tuples) :
  AbstractConstraint(stateObj), vars(_vars), data(new TableDataType(_tuples)), state(data), tuples(_tuples)
  {
    CheckNotBound(vars, "table constraint");
      if(_tuples->tuple_size()!=_vars.size())
      {
          cout << "Table constraint: Number of variables "
            << _vars.size() << " does not match length of tuples "
            << _tuples->tuple_size() << "." << endl;
          FAIL_EXIT();
      }
  }

  LiteralSpecificLists* lists;

  MemOffset _current_support;

  virtual SysInt dynamic_trigger_count()
    { return checked_cast<SysInt>(data->getLiteralCount() * ( vars.size() - 1)) ; }

  virtual void propagate(DynamicTrigger* propagated_trig)
  {
    PROP_INFO_ADDONE(DynGACTable);

    DynamicTrigger* dt = dynamic_trigger_start();
    SysInt trigger_pos = propagated_trig - dt;
    SysInt propagated_literal = trigger_pos / (vars.size() - 1);

    Literal lit = data->getLiteralFromPos(propagated_literal);

    P(propagated_literal << "." << vars.size() << "." << lit.var << "." << lit.val);
    if(!vars[lit.var].inDomain(lit.val))
    {
      //releaseTrigger(stateObj, propagated_trig BT_CALL_BACKTRACK);
      P("Quick return");
      return;
    }

    vector<DomainInt>* supporting_tuple = state.findSupportingTuple(vars, lit);
    if(supporting_tuple)
    {
      P("Found new support!");
      setup_watches(lit, propagated_literal, *supporting_tuple);
    }
    else
    {
      P("Failed to find new support");
      vars[lit.var].removeFromDomain(lit.val);
      //clear_watches(lit, propagated_literal);
    }
  }

  void setup_watches(Literal lit, DomainInt lit_pos, const vector<DomainInt>& support)
  {
    DynamicTrigger* dt = dynamic_trigger_start();
    D_ASSERT(data->getLiteralPos(lit) == lit_pos);
    SysInt vars_size = vars.size();
    dt += checked_cast<SysInt>(lit_pos * (vars_size - 1));
    for(SysInt v = 0; v < vars_size; ++v)
    {
      if(v != lit.var)
      {
        P(vars.size() << ".Watching " << v << "." << support[v] << " for " << lit.var << "." << lit.val);
        D_ASSERT(vars[v].inDomain(support[v]));
        PROP_INFO_ADDONE(CounterA);
        vars[v].addDynamicTrigger(dt, DomainRemoval, support[v] BT_CALL_STORE);
        ++dt;
      }
    }
  }

  void clear_watches(Literal lit, SysInt lit_pos)
  {
    DynamicTrigger* dt = dynamic_trigger_start();
    D_ASSERT(data->getLiteralPos(lit) == lit_pos);
    SysInt vars_size = vars.size();
    dt += lit_pos * (vars_size - 1);
    for(SysInt v = 0; v < vars_size; ++v)
    {
      releaseTrigger(stateObj, dt BT_CALL_BACKTRACK);
      ++dt;
    }
  }

  virtual void full_propagate()
  {
    if(vars.size() == 0)
    {
      getState(stateObj).setFailed(true);
      return;
    }

    for(UnsignedSysInt i = 0; i < vars.size(); ++i)
    {
      pair<DomainInt, DomainInt> bounds = data->getDomainBounds(i);
      vars[i].setMin(bounds.first);
      vars[i].setMax(bounds.second);

      if(getState(stateObj).isFailed()) return;

      for(DomainInt x = vars[i].getMin(); x <= vars[i].getMax(); ++x)
      {
        vector<DomainInt>* support = state.findSupportingTuple(vars, Literal(i, x));
        if(support)
        {
          setup_watches(Literal(i, x), data->getLiteralPos(Literal(i, x)), *support);
        }
        else
        {
          vars[i].removeFromDomain(x);
        }
      }
    }
  }
  
//  inline DomainInt min(SysInt x, SysInt y) {return (x<y)?x:y; }
//  inline DomainInt max(SysInt x, SysInt y) {return (x>y)?x:y; }
  
  virtual bool get_satisfying_assignment(box<pair<SysInt,DomainInt> >& assignment)
  {
      pair<DomainInt, DomainInt> bounds = data->getDomainBounds(0);
      
      for(DomainInt x = max(vars[0].getMin(), bounds.first); x <= min(vars[0].getMax(), bounds.second); ++x)
      {
          if(vars[0].inDomain(x)) {
            vector<DomainInt>* support = state.findSupportingTuple(vars, Literal(0, x));
            if(support)
            {
                for(SysInt i=0; i<vars.size(); i++) {
                    D_ASSERT(vars[i].inDomain((*support)[i]));
                    assignment.push_back(make_pair(i, (*support)[i]));
                }
                return true;
            }
          }
      }
      
      return false;
  }
  
  virtual AbstractConstraint* reverse_constraint()
  {
    return GACNegativeTableCon(stateObj, vars, tuples);
  }

  virtual BOOL check_assignment(DomainInt* v, SysInt v_size)
  {
    return data->checkTuple(v, v_size);
  }

  virtual vector<AnyVarRef> get_vars()
  {
    vector<AnyVarRef> anyvars;
    for(UnsignedSysInt i = 0; i < vars.size(); ++i)
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
  GACTableCon(StateObj* stateObj, const VarArray& vars, TupleList* tuples)
  { return new NewTableConstraint<VarArray>(stateObj, vars, tuples); }
