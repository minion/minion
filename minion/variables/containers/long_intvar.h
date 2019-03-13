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
    var_offset.push_back(0);
  }

  typedef DomainInt domain_bound_type;
  ExtendableBlock bound_data;
  MonotonicSet* bms_array;
  TriggerList trigger_list;

  /// Initial bounds of each variable
  vector<pair<DomainInt, DomainInt>> initial_bounds;
  /// Position in the variable data (in counts of d_type) of where each variable
  /// starts
  vector<DomainInt> var_offset;
  /// Constraints variable participates in
  vector<vector<AbstractConstraint*>> constraints;
#ifdef WDEG
  vector<DomainInt> wdegs;
#endif

  UnsignedSysInt varCount_m;
  BOOL lock_m;

#define BOUND_DATA_SIZE 3

  domain_bound_type& lower_bound(BigRangeVarRef_internal i) const {
    return ((domain_bound_type*)bound_data())[i.varNum * BOUND_DATA_SIZE];
  }

  domain_bound_type& upper_bound(BigRangeVarRef_internal i) const {
    return ((domain_bound_type*)bound_data())[i.varNum * BOUND_DATA_SIZE + 1];
  }

  domain_bound_type& domSize(BigRangeVarRef_internal i) const {
    return ((domain_bound_type*)bound_data())[i.varNum * BOUND_DATA_SIZE + 2];
  }

  void reduceDomSize(BigRangeVarRef_internal i) {
    domSize(i) -= 1;
  }

  /// Find new "true" upper bound.
  /// This should be used by first setting the value of upper_bound(d), then
  /// calling
  /// this function to move this value past any removed values.
  DomainInt find_new_upper_bound(BigRangeVarRef_internal d) {
    DomainInt lower = lower_bound(d);
    DomainInt old_up_bound = upper_bound(d);
    DomainInt loopvar = old_up_bound;
    // DomainInt low_bound = initial_bounds[d.varNum].first;
    if(loopvar < lower) {
      getState().setFailed(true);
      /// Here just remove the value which should lead to the least work.
      return upper_bound(d);
    }
    if(bms_array->isMember(var_offset[d.varNum] + loopvar) && (loopvar >= lower))
      return upper_bound(d);
    --loopvar;
    for(; loopvar >= lower; --loopvar) {
      if(bms_array->isMember(var_offset[d.varNum] + loopvar))
        return loopvar;
    }
    getState().setFailed(true);
    return old_up_bound;
  }

  /// Find new "true" lower bound.
  /// This should be used by first setting the value of lower_bound(d), then
  /// calling
  /// this function to move this value past any removed values.
  DomainInt find_new_lower_bound(BigRangeVarRef_internal d) {
    DomainInt upper = upper_bound(d);
    DomainInt old_low_bound = lower_bound(d);
    DomainInt loopvar = old_low_bound;
    // DomainInt low_bound = initial_bounds[d.varNum].first;
    if(loopvar > upper) {
      getState().setFailed(true);
      /// Here just remove the value which should lead to the least work.
      return lower_bound(d);
    }
    if(bms_array->isMember(var_offset[d.varNum] + loopvar) && (loopvar <= upper))
      return lower_bound(d);
    ++loopvar;
    for(; loopvar <= upper; ++loopvar) {
      if(bms_array->isMember(var_offset[d.varNum] + loopvar))
        return loopvar;
    }
    getState().setFailed(true);
    return old_low_bound;
  }

  void lock() {
    D_ASSERT(!lock_m);
    lock_m = true;
    // bms_array->lock(); // gets locked in constraint_setup.cpp
  }

  void addVariables(const vector<Bounds>& newDomains) {
    D_ASSERT(!lock_m);
    for(SysInt i = 0; i < (SysInt)newDomains.size(); ++i) {
      initial_bounds.push_back(make_pair(newDomains[i].lower_bound, newDomains[i].upper_bound));
      DomainInt domainSize;
      domainSize = newDomains[i].upper_bound - newDomains[i].lower_bound + 1;
      var_offset.push_back(var_offset.back() + domainSize);
      varCount_m++;
    }
    constraints.resize(newDomains.size());
#ifdef WDEG
    wdegs.resize(newDomains.size());
#endif

    bound_data = getMemory().backTrack().requestBytesExtendable(varCount_m * BOUND_DATA_SIZE *
                                                                sizeof(domain_bound_type));
    DomainInt temp1 = bms_array->request_storage(var_offset.back());

    // correct var_offsets to start at the start of our block.
    if(temp1 > 0) {
      for(SysInt i = 0; i < (SysInt)var_offset.size(); ++i)
        var_offset[i] += temp1;
    }

    for(SysInt j = 0; j < (SysInt)varCount_m; ++j) {
      var_offset[j] = var_offset[j] - initial_bounds[j].first;
    };

    domain_bound_type* bound_ptr = (domain_bound_type*)(bound_data());

    DomainInt minDomainVal = 0;
    DomainInt maxDomainVal = 0;
    if(!initial_bounds.empty()) {
      minDomainVal = initial_bounds[0].first;
      maxDomainVal = initial_bounds[0].second;
      for(UnsignedSysInt i = 0; i < varCount_m; ++i) {
        bound_ptr[BOUND_DATA_SIZE * i] = initial_bounds[i].first;
        bound_ptr[BOUND_DATA_SIZE * i + 1] = initial_bounds[i].second;
        bound_ptr[BOUND_DATA_SIZE * i + 2] = initial_bounds[i].second - initial_bounds[i].first + 1;
        minDomainVal = mymin(initial_bounds[i].first, minDomainVal);
        maxDomainVal = mymax(initial_bounds[i].second, maxDomainVal);
      }
    }

    trigger_list.addVariables(initial_bounds);
  }

  BOOL isAssigned(BigRangeVarRef_internal d) const {
    D_ASSERT(lock_m);
    return lower_bound(d) == upper_bound(d);
  }

  DomainInt getAssignedValue(BigRangeVarRef_internal d) const {
    D_ASSERT(lock_m);
    D_ASSERT(isAssigned(d));
    return getMin(d);
  }

  BOOL inDomain(BigRangeVarRef_internal d, DomainInt i) const {
    D_ASSERT(lock_m);
    if(i < lower_bound(d) || i > upper_bound(d))
      return false;
    return bms_array->isMember(var_offset[d.varNum] + i);
  }

  // Warning: If this is ever changed, be sure to check through the code for
  // other places
  // where bms_array is used directly.
  BOOL inDomain_noBoundCheck(BigRangeVarRef_internal d, DomainInt i) const {
    D_ASSERT(lock_m);
    D_ASSERT(i >= lower_bound(d));
    D_ASSERT(i <= upper_bound(d));
    return bms_array->isMember(var_offset[d.varNum] + i);
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
    D_ASSERT(getState().isFailed() || (inDomain(d, lower_bound(d)) && inDomain(d, upper_bound(d))));
    return lower_bound(d);
  }

  DomainInt getMax(BigRangeVarRef_internal d) const {
    D_ASSERT(lock_m);
    D_ASSERT(getState().isFailed() || (inDomain(d, lower_bound(d)) && inDomain(d, upper_bound(d))));
    return upper_bound(d);
  }

  DomainInt initialMin(BigRangeVarRef_internal d) const {
    return initial_bounds[d.varNum].first;
  }

  DomainInt initialMax(BigRangeVarRef_internal d) const {
    return initial_bounds[d.varNum].second;
  }

  void removeFromDomain(BigRangeVarRef_internal d, DomainInt i) {
#ifdef DEBUG
    cout << "Calling removeFromDomain: " << d.varNum << " " << i << " [" << lower_bound(d) << ":"
         << upper_bound(d) << "] original [" << initialMin(d) << ":" << initialMax(d) << "]"
         << endl;
    // bms_pointer(d)->print_state();
    bms_array->print_state();
#endif
    D_ASSERT(lock_m);
    D_ASSERT(getState().isFailed() || (inDomain(d, lower_bound(d)) && inDomain(d, upper_bound(d))));
    if((i < lower_bound(d)) || (i > upper_bound(d)) ||
       !(bms_array->ifMember_remove(var_offset[d.varNum] + i))) {
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
    D_ASSERT(!bms_array->isMember(var_offset[d.varNum] + i));

    domain_bound_type up_bound = upper_bound(d);
    if(i == up_bound) {
      upper_bound(d) = find_new_upper_bound(d);
      trigger_list.push_upper(d.varNum, up_bound - upper_bound(d));
    }

    domain_bound_type low_bound = lower_bound(d);
    if(i == low_bound) {
      lower_bound(d) = find_new_lower_bound(d);
      trigger_list.push_lower(d.varNum, lower_bound(d) - low_bound);
    }

    if(upper_bound(d) == lower_bound(d)) {
      trigger_list.push_assign(d.varNum, getAssignedValue(d));
    }

    D_ASSERT(getState().isFailed() || (inDomain(d, lower_bound(d)) && inDomain(d, upper_bound(d))));

#ifdef DEBUG
    cout << "Exiting removeFromDomain: " << d.varNum << " " << i << " [" << lower_bound(d) << ":"
         << upper_bound(d) << "] original [" << initialMin(d) << ":" << initialMax(d) << "]"
         << endl;
    bms_array->print_state();
#endif
    return;
  }

  BOOL validAssignment(BigRangeVarRef_internal d, DomainInt offset, DomainInt lower,
                       DomainInt upper) {
    D_ASSERT(getState().isFailed() || (inDomain(d, lower_bound(d)) && inDomain(d, upper_bound(d))));
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
    DomainInt lower = lower_bound(d);
    DomainInt upper = upper_bound(d);
    if(!validAssignment(d, offset, lower, upper))
      return;
    commonAssign(d, offset, lower, upper);
  }

  void uncheckedAssign(BigRangeVarRef_internal d, DomainInt i) {
    D_ASSERT(inDomain(d, i));
    D_ASSERT(!isAssigned(d));
    commonAssign(d, i, lower_bound(d), upper_bound(d));
  }

private:
  // This function just unifies part of assign and uncheckedAssign
  void commonAssign(BigRangeVarRef_internal d, DomainInt offset, DomainInt lower, DomainInt upper) {
    // TODO : Optimise this function to only check values in domain.
    DomainInt domainOffset = var_offset[d.varNum] /*- initial_bounds[d.varNum].first*/;
    for(DomainInt loop = lower; loop <= upper; ++loop) {
      // def of inDomain: bms_array->isMember(var_offset[d.varNum] + i -
      // initial_bounds[d.varNum].first);
      if(bms_array->isMember(loop + domainOffset) && loop != offset) {
        trigger_list.pushDomain_removal(d.varNum, loop);
        reduceDomSize(d);
      }
    }
    trigger_list.pushDomainChanged(d.varNum);
    trigger_list.push_assign(d.varNum, offset);

    DomainInt low_bound = lower_bound(d);
    if(offset != low_bound) {
      trigger_list.push_lower(d.varNum, offset - low_bound);
      lower_bound(d) = offset;
    }

    DomainInt up_bound = upper_bound(d);
    if(offset != up_bound) {
      trigger_list.push_upper(d.varNum, up_bound - offset);
      upper_bound(d) = offset;
    }
    D_ASSERT(getState().isFailed() || (inDomain(d, lower_bound(d)) && inDomain(d, upper_bound(d))));
  }

public:
  void setMax(BigRangeVarRef_internal d, DomainInt offset) {
#ifdef DEBUG
    cout << "Calling setMax: " << d.varNum << " " << offset << " [" << lower_bound(d) << ":"
         << upper_bound(d) << "] original [" << initialMin(d) << ":" << initialMax(d) << "]"
         << endl;
    bms_array->print_state();
#endif

    D_ASSERT(getState().isFailed() || (inDomain(d, lower_bound(d)) && inDomain(d, upper_bound(d))));
    DomainInt up_bound = upper_bound(d);
    DomainInt low_bound = lower_bound(d);

    if(offset < low_bound) {
      getState().setFailed(true);
      return;
    }

    if(offset < up_bound) {
      // TODO : Optimise this function to only check values in domain.
      DomainInt domainOffset = var_offset[d.varNum] /*- initial_bounds[d.varNum].first*/;
      for(DomainInt loop = offset + 1; loop <= up_bound; ++loop) {
        // Def of inDomain: bms_array->isMember(var_offset[d.varNum] + i -
        // initial_bounds[d.varNum].first);
        if(bms_array->isMember(domainOffset + loop)) {
          trigger_list.pushDomain_removal(d.varNum, loop);
          reduceDomSize(d);
        }
      }
      upper_bound(d) = offset;
      DomainInt new_upper = find_new_upper_bound(d);
      upper_bound(d) = new_upper;

#ifndef NO_DOMAIN_TRIGGERS
      trigger_list.pushDomainChanged(d.varNum);
#endif
      trigger_list.push_upper(d.varNum, up_bound - upper_bound(d));

      if(lower_bound(d) == upper_bound(d)) {
        trigger_list.push_assign(d.varNum, getAssignedValue(d));
      }
    }
    D_ASSERT(getState().isFailed() || (inDomain(d, lower_bound(d)) && inDomain(d, upper_bound(d))));
#ifdef DEBUG
    cout << "Exiting setMax: " << d.varNum << " " << upper_bound(d) << " [" << lower_bound(d)
         << ":" << upper_bound(d) << "] original [" << initialMin(d) << ":" << initialMax(d)
         << "]" << endl;
    bms_array->print_state();
#endif
  }

  void setMin(BigRangeVarRef_internal d, DomainInt offset) {
#ifdef DEBUG
    cout << "Calling setMin: " << d.varNum << " " << offset << " [" << lower_bound(d) << ":"
         << upper_bound(d) << "] original [" << initialMin(d) << ":" << initialMax(d) << "]"
         << endl;
    bms_array->print_state();
#endif
    D_ASSERT(getState().isFailed() || (inDomain(d, lower_bound(d)) && inDomain(d, upper_bound(d))));

    DomainInt up_bound = upper_bound(d);
    DomainInt low_bound = lower_bound(d);

    if(offset > up_bound) {
      getState().setFailed(true);
      return;
    }

    if(offset > low_bound) {
      // TODO : Optimise this function to only check values in domain.
      DomainInt domainOffset = var_offset[d.varNum] /*- initial_bounds[d.varNum].first*/;
      for(DomainInt loop = low_bound; loop < offset; ++loop) {
        // def of inDomain: bms_array->isMember(var_offset[d.varNum] + i -
        // initial_bounds[d.varNum].first);
        if(bms_array->isMember(loop + domainOffset)) {
          trigger_list.pushDomain_removal(d.varNum, loop);
          reduceDomSize(d);
        }
      }
      D_ASSERT(getState().isFailed() ||
               (inDomain(d, lower_bound(d)) && inDomain(d, upper_bound(d))));

      lower_bound(d) = offset;
      DomainInt new_lower = find_new_lower_bound(d);
      lower_bound(d) = new_lower;

#ifndef NO_DOMAIN_TRIGGERS
      trigger_list.pushDomainChanged(d.varNum);
#endif
      trigger_list.push_lower(d.varNum, lower_bound(d) - low_bound);
      if(lower_bound(d) == upper_bound(d)) {
        trigger_list.push_assign(d.varNum, getAssignedValue(d));
      }
    }
    D_ASSERT(getState().isFailed() || (inDomain(d, lower_bound(d)) && inDomain(d, upper_bound(d))));
#ifdef DEBUG
    cout << "Exiting setMin: " << d.varNum << " " << lower_bound(d) << " [" << lower_bound(d)
         << ":" << upper_bound(d) << "] original [" << initialMin(d) << ":" << initialMax(d)
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
