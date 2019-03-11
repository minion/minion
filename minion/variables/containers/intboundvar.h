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
  DomainInt var_num;

  const DomType& lower_bound() const {
    return *static_cast<DomType*>(var_bound_data);
  }

  const DomType& upper_bound() const {
    return *(static_cast<DomType*>(var_bound_data) + 1);
  }

  static BoundVarContainer<DomType>& getCon_Static();
  BoundVarRef_internal() : var_num(-1) {}

  explicit BoundVarRef_internal(BoundVarContainer<DomType>*, DomainInt i, DomType* ptr)
      : var_bound_data(ptr), var_num(i) {}

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
    return Var(VAR_BOUND, var_num);
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
    return o << "BoundVar:" << v.var_num;
  }

  DomainInt getDomainChange(DomainDelta d) {
    return d.XXX_get_domain_diff();
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

  BoundVarContainer() : trigger_list(true), var_count_m(0), lock_m(0) {}

  ExtendableBlock bound_data;
  TriggerList trigger_list;
  vector<pair<BoundType, BoundType>> initial_bounds;
  vector<vector<AbstractConstraint*>> constraints;
#ifdef WDEG
  vector<UnsignedSysInt> wdegs;
#endif
  UnsignedSysInt var_count_m;
  BOOL lock_m;

  const BoundType& lower_bound(const BoundVarRef_internal<BoundType>& i) const {
    return ((BoundType*)bound_data())[checked_cast<SysInt>(i.var_num * 2)];
  }

  const BoundType& upper_bound(const BoundVarRef_internal<BoundType>& i) const {
    return ((BoundType*)bound_data())[checked_cast<SysInt>(i.var_num * 2 + 1)];
  }

  BoundType& lower_bound(const BoundVarRef_internal<BoundType>& i) {
    return ((BoundType*)bound_data())[checked_cast<SysInt>(i.var_num * 2)];
  }

  BoundType& upper_bound(const BoundVarRef_internal<BoundType>& i) {
    return ((BoundType*)bound_data())[checked_cast<SysInt>(i.var_num * 2 + 1)];
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
    return initial_bounds[checked_cast<SysInt>(d.var_num)].first;
  }

  DomainInt initialMax(const BoundVarRef_internal<BoundType>& d) const {
    return initial_bounds[checked_cast<SysInt>(d.var_num)].second;
  }

  void removeFromDomain(const BoundVarRef_internal<BoundType>&, DomainInt) {
    USER_ERROR("Some constraint you are using does not work with BOUND variables\n"
               "Unfortunatly we cannot tell you which one. Sorry!");
  }

  void internalAssign(const BoundVarRef_internal<BoundType>& d, DomainInt i) {
    DomainInt min_val = getMin(d);
    DomainInt max_val = getMax(d);
    if(min_val > i || max_val < i) {
      getState().setFailed(true);
      return;
    }

    if(min_val == max_val)
      return;

    trigger_list.push_domain_changed(d.var_num);
    trigger_list.push_assign(d.var_num, i);

    if(min_val != i) {
      trigger_list.push_lower(d.var_num, i - min_val);
    }

    if(max_val != i) {
      trigger_list.push_upper(d.var_num, max_val - i);
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
      trigger_list.push_upper(d.var_num, up_bound - i);
      trigger_list.push_domain_changed(d.var_num);
      upper_bound(d) = i;
      if(low_bound == i) {
        trigger_list.push_assign(d.var_num, i);
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
      trigger_list.push_lower(d.var_num, i - low_bound);
      trigger_list.push_domain_changed(d.var_num);
      lower_bound(d) = i;
      if(up_bound == i) {
        trigger_list.push_assign(d.var_num, i);
      }
    }
  }

  UnsignedSysInt var_count() {
    return var_count_m;
  }

  //  BoundVarRef get_new_var();
  //  BoundVarRef get_new_var(SysInt i, SysInt j);
  BoundVarRef get_var_num(DomainInt i);

  void addVariables(Bounds bounds, SysInt count = 1) {
    D_ASSERT(!lock_m);
    D_ASSERT(count > 0);
    SysInt old_var_count = var_count_m;

    D_ASSERT(bounds.lower_bound >= DomainInt_Min);
    D_ASSERT(bounds.upper_bound <= DomainInt_Max);
    for(SysInt j = 0; j < count; ++j) {
      var_count_m++;
      initial_bounds.push_back(make_pair(bounds.lower_bound, bounds.upper_bound));
    }

    constraints.resize(var_count_m);
#ifdef WDEG
    wdegs.resize(var_count_m);
#endif

    if(bound_data.empty()) {
      bound_data =
          getMemory().backTrack().requestBytesExtendable(var_count_m * 2 * sizeof(BoundType));
    } else {
      getMemory().backTrack().resizeExtendableBlock(bound_data,
                                                    var_count_m * 2 * sizeof(BoundType));
    }
    BoundType* bound_ptr = (BoundType*)(bound_data());
    for(UnsignedSysInt i = old_var_count; i < var_count_m; ++i) {
      bound_ptr[2 * i] = initial_bounds[i].first;
      bound_ptr[2 * i + 1] = initial_bounds[i].second;
    }

    DomainInt min_domain_val = 0;
    DomainInt max_domain_val = 0;
    if(!initial_bounds.empty()) {
      min_domain_val = initial_bounds[0].first;
      max_domain_val = initial_bounds[0].second;
      for(UnsignedSysInt i = old_var_count; i < var_count_m; ++i) {
        bound_ptr[2 * i] = initial_bounds[i].first;
        bound_ptr[2 * i + 1] = initial_bounds[i].second;

        min_domain_val = mymin(initial_bounds[i].first, min_domain_val);
        max_domain_val = mymax(initial_bounds[i].second, max_domain_val);
      }
    }
  }

  vector<AbstractConstraint*>* getConstraints(const BoundVarRef_internal<BoundType>& b) {
    return &constraints[checked_cast<SysInt>(b.var_num)];
  }

  void addConstraint(const BoundVarRef_internal<BoundType>& b, AbstractConstraint* c) {
    constraints[checked_cast<SysInt>(b.var_num)].push_back(c);
#ifdef WDEG
    wdegs[checked_cast<SysInt>(b.var_num)] += c->getWdeg(); // add constraint score to base var wdeg
#endif
  }

#ifdef WDEG
  DomainInt getBaseWdeg(const BoundVarRef_internal<BoundType>& b) {
    return wdegs[checked_cast<SysInt>(b.var_num)];
  }

  void incWdeg(const BoundVarRef_internal<BoundType>& b) {
    wdegs[checked_cast<SysInt>(b.var_num)]++;
  }
#endif

  void addDynamicTrigger(BoundVarRef_internal<BoundType>& b, Trig_ConRef t, TrigType type,
                         DomainInt pos = NoDomainValue, TrigOp op = TO_Default) {
    D_ASSERT(lock_m);
    if(type == DomainRemoval) {
      USER_ERROR("Some constraint you are using does not work with BOUND variables\n"
                 "Unfortunatly we cannot tell you which one. Sorry!");
    }
    trigger_list.addDynamicTrigger(b.var_num, t, type, pos, op);
  }

  operator std::string() {
    D_ASSERT(lock_m);
    stringstream s;
    SysInt char_count = 0;
    for(UnsignedSysInt i = 0; i < var_count_m; i++) {
      if(!isAssigned(BoundVarRef_internal<BoundType>(i)))
        s << "X";
      else {
        s << (getAssignedValue(BoundVarRef_internal<BoundType>(i)) ? 1 : 0);
      }
      char_count++;
      if(char_count % 7 == 0)
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

  return BoundVarRef(BoundVarRef_internal<BoundType>(var_count_m++));
}
*/

template <typename T>
inline BoundVarRef BoundVarContainer<T>::get_var_num(DomainInt i) {
  D_ASSERT(i < (DomainInt)var_count_m);
  // Note we assume in BoundVarRef_internal that upper_bound(i) is just after
  // lower_bound(i)...
  return BoundVarRef(
      BoundVarRef_internal<>(this, i, (DomainInt*)(bound_data()) + checked_cast<SysInt>(i) * 2));
}
