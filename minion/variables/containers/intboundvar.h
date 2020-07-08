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

  void* varBound_data;
  DomainInt varNum;

  const DomType& lowerBound() const {
    return *static_cast<DomType*>(varBound_data);
  }

  const DomType& upperBound() const {
    return *(static_cast<DomType*>(varBound_data) + 1);
  }

  static BoundVarContainer<DomType>& getCon_Static();
  BoundVarRef_internal() : varNum(-1) {}

  explicit BoundVarRef_internal(BoundVarContainer<DomType>*, DomainInt i, DomType* ptr)
      : varBound_data(ptr), varNum(i) {}

  BOOL isAssigned() const {
    return lowerBound() == upperBound();
  }

  BOOL isAssignedValue(DomainInt i) const {
    return isAssigned() && assignedValue() == i;
  }

  DomainInt assignedValue() const {
    D_ASSERT(isAssigned());
    return lowerBound();
  }

  BOOL inDomain(DomainInt i) const {
    if(i < lowerBound() || i > upperBound())
      return false;
    return true;
  }

  BOOL inDomain_noBoundCheck(DomainInt i) const {
    D_ASSERT(i >= lowerBound());
    D_ASSERT(i <= upperBound());
    return true;
  }

  DomainInt domSize() const {
    return max() - min() + 1;
  }

  DomainInt min() const {
    return lowerBound();
  }

  DomainInt max() const {
    return upperBound();
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

  BoundVarContainer() : triggerList(true), varCount_m(0) {}

  ExtendableBlock bound_data;
  TriggerList triggerList;
  vector<pair<BoundType, BoundType>> initialBounds;
  vector<vector<AbstractConstraint*>> constraints;
#ifdef WDEG
  vector<UnsignedSysInt> wdegs;
#endif
  UnsignedSysInt varCount_m;

  const BoundType& lowerBound(const BoundVarRef_internal<BoundType>& i) const {
    return ((BoundType*)bound_data())[checked_cast<SysInt>(i.varNum * 2)];
  }

  const BoundType& upperBound(const BoundVarRef_internal<BoundType>& i) const {
    return ((BoundType*)bound_data())[checked_cast<SysInt>(i.varNum * 2 + 1)];
  }

  BoundType& lowerBound(const BoundVarRef_internal<BoundType>& i) {
    return ((BoundType*)bound_data())[checked_cast<SysInt>(i.varNum * 2)];
  }

  BoundType& upperBound(const BoundVarRef_internal<BoundType>& i) {
    return ((BoundType*)bound_data())[checked_cast<SysInt>(i.varNum * 2 + 1)];
  }

  BOOL isAssigned(const BoundVarRef_internal<BoundType>& d) const {
    return lowerBound(d) == upperBound(d);
  }

  DomainInt getAssignedValue(const BoundVarRef_internal<BoundType>& d) const {
    D_ASSERT(isAssigned(d));
    return lowerBound(d);
  }

  BOOL inDomain(const BoundVarRef_internal<BoundType>& d, DomainInt i) const {
    if(i < lowerBound(d) || i > upperBound(d))
      return false;
    return true;
  }

  BOOL inDomain_noBoundCheck(const BoundVarRef_internal<BoundType>& d, DomainInt i) const {
    D_ASSERT(i >= lowerBound(d));
    D_ASSERT(i <= upperBound(d));
    return true;
  }

  DomainInt getMin(const BoundVarRef_internal<BoundType>& d) const {
    D_ASSERT(getState().isFailed() || inDomain(d, lowerBound(d)));
    return lowerBound(d);
  }

  DomainInt getMax(const BoundVarRef_internal<BoundType>& d) const {
    D_ASSERT(getState().isFailed() || inDomain(d, upperBound(d)));
    return upperBound(d);
  }

  DomainInt initialMin(const BoundVarRef_internal<BoundType>& d) const {
    return initialBounds[checked_cast<SysInt>(d.varNum)].first;
  }

  DomainInt initialMax(const BoundVarRef_internal<BoundType>& d) const {
    return initialBounds[checked_cast<SysInt>(d.varNum)].second;
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

    triggerList.pushDomainChanged(d.varNum);
    triggerList.push_assign(d.varNum, i);

    if(minVal != i) {
      triggerList.pushLower(d.varNum, i - minVal);
    }

    if(maxVal != i) {
      triggerList.pushUpper(d.varNum, maxVal - i);
    }

    upperBound(d) = i;
    lowerBound(d) = i;
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
    DomainInt lowBound = lowerBound(d);
    DomainInt upBound = upperBound(d);

    if(i < lowBound) {
      getState().setFailed(true);
      return;
    }

    if(i < upBound) {
      triggerList.pushUpper(d.varNum, upBound - i);
      triggerList.pushDomainChanged(d.varNum);
      upperBound(d) = i;
      if(lowBound == i) {
        triggerList.push_assign(d.varNum, i);
      }
    }
  }

  void setMin(const BoundVarRef_internal<BoundType>& d, DomainInt i) {
    DomainInt lowBound = lowerBound(d);
    DomainInt upBound = upperBound(d);

    if(i > upBound) {
      getState().setFailed(true);
      return;
    }

    if(i > lowBound) {
      triggerList.pushLower(d.varNum, i - lowBound);
      triggerList.pushDomainChanged(d.varNum);
      lowerBound(d) = i;
      if(upBound == i) {
        triggerList.push_assign(d.varNum, i);
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
    D_ASSERT(count > 0);


    SysInt old_varCount = varCount_m;

    D_ASSERT(bounds.lowerBound >= DomainInt_Min);
    D_ASSERT(bounds.upperBound <= DomainInt_Max);

    vector<pair<BoundType, BoundType>> newInitialBounds;

    for(SysInt j = 0; j < count; ++j) {
      newInitialBounds.push_back(make_pair(bounds.lowerBound, bounds.upperBound));
    }

    triggerList.addVariables(newInitialBounds);

    initialBounds.insert(initialBounds.end(), newInitialBounds.begin(), newInitialBounds.end());

    varCount_m += count;

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
      bound_ptr[2 * i] = initialBounds[i].first;
      bound_ptr[2 * i + 1] = initialBounds[i].second;
    }

    DomainInt minDomainVal = 0;
    DomainInt maxDomainVal = 0;
    if(!initialBounds.empty()) {
      minDomainVal = initialBounds[0].first;
      maxDomainVal = initialBounds[0].second;
      for(UnsignedSysInt i = old_varCount; i < varCount_m; ++i) {
        bound_ptr[2 * i] = initialBounds[i].first;
        bound_ptr[2 * i + 1] = initialBounds[i].second;

        minDomainVal = mymin(initialBounds[i].first, minDomainVal);
        maxDomainVal = mymax(initialBounds[i].second, maxDomainVal);
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
    if(type == DomainRemoval) {
      USER_ERROR("Some constraint you are using does not work with BOUND variables\n"
                 "Unfortunatly we cannot tell you which one. Sorry!");
    }
    triggerList.addDynamicTrigger(b.varNum, t, type, pos, op);
  }

  operator std::string() {
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
  // Note we assume in BoundVarRef_internal that upperBound(i) is just after
  // lowerBound(i)...
  return BoundVarRef(
      BoundVarRef_internal<>(this, i, (DomainInt*)(bound_data()) + checked_cast<SysInt>(i) * 2));
}
