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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

/** @help constraints;table Description
An extensional constraint that enforces GAC. The constraint is
specified via a list of tuples.

The variables used in the constraint have to be BOOL or DISCRETE variables.
Other types are not supported.
*/

/** @help constraints;table Example
To specify a constraint over 3 variables that allows assignments
(0,0,0), (1,0,0), (0,1,0) or (0,0,1) do the following.

1) Add a tuplelist to the **TUPLELIST** section, e.g.:

**TUPLELIST**
myext 4 3
0 0 0
1 0 0
0 1 0
0 0 1

N.B. the number 4 is the number of tuples in the constraint, the
number 3 is the -arity.

2) Add a table constraint to the **CONSTRAINTS** section, e.g.:

**CONSTRAINTS**
table(myvec, myext)

and now the variables of myvec will satisfy the constraint myext.
*/

/** @help constraints;table Example
The constraints extension can also be specified in the constraint
definition, e.g.:

table(myvec, {<0,0,0>,<1,0,0>,<0,1,0>,<0,0,1>})
*/

/** @help constraints;table References
help input tuplelist
help input gacschema
help input negativetable
help input haggisgac
*/

/** @help constraints;negativetable Description
An extensional constraint that enforces GAC. The constraint is
specified via a list of disallowed tuples.
*/

/** @help constraints;negativetable Notes
See entry

   help input negativetable

for how to specify a table constraint in minion input. The only
difference for negativetable is that the specified tuples are
disallowed.
*/

/** @help constraints;negativetable References
help input table
help input tuplelist
*/

#ifndef CONSTRAINT_GACTABLE_TRIES_H
#define CONSTRAINT_GACTABLE_TRIES_H

#include "constraint_checkassign.h"
#include "tries.h"

template <typename VarArray, SysInt negative>
struct GACTableConstraint : public AbstractConstraint {
  virtual string extendedName() {
    return "table(trie)";
  }

  virtual string constraintName() {
    if(negative)
      return "negativetable";
    else
      return "table";
  }

  virtual AbstractConstraint* reverseConstraint() {
    return forwardCheckNegation(this);
  }

  CONSTRAINT_ARG_LIST2(vars, tuples);

  typedef typename VarArray::value_type VarRef;
  VarArray vars;

  TupleTrieArray* tupleTrieArrayptr;

  // Following is setup globally in constraint to be passed by reference &
  // recycled
  DomainInt* recyclableTuple;

  /// For each literal, the number of the tuple that supports it.
  //   renamed off from currentSupport in case both run in parallel
  vector<TrieObj**> trieCurrentSupport;

  /// Check if all allowed values in a given tuple are still in the domains of
  /// the variables.
  BOOL check_tuple(const vector<DomainInt>& v) {
    for(UnsignedSysInt i = 0; i < v.size(); ++i) {
      if(!vars[i].inDomain(v[i]))
        return false;
    }
    return true;
  }

  TupleList* tuples;

  GACTableConstraint(const VarArray& _vars, TupleList* _tuples) : vars(_vars), tuples(_tuples) {
    CheckNotBound(vars, "table constraints", "");
    tupleTrieArrayptr = tuples->getTries();
    const SysInt arity = checked_cast<SysInt>(tuples->tupleSize());
    D_ASSERT((SysInt)_vars.size() == arity);
    const SysInt litnum = checked_cast<SysInt>(tuples->literalNum);
    trieCurrentSupport.resize(litnum);
    for(SysInt i = 0; i < litnum; ++i) {
      trieCurrentSupport[i] = new TrieObj*[arity];
      for(SysInt j = 0; j < arity; j++)
        trieCurrentSupport[i][j] = NULL;
    }
    // initialise supportting tuple for recycle
    recyclableTuple = new DomainInt[arity];
  }

  ~GACTableConstraint() {
    delete[] recyclableTuple;
    for(SysInt i = 0; i < (SysInt)trieCurrentSupport.size(); ++i)
      delete[] trieCurrentSupport[i];
  }

  SysInt dynamicTriggerCount() {
    return checked_cast<SysInt>(tuples->literalNum * ((SysInt)vars.size() - 1));
  }

  BOOL findNewSupport(DomainInt literal) {
    const SysInt sysLiteral = checked_cast<SysInt>(literal);
    pair<DomainInt, DomainInt> varval = tuples->get_varval_from_literal(literal);
    DomainInt varIndex = varval.first;
    DomainInt val = varval.second;
    if(negative == 0) {
      DomainInt newSupport = tupleTrieArrayptr->getTrie(varIndex).nextSupportingTuple(
          val, vars, trieCurrentSupport[sysLiteral]);
      if(newSupport < 0) { // cout << "findNewSupport failed literal: " <<
                            // literal << " var: " << varIndex << " val: " <<
                            // getVal_from_literal(literal) << endl ;
        return false;
      }
    } else {
      DomainInt newSupport = tupleTrieArrayptr->getTrie(varIndex).nextSupportingTupleNegative(
          val, vars, trieCurrentSupport[sysLiteral], recyclableTuple);
      if(newSupport < 0) { // cout << "findNewSupport failed literal: " <<
                            // literal << " var: " << varIndex << " val: " <<
                            // getVal_from_literal(literal) << endl ;
        return false;
      }
    }
    // cout << "findNewSupport sup= "<< newSupport << " literal: " << literal
    // << " var: " << varIndex << " val: " << getVal_from_literal(literal) <<
    // endl;
    // trieCurrentSupport[literal] = newSupport;
    return true;
  }

  virtual void propagateDynInt(SysInt trigger_pos, DomainDelta) {
    PROP_INFO_ADDONE(DynGACTable);
    SysInt propagated_literal = trigger_pos / ((SysInt)vars.size() - 1);

    BOOL isNewSupport = findNewSupport(propagated_literal);

    pair<DomainInt, DomainInt> varval = tuples->get_varval_from_literal(propagated_literal);
    DomainInt varIndex = varval.first;
    DomainInt val = varval.second;

    if(isNewSupport) {
      setup_watches(varIndex, propagated_literal);
    } else {
      vars[checked_cast<SysInt>(varIndex)].removeFromDomain(val);
    }
  }

  void setup_watches(DomainInt var, DomainInt lit) {
    // cout << "setup_watches lit= "<< lit << endl ; cout << "calling
    // reconstructTuple from setup_watches" << endl ;
    if(negative == 0) {
      tupleTrieArrayptr->getTrie(var).reconstructTuple(
          recyclableTuple, trieCurrentSupport[checked_cast<SysInt>(lit)]);
    }
    // otherwise, the support is already in recyclableTuple.

    // cout << "  " << var << ", literal" << lit << ":";
    // for(SysInt z = 0; z < (SysInt)vars.size(); ++z) cout <<
    // recyclableTuple[z] << " "; cout << endl;

    const SysInt varsSize = vars.size();
    SysInt dt = checked_cast<SysInt>(lit * (varsSize - 1));
    for(SysInt v = 0; v < varsSize; ++v) {
      if(v != var) {
        D_ASSERT(vars[v].inDomain(recyclableTuple[v]));
        moveTriggerInt(vars[v], dt, DomainRemoval, recyclableTuple[v]);
        ++dt;
      }
    }
  }

  virtual void fullPropagate() {
    if(negative == 0 && tuples->size() == 0) { // it seems to work without this
                                               // explicit check, but I put it
                                               // in anyway.
      getState().setFailed(true);
      return;
    }
    for(SysInt varIndex = 0; varIndex < (SysInt)vars.size(); ++varIndex) {
      if(negative == 0) {
        vars[varIndex].setMin((tuples->domSmallest)[varIndex]);
        vars[varIndex].setMax((tuples->domSmallest)[varIndex] + (tuples->domSize)[varIndex]);
      }

      if(getState().isFailed())
        return;

      DomainInt max = vars[varIndex].max();
      for(DomainInt i = vars[varIndex].min(); i <= max; ++i) {
        if(i >= (tuples->domSmallest)[varIndex] &&
           i < (tuples->domSmallest)[varIndex] + (tuples->domSize)[varIndex]) {
          const SysInt literal = checked_cast<SysInt>(tuples->getLiteral(varIndex, i));

          DomainInt sup;
          if(negative == 0) {
            sup = tupleTrieArrayptr->getTrie(varIndex).nextSupportingTuple(
                i, vars, trieCurrentSupport[literal]);
          } else {
            sup = tupleTrieArrayptr->getTrie(varIndex).nextSupportingTupleNegative(
                i, vars, trieCurrentSupport[literal], recyclableTuple);
          }

          // trieCurrentSupport[literal] = sup;
          // cout << "    var " << varIndex << " val: " << i << " sup " << sup
          // << " " << endl;
          if(sup < 0) {
            // cout <<"No valid support for " + tostring(i) + " in var " +
            // tostring(varIndex) << endl;
            // volatile SysInt * myptr=NULL;
            // SysInt crashit=*(myptr);
            vars[varIndex].removeFromDomain(i);
          } else {
            setup_watches(varIndex, literal);
          }
        } else {
          D_ASSERT(negative == 1);
          // else: if the literal is not contained in any forbidden tuple, then
          // it is
          // not necessary to find a support for it or set watches. The else
          // case
          // only occurs with negative tuple constraints.
        }
      }
    }
    // cout << endl; cout << "  fp: finished finding supports: " << endl ;
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    D_ASSERT(!getState().isFailed());
    DomainInt max = vars[0].max();
    for(DomainInt i = vars[0].min(); i <= max; ++i) {
      if(vars[0].inDomain(i)) {
        DomainInt sup = -1;
        DomainInt literal = 0xdeadbeef;

        if(i >= (tuples->domSmallest)[0] &&
           i < (tuples->domSmallest)[0] + (tuples->domSize)[0]) {
          literal = tuples->getLiteral(0, i);
          if(negative) {
            sup = tupleTrieArrayptr->getTrie(0).nextSupportingTupleNegative(
                i, vars, trieCurrentSupport[checked_cast<SysInt>(literal)], recyclableTuple);
          } else {
            sup = tupleTrieArrayptr->getTrie(0).nextSupportingTuple(
                i, vars, trieCurrentSupport[checked_cast<SysInt>(literal)]);
          }
        } else {
          // If the value i is in domain but outside all tuples passed in,
          // and the constraint is negated, then all tuples containing i
          // are valid. Just make something up.
          if(negative) {
            assignment.push_back(make_pair(0, i));
            for(SysInt varidx = 1; varidx < (SysInt)vars.size(); ++varidx) {
              assignment.push_back(make_pair(varidx, vars[varidx].min()));
            }
            return true;
          }
        }

        if(sup >= 0) {
          if(!negative)
            tupleTrieArrayptr->getTrie(0).reconstructTuple(
                recyclableTuple, trieCurrentSupport[checked_cast<SysInt>(literal)]);
          // recyclableTuple[0]=i;
          for(SysInt varidx = 0; varidx < (SysInt)vars.size(); varidx++) {
            D_ASSERT(recyclableTuple[0] == i);
            D_ASSERT(vars[varidx].inDomain(recyclableTuple[varidx]));
            assignment.push_back(make_pair(varidx, recyclableTuple[varidx]));
          }
          return true;
        }
      }
    }

    return false;
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    if(negative == 0) {
      for(SysInt i = 0; i < tuples->size(); ++i) {
        if(std::equal(v, v + checked_cast<SysInt>(vSize), (*tuples)[i]))
          return true;
      }
      return false;
    } else {
      for(SysInt i = 0; i < tuples->size(); ++i) {
        if(std::equal(v, v + checked_cast<SysInt>(vSize), (*tuples)[i]))
          return false;
      }
      return true;
    }
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> anyvars;
    for(UnsignedSysInt i = 0; i < vars.size(); ++i)
      anyvars.push_back(vars[i]);
    return anyvars;
  }
};

// template<typename VarArray>
// AbstractConstraint*
// GACTableCon(const VarArray& vars, TupleList* tuples)
//{ return new GACTableConstraint<VarArray, 0>(vars, tuples); }

template <typename VarArray>
AbstractConstraint* GACNegativeTableCon(const VarArray& vars, TupleList* tuples) {
  return new GACTableConstraint<VarArray, 1>(vars, tuples);
}

#endif
