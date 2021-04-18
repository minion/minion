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





/** @help constraints;watchelement Description
The constraint

   watchelement(vec, i, e)

specifies that, in any solution, vec[i] = e and i is in the range
[0 .. |vec|-1].
*/











// The triggers in this constraint are set up as follows:
// If the length of the vector is L.

// The first 2 * Dom(Result) literals are, for some j
//   literal 2 * i : attached to assignment i to V[j]
//   literal 2 * i + 1 : attached to the assignment j in IndexVar

// After this there are 2 * Dom(Index) literals are, for some j
// literal 2 * i : attached to j in V[i]
// literal 2 * i + 1 : attached to j in Result

#ifndef CONSTRAINT_DYNAMIC_ELEMENT_H
#define CONSTRAINT_DYNAMIC_ELEMENT_H

// for the reverse constraint.
#include "constraint_equal.h"
#include "dynamic_new_and.h"
#include "dynamic_new_or.h"
#include "unary/dynamic_literal.h"
#include "unary/dynamic_notinrange.h"

template <typename VarArray, typename Index, typename Result, bool undefMapsZero = false>
struct ElementConstraintDynamic : public AbstractConstraint {

  virtual string constraintName() {
    return "watchelement";
  }

  //  typedef BoolLessSumConstraintDynamic<VarArray, VarSum,1-VarToCount>
  //  NegConstraintType;
  typedef typename VarArray::value_type VarRef;

  // ReversibleInt count;
  VarArray varArray;
  Index indexvar;
  Result resultvar;

  virtual string fullOutputName() {
    string undef_name = "";
    if(undefMapsZero)
      undef_name = "_undefzero";

    vector<Mapper> v = indexvar.getMapperStack();
    if(!v.empty() && v.back() == Mapper(MAP_SHIFT, -1)) {
      return ConOutput::printCon("watchelement_one" + undef_name, varArray,
                                  indexvar.popOneMapper(), resultvar);
    } else {
      return ConOutput::printCon("watchelement" + undef_name, varArray, indexvar, resultvar);
    }
  }

  DomainInt initialResultDomMin;
  DomainInt initialResultDomMax;

  vector<DomainInt> currentSupport;

  ElementConstraintDynamic(const VarArray& _varArray, const Index& _indexvar, const Result& _result)
      : varArray(_varArray), indexvar(_indexvar), resultvar(_result) {
    CheckNotBound(varArray, "watchelement", "element");
    CheckNotBoundSingle(indexvar, "watchelement", "element");
    CheckNotBoundSingle(resultvar, "watchelement", "element");
    initialResultDomMin = std::min<DomainInt>(0, resultvar.initialMin());
    initialResultDomMax = std::max<DomainInt>(0, resultvar.initialMax());
  }

  virtual SysInt dynamicTriggerCount() {
    SysInt count = varArray.size() * 2 +
                   checked_cast<SysInt>(initialResultDomMax - initialResultDomMin + 1) * 2 +
                   1 + 1;
    if(undefMapsZero)
      count++;
    currentSupport.resize(count / 2); // is SysInt the right type?
    return count;
  }

  void findNewSupportForResult(SysInt j) {
    DomainInt realj = j + initialResultDomMin;

    if(!resultvar.inDomain(realj))
      return;

    if(undefMapsZero && realj == 0) {
      if(indexvar.min() < 0) {
        moveTriggerInt(indexvar, 2 * j, DomainRemoval, indexvar.min());
        releaseTriggerInt(2 * j + 1);
        return;
      }

      if(indexvar.max() >= (SysInt)varArray.size()) {
        moveTriggerInt(indexvar, 2 * j, DomainRemoval, indexvar.max());
        releaseTriggerInt(2 * j + 1);
        return;
      }
    }

    SysInt arraySize = varArray.size();

    DomainInt indexvarMin = indexvar.min();
    DomainInt indexvarMax = indexvar.max();

    if(undefMapsZero) {
      indexvarMin = std::max<DomainInt>(indexvarMin, 0);
      indexvarMax = std::min<DomainInt>(indexvarMax, (DomainInt)varArray.size() - 1);
    }

    D_ASSERT(indexvarMin >= 0);
    D_ASSERT(indexvarMax < (DomainInt)varArray.size());

    // support is value of index
    DomainInt oldsupport = max(currentSupport[j + arraySize],
                               indexvarMin); // old support probably just removed
    DomainInt maxsupport = indexvarMax;

    DomainInt support = oldsupport;

    while(support <= maxsupport &&
          !(indexvar.inDomain(support) && varArray[checked_cast<SysInt>(support)].inDomain(realj)))
      ++support;
    if(support > maxsupport) {
      support = indexvarMin;
      DomainInt maxCheck = min(oldsupport, maxsupport + 1);
      while(support < maxCheck && !(indexvar.inDomain(support) &&
                                     varArray[checked_cast<SysInt>(support)].inDomain(realj)))
        ++support;
      if(support >= maxCheck) {
        resultvar.removeFromDomain(realj);
        return;
      }
    }
    moveTriggerInt(varArray[checked_cast<SysInt>(support)], 2 * j, DomainRemoval, realj);
    moveTriggerInt(indexvar, 2 * j + 1, DomainRemoval, support);
    currentSupport[j + arraySize] = support;
  }

  void check_outOfBoundsIndex() {
    if(!undefMapsZero || !resultvar.inDomain(0)) {
      indexvar.setMin(0);
      indexvar.setMax((DomainInt)varArray.size() - 1);
    }
  }

  void findNewSupportForIndex(SysInt i) {
    if(!indexvar.inDomain(i))
      return;

    DomainInt resultvarmin = resultvar.min();
    DomainInt resultvarmax = resultvar.max();
    DomainInt trigPos = (initialResultDomMax - initialResultDomMin + 1) * 2;

    if(resultvarmin == resultvarmax) {
      if(!varArray[i].inDomain(resultvarmin))
        indexvar.removeFromDomain(i);
      else {
        moveTriggerInt(varArray[i], trigPos + 2 * i, DomainRemoval, resultvarmin);
        moveTriggerInt(resultvar, trigPos + 2 * i + 1, DomainRemoval, resultvarmin);
        currentSupport[i] = resultvarmin;
      }
      return;
    }

    // support is value of result
    DomainInt oldsupport =
        max(currentSupport[i], resultvarmin); // old support probably just removed
    DomainInt maxsupport = resultvarmax;
    DomainInt support = oldsupport;

    // SysInt support = initialResultDomMin;
    while(support <= maxsupport &&
          !(resultvar.inDomain_noBoundCheck(support) && varArray[i].inDomain(support)))
      ++support;

    if(support > maxsupport) {
      support = resultvarmin;
      DomainInt maxCheck = min(oldsupport, maxsupport + 1);
      while(support < maxCheck &&
            !(resultvar.inDomain_noBoundCheck(support) && varArray[i].inDomain(support)))
        ++support;
      if(support >= maxCheck) {
        indexvar.removeFromDomain(i);
        return;
      }
    }

    moveTriggerInt(varArray[i], trigPos + 2 * i, DomainRemoval, support);
    moveTriggerInt(resultvar, trigPos + 2 * i + 1, DomainRemoval, support);
    currentSupport[i] = support;
  }

  void dealWithAssignedIndex() {
    D_ASSERT(indexvar.isAssigned());
    SysInt indexval = checked_cast<SysInt>(indexvar.assignedValue());
    if(undefMapsZero) {
      if(indexval < 0 || indexval >= (SysInt)varArray.size()) {
        resultvar.assign(0);
        return;
      }
    }

    VarRef& var = varArray[indexval];

    DomainInt lower = resultvar.min();
    if(lower > var.min()) {
      var.setMin(lower);
      ++lower; // do not need to check lower bound, we know it's in resultvar
    }

    DomainInt upper = resultvar.max();
    if(upper < var.max()) {
      var.setMax(upper);
      --upper; // do not need to check upper bound, we know it's in resultvar
    }

    for(DomainInt i = lower; i <= upper; ++i) {
      if(!(resultvar.inDomain(i)))
        var.removeFromDomain(i);
    }
  }

  virtual void fullPropagate() {
    for(SysInt i = 0; i < (SysInt)varArray.size(); i++) {
      if(varArray[i].isBound() && !varArray[i].isAssigned()) { // isassigned excludes constants.
        cerr << "Warning: watchelement is not designed to be used on bound "
                "variables and may cause crashes."
             << endl;
      }
    }
    if((indexvar.isBound() && !indexvar.isAssigned()) ||
       (resultvar.isBound() && !resultvar.isAssigned())) {
      cerr << "Warning: watchelement is not designed to be used on bound "
              "variables and may cause crashes."
           << endl;
    }

    SysInt arraySize = varArray.size();
    DomainInt resultDomSize = initialResultDomMax - initialResultDomMin + 1;

    // Setup SupportLostForIndexValue(i,j)
    // Here we are supporting values in the index variable
    // So for each variable in the index variable, we want to ensure

    // Couple of quick sanity-propagations.
    // We define UNDEF = false ;)

    check_outOfBoundsIndex();

    if(getState().isFailed())
      return;

    for(SysInt i = 0; i < arraySize; ++i) {
      currentSupport[i] = initialResultDomMin - 1; // will be incremented if support sought
      if(indexvar.inDomain(i))
        findNewSupportForIndex(i);
    }

    for(SysInt i = 0; i < resultDomSize; ++i) {
      currentSupport[i + arraySize] = -1; // will be incremented if support sought
      if(resultvar.inDomain(i + initialResultDomMin))
        findNewSupportForResult(i);
    }

    if(indexvar.isAssigned())
      dealWithAssignedIndex();

    DomainInt trigPos =
        varArray.size() * 2 +
        checked_cast<SysInt>((initialResultDomMax - initialResultDomMin + 1) * 2);

    moveTriggerInt(resultvar, trigPos,
                   DomainChanged); // Why is this always here-- why not place it
                                   // when indexvar becomes assigned, lift it
    // whenever it triggers when indexvar is not assigned.
    ++trigPos;

    moveTriggerInt(indexvar, trigPos, Assigned);
    if(undefMapsZero) {
      ++trigPos;
      if(resultvar.inDomain(0))
        moveTriggerInt(resultvar, trigPos, DomainRemoval, 0);
    }
  }

  virtual void propagateDynInt(SysInt pos, DomainDelta) {
    PROP_INFO_ADDONE(DynElement);
    UnsignedSysInt arraySize = varArray.size();
    UnsignedSysInt resultSupportTriggers =
        checked_cast<UnsignedSysInt>((initialResultDomMax - initialResultDomMin + 1) * 2);
    UnsignedSysInt indexSupportTriggers = arraySize * 2;
    // SysInt whenIndexAssignedTriggers = (initialResultDomMax -
    // initialResultDomMin + 1);
    if(pos < resultSupportTriggers) { // It was a value in the result var
                                        // which lost support
      findNewSupportForResult(pos / 2);
      return;
    }
    pos -= resultSupportTriggers;

    if(pos < indexSupportTriggers) { // A value in the index var lost support
      findNewSupportForIndex(pos / 2);
      return;
    }
    pos -= indexSupportTriggers;

    // if(pos < whenIndexAssignedTriggers)
    if(pos == 0) { // A value was removed from result var
      if(indexvar.isAssigned()) {
        dealWithAssignedIndex();
      }
      return;
    }

    pos--;
    if(pos == 0) {
      dealWithAssignedIndex();
      return;
    }

    if(undefMapsZero) {
      // 0 has been lost from the domain
      D_ASSERT(pos == 1);
      D_ASSERT(!resultvar.inDomain(0));
      indexvar.setMin(0);
      indexvar.setMax((DomainInt)varArray.size() - 1);
      return;
    }

    D_FATAL_ERROR("Fatal error in watch-element");
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    D_ASSERT(vSize == (SysInt)varArray.size() + 2);
    DomainInt resultvariable = v[vSize - 1];
    DomainInt indexvariable = v[vSize - 2];
    if(indexvariable < 0 || indexvariable >= (SysInt)vSize - 2) {
      if(undefMapsZero)
        return resultvariable == 0;
      else
        return false;
    }

    return v[checked_cast<SysInt>(indexvariable)] == resultvariable;
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> vars;
    vars.reserve(varArray.size() + 2);
    for(UnsignedSysInt i = 0; i < varArray.size(); ++i)
      vars.push_back(varArray[i]);
    vars.push_back(indexvar);
    vars.push_back(resultvar);
    return vars;
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    DomainInt arrayStart = max(DomainInt(0), indexvar.min());
    DomainInt arrayEnd = min(DomainInt(varArray.size()) - 1, indexvar.max());

    if(undefMapsZero) {
      if(resultvar.inDomain(0)) {
        if(indexvar.min() < 0) {
          assignment.push_back(make_pair(varArray.size(), indexvar.min()));
          assignment.push_back(make_pair(varArray.size() + 1, 0));
          return true;
        }
        if(indexvar.max() >= (SysInt)varArray.size()) {
          assignment.push_back(make_pair(varArray.size(), indexvar.max()));
          assignment.push_back(make_pair(varArray.size() + 1, 0));
          return true;
        }
      }
    }

    for(SysInt i = checked_cast<SysInt>(arrayStart); i <= checked_cast<SysInt>(arrayEnd); ++i) {
      if(indexvar.inDomain(i)) {
        DomainInt domStart = max(resultvar.min(), varArray[i].min());
        DomainInt domEnd = min(resultvar.max(), varArray[i].max());
        for(DomainInt domval = domStart; domval <= domEnd; ++domval) {
          if(varArray[i].inDomain(domval) && resultvar.inDomain(domval)) {
            // indexvar = i
            assignment.push_back(make_pair(varArray.size(), i));
            // resultvar = domval
            assignment.push_back(make_pair(varArray.size() + 1, domval));
            // vararray[i] = domval
            assignment.push_back(make_pair(i, domval));
            return true;
          }
        }
      }
    }
    return false;
  }

  virtual AbstractConstraint* reverseConstraint() {
    // This is a slow-ish temporary solution.
    // (i=1 and X[1]!=r) or (i=2 ...
    vector<AbstractConstraint*> con;
    // or the index is out of range:

    if(!undefMapsZero) {
      // Constraint is satisfied if the index is out of range.
      vector<DomainInt> r;
      r.push_back(0);
      r.push_back((DomainInt)varArray.size() - 1);
      AbstractConstraint* t4 =
          (AbstractConstraint*)new WatchNotInRangeConstraint<Index>(indexvar, r);
      con.push_back(t4);
    }

    for(SysInt i = 0; i < (SysInt)varArray.size(); i++) {
      vector<AbstractConstraint*> con2;
      WatchLiteralConstraint<Index>* t = new WatchLiteralConstraint<Index>(indexvar, i);
      con2.push_back((AbstractConstraint*)t);
      NeqConstraintBinary<AnyVarRef, Result>* t2 =
          new NeqConstraintBinary<AnyVarRef, Result>(varArray[i], resultvar);
      con2.push_back((AbstractConstraint*)t2);

      Dynamic_AND* t3 = new Dynamic_AND(con2);
      con.push_back((AbstractConstraint*)t3);
    }

    if(undefMapsZero) {
      // or (i not in {0..size-1} /\ r!=0)
      vector<AbstractConstraint*> outBounds;
      outBounds.push_back(new WatchNotLiteralConstraint<Result>(resultvar, 0));
      outBounds.push_back(new WatchNotInRangeConstraint<Index>(
          indexvar, makeVec<DomainInt>(0, (DomainInt)varArray.size() - 1)));
      con.push_back(new Dynamic_AND(outBounds));
    }
    return new Dynamic_OR(con);
  }
};

template <typename Var1, typename Var2>
AbstractConstraint* BuildCT_WATCHED_ELEMENT(const Var1& vararray, const Var2& v1, const Var1& v2,
                                            ConstraintBlob&) {
  return new ElementConstraintDynamic<Var1, typename Var2::value_type, typename Var1::value_type>(
      vararray, v1[0], v2[0]);
}

template <typename Var1, typename Var2, typename Var3>
AbstractConstraint* BuildCT_WATCHED_ELEMENT(Var1 vararray, const Var2& v1, const Var3& v2,
                                            ConstraintBlob&) {
  return new ElementConstraintDynamic<Var1, typename Var2::value_type, AnyVarRef>(vararray, v1[0],
                                                                                  AnyVarRef(v2[0]));
}

/* JSON
{ "type": "constraint",
  "name": "watchelement",
  "internal_name": "CT_WATCHED_ELEMENT",
  "args": [ "read_list", "read_var", "read_var" ]
}
*/

template <typename Var1, typename Var2, typename Var3>
AbstractConstraint* BuildCT_WATCHED_ELEMENT_ONE(const Var1& vararray, const Var2& v1,
                                                const Var3& v2, ConstraintBlob& b) {
  typedef typename ShiftType<typename Var2::value_type, compiletimeVal<SysInt, -1>>::type ShiftVal;
  vector<ShiftVal> replace_v1;
  replace_v1.push_back(ShiftVarRef(v1[0], compiletimeVal<SysInt, -1>()));
  return BuildCT_WATCHED_ELEMENT(vararray, replace_v1, v2, b);
}

/* JSON
{ "type": "constraint",
  "name": "watchelement_one",
  "internal_name": "CT_WATCHED_ELEMENT_ONE",
  "args": [ "read_list", "read_var", "read_var" ]
}
*/

template <typename Var1, typename Var2>
AbstractConstraint* BuildCT_WATCHED_ELEMENT_UNDEFZERO(const Var1& vararray, const Var2& v1,
                                                      const Var1& v2, ConstraintBlob&) {
  return new ElementConstraintDynamic<Var1, typename Var2::value_type, typename Var1::value_type,
                                      true>(vararray, v1[0], v2[0]);
}

template <typename Var1, typename Var2, typename Var3>
AbstractConstraint* BuildCT_WATCHED_ELEMENT_UNDEFZERO(Var1 vararray, const Var2& v1, const Var3& v2,
                                                      ConstraintBlob&) {
  return new ElementConstraintDynamic<Var1, typename Var2::value_type, AnyVarRef, true>(
      vararray, v1[0], AnyVarRef(v2[0]));
}

/* JSON
{ "type": "constraint",
  "name": "watchelement_undefzero",
  "internal_name": "CT_WATCHED_ELEMENT_UNDEFZERO",
  "args": [ "read_list", "read_var", "read_var" ]
}
*/

template <typename Var1, typename Var2, typename Var3>
AbstractConstraint* BuildCT_WATCHED_ELEMENT_ONE_UNDEFZERO(const Var1& vararray, const Var2& v1,
                                                          const Var3& v2, ConstraintBlob& b) {
  typedef typename ShiftType<typename Var2::value_type, compiletimeVal<SysInt, -1>>::type ShiftVal;
  vector<ShiftVal> replace_v1;
  replace_v1.push_back(ShiftVarRef(v1[0], compiletimeVal<SysInt, -1>()));
  return BuildCT_WATCHED_ELEMENT_UNDEFZERO(vararray, replace_v1, v2, b);
}

/* JSON
{ "type": "constraint",
  "name": "watchelement_one_undefzero",
  "internal_name": "CT_WATCHED_ELEMENT_ONE_UNDEFZERO",
  "args": [ "read_list", "read_var", "read_var" ]
}
*/

#endif
