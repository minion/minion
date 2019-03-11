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

/** @help variables;bounds Description
Bounds variables, where only the upper and lower bounds of the domain
are maintained. These domains must be continuous ranges of integers
i.e. holes cannot be put in the domains of the variables.
*/

/** @help variables;bounds Example

Declaration of a bound variable called myvar with domain between 1
and 7 in input file:

BOUND myvar {1..7}

Use of this variable in a constraint:

eq(myvar, 4) #variable myvar equals 4
*/

#include "../../triggering/constraint_abstract.h"

template <typename BoundType>
struct BoundVarContainer;

template <typename DomType = DomainInt>
struct BoundVarRef_internal {
  static const BOOL isBool = false;
  static const BoundType isBoundConst = Bound_Yes;
  static string name() {
    return "BoundVar";
  }
  BOOL isBound() const {
    return true;
  }

  AnyVarRef popOneMapper() const {
    FATAL_REPORTABLE_ERROR();
  }

  void* var_bound_data;
  DomainInt varNum;

  const DomType& lower_bound() const {
    return *static_cast<DomType*>(var_bound_data);
  }

  const DomType& upper_bound() const {
    return *(static_cast<DomType*>(var_bound_data) + 1);
  }

  static BoundVarContainer<DomType>& getCon_Static();
  BoundVarRef_internal() : varNum(-1) {}

  explicit BoundVarRef_internal(BoundVarContainer<DomType>*, DomainInt i, DomType* ptr)
      : var_bound_data(ptr), varNum(i) {}

  BOOL isAssigned() const {
    return lower_bound() == upper_bound();
  }

  BOOL isAssignedValue(DomainInt i) const {
    return isAssigned() && assignedValue() == i;
  }

  DomainInt assignedValue() const {
    D_ASSERT(isAssigned());
    return lower_bound();
  }

  BOOL inDomain(DomainInt i) const {
    if(i < lower_bound() || i > upper_bound())
      return false;
    return true;
  }

  BOOL inDomain_noBoundCheck(DomainInt i) const {
    D_ASSERT(i >= lower_bound());
    D_ASSERT(i <= upper_bound());
    return true;
  }

  DomainInt domSize() const {
    return max() - min() + 1;
  }

  DomainInt min() const {
    return lower_bound();
  }

  DomainInt max() const {
    return upper_bound();
  }

  DomainInt initialMax() const {
    return GET_LOCAL_CON().initialMax(*this);
  }

  DomainInt initialMin() const {
    return GET_LOCAL_CON().initialMin(*this);
  }

  void setMax(DomainInt i) {
    GET_LOCAL_CON().setMax(*this, i);
  }

  void setMin(DomainInt i) {
    GET_LOCAL_CON().setMin(*this, i);
  }

  void uncheckedAssign(DomainInt b) {
    GET_LOCAL_CON().uncheckedAssign(*this, b);
  }

  void assign(DomainInt b) {
    GET_LOCAL_CON().assign(*this, b);
  }

  void removeFromDomain(DomainInt b) {
    GET_LOCAL_CON().removeFromDomain(*this, b);
  }

  vector<AbstractConstraint*>* getConstraints() {
    return GET_LOCAL_CON().getConstraints(*this);
  }

  void addConstraint(AbstractConstraint* c) {
    GET_LOCAL_CON().addConstraint(*this, c);
  }

  DomainInt getBaseVal(DomainInt v) const {
    D_ASSERT(inDomain(v));
    return v;
  }

  Var getBaseVar() const {
    return Var(VAR_BOUND, varNum);
  }

  vector<Mapper> getMapperStack() const {
    return vector<Mapper>();
  }

#ifdef WDEG
  DomainInt getBaseWdeg() {
    return GET_LOCAL_CON().getBaseWdeg(*this);
  }

  void incWdeg() {
    GET_LOCAL_CON().incWdeg(*this);
  }
#endif

  friend std::ostream& operator<<(std::ostream& o, const BoundVarRef_internal& v) {
    return o << "BoundVar:" << v.varNum;
  }

  DomainInt getDomainChange(DomainDelta d) {
    return d.XXX_getDomain_diff();
  }

  void addDynamicTrigger(Trig_ConRef t, TrigType type, DomainInt pos = NoDomainValue,
                         TrigOp op = TO_Default) {
    GET_LOCAL_CON().addDynamicTrigger(*this, t, type, pos, op);
  }
};

#ifdef MORE_SEARCH_INFO
typedef InfoRefType<BoundVarRef_internal<>, VAR_INFO_BOUNDVAR> BoundVarRef;
#else
typedef BoundVarRef_internal<> BoundVarRef;
#endif

template <typename BoundType = DomainInt>
struct BoundVarContainer {

  BoundVarContainer() : trigger_list(true), varCount_m(0), lock_m(0) {}

  ExtendableBlock bound_data;
  TriggerList trigger_list;
  vector<pair<BoundType, BoundType>> initial_bounds;
  vector<vector<AbstractConstraint*>> constraints;
#ifdef WDEG
  vector<UnsignedSysInt> wdegs;
#endif
  UnsignedSysInt varCount_m;
  BOOL lock_m;

  const BoundType& lower_bound(const BoundVarRef_internal<BoundType>& i) const {
    return ((BoundType*)bound_data())[checked_cast<SysInt>(i.varNum * 2)];
  }

  const BoundType& upper_bound(const BoundVarRef_internal<BoundType>& i) const {
    return ((BoundType*)bound_data())[checked_cast<SysInt>(i.varNum * 2 + 1)];
  }

  BoundType& lower_bound(const BoundVarRef_internal<BoundType>& i) {
    return ((BoundType*)bound_data())[checked_cast<SysInt>(i.varNum * 2)];
  }

  BoundType& upper_bound(const BoundVarRef_internal<BoundType>& i) {
    return ((BoundType*)bound_data())[checked_cast<SysInt>(i.varNum * 2 + 1)];
  }

  void lock() {
    D_ASSERT(!lock_m);
    lock_m = true;
    trigger_list.addVariables(initial_bounds);
  }

  BOOL isAssigned(const BoundVarRef_internal<BoundType>& d) const {
    D_ASSERT(lock_m);
    return lower_bound(d) == upper_bound(d);
  }

  DomainInt getAssignedValue(const BoundVarRef_internal<BoundType>& d) const {
    D_ASSERT(lock_m);
    D_ASSERT(isAssigned(d));
    return lower_bound(d);
  }

  BOOL inDomain(const BoundVarRef_internal<BoundType>& d, DomainInt i) const {
    D_ASSERT(lock_m);
    if(i < lower_bound(d) || i > upper_bound(d))
      return false;
    return true;
  }

  BOOL inDomain_noBoundCheck(const BoundVarRef_internal<BoundType>& d, DomainInt i) const {
    D_ASSERT(lock_m);
    D_ASSERT(i >= lower_bound(d));
    D_ASSERT(i <= upper_bound(d));
    return true;
  }

  DomainInt getMin(const BoundVarRef_internal<BoundType>& d) const {
    D_ASSERT(lock_m);
    D_ASSERT(getState().isFailed() || inDomain(d, lower_bound(d)));
    return lower_bound(d);
  }

  DomainInt getMax(const BoundVarRef_internal<BoundType>& d) const {
    D_ASSERT(lock_m);
    D_ASSERT(getState().isFailed() || inDomain(d, upper_bound(d)));
    return upper_bound(d);
  }

  DomainInt initialMin(const BoundVarRef_internal<BoundType>& d) const {
    return initial_bounds[checked_cast<SysInt>(d.varNum)].first;
  }

  DomainInt initialMax(const BoundVarRef_internal<BoundType>& d) const {
    return initial_bounds[checked_cast<SysInt>(d.varNum)].second;
  }

  void removeFromDomain(const BoundVarRef_internal<BoundType>&, DomainInt) {
    USER_ERROR("Some constraint you are using does not work with BOUND variables\n"
               "Unfortunatly we cannot tell you which one. Sorry!");
  }

  void internalAssign(const BoundVarRef_internal<BoundType>& d, DomainInt i) {
    DomainInt minVal = getMin(d);
    DomainInt maxVal = getMax(d);
    if(minVal > i || maxVal < i) {
      getState().setFailed(true);
      return;
    }

    if(minVal == maxVal)
      return;

    trigger_list.pushDomain_changed(d.varNum);
    trigger_list.push_assign(d.varNum, i);

    if(minVal != i) {
      trigger_list.push_lower(d.varNum, i - minVal);
    }

    if(maxVal != i) {
      trigger_list.push_upper(d.varNum, maxVal - i);
    }

    upper_bound(d) = i;
    lower_bound(d) = i;
  }

  void assign(const BoundVarRef_internal<BoundType>& d, DomainInt i) {
    internalAssign(d, i);
  }

  // TODO : Optimise
  void uncheckedAssign(const BoundVarRef_internal<BoundType>& d, DomainInt i) {
    D_ASSERT(inDomain(d, i));
    internalAssign(d, i);
  }

  void setMax(const BoundVarRef_internal<BoundType>& d, DomainInt i) {
    DomainInt low_bound = lower_bound(d);
    DomainInt up_bound = upper_bound(d);

    if(i < low_bound) {
      getState().setFailed(true);
      return;
    }

    if(i < up_bound) {
      trigger_list.push_upper(d.varNum, up_bound - i);
      trigger_list.pushDomain_changed(d.varNum);
      upper_bound(d) = i;
      if(low_bound == i) {
        trigger_list.push_assign(d.varNum, i);
      }
    }
  }

  void setMin(const BoundVarRef_internal<BoundType>& d, DomainInt i) {
    DomainInt low_bound = lower_bound(d);
    DomainInt up_bound = upper_bound(d);

    if(i > up_bound) {
      getState().setFailed(true);
      return;
    }

    if(i > low_bound) {
      trigger_list.push_lower(d.varNum, i - low_bound);
      trigger_list.pushDomain_changed(d.varNum);
      lower_bound(d) = i;
      if(up_bound == i) {
        trigger_list.push_assign(d.varNum, i);
      }
    }
  }

  UnsignedSysInt varCount() {
    return varCount_m;
  }

  //  BoundVarRef get_new_var();
  //  BoundVarRef get_new_var(SysInt i, SysInt j);
  BoundVarRef getVarNum(DomainInt i);

  void addVariables(Bounds bounds, SysInt count = 1) {
    D_ASSERT(!lock_m);
    D_ASSERT(count > 0);
    SysInt old_varCount = varCount_m;

    D_ASSERT(bounds.lower_bound >= DomainInt_Min);
    D_ASSERT(bounds.upper_bound <= DomainInt_Max);
    for(SysInt j = 0; j < count; ++j) {
      varCount_m++;
      initial_bounds.push_back(make_pair(bounds.lower_bound, bounds.upper_bound));
    }

    constraints.resize(varCount_m);
#ifdef WDEG
    wdegs.resize(varCount_m);
#endif

    if(bound_data.empty()) {
      bound_data =
          getMemory().backTrack().requestBytesExtendable(varCount_m * 2 * sizeof(BoundType));
    } else {
      getMemory().backTrack().resizeExtendableBlock(bound_data,
                                                    varCount_m * 2 * sizeof(BoundType));
    }
    BoundType* bound_ptr = (BoundType*)(bound_data());
    for(UnsignedSysInt i = old_varCount; i < varCount_m; ++i) {
      bound_ptr[2 * i] = initial_bounds[i].first;
      bound_ptr[2 * i + 1] = initial_bounds[i].second;
    }

    DomainInt minDomainVal = 0;
    DomainInt maxDomainVal = 0;
    if(!initial_bounds.empty()) {
      minDomainVal = initial_bounds[0].first;
      maxDomainVal = initial_bounds[0].second;
      for(UnsignedSysInt i = old_varCount; i < varCount_m; ++i) {
        bound_ptr[2 * i] = initial_bounds[i].first;
        bound_ptr[2 * i + 1] = initial_bounds[i].second;

        minDomainVal = mymin(initial_bounds[i].first, minDomainVal);
        maxDomainVal = mymax(initial_bounds[i].second, maxDomainVal);
      }
    }
  }

  vector<AbstractConstraint*>* getConstraints(const BoundVarRef_internal<BoundType>& b) {
    return &constraints[checked_cast<SysInt>(b.varNum)];
  }

  void addConstraint(const BoundVarRef_internal<BoundType>& b, AbstractConstraint* c) {
    constraints[checked_cast<SysInt>(b.varNum)].push_back(c);
#ifdef WDEG
    wdegs[checked_cast<SysInt>(b.varNum)] += c->getWdeg(); // add constraint score to base var wdeg
#endif
  }

#ifdef WDEG
  DomainInt getBaseWdeg(const BoundVarRef_internal<BoundType>& b) {
    return wdegs[checked_cast<SysInt>(b.varNum)];
  }

  void incWdeg(const BoundVarRef_internal<BoundType>& b) {
    wdegs[checked_cast<SysInt>(b.varNum)]++;
  }
#endif

  void addDynamicTrigger(BoundVarRef_internal<BoundType>& b, Trig_ConRef t, TrigType type,
                         DomainInt pos = NoDomainValue, TrigOp op = TO_Default) {
    D_ASSERT(lock_m);
    if(type == DomainRemoval) {
      USER_ERROR("Some constraint you are using does not work with BOUND variables\n"
                 "Unfortunatly we cannot tell you which one. Sorry!");
    }
    trigger_list.addDynamicTrigger(b.varNum, t, type, pos, op);
  }

  operator std::string() {
    D_ASSERT(lock_m);
    stringstream s;
    SysInt charCount = 0;
    for(UnsignedSysInt i = 0; i < varCount_m; i++) {
      if(!isAssigned(BoundVarRef_internal<BoundType>(i)))
        s << "X";
      else {
        s << (getAssignedValue(BoundVarRef_internal<BoundType>(i)) ? 1 : 0);
      }
      charCount++;
      if(charCount % 7 == 0)
        s << endl;
    }
    return s.str();
  }
};

/*
template<typename T>
inline BoundVarRef
BoundVarContainer<T>::get_new_var(SysInt i, SysInt j)
{

  return BoundVarRef(BoundVarRef_internal<BoundType>(varCount_m++));
}
*/

template <typename T>
inline BoundVarRef BoundVarContainer<T>::getVarNum(DomainInt i) {
  D_ASSERT(i < (DomainInt)varCount_m);
  // Note we assume in BoundVarRef_internal that upper_bound(i) is just after
  // lower_bound(i)...
  return BoundVarRef(
      BoundVarRef_internal<>(this, i, (DomainInt*)(bound_data()) + checked_cast<SysInt>(i) * 2));
}
