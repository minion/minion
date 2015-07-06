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

#ifndef CONSTRAINT_GACTABLE_TRIES_H
#define CONSTRAINT_GACTABLE_TRIES_H

#include "tries.h"
#include "constraint_checkassign.h"

template<typename VarArray, SysInt negative>
struct GACTableConstraint : public AbstractConstraint
{
  virtual string extended_name()
  { return "table(trie)"; }

  virtual string constraint_name()
  { if(negative) return "negativetable"; else return "table"; }

   virtual AbstractConstraint* reverse_constraint()
  {
      return forward_check_negation(this);
  }

  CONSTRAINT_ARG_LIST2(vars, tuples);

  typedef typename VarArray::value_type VarRef;
  VarArray vars;

  TupleTrieArray* tupleTrieArrayptr;

  //Following is setup globally in constraint to be passed by reference & recycled
  DomainInt* recyclableTuple;

  /// For each literal, the number of the tuple that supports it.
  //   renamed off from current_support in case both run in parallel
  vector<TrieObj**> trie_current_support;

  /// Check if all allowed values in a given tuple are still in the domains of the variables.
  BOOL check_tuple(const vector<DomainInt>& v)
  {
    for(UnsignedSysInt i = 0; i < v.size(); ++i)
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
    CheckNotBound(vars, "table constraints","");
    tupleTrieArrayptr = tuples->getTries();
    const SysInt arity = checked_cast<SysInt>(tuples->tuple_size());
    D_ASSERT((SysInt)_vars.size() == arity);
    const SysInt litnum = checked_cast<SysInt>(tuples->literal_num);
    trie_current_support.resize(litnum);
    for(SysInt i = 0; i < litnum; ++i)
    {
      trie_current_support[i] = new TrieObj*[arity];
      for(SysInt j = 0; j < arity; j++)
        trie_current_support[i][j] = NULL;
    }
    // initialise supportting tuple for recycle
    recyclableTuple = new DomainInt[arity] ;
  }

  ~GACTableConstraint()
  {
    delete[] recyclableTuple;
    for(SysInt i = 0; i < (SysInt)trie_current_support.size(); ++i)
      delete[] trie_current_support[i];
  }

  SysInt dynamic_trigger_count()
  { return checked_cast<SysInt>(tuples->literal_num * ( (SysInt)vars.size() - 1)) ; }

  BOOL find_new_support(DomainInt literal)
  {
     const SysInt sysLiteral = checked_cast<SysInt>(literal);
     pair<DomainInt,DomainInt> varval = tuples->get_varval_from_literal(literal);
     DomainInt varIndex = varval.first;
     DomainInt val = varval.second;
     if(negative==0)
     {
         DomainInt new_support =
           tupleTrieArrayptr->getTrie(varIndex).
                                nextSupportingTuple(val, vars, trie_current_support[sysLiteral]);
         if (new_support < 0)
         { // cout << "find_new_support failed literal: " << literal << " var: " << varIndex << " val: " << get_val_from_literal(literal) << endl ;
             return false;
         }
     }
     else
     {
         DomainInt new_support =
           tupleTrieArrayptr->getTrie(varIndex).
                                nextSupportingTupleNegative(val, vars, trie_current_support[sysLiteral], recyclableTuple);
         if (new_support < 0)
         { // cout << "find_new_support failed literal: " << literal << " var: " << varIndex << " val: " << get_val_from_literal(literal) << endl ;
             return false;
         }
     }
         // cout << "find_new_support sup= "<< new_support << " literal: " << literal << " var: " << varIndex << " val: " << get_val_from_literal(literal) << endl;
         //trie_current_support[literal] = new_support;
     return true;
  }

  virtual void propagate(DynamicTrigger* propagated_trig)
  {
    PROP_INFO_ADDONE(DynGACTable);
    DynamicTrigger* dt = dynamic_trigger_start();
    SysInt trigger_pos = propagated_trig - dt;
    SysInt propagated_literal = trigger_pos / ( (SysInt)vars.size() - 1);

    BOOL is_new_support = find_new_support(propagated_literal);

    pair<DomainInt,DomainInt> varval = tuples->get_varval_from_literal(propagated_literal);
    DomainInt varIndex = varval.first;
    DomainInt val = varval.second;

    if(is_new_support)
    {
      setup_watches(varIndex, propagated_literal);
    }
    else
    {
      vars[checked_cast<SysInt>(varIndex)].removeFromDomain(val);
    }
  }

  void setup_watches(DomainInt var, DomainInt lit)
  {
    // cout << "setup_watches lit= "<< lit << endl ; cout << "calling reconstructTuple from setup_watches" << endl ;
    if(negative==0)
    {
        tupleTrieArrayptr->getTrie(var).reconstructTuple(recyclableTuple,trie_current_support[checked_cast<SysInt>(lit)]);
    }
    // otherwise, the support is already in recyclableTuple.

    // cout << "  " << var << ", literal" << lit << ":";
    // for(SysInt z = 0; z < (SysInt)vars.size(); ++z) cout << recyclableTuple[z] << " "; cout << endl;

    DynamicTrigger* dt = dynamic_trigger_start();

    const SysInt vars_size = vars.size();
    dt += checked_cast<SysInt>(lit * (vars_size - 1));
    for(SysInt v = 0; v < vars_size; ++v)
    {
      if(v != var)
      {
        D_ASSERT(vars[v].inDomain(recyclableTuple[v]));
        moveTrigger(vars[v], dt, DomainRemoval, recyclableTuple[v]);
        ++dt;
      }
    }
  }

  virtual void full_propagate()
  {
      if(negative==0 && tuples->size()==0)
      {   // it seems to work without this explicit check, but I put it in anyway.
          getState().setFailed(true);
          return;
      }
      for(SysInt varIndex = 0; varIndex < (SysInt)vars.size(); ++varIndex)
      {
        if(negative==0)
        {
            vars[varIndex].setMin((tuples->dom_smallest)[varIndex]);
            vars[varIndex].setMax((tuples->dom_smallest)[varIndex] + (tuples->dom_size)[varIndex]);
        }

        if(getState().isFailed()) return;

        DomainInt max = vars[varIndex].getMax();
        for(DomainInt i = vars[varIndex].getMin(); i <= max; ++i)
        {
            if(i>= (tuples->dom_smallest)[varIndex]
                && i<(tuples->dom_smallest)[varIndex] + (tuples->dom_size)[varIndex])
            {
                const SysInt literal = checked_cast<SysInt>(tuples->get_literal(varIndex, i));

                DomainInt sup;
                if(negative==0)
                {
                    sup = tupleTrieArrayptr->getTrie(varIndex).
                        nextSupportingTuple(i, vars, trie_current_support[literal]);
                }
                else
                {
                    sup = tupleTrieArrayptr->getTrie(varIndex).
                        nextSupportingTupleNegative(i, vars, trie_current_support[literal], recyclableTuple);
                }

                //trie_current_support[literal] = sup;
                // cout << "    var " << varIndex << " val: " << i << " sup " << sup << " " << endl;
                if(sup < 0)
                {
                  //cout <<"No valid support for " + tostring(i) + " in var " + tostring(varIndex) << endl;
                  //volatile SysInt * myptr=NULL;
                  //SysInt crashit=*(myptr);
                  vars[varIndex].removeFromDomain(i);
                }
                else
                {
                  setup_watches(varIndex, literal);
                }
            }
            else
            {
                D_ASSERT(negative==1);
                // else: if the literal is not contained in any forbidden tuple, then it is
                // not necessary to find a support for it or set watches. The else case
                // only occurs with negative tuple constraints.
            }
        }
      }
      // cout << endl; cout << "  fp: finished finding supports: " << endl ;
  }

    virtual bool get_satisfying_assignment(box<pair<SysInt,DomainInt> >& assignment)
  {
      D_ASSERT(!getState().isFailed());
      DomainInt max = vars[0].getMax();
      for(DomainInt i = vars[0].getMin(); i <= max; ++i)
      {
          if(vars[0].inDomain(i))
          {
              DomainInt sup=-1;
              DomainInt literal=0xdeadbeef;

              if(i>= (tuples->dom_smallest)[0]
                 && i<(tuples->dom_smallest)[0] + (tuples->dom_size)[0])
              {
                  literal = tuples->get_literal(0, i);
                  if(negative) {
                      sup = tupleTrieArrayptr->getTrie(0).
                                nextSupportingTupleNegative(i, vars, trie_current_support[checked_cast<SysInt>(literal)], recyclableTuple);
                  }
                  else
                  {
                      sup = tupleTrieArrayptr->getTrie(0).
                                nextSupportingTuple(i, vars, trie_current_support[checked_cast<SysInt>(literal)]);
                  }
              }
              else
              {
                  // If the value i is in domain but outside all tuples passed in,
                  // and the constraint is negated, then all tuples containing i
                  // are valid. Just make something up.
                  if(negative) {
                      assignment.push_back(make_pair(0,i));
                      for(SysInt varidx = 1; varidx < (SysInt)vars.size(); ++varidx) {
                          assignment.push_back(make_pair(varidx, vars[varidx].getMin()));
                      }
                      return true;
                  }
              }

              if(sup>=0) {
                  if(!negative) tupleTrieArrayptr->getTrie(0).reconstructTuple(recyclableTuple,trie_current_support[checked_cast<SysInt>(literal)]);
                  //recyclableTuple[0]=i;
                  for(SysInt varidx=0; varidx<(SysInt)vars.size(); varidx++) {
                      D_ASSERT(recyclableTuple[0]==i);
                      D_ASSERT(vars[varidx].inDomain(recyclableTuple[varidx]));
                      assignment.push_back(make_pair(varidx, recyclableTuple[varidx]));
                  }
                  return true;
              }
          }
      }

      return false;
  }

  virtual BOOL check_assignment(DomainInt* v, SysInt v_size)
  {
    if(negative==0)
    {
        for(SysInt i = 0; i < tuples->size(); ++i)
        {
          if( std::equal(v, v + checked_cast<SysInt>(v_size), (*tuples)[i]) )
            return true;
        }
        return false;
    }
    else
    {
        for(SysInt i = 0; i < tuples->size(); ++i)
        {
          if( std::equal(v, v + checked_cast<SysInt>(v_size), (*tuples)[i]) )
            return false;
        }
        return true;
    }
  }

  virtual vector<AnyVarRef> get_vars()
  {
    vector<AnyVarRef> anyvars;
    for(UnsignedSysInt i = 0; i < vars.size(); ++i)
      anyvars.push_back(vars[i]);
    return anyvars;
  }


};


//template<typename VarArray>
//AbstractConstraint*
//GACTableCon(const VarArray& vars, TupleList* tuples)
//{ return new GACTableConstraint<VarArray, 0>(vars, tuples); }

template<typename VarArray>
AbstractConstraint*
GACNegativeTableCon(const VarArray& vars, TupleList* tuples)
{ return new GACTableConstraint<VarArray, 1>(vars, tuples); }

#endif
