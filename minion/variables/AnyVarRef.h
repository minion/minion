// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0















#ifndef _ANYVARREF_H
#define _ANYVARREF_H


#include "../system/system.h"

#include "../solver.h"

#include "../constants.h"

#include "../inputfile_parse/InputVariableDef.h"

#include "../triggering/triggers.h"

#include "../triggering/dynamic_trigger.h"

class AbstractConstraint;

/// Internal type used by AnyVarRef.
struct AnyVarRef_Abstract {
  virtual AnyVarRef_Abstract* ptr_clone() = 0;
  virtual BOOL isBound() const = 0;
  virtual AnyVarRef popOneMapper() const = 0;
  virtual BOOL isAssigned() const = 0;
  virtual DomainInt assignedValue() const = 0;
  virtual BOOL isAssignedValue(DomainInt i) const = 0;
  virtual BOOL inDomain(DomainInt b) const = 0;
  virtual BOOL inDomain_noBoundCheck(DomainInt b) const = 0;
  virtual DomainInt domSize() const = 0;
  virtual DomainInt max() const = 0;
  virtual DomainInt min() const = 0;
  virtual DomainInt initialMax() const = 0;
  virtual DomainInt initialMin() const = 0;
  virtual void setMax(DomainInt i) = 0;
  virtual void setMin(DomainInt i) = 0;
  virtual void uncheckedAssign(DomainInt b) = 0;
  virtual void assign(DomainInt b) = 0;
  virtual void removeFromDomain(DomainInt b) = 0;
  virtual vector<AbstractConstraint*>* getConstraints() = 0;
  virtual void addConstraint(AbstractConstraint* c) = 0;
  virtual DomainInt getBaseVal(DomainInt) const = 0;
  virtual Var getBaseVar() const = 0;
  virtual vector<Mapper> getMapperStack() const = 0;
#ifdef WDEG
  virtual DomainInt getBaseWdeg() = 0;
  virtual void incWdeg() = 0;
#endif

  virtual string virtualTostring() = 0;

  virtual ~AnyVarRef_Abstract() {}

  virtual DomainInt getDomainChange(DomainDelta d) = 0;
  virtual void addDynamicTrigger(Trig_ConRef t, TrigType type, DomainInt pos = NoDomainValue,
                                 TrigOp op = TO_Default) = 0;
};

/// Internal type used by AnyVarRef.
template <typename VarRef>
struct AnyVarRef_Concrete : public AnyVarRef_Abstract {

  virtual AnyVarRef_Abstract* ptr_clone() {
    return new AnyVarRef_Concrete<VarRef>(*this);
  }

  virtual BOOL isBound() const {
    return data.isBound();
  }

  AnyVarRef popOneMapper() const;

  VarRef data;
  AnyVarRef_Concrete(const VarRef& _data) : data(_data) {}

  AnyVarRef_Concrete() {}

  AnyVarRef_Concrete(const AnyVarRef_Concrete& b) : data(b.data) {}

  virtual BOOL isAssigned() const {
    return data.isAssigned();
  }

  virtual DomainInt assignedValue() const {
    return data.assignedValue();
  }

  virtual BOOL isAssignedValue(DomainInt i) const {
    return data.isAssignedValue(i);
  }

  virtual BOOL inDomain(DomainInt b) const {
    return data.inDomain(b);
  }

  virtual BOOL inDomain_noBoundCheck(DomainInt b) const {
    return data.inDomain_noBoundCheck(b);
  }

  virtual DomainInt domSize() const {
    return data.domSize();
  }

  virtual DomainInt max() const {
    return data.max();
  }

  virtual DomainInt min() const {
    return data.min();
  }

  virtual DomainInt initialMax() const {
    return data.initialMax();
  }

  virtual DomainInt initialMin() const {
    return data.initialMin();
  }

  virtual void setMax(DomainInt i) {
    data.setMax(i);
  }

  virtual void setMin(DomainInt i) {
    data.setMin(i);
  }

  virtual void uncheckedAssign(DomainInt b) {
    data.uncheckedAssign(b);
  }

  virtual void assign(DomainInt b) {
    data.assign(b);
  }

  virtual void removeFromDomain(DomainInt b) {
    data.removeFromDomain(b);
  }

  virtual vector<AbstractConstraint*>* getConstraints() {
    return data.getConstraints();
  }

  virtual void addConstraint(AbstractConstraint* c) {
    data.addConstraint(c);
  }

  virtual DomainInt getBaseVal(DomainInt v) const {
    return data.getBaseVal(v);
  }

  virtual vector<Mapper> getMapperStack() const {
    return data.getMapperStack();
  }

  virtual Var getBaseVar() const {
    return data.getBaseVar();
  }

#ifdef WDEG
  virtual DomainInt getBaseWdeg() {
    return data.getBaseWdeg();
  }
  virtual void incWdeg() {
    data.incWdeg();
  }
#endif

  virtual string virtualTostring() {
    return tostring(data);
  }

  virtual ~AnyVarRef_Concrete() {}

  DomainInt getDomainChange(DomainDelta d) {
    return data.getDomainChange(d);
  }

  void addDynamicTrigger(Trig_ConRef t, TrigType type, DomainInt pos = NoDomainValue,
                         TrigOp op = TO_Default) {
    data.addDynamicTrigger(t, type, pos, op);
  }
};

template <>
class AnyVarRef_Concrete<DomainInt> {};

template <typename T>
class AnyVarRef_Concrete<vector<T>> {};

/// Provides a method of wrapping any variable type in a general wrapper.
class AnyVarRef {
public:
  static const BOOL isBool = false;
  static const BoundType isBoundConst = Bound_Maybe;
  unique_ptr<AnyVarRef_Abstract> data;

  BOOL isBound() const {
    return data->isBound();
  }

  AnyVarRef popOneMapper() const {
    return data->popOneMapper();
  }

  template <typename VarRef>
  AnyVarRef(const VarRef& _data) {
    data = unique_ptr<AnyVarRef_Abstract>(new AnyVarRef_Concrete<VarRef>(_data));
  }

  AnyVarRef() {}

  AnyVarRef(const AnyVarRef& b) : data(std::unique_ptr<AnyVarRef_Abstract>(b.data->ptr_clone())) {}

  void operator=(const AnyVarRef& b) {
    this->data = std::unique_ptr<AnyVarRef_Abstract>(b.data->ptr_clone());
  }

  BOOL isAssigned() const {
    return data->isAssigned();
  }

  DomainInt assignedValue() const {
    return data->assignedValue();
  }

  BOOL isAssignedValue(DomainInt i) const {
    return data->isAssigned() && data->assignedValue() == i;
  }

  BOOL inDomain(DomainInt b) const {
    return data->inDomain(b);
  }

  BOOL inDomain_noBoundCheck(DomainInt b) const {
    return data->inDomain_noBoundCheck(b);
  }

  DomainInt domSize() const {
    return data->domSize();
  }

  DomainInt max() const {
    return data->max();
  }

  DomainInt min() const {
    return data->min();
  }

  DomainInt initialMax() const {
    return data->initialMax();
  }

  DomainInt initialMin() const {
    return data->initialMin();
  }

  void setMax(DomainInt i) {
    data->setMax(i);
  }

  void setMin(DomainInt i) {
    data->setMin(i);
  }

  void uncheckedAssign(DomainInt b) {
    data->uncheckedAssign(b);
  }

  void assign(DomainInt b) {
    data->assign(b);
  }

  void removeFromDomain(DomainInt b) {
    data->removeFromDomain(b);
  }

  vector<AbstractConstraint*>* getConstraints() {
    return data->getConstraints();
  }

  void addConstraint(AbstractConstraint* c) {
    data->addConstraint(c);
  }

  DomainInt getBaseVal(DomainInt v) const {
    return data->getBaseVal(v);
  }

  Var getBaseVar() const {
    return data->getBaseVar();
  }

  vector<Mapper> getMapperStack() const {
    return data->getMapperStack();
  }

#ifdef WDEG
  DomainInt getBaseWdeg() {
    return data->getBaseWdeg();
  }

  void incWdeg() {
    data->incWdeg();
  }
#endif

  friend std::ostream& operator<<(std::ostream& o, const AnyVarRef& avr) {
    return o << "AnyVarRef:" << avr.data->virtualTostring();
  }

  DomainInt getDomainChange(DomainDelta d) {
    return data->getDomainChange(d);
  }

  void addDynamicTrigger(Trig_ConRef t, TrigType type, DomainInt pos = NoDomainValue,
                         TrigOp op = TO_Default) {
    data->addDynamicTrigger(t, type, pos, op);
  }

  friend bool operator==(const AnyVarRef& lhs, const AnyVarRef& rhs) {
    return lhs.getBaseVar() == rhs.getBaseVar();
  }

  friend bool operator!=(const AnyVarRef& lhs, const AnyVarRef& rhs) {
    return lhs.getBaseVar() != rhs.getBaseVar();
  }

  friend bool operator<(const AnyVarRef& lhs, const AnyVarRef& rhs) {
    return lhs.getBaseVar() < rhs.getBaseVar();
  }
};

namespace std {
template <>
struct hash<AnyVarRef> {
  size_t operator()(const AnyVarRef& avr) const {
    return getHash(avr.getBaseVar());
  }
};
} // namespace std

template <typename VarRef>
AnyVarRef AnyVarRef_Concrete<VarRef>::popOneMapper() const {
  return data.popOneMapper();
}

template <typename T, typename U>
struct commonVarType2 {
  typedef AnyVarRef type;
};

template <typename T>
struct commonVarType2<T, T> {
  typedef T type;
};

template <typename T, typename U, typename V>
struct commonVarType3 {
  typedef AnyVarRef type;
};

template <typename T>
struct commonVarType3<T, T, T> {
  typedef T type;
};

template <typename T>
struct make_AnyVarRef_type {
  typedef AnyVarRef type;
};

template <typename T>
struct make_AnyVarRef_type<vector<T>> {
  typedef vector<typename make_AnyVarRef_type<T>::type> type;
};

template <typename T, size_t i>
struct make_AnyVarRef_type<std::array<T, i>> {
  typedef vector<typename make_AnyVarRef_type<T>::type> type;
};
/*
template<typename T>
typename make_AnyVarRef_type<T>::type
make_AnyVarRef(T t)
{
  return AnyVarRef(t);
}
*/
template <typename T>
typename make_AnyVarRef_type<vector<T>>::type make_AnyVarRef(vector<T> t) {
  vector<AnyVarRef> v;
  for(size_t i = 0; i < t.size(); ++i)
    v.push_back(AnyVarRef(t[i]));
  return v;
}

template <typename T, size_t param>
typename make_AnyVarRef_type<std::array<T, param>>::type make_AnyVarRef(std::array<T, param> t) {
  vector<AnyVarRef> v;
  for(size_t i = 0; i < t.size(); ++i)
    v.push_back(AnyVarRef(t[i]));
  return v;
}

inline std::ostream& json_dump(const AnyVarRef& v, std::ostream& o) {
  o << "{";
  if(v.isAssigned()) {
    o << '"' << getBaseVarName(v) << "\":" << v.assignedValue();
  } else {
    add_varDomTo_json(v, o);
  }
  return o << "}";
}


#endif
