// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef CONSTANT_VAR_FDSK
#define CONSTANT_VAR_FDSK

class AbstractConstraint;

struct ConstantVar {
  // TODO: This really only needs enough to get 'fail'

  static const BOOL isBool = false;
  static const BoundType isBoundConst = Bound_No;

  // Hmm.. no sure if it's better to make this true or false.
  BOOL isBound() const {
    return false;
  }

  DomainInt val;

  AnyVarRef popOneMapper() const {
    FATAL_REPORTABLE_ERROR();
  }

  explicit ConstantVar(DomainInt _val) : val(_val) {}

  ConstantVar() : val(0) {}

  ConstantVar(const ConstantVar& b) : val(b.val) {}

  void operator=(const ConstantVar& v) {
    val = v.val;
  }

  BOOL isAssigned() const {
    return true;
  }

  DomainInt assignedValue() const {
    return val;
  }

  BOOL isAssignedValue(DomainInt i) const {
    return i == val;
  }

  BOOL inDomain(DomainInt b) const {
    return b == val;
  }

  BOOL inDomain_noBoundCheck(DomainInt b) const {
    D_ASSERT(b == val);
    return true;
  }

  DomainInt domSize() const {
    return 1;
  }

  DomainInt max() const {
    return val;
  }

  DomainInt min() const {
    return val;
  }

  DomainInt initialMax() const {
    return val;
  }

  DomainInt initialMin() const {
    return val;
  }

  void setMax(DomainInt i) {
    if(i < val)
      getState().setFailed();
  }

  void setMin(DomainInt i) {
    if(i > val)
      getState().setFailed();
  }

  void uncheckedAssign(DomainInt) {
    FAIL_EXIT();
  }

  void assign(DomainInt b) {
    if(b != val)
      getState().setFailed();
  }

  void removeFromDomain(DomainInt b) {
    if(b == val)
      getState().setFailed();
  }

  void addDynamicTrigger(Trig_ConRef t, TrigType, DomainInt = NoDomainValue,
                         TrigOp op = TO_Default) {
    attachTriggerToNullList(t, op);
  }

  vector<AbstractConstraint*>* getConstraints() {
    return NULL;
  }

  void addConstraint(AbstractConstraint* c) {
    ;
  }

  DomainInt getBaseVal(DomainInt v) const {
    D_ASSERT(v == val);
    return val;
  }

  Var getBaseVar() const {
    return Var(VAR_CONSTANT, val);
  }

  vector<Mapper> getMapperStack() const {
    return vector<Mapper>();
  }

#ifdef WDEG
  DomainInt getBaseWdeg() {
    return 0;
  } // wdeg is irrelevant for non-search var

  void incWdeg() {
    ;
  }
#endif

  DomainInt getDomainChange(DomainDelta d) {
    D_ASSERT(d.XXX_getDomain_diff() == 0);
    return 0;
  }

  friend std::ostream& operator<<(std::ostream& o, const ConstantVar& constant) {
    return o << "Constant" << constant.val;
  }
};

#endif
