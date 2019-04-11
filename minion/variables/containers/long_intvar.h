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

/** @help variables;discrete Description
In discrete variables, the domain ranges between the specified lower and upper
bounds, but during search any domain value may be pruned, i.e., propagation and
search may punch arbitrary holes in the domain.
*/

/** @help variables;discrete Example
Declaration of a discrete variable x with domain {1,2,3,4} in input file:

DISCRETE x {1..4}

Use of this variable in a constraint:

eq(x, 2) #variable x equals 2
*/

#include "../../triggering/constraint_abstract.h"

template <typename d_type>
struct BigRangeVarContainer;

template <typename d_type>
struct BigRangeVarRef_internal_template {
  static const BOOL isBool = false;
  static const BoundType isBoundConst = Bound_No;
  static string name() {
    return "LongRange";
  }
  BOOL isBound() const {
    return false;
  }

  AnyVarRef popOneMapper() const {
    FATAL_REPORTABLE_ERROR();
  }

  SysInt varNum;

  static BigRangeVarContainer<d_type>& getCon_Static();
  BigRangeVarRef_internal_template() : varNum(-1) {}

  explicit BigRangeVarRef_internal_template(BigRangeVarContainer<d_type>*, DomainInt i)
      : varNum(checked_cast<SysInt>(i)) {}
};

#ifdef MORE_SEARCH_INFO
typedef InfoRefType<VarRefType<BigRangeVarRef_internal_template<UnsignedSysInt>>,
                    VAR_INFO_BIGRANGEVAR>
    BigRangeVarRef;
#else
typedef VarRefType<BigRangeVarRef_internal_template<UnsignedSysInt>> BigRangeVarRef;
#endif

template <typename d_type>
struct BigRangeVarContainer {
  typedef BigRangeVarRef_internal_template<UnsignedSysInt> BigRangeVarRef_internal;

  BigRangeVarContainer()
      : bms_array(&getMemory().monotonicSet()), trigger_list(false), varCount_m(0), lock_m(0) {
    // Store where the first variable will go.
    varOffset.push_back(0);
  }

  typedef DomainInt domainBound_type;
  ExtendableBlock bound_data;
  MonotonicSet* bms_array;
  TriggerList trigger_list;

  /// Initial bounds of each variable
  vector<pair<DomainInt, DomainInt>> initialBounds;
  /// Position in the variable data (in counts of d_type) of where each variable
  /// starts
  vector<DomainInt> varOffset;
  /// Constraints variable participates in
  vector<vector<AbstractConstraint*>> constraints;
#ifdef WDEG
  vector<DomainInt> wdegs;
#endif

  UnsignedSysInt varCount_m;
  BOOL lock_m;

#define BOUND_DATA_SIZE 3

  domainBound_type& lowerBound(BigRangeVarRef_internal i) const {
    return ((domainBound_type*)bound_data())[i.varNum * BOUND_DATA_SIZE];
  }

  domainBound_type& upperBound(BigRangeVarRef_internal i) const {
    return ((domainBound_type*)bound_data())[i.varNum * BOUND_DATA_SIZE + 1];
  }

  domainBound_type& domSize(BigRangeVarRef_internal i) const {
    return ((domainBound_type*)bound_data())[i.varNum * BOUND_DATA_SIZE + 2];
  }

  void reduceDomSize(BigRangeVarRef_internal i) {
    domSize(i) -= 1;
  }

  /// Find new "true" upper bound.
  /// This should be used by first setting the value of upperBound(d), then
  /// calling
  /// this function to move this value past any removed values.
  DomainInt findNewUpperBound(BigRangeVarRef_internal d) {
    DomainInt lower = lowerBound(d);
    DomainInt old_upBound = upperBound(d);
    DomainInt loopvar = old_upBound;
    // DomainInt lowBound = initialBounds[d.varNum].first;
    if(loopvar < lower) {
      getState().setFailed(true);
      /// Here just remove the value which should lead to the least work.
      return upperBound(d);
    }
    if(bms_array->isMember(varOffset[d.varNum] + loopvar) && (loopvar >= lower))
      return upperBound(d);
    --loopvar;
    for(; loopvar >= lower; --loopvar) {
      if(bms_array->isMember(varOffset[d.varNum] + loopvar))
        return loopvar;
    }
    getState().setFailed(true);
    return old_upBound;
  }

  /// Find new "true" lower bound.
  /// This should be used by first setting the value of lowerBound(d), then
  /// calling
  /// this function to move this value past any removed values.
  DomainInt findNewLowerBound(BigRangeVarRef_internal d) {
    DomainInt upper = upperBound(d);
    DomainInt old_lowBound = lowerBound(d);
    DomainInt loopvar = old_lowBound;
    // DomainInt lowBound = initialBounds[d.varNum].first;
    if(loopvar > upper) {
      getState().setFailed(true);
      /// Here just remove the value which should lead to the least work.
      return lowerBound(d);
    }
    if(bms_array->isMember(varOffset[d.varNum] + loopvar) && (loopvar <= upper))
      return lowerBound(d);
    ++loopvar;
    for(; loopvar <= upper; ++loopvar) {
      if(bms_array->isMember(varOffset[d.varNum] + loopvar))
        return loopvar;
    }
    getState().setFailed(true);
    return old_lowBound;
  }

  void lock() {
    D_ASSERT(!lock_m);
    lock_m = true;
    // bms_array->lock(); // gets locked in constraintSetup.cpp
  }

  void addVariables(const vector<Bounds>& newDomains) {
    D_ASSERT(!lock_m);
    for(SysInt i = 0; i < (SysInt)newDomains.size(); ++i) {
      initialBounds.push_back(make_pair(newDomains[i].lowerBound, newDomains[i].upperBound));
      DomainInt domainSize;
      domainSize = newDomains[i].upperBound - newDomains[i].lowerBound + 1;
      varOffset.push_back(varOffset.back() + domainSize);
      varCount_m++;
    }
    constraints.resize(newDomains.size());
#ifdef WDEG
    wdegs.resize(newDomains.size());
#endif

    bound_data = getMemory().backTrack().requestBytesExtendable(varCount_m * BOUND_DATA_SIZE *
                                                                sizeof(domainBound_type));
    DomainInt temp1 = bms_array->request_storage(varOffset.back());

    // correct varOffsets to start at the start of our block.
    if(temp1 > 0) {
      for(SysInt i = 0; i < (SysInt)varOffset.size(); ++i)
        varOffset[i] += temp1;
    }

    for(SysInt j = 0; j < (SysInt)varCount_m; ++j) {
      varOffset[j] = varOffset[j] - initialBounds[j].first;
    };

    domainBound_type* bound_ptr = (domainBound_type*)(bound_data());

    DomainInt minDomainVal = 0;
    DomainInt maxDomainVal = 0;
    if(!initialBounds.empty()) {
      minDomainVal = initialBounds[0].first;
      maxDomainVal = initialBounds[0].second;
      for(UnsignedSysInt i = 0; i < varCount_m; ++i) {
        bound_ptr[BOUND_DATA_SIZE * i] = initialBounds[i].first;
        bound_ptr[BOUND_DATA_SIZE * i + 1] = initialBounds[i].second;
        bound_ptr[BOUND_DATA_SIZE * i + 2] = initialBounds[i].second - initialBounds[i].first + 1;
        minDomainVal = mymin(initialBounds[i].first, minDomainVal);
        maxDomainVal = mymax(initialBounds[i].second, maxDomainVal);
      }
    }

    trigger_list.addVariables(initialBounds);
  }

  BOOL isAssigned(BigRangeVarRef_internal d) const {
    D_ASSERT(lock_m);
    return lowerBound(d) == upperBound(d);
  }

  DomainInt getAssignedValue(BigRangeVarRef_internal d) const {
    D_ASSERT(lock_m);
    D_ASSERT(isAssigned(d));
    return getMin(d);
  }

  BOOL inDomain(BigRangeVarRef_internal d, DomainInt i) const {
    D_ASSERT(lock_m);
    if(i < lowerBound(d) || i > upperBound(d))
      return false;
    return bms_array->isMember(varOffset[d.varNum] + i);
  }

  // Warning: If this is ever changed, be sure to check through the code for
  // other places
  // where bms_array is used directly.
  BOOL inDomain_noBoundCheck(BigRangeVarRef_internal d, DomainInt i) const {
    D_ASSERT(lock_m);
    D_ASSERT(i >= lowerBound(d));
    D_ASSERT(i <= upperBound(d));
    return bms_array->isMember(varOffset[d.varNum] + i);
  }

  DomainInt getDomSize_Check(BigRangeVarRef_internal d) const {
    DomainInt domSize = 0;
    for(DomainInt i = this->getMin(d); i <= this->getMax(d); ++i) {
      if(this->inDomain(d, i))
        domSize++;
    }
    return domSize;
  }

  DomainInt getDomSize(BigRangeVarRef_internal d) const {
    D_ASSERT(getDomSize_Check(d) == domSize(d));
    return domSize(d);
  }

  DomainInt getMin(BigRangeVarRef_internal d) const {
    D_ASSERT(lock_m);
    D_ASSERT(getState().isFailed() || (inDomain(d, lowerBound(d)) && inDomain(d, upperBound(d))));
    return lowerBound(d);
  }

  DomainInt getMax(BigRangeVarRef_internal d) const {
    D_ASSERT(lock_m);
    D_ASSERT(getState().isFailed() || (inDomain(d, lowerBound(d)) && inDomain(d, upperBound(d))));
    return upperBound(d);
  }

  DomainInt initialMin(BigRangeVarRef_internal d) const {
    return initialBounds[d.varNum].first;
  }

  DomainInt initialMax(BigRangeVarRef_internal d) const {
    return initialBounds[d.varNum].second;
  }

  void removeFromDomain(BigRangeVarRef_internal d, DomainInt i) {
#ifdef DEBUG
    cout << "Calling removeFromDomain: " << d.varNum << " " << i << " [" << lowerBound(d) << ":"
         << upperBound(d) << "] original [" << initialMin(d) << ":" << initialMax(d) << "]"
         << endl;
    // bms_pointer(d)->print_state();
    bms_array->print_state();
#endif
    D_ASSERT(lock_m);
    D_ASSERT(getState().isFailed() || (inDomain(d, lowerBound(d)) && inDomain(d, upperBound(d))));
    if((i < lowerBound(d)) || (i > upperBound(d)) ||
       !(bms_array->ifMember_remove(varOffset[d.varNum] + i))) {
#ifdef DEBUG
      cout << "Exiting removeFromDomain: " << d.varNum << " nothing to do" << endl;
#endif
      return;
    }
    trigger_list.pushDomain_removal(d.varNum, i);
    reduceDomSize(d);
#ifndef NO_DOMAIN_TRIGGERS
    trigger_list.pushDomainChanged(d.varNum);
#endif
    D_ASSERT(!bms_array->isMember(varOffset[d.varNum] + i));

    domainBound_type upBound = upperBound(d);
    if(i == upBound) {
      upperBound(d) = findNewUpperBound(d);
      trigger_list.pushUpper(d.varNum, upBound - upperBound(d));
    }

    domainBound_type lowBound = lowerBound(d);
    if(i == lowBound) {
      lowerBound(d) = findNewLowerBound(d);
      trigger_list.pushLower(d.varNum, lowerBound(d) - lowBound);
    }

    if(upperBound(d) == lowerBound(d)) {
      trigger_list.push_assign(d.varNum, getAssignedValue(d));
    }

    D_ASSERT(getState().isFailed() || (inDomain(d, lowerBound(d)) && inDomain(d, upperBound(d))));

#ifdef DEBUG
    cout << "Exiting removeFromDomain: " << d.varNum << " " << i << " [" << lowerBound(d) << ":"
         << upperBound(d) << "] original [" << initialMin(d) << ":" << initialMax(d) << "]"
         << endl;
    bms_array->print_state();
#endif
    return;
  }

  BOOL validAssignment(BigRangeVarRef_internal d, DomainInt offset, DomainInt lower,
                       DomainInt upper) {
    D_ASSERT(getState().isFailed() || (inDomain(d, lowerBound(d)) && inDomain(d, upperBound(d))));
    if(!inDomain(d, offset)) {
      getState().setFailed(true);
      return false;
    }
    if(offset == upper && offset == lower)
      return false;
    if(offset > upper || offset < lower) {
      getState().setFailed(true);
      return false;
    }
    return true;
  }

  void assign(BigRangeVarRef_internal d, DomainInt offset) {
    DomainInt lower = lowerBound(d);
    DomainInt upper = upperBound(d);
    if(!validAssignment(d, offset, lower, upper))
      return;
    commonAssign(d, offset, lower, upper);
  }

  void uncheckedAssign(BigRangeVarRef_internal d, DomainInt i) {
    D_ASSERT(inDomain(d, i));
    D_ASSERT(!isAssigned(d));
    commonAssign(d, i, lowerBound(d), upperBound(d));
  }

private:
  // This function just unifies part of assign and uncheckedAssign
  void commonAssign(BigRangeVarRef_internal d, DomainInt offset, DomainInt lower, DomainInt upper) {
    // TODO : Optimise this function to only check values in domain.
    DomainInt domainOffset = varOffset[d.varNum] /*- initialBounds[d.varNum].first*/;
    for(DomainInt loop = lower; loop <= upper; ++loop) {
      // def of inDomain: bms_array->isMember(varOffset[d.varNum] + i -
      // initialBounds[d.varNum].first);
      if(bms_array->isMember(loop + domainOffset) && loop != offset) {
        trigger_list.pushDomain_removal(d.varNum, loop);
        reduceDomSize(d);
      }
    }
    trigger_list.pushDomainChanged(d.varNum);
    trigger_list.push_assign(d.varNum, offset);

    DomainInt lowBound = lowerBound(d);
    if(offset != lowBound) {
      trigger_list.pushLower(d.varNum, offset - lowBound);
      lowerBound(d) = offset;
    }

    DomainInt upBound = upperBound(d);
    if(offset != upBound) {
      trigger_list.pushUpper(d.varNum, upBound - offset);
      upperBound(d) = offset;
    }
    D_ASSERT(getState().isFailed() || (inDomain(d, lowerBound(d)) && inDomain(d, upperBound(d))));
  }

public:
  void setMax(BigRangeVarRef_internal d, DomainInt offset) {
#ifdef DEBUG
    cout << "Calling setMax: " << d.varNum << " " << offset << " [" << lowerBound(d) << ":"
         << upperBound(d) << "] original [" << initialMin(d) << ":" << initialMax(d) << "]"
         << endl;
    bms_array->print_state();
#endif

    D_ASSERT(getState().isFailed() || (inDomain(d, lowerBound(d)) && inDomain(d, upperBound(d))));
    DomainInt upBound = upperBound(d);
    DomainInt lowBound = lowerBound(d);

    if(offset < lowBound) {
      getState().setFailed(true);
      return;
    }

    if(offset < upBound) {
      // TODO : Optimise this function to only check values in domain.
      DomainInt domainOffset = varOffset[d.varNum] /*- initialBounds[d.varNum].first*/;
      for(DomainInt loop = offset + 1; loop <= upBound; ++loop) {
        // Def of inDomain: bms_array->isMember(varOffset[d.varNum] + i -
        // initialBounds[d.varNum].first);
        if(bms_array->isMember(domainOffset + loop)) {
          trigger_list.pushDomain_removal(d.varNum, loop);
          reduceDomSize(d);
        }
      }
      upperBound(d) = offset;
      DomainInt newUpper = findNewUpperBound(d);
      upperBound(d) = newUpper;

#ifndef NO_DOMAIN_TRIGGERS
      trigger_list.pushDomainChanged(d.varNum);
#endif
      trigger_list.pushUpper(d.varNum, upBound - upperBound(d));

      if(lowerBound(d) == upperBound(d)) {
        trigger_list.push_assign(d.varNum, getAssignedValue(d));
      }
    }
    D_ASSERT(getState().isFailed() || (inDomain(d, lowerBound(d)) && inDomain(d, upperBound(d))));
#ifdef DEBUG
    cout << "Exiting setMax: " << d.varNum << " " << upperBound(d) << " [" << lowerBound(d)
         << ":" << upperBound(d) << "] original [" << initialMin(d) << ":" << initialMax(d)
         << "]" << endl;
    bms_array->print_state();
#endif
  }

  void setMin(BigRangeVarRef_internal d, DomainInt offset) {
#ifdef DEBUG
    cout << "Calling setMin: " << d.varNum << " " << offset << " [" << lowerBound(d) << ":"
         << upperBound(d) << "] original [" << initialMin(d) << ":" << initialMax(d) << "]"
         << endl;
    bms_array->print_state();
#endif
    D_ASSERT(getState().isFailed() || (inDomain(d, lowerBound(d)) && inDomain(d, upperBound(d))));

    DomainInt upBound = upperBound(d);
    DomainInt lowBound = lowerBound(d);

    if(offset > upBound) {
      getState().setFailed(true);
      return;
    }

    if(offset > lowBound) {
      // TODO : Optimise this function to only check values in domain.
      DomainInt domainOffset = varOffset[d.varNum] /*- initialBounds[d.varNum].first*/;
      for(DomainInt loop = lowBound; loop < offset; ++loop) {
        // def of inDomain: bms_array->isMember(varOffset[d.varNum] + i -
        // initialBounds[d.varNum].first);
        if(bms_array->isMember(loop + domainOffset)) {
          trigger_list.pushDomain_removal(d.varNum, loop);
          reduceDomSize(d);
        }
      }
      D_ASSERT(getState().isFailed() ||
               (inDomain(d, lowerBound(d)) && inDomain(d, upperBound(d))));

      lowerBound(d) = offset;
      DomainInt newLower = findNewLowerBound(d);
      lowerBound(d) = newLower;

#ifndef NO_DOMAIN_TRIGGERS
      trigger_list.pushDomainChanged(d.varNum);
#endif
      trigger_list.pushLower(d.varNum, lowerBound(d) - lowBound);
      if(lowerBound(d) == upperBound(d)) {
        trigger_list.push_assign(d.varNum, getAssignedValue(d));
      }
    }
    D_ASSERT(getState().isFailed() || (inDomain(d, lowerBound(d)) && inDomain(d, upperBound(d))));
#ifdef DEBUG
    cout << "Exiting setMin: " << d.varNum << " " << lowerBound(d) << " [" << lowerBound(d)
         << ":" << upperBound(d) << "] original [" << initialMin(d) << ":" << initialMax(d)
         << "]" << endl;
    bms_array->print_state();
#endif
  }

  BigRangeVarRef getVarNum(DomainInt i);

  UnsignedSysInt varCount() {
    return varCount_m;
  }

  BigRangeVarRef get_new_var(DomainInt i, DomainInt j);

  void addDynamicTrigger(BigRangeVarRef_internal b, Trig_ConRef t, TrigType type,
                         DomainInt pos = NoDomainValue, TrigOp op = TO_Default) {
    D_ASSERT(lock_m);
    D_ASSERT(b.varNum >= 0);
    D_ASSERT(b.varNum <= (SysInt)varCount_m);
    D_ASSERT(type != DomainRemoval || (pos >= initialMin(b) && pos <= initialMax(b)));
    trigger_list.addDynamicTrigger(b.varNum, t, type, pos, op);
  }

  vector<AbstractConstraint*>* getConstraints(const BigRangeVarRef_internal& b) {
    return &constraints[b.varNum];
  }

  void addConstraint(const BigRangeVarRef_internal& b, AbstractConstraint* c) {
    constraints[b.varNum].push_back(c);
#ifdef WDEG
    wdegs[b.varNum] += c->getWdeg(); // add constraint score to base var wdeg
#endif
  }

  DomainInt getBaseVal(const BigRangeVarRef_internal& b, DomainInt v) const {
    D_ASSERT(inDomain(b, v));
    return v;
  }

  Var getBaseVar(const BigRangeVarRef_internal& b) const {
    return Var(VAR_DISCRETE, b.varNum);
  }

  vector<Mapper> getMapperStack() const {
    return vector<Mapper>();
  }

#ifdef WDEG
  DomainInt getBaseWdeg(const BigRangeVarRef_internal& b) {
    return wdegs[b.varNum];
  }

  void incWdeg(const BigRangeVarRef_internal& b) {
    wdegs[b.varNum]++;
  }
#endif

  ~BigRangeVarContainer() {
    for(UnsignedSysInt i = 0; i < varCount_m; i++) {
      // should delete space really!
    };
  }
};

template <typename T>
inline BigRangeVarRef BigRangeVarContainer<T>::getVarNum(DomainInt i) {
  D_ASSERT(i < (DomainInt)varCount_m);
  return BigRangeVarRef(BigRangeVarRef_internal(this, i));
}
