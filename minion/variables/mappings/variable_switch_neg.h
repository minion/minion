// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#include "../../triggering/constraint_abstract.h"

template <typename VarT>
struct SwitchNeg {
  static const BOOL isBool = false;
  static const BoundType isBoundConst = VarT::isBoundConst;
  VarT data;

  BOOL isBound() const {
    return data.isBound();
  }

  AnyVarRef popOneMapper() const {
    return data;
  }

  DomainInt multiplier;
  SwitchNeg(VarT _data, DomainInt _multiplier) : data(_data), multiplier(_multiplier) {
    D_ASSERT(multiplier == -1 || multiplier == 1);
  }

  SwitchNeg() : data() {}

  SwitchNeg(const SwitchNeg& b) = default;
  SwitchNeg& operator=(const SwitchNeg&) = default;

  BOOL isAssigned() const {
    return data.isAssigned();
  }

  DomainInt assignedValue() const {
    return multiplier * data.assignedValue();
  }

  BOOL isAssignedValue(DomainInt i) const {
    return data.isAssigned() && data.assignedValue() == i * multiplier;
  }

  BOOL inDomain(DomainInt b) const {
    return data.inDomain(b * multiplier);
  }

  BOOL inDomain_noBoundCheck(DomainInt b) const {
    return data.inDomain(b * multiplier);
  }

  DomainInt domSize() const {
    return data.domSize();
  }

  DomainInt max() const {
    if(multiplier == 1)
      return data.max();
    else
      return -data.min();
  }

  DomainInt min() const {
    if(multiplier == 1)
      return data.min();
    else
      return -data.max();
  }

  DomainInt initialMax() const {
    if(multiplier == 1)
      return data.initialMax();
    else
      return -data.initialMin();
  }

  DomainInt initialMin() const {
    if(multiplier == 1)
      return data.initialMin();
    else
      return -data.initialMax();
  }

  void setMax(DomainInt i) {
    if(multiplier == 1)
      data.setMax(i);
    else
      data.setMin(-i);
  }

  void setMin(DomainInt i) {
    if(multiplier == 1)
      data.setMin(i);
    else
      data.setMax(-i);
  }

  void uncheckedAssign(DomainInt b) {
    data.uncheckedAssign(b * multiplier);
  }

  void assign(DomainInt b) {
    data.assign(b * multiplier);
  }

  void removeFromDomain(DomainInt b) {
    data.removeFromDomain(b * multiplier);
  }

  /// There isn't a minus sign here as domain changes from both the top and
  /// bottom of the domain are positive numbers.
  DomainInt getDomainChange(DomainDelta d) {
    return data.getDomainChange(d);
  }

  friend std::ostream& operator<<(std::ostream& o, const SwitchNeg& v) {
    return o << "SwitchNeg " << v.multiplier << ":" << v.data;
  }

  void addDynamicTrigger(Trig_ConRef t, TrigType type, DomainInt pos = NoDomainValue,
                         TrigOp op = TO_Default) {
    if(multiplier == 1) {
      data.addDynamicTrigger(t, type, pos, op);
      return;
    }

    switch(type) {
    case UpperBound: data.addDynamicTrigger(t, LowerBound, pos, op); break;
    case LowerBound: data.addDynamicTrigger(t, UpperBound, pos, op); break;
    case Assigned:
    case DomainChanged: data.addDynamicTrigger(t, type, pos, op); break;
    case DomainRemoval: data.addDynamicTrigger(t, DomainRemoval, -pos, op); break;
    default: D_FATAL_ERROR("Broken dynamic trigger");
    }
  }

  vector<AbstractConstraint*>* getConstraints() {
    return data.getConstraints();
  }

  void addConstraint(AbstractConstraint* c) {
    data.addConstraint(c);
  }

  DomainInt getBaseVal(DomainInt v) const {
    return data.getBaseVal(v * multiplier);
  }

  Var getBaseVar() const {
    return data.getBaseVar();
  }

  vector<Mapper> getMapperStack() const {
    vector<Mapper> v = data.getMapperStack();
    v.push_back(Mapper(MAP_SWITCH_NEG, multiplier));
    return v;
  }

#ifdef WDEG
  DomainInt getBaseWdeg() {
    return data.getBaseWdeg();
  }

  void incWdeg() {
    data.incWdeg();
  }
#endif
};

template <typename T>
struct SwitchNegType {
  typedef SwitchNeg<T> type;
};

template <typename T>
struct SwitchNegType<vector<T>> {
  typedef vector<SwitchNeg<T>> type;
};

template <typename T, std::size_t i>
struct SwitchNegType<std::array<T, i>> {
  typedef std::array<SwitchNeg<T>, i> type;
};

template <typename VRef>
typename SwitchNegType<VRef>::type SwitchNegRef(const VRef& var_ref) {
  return SwitchNeg<VRef>(var_ref);
}

template <typename VarRef>
vector<SwitchNeg<VarRef>> SwitchNegRef(const vector<VarRef>& varArray) {
  vector<SwitchNeg<VarRef>> neg_array;
  neg_array.reserve(varArray.size());
  for(UnsignedSysInt i = 0; i < varArray.size(); ++i)
    neg_array.push_back(SwitchNegRef(varArray[i]));
  return neg_array;
}

template <typename VarRef, std::size_t i>
std::array<SwitchNeg<VarRef>, i> SwitchNegRef(const std::array<VarRef, i>& varArray) {
  std::array<SwitchNeg<VarRef>, i> neg_array;
  for(UnsignedSysInt l = 0; l < i; ++l)
    neg_array[l] = SwitchNegRef(varArray[l]);
  return neg_array;
}
