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

/** @help constraints;element_one Description
The constraint element one is identical to element, except that the
vector is indexed from 1 rather than from 0.
*/

/** @help constraints;element_one References
See

   help constraints element

for details of the element constraint which is almost identical to this
one.
*/

/** @help constraints;element Description
The constraint

   element(vec, i, e)

specifies that, in any solution, vec[i] = e and i is in the range
[0 .. |vec|-1].
*/

/** @help constraints;element Notes

Warning: This constraint is not confluent. Depending on the order the
propagators are called in Minion, the number of search nodes may vary when
using element. To avoid this problem, use watchelement instead. More details
below.

The level of propagation enforced by this constraint is not named, however it
works as follows. For constraint vec[i]=e:

- After i is assigned, ensures that min(vec[i]) = min(e) and
  max(vec[i]) = max(e).

- When e is assigned, removes idx from the domain of i whenever e is not an
  element of the domain of vec[idx].

- When m[idx] is assigned, removes idx from i when m[idx] is not in the domain
  of e.

This level of consistency is designed to avoid the propagator having to scan
through vec, except when e is assigned. It does a quantity of cheap propagation
and may work well in practise on certain problems.

Element is not confluent, which may cause the number of search nodes to vary
depending on the order in which constraints are listed in the input file, or
the order they are called in Minion. For example, the following input causes
Minion to search 41 nodes.

MINION 3
**VARIABLES**
DISCRETE x[5] {1..5}
**CONSTRAINTS**
element([x[0],x[1],x[2]], x[3], x[4])
alldiff([x])
**EOF**

However if the two constraints are swapped over, Minion explores 29 nodes.
As a rule of thumb, to get a lower node count, move element constraints
to the end of the list.
*/

/** @help constraints;element References
See the entry

   constraints watchelement

for details of an identical constraint that enforces generalised arc
consistency.
*/

#ifndef CONSTRAINT_ELEMENT_H_EIFESJKLNDFSNMDFS
#define CONSTRAINT_ELEMENT_H_EIFESJKLNDFSNMDFS

#include "constraint_equal.h"
#include "dynamic_new_and.h"
#include "dynamic_new_or.h"
#include "unary/dynamic_literal.h"
#include "unary/dynamic_notinrange.h"

template <typename VarArray, typename Index, typename Result, bool undefMapsZero = false>
struct ElementConstraint : public AbstractConstraint {
  virtual string constraintName() {
    return "element";
  }

  VarArray varArray;
  Index indexvar;
  Result resultvar;

  virtual string fullOutputName() {
    string undef_name = "";
    if(undefMapsZero)
      undef_name = "_undefzero";

    vector<Mapper> v = indexvar.getMapperStack();
    if(!v.empty() && v.back() == Mapper(MAP_SHIFT, -1)) {
      return ConOutput::printCon("element_one" + undef_name, varArray, indexvar.popOneMapper(),
                                  resultvar);
    } else {
      return ConOutput::printCon("element" + undef_name, varArray, indexvar, resultvar);
    }
  }

  ElementConstraint(const VarArray& _varArray, const Index& Indexvar, const Result& _resultvar)
      : varArray(_varArray), indexvar(Indexvar), resultvar(_resultvar) {}

  virtual SysInt dynamicTriggerCount() {
    return varArray.size() + 2;
  }

  void setupTriggers() {
    SysInt arraySize = varArray.size();
    DomainInt loopStart = std::max(DomainInt(0), indexvar.initialMin());
    DomainInt loopMax = std::min(DomainInt(arraySize), indexvar.initialMax() + 1);
    for(DomainInt i = loopStart; i < loopMax; ++i)
      moveTriggerInt(varArray[checked_cast<SysInt>(i)], i, Assigned);

    moveTriggerInt(indexvar, arraySize, Assigned);
    moveTriggerInt(resultvar, arraySize + 1, Assigned);
  }

  virtual void propagateDynInt(SysInt propVal, DomainDelta) {
    PROP_INFO_ADDONE(NonGACElement);
    SysInt varSize = varArray.size();
    if(indexvar.isAssigned()) {
      SysInt index = checked_cast<SysInt>(indexvar.assignedValue());
      if(index < 0 || index >= (SysInt)varArray.size()) {
        if(!undefMapsZero) {
          getState().setFailed(true);
          return;
        } else {
          resultvar.assign(0);
          return;
        }
      }

      // Index is within bounds. Now both undefzero and standard ct behave the
      // same.
      DomainInt valMin = max(resultvar.min(), varArray[index].min());
      DomainInt valMax = min(resultvar.max(), varArray[index].max());
      resultvar.setMin(valMin);
      varArray[index].setMin(valMin);
      resultvar.setMax(valMax);
      varArray[index].setMax(valMax);
    } else {
      if(propVal < varSize) {
        DomainInt assignedVal = varArray[checked_cast<SysInt>(propVal)].assignedValue();
        if(indexvar.inDomain(propVal) &&
           !resultvar.inDomain(assignedVal)) // perhaps the check if propVal
                                              // is indomain of indexvar is not
                                              // necessary.
        {
          if(indexvar.isBound()) {
            if(propVal == indexvar.max())
              indexvar.setMax(propVal - 1);
            if(propVal == indexvar.min())
              indexvar.setMin(propVal + 1);
          } else {
            indexvar.removeFromDomain(propVal);
          }
        }

      } else {
        D_ASSERT(propVal == varSize + 1);
        DomainInt assignedVal = resultvar.assignedValue();
        SysInt arraySize = varArray.size();
        for(SysInt i = 0; i < arraySize; ++i) {
          if(indexvar.inDomain(i) && !varArray[i].inDomain(assignedVal)) // fixed here.
          {
            if(indexvar.isBound()) {
              if(i == indexvar.max())
                indexvar.setMax(i - 1);
              if(i == indexvar.min())
                indexvar.setMin(i + 1);
            } else {
              indexvar.removeFromDomain(i);
            }
          }
        }
      }
    }
  }

  virtual void fullPropagate() {
    setupTriggers();
    if(indexvar.isAssigned()) {
      SysInt index = checked_cast<SysInt>(indexvar.assignedValue());
      if(!undefMapsZero) {
        if(index < 0 || index >= (SysInt)varArray.size()) {
          getState().setFailed(true);
          return;
        }
      } else {
        if(index < 0 || index >= (SysInt)varArray.size()) {
          resultvar.setMin(0);
          resultvar.setMax(0);
          return;
        }
      }

      // Index assigned and within range.
      DomainInt valMin = max(resultvar.min(), varArray[index].min());
      DomainInt valMax = min(resultvar.max(), varArray[index].max());
      resultvar.setMin(valMin);
      varArray[index].setMin(valMin);
      resultvar.setMax(valMax);
      varArray[index].setMax(valMax);
    }

    SysInt arraySize = varArray.size();
    // Constrain the index variable to have only indices in range, if
    // undefMapsZero is false.
    if(indexvar.min() < 0 && !undefMapsZero) {
      indexvar.setMin(0);
    }
    if(indexvar.max() >= arraySize && !undefMapsZero) {
      indexvar.setMax(arraySize - 1);
    }
    if(getState().isFailed())
      return;

    // Should use the new iterators here. Check each value of resultvar to see
    // if it's in one of varArray.
    // Only done at root, so who cares that it takes a while?
    if(!resultvar.isBound()) {
      for(DomainInt i = resultvar.min(); i <= resultvar.max(); i++) {
        if(i == 0 && undefMapsZero &&
           (indexvar.min() < 0 || indexvar.max() >= arraySize)) {
          // 0 is supported.
          continue;
        }
        if(resultvar.inDomain(i)) {
          BOOL supported = false;
          for(DomainInt j = max(indexvar.min(), (DomainInt)0);
              j <= min(indexvar.max(), (DomainInt)arraySize - 1); j++) {
            if(varArray[checked_cast<SysInt>(j)].inDomain(i)) {
              supported = true;
              break;
            }
          }
          if(!supported) {
            resultvar.removeFromDomain(i);
          }
        }
      }
    } else { // resultvar is a bound variable
      // iterate up from the minimum
      while(!getState().isFailed()) {
        DomainInt i = resultvar.min();
        BOOL supported = false;
        for(DomainInt j = max(indexvar.min(), (DomainInt)0);
            j <= min(indexvar.max(), (DomainInt)arraySize - 1); j++) {
          if(varArray[checked_cast<SysInt>(j)].inDomain(i)) {
            supported = true;
            break;
          }
        }
        if(i == 0 && undefMapsZero &&
           (indexvar.min() < 0 || indexvar.max() >= arraySize)) {
          // 0 is supported.
          supported = true;
        }
        if(!supported) {
          resultvar.setMin(i + 1);
        } else
          break;
      }
      // now iterate down from the top.
      while(!getState().isFailed()) {
        DomainInt i = resultvar.max();
        BOOL supported = false;
        for(DomainInt j = max(indexvar.min(), (DomainInt)0);
            j <= min(indexvar.max(), (DomainInt)arraySize - 1); j++) {
          if(varArray[checked_cast<SysInt>(j)].inDomain(i)) {
            supported = true;
            break;
          }
        }
        if(i == 0 && undefMapsZero &&
           (indexvar.min() < 0 || indexvar.max() >= arraySize)) {
          // 0 is supported.
          supported = true;
        }
        if(!supported) {
          resultvar.setMax(i - 1);
        } else
          break;
      }
    }

    if(getState().isFailed())
      return;

    // Check values of index variable for support.
    for(DomainInt i = max(indexvar.min(), (DomainInt)0);
        i <= min(indexvar.max(), (DomainInt)arraySize - 1); i++) {
      if(indexvar.inDomain(i) && varArray[checked_cast<SysInt>(i)].isAssigned()) {
        DomainInt assignedVal = varArray[checked_cast<SysInt>(i)].assignedValue();
        if(!resultvar.inDomain(assignedVal)) {
          if(indexvar.isBound()) {
            if(i == indexvar.max())
              indexvar.setMax(i - 1);
            if(i == indexvar.min())
              indexvar.setMin(i + 1);
          } else {

            indexvar.removeFromDomain(i);
          }
        }
      }
    }

    if(resultvar.isAssigned()) {
      DomainInt assignedVal = resultvar.assignedValue();
      for(SysInt i = 0; i < arraySize; ++i) {
        if(indexvar.inDomain(i) && !varArray[i].inDomain(assignedVal)) {
          if(indexvar.isBound()) {
            if(i == indexvar.max())
              indexvar.setMax(i - 1);
            if(i == indexvar.min())
              indexvar.setMin(i + 1);
          } else {
            indexvar.removeFromDomain(i);
          }
        }
      }
    }
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
AbstractConstraint* BuildCT_ELEMENT(const Var1& vararray, const Var2& v1, const Var1& v2,
                                    ConstraintBlob&) {
  return new ElementConstraint<Var1, typename Var2::value_type, typename Var1::value_type>(
      vararray, v1[0], v2[0]);
}

template <typename Var1, typename Var2, typename Var3>
inline AbstractConstraint* BuildCT_ELEMENT(Var1 vararray, const Var2& v1, const Var3& v2,
                                           ConstraintBlob&) {
  return new ElementConstraint<Var1, typename Var2::value_type, AnyVarRef>(vararray, v1[0],
                                                                           AnyVarRef(v2[0]));
}

/* JSON
{ "type": "constraint",
  "name": "element",
  "internal_name": "CT_ELEMENT",
  "args": [ "read_list", "read_var", "read_var" ]
}
*/

template <typename Var1, typename Var2, typename Var3>
AbstractConstraint* BuildCT_ELEMENT_ONE(const Var1& vararray, const Var2& v1, const Var3& v2,
                                        ConstraintBlob& b) {
  typedef typename ShiftType<typename Var2::value_type, compiletimeVal<SysInt, -1>>::type ShiftVal;
  vector<ShiftVal> replace_v1;
  replace_v1.push_back(ShiftVarRef(v1[0], compiletimeVal<SysInt, -1>()));
  return BuildCT_ELEMENT(vararray, replace_v1, v2, b);
}

/* JSON
{ "type": "constraint",
  "name": "element_one",
  "internal_name": "CT_ELEMENT_ONE",
  "args": [ "read_list", "read_var", "read_var" ]
}
*/

template <typename Var1, typename Var2>
inline AbstractConstraint* BuildCT_ELEMENT_UNDEFZERO(const Var1& vararray, const Var2& v1,
                                                     const Var1& v2, ConstraintBlob&) {
  return new ElementConstraint<Var1, typename Var2::value_type, typename Var1::value_type, true>(
      vararray, v1[0], v2[0]);
}

template <typename Var1, typename Var2, typename Var3>
AbstractConstraint* BuildCT_ELEMENT_UNDEFZERO(Var1 vararray, const Var2& v1, const Var3& v2,
                                              ConstraintBlob&) {
  return new ElementConstraint<Var1, typename Var2::value_type, AnyVarRef, true>(vararray, v1[0],
                                                                                 AnyVarRef(v2[0]));
}

/* JSON
{ "type": "constraint",
  "name": "element_undefzero",
  "internal_name": "CT_ELEMENT_UNDEFZERO",
  "args": [ "read_list", "read_var", "read_var" ]
}
*/

#endif
