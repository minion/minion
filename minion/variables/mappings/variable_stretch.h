// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#include "../../triggering/constraint_abstract.h"

template <typename T>
struct MultiplyHelp {
  static inline DomainInt round_down(DomainInt val, DomainInt divisor) {
    if(val > 0)
      return val / divisor;

    DomainInt newval = val / divisor;
    if(newval * divisor == val)
      return newval;

    return newval - 1;
  }

  static inline DomainInt roundUp(DomainInt val, DomainInt divisor) {
    D_ASSERT(divisor > 0);
    if(val < 0)
      return val / divisor;

    DomainInt newval = val / divisor;
    if(newval * divisor == val)
      return newval;

    return newval + 1;
  }

  static inline DomainInt divide_exact(DomainInt val, DomainInt divisor) {
    D_ASSERT(val % divisor == 0);
    return val / divisor;
  }
};
/*
template<>
struct MultiplyHelp<BoolVarRef>
{
  static inline SysInt round_down(SysInt val, SysInt divisor)
  {
    if(val < divisor)
      return 0;
    else
      return 1;
  }

  static inline SysInt roundUp(SysInt val, SysInt divisor)
  {
    if(
  }

};*/

#if 0
struct TrivialMapData
{
  SysInt multiply()
    { return 1 };

  SysInt shift()
    { return 1 };

  static BoundType ;
    { return true; }

    static bool MultEqualsOne = true;
};

template<typename VarRef, typename DataMap = TrivialDataMap>
{
  static const BOOL isBool = VarRef::isBool;
  static const BoundType isBoundConst = MapData::BoundType || VarRef::isBoundConst;
  BOOL isBound() const
  { return MapData::BoundType || data.isBound(); }

  VarRef data;
  DataMap dataMap;
  MultiplyVar(const VarRef& _data, DataMap _dataMap) : data(_data), dataMap(_dataMap)
  {
    D_ASSERT(DOMAIN_CHECK(checked_cast<BigInt>(data.initialMax()) * dataMap.multiply() + dataMap.shift()));
    D_ASSERT(DOMAIN_CHECK(checked_cast<BigInt>(data.initialMin()) * dataMap.multiply() + dataMap.shift()));
    D_ASSERT(Multiply != 0);
  }

  MultiplyVar() : data()
  { }

  MultiplyVar(const MultiplyVar& b) : data(b.data), dataMap(b.dataMap)
  { }

  BOOL isAssigned() const
  { return data.isAssigned(); }

  DomainInt assignedValue() const
  { return data.assignedValue() * dataMap.multiply() + dataMap.shift(); }

  BOOL isAssignedValue(DomainInt i) const
  {
    if(!data.isAssigned()) return false;
      return this->assignedValue() == i;
  }

  BOOL inDomain(DomainInt b) const
  {
    if((b - dataMap.shift()) % dataMap.multiply() != 0)
        return false;
      return data.inDomain(MapHelp::divide_exact(b - dataMap.shift(), dataMap));
  }

  BOOL inDomain_noBoundCheck(DomainInt b) const
  {
    if(b % dataMap.multiply() != 0)
      return false;
    return data.inDomain(MapHelp::divide_exact(b - dataMap.shift(), dataMap));
  }

  DomainInt domSize() const
  { return data.domSize(); }

  DomainInt max() const
  {
    if(dataMap.multiply() >= 0)
      return data.max() * dataMap.multiply() + dataMap.shift();
    else
      return data.min() * dataMap.multiply() + dataMap.shift();
  }

  DomainInt min() const
  {
    if(dataMap.multiply() >= 0)
      return data.min() * dataMap.multiply() + dataMap.shift();
        else
      return data.max() * dataMap.multiply() + dataMap.shift();
  }

  DomainInt initialMax() const
  {
    if(dataMap.multiply() >= 0)
      return data.initialMax() * dataMap.multiply() + dataMap.shift();
    else
      return data.initialMin() * dataMap.multiply() + dataMap.shift();
  }

  DomainInt initialMin() const
  {
    if(dataMap.multiply() >= 0)
      return data.initialMin() * dataMap.multiply() + dataMap.shift();
    else
      return data.initialMax() * dataMap.multiply() + dataMap.shift();
  }

  void setMax(DomainInt i)
  {
    if(dataMap.multiply() >= 0)
      data.setMax(MapHelp::round_down(i, dataMap.multiply()));
    else
      data.setMin(MapHelp::roundUp(-i, -dataMap.multiply()));
  }

  void setMin(DomainInt i)
  {
    if(Multiply >= 0)
      data.setMin(MapHelp::roundUp(i, dataMap.multiply()));
    else
      data.setMax(MapHelp::round_down(-i, dataMap.multiply()));
  }

  void uncheckedAssign(DomainInt b)
  {
    D_ASSERT(b % dataMap.multiply() == 0);
    data.uncheckedAssign(MultiplyHelp<VarRef>::divide_exact(b, Multiply));
  }

  void assign(DomainInt b)
  { data.assign(MultiplyHelp<VarRef>::divide_exact(b, Multiply)); }

  void removeFromDomain(DomainInt)
  { FAIL_EXIT(); }

  void addDynamicTrigger(AbstractConstraint* ac, DynamicTrigger* t, TrigType type, DomainInt pos = NoDomainValue , TrigOp op = TO_Default)
  {  data.addDynamicTrigger(ac, t, type, pos); }


  friend std::ostream& operator<<(std::ostream& o, const MultiplyVar& n)
  { return o << "Mult:" << n.data << "*" << n.Multiply; }

  DomainInt getDomainChange(DomainDelta d)
  { return abs(Multiply) * data.getDomainChange(d); }

  vector<AbstractConstraint*>* getConstraints()
  { return data.getConstraints(); }

  void addConstraint(AbstractConstraint* c)
  { data.addConstraint(c); }

  VarIdent getIdent()
  { return VarIdent(stretchT, Multiply, data.getIdent()); }

#ifdef WDEG
  DomainInt getBaseWdeg()
  { return data.getBaseWdeg(); }

  void incWdeg()
  { data.incWdeg(); }
#endif
};

#endif

template <typename VarRef>
struct MultiplyVar {
  static const BOOL isBool = true;
  static const BoundType isBoundConst = Bound_Yes;
  BOOL isBound() const {
    return true;
  }

  AnyVarRef popOneMapper() const {
    return data;
  }

  VarRef data;
  DomainInt Multiply;
  MultiplyVar(const VarRef& _data, DomainInt _Multiply) : data(_data), Multiply(_Multiply) {
    DOMAIN_CHECK(checked_cast<BigInt>(data.initialMax()) * checked_cast<BigInt>(Multiply));
    DOMAIN_CHECK(checked_cast<BigInt>(data.initialMin()) * checked_cast<BigInt>(Multiply));
    CHECK(Multiply != 0, "Cannot divide variable by 0");
  }

  MultiplyVar() : data() {
    Multiply = 0;
  }

  MultiplyVar(const MultiplyVar& b) = default;
  MultiplyVar& operator=(const MultiplyVar&) = default;

  BOOL isAssigned() const {
    return data.isAssigned();
  }

  DomainInt assignedValue() const {
    return data.assignedValue() * Multiply;
  }

  BOOL isAssignedValue(DomainInt i) const {
    if(!data.isAssigned())
      return false;

    return data.assignedValue() == i * Multiply;
  }

  BOOL inDomain(DomainInt b) const {
    if(b % Multiply != 0)
      return false;
    return data.inDomain(MultiplyHelp<VarRef>::divide_exact(b, Multiply));
  }

  BOOL inDomain_noBoundCheck(DomainInt b) const {
    if(b % Multiply != 0)
      return false;
    return data.inDomain(MultiplyHelp<VarRef>::divide_exact(b, Multiply));
  }

  DomainInt domSize() const {
    return data.domSize();
  }

  DomainInt max() const {
    if(Multiply >= 0)
      return data.max() * Multiply;
    else
      return data.min() * Multiply;
  }

  DomainInt min() const {
    if(Multiply >= 0)
      return data.min() * Multiply;
    else
      return data.max() * Multiply;
  }

  DomainInt initialMax() const {
    if(Multiply >= 0)
      return data.initialMax() * Multiply;
    else
      return data.initialMin() * Multiply;
  }

  DomainInt initialMin() const {
    if(Multiply >= 0)
      return data.initialMin() * Multiply;
    else
      return data.initialMax() * Multiply;
  }

  void setMax(DomainInt i) {
    if(Multiply >= 0)
      data.setMax(MultiplyHelp<VarRef>::round_down(i, Multiply));
    else
      data.setMin(MultiplyHelp<VarRef>::roundUp(-i, -Multiply));
  }

  void setMin(DomainInt i) {
    if(Multiply >= 0)
      data.setMin(MultiplyHelp<VarRef>::roundUp(i, Multiply));
    else
      data.setMax(MultiplyHelp<VarRef>::round_down(-i, -Multiply));
  }

  void uncheckedAssign(DomainInt b) {
    D_ASSERT(b % Multiply == 0);
    data.uncheckedAssign(MultiplyHelp<VarRef>::divide_exact(b, Multiply));
  }

  void assign(DomainInt b) {
    data.assign(MultiplyHelp<VarRef>::divide_exact(b, Multiply));
  }

  void removeFromDomain(DomainInt) {
    FAIL_EXIT();
  }

  void addDynamicTrigger(Trig_ConRef t, TrigType type, DomainInt pos = NoDomainValue,
                         TrigOp op = TO_Default) {
    switch(type) {
    case UpperBound:
      if(Multiply >= 0)
        data.addDynamicTrigger(t, UpperBound, pos, op);
      else
        data.addDynamicTrigger(t, LowerBound, pos, op);
      break;
    case LowerBound:
      if(Multiply >= 0)
        data.addDynamicTrigger(t, LowerBound, pos, op);
      else
        data.addDynamicTrigger(t, UpperBound, pos, op);
      break;
    case Assigned:
    case DomainChanged: data.addDynamicTrigger(t, type, pos, op); break;
    case DomainRemoval:
      data.addDynamicTrigger(t, DomainRemoval, MultiplyHelp<VarRef>::divide_exact(pos, Multiply),
                             op);
      break;
    default: D_FATAL_ERROR("Broken dynamic trigger");
    }
  }

  friend std::ostream& operator<<(std::ostream& o, const MultiplyVar& n) {
    return o << "Mult:" << n.data << "*" << n.Multiply;
  }

  DomainInt getDomainChange(DomainDelta d) {
    return abs(checked_cast<SysInt>(Multiply)) * data.getDomainChange(d);
  }

  vector<AbstractConstraint*>* getConstraints() {
    return data.getConstraints();
  }

  void addConstraint(AbstractConstraint* c) {
    data.addConstraint(c);
  }

  DomainInt getBaseVal(DomainInt v) const {
    return data.getBaseVal(MultiplyHelp<VarRef>::divide_exact(v, Multiply));
  }

  Var getBaseVar() const {
    return data.getBaseVar();
  }

  vector<Mapper> getMapperStack() const {
    vector<Mapper> v = data.getMapperStack();
    v.push_back(Mapper(MAP_MULT, (DomainInt)Multiply));
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
struct MultiplyType {
  typedef MultiplyVar<T> type;
};

template <typename T>
struct MultiplyType<vector<T>> {
  typedef vector<MultiplyVar<T>> type;
};

template <typename T, std::size_t i>
struct MultiplyType<std::array<T, i>> {
  typedef std::array<MultiplyVar<T>, i> type;
};

template <typename VRef>
typename MultiplyType<VRef>::type MultiplyVarRef(VRef var_ref, SysInt i) {
  return MultiplyVar<VRef>(var_ref, i);
}

template <typename VarRef>
vector<MultiplyVar<VarRef>> MultiplyVarRef(const vector<VarRef>& varArray,
                                           const vector<DomainInt>& multiplies) {
  vector<MultiplyVar<VarRef>> Multiply_array(varArray.size());
  for(UnsignedSysInt i = 0; i < varArray.size(); ++i)
    Multiply_array[i] = MultiplyVarRef(varArray[i], multiplies[i]);
  return Multiply_array;
}

template <typename VarRef, std::size_t i>
std::array<MultiplyVar<VarRef>, i> MultiplyVarRef(const std::array<VarRef, i>& varArray,
                                                  const std::array<SysInt, i>& multiplies) {
  std::array<MultiplyVar<VarRef>, i> Multiply_array;
  for(UnsignedSysInt l = 0; l < i; ++l)
    Multiply_array[l] = MultiplyVarRef(varArray[l], multiplies[i]);
  return Multiply_array;
}
