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

/** @help variables;01 Description
01 variables are used very commonly for logical expressions, and for
encoding the characteristic functions of sets and relations. Note that
wherever a 01 variable can appear, the negation of that variable can
also appear. A boolean variable x's negation is identified by !x.
*/

/** @help variables;01 Example
Declaration of a 01 variable called bool in input file:

BOOL bool

Use of this variable in a constraint:

eq(bool, 0) #variable bool equals 0
*/

#ifndef _BOOLEANVARIABLES_H
#define _BOOLEANVARIABLES_H

#include "../../system/system.h"

#include "../../memory_management/MemoryBlock.h"

#include "../../triggering/constraint_abstract.h"

/// Standard data type used for storing compressed booleans
typedef unsigned long data_type;
static const data_type one = 1;
static const data_type maxData = one << (sizeof(data_type) - 1);

struct BoolVarContainer;

/// A reference to a boolean variable
struct BoolVarRef_internal {
  static const BOOL isBool = true;
  static const BoundType isBoundConst = Bound_No;
  static string name() {
    return "Bool";
  }
  BOOL isBound() const {
    return false;
  }

  AnyVarRef popOneMapper() const {
    FATAL_REPORTABLE_ERROR();
  }

  data_type shiftOffset;
  SysInt varNum;
  void* data_position;
  void* value_position;

  UnsignedSysInt dataOffset() const {
    return varNum / (sizeof(data_type) * 8);
  }

  static BoolVarContainer& getCon_Static();
  BoolVarRef_internal(const BoolVarRef_internal& b)
      : shiftOffset(b.shiftOffset),
        varNum(b.varNum),
        data_position(b.data_position),
        value_position(b.value_position) {}

  BoolVarRef_internal() : shiftOffset(~1), varNum(~1) {}

  BoolVarRef_internal(DomainInt value, BoolVarContainer* b_con);

  data_type& assign_ptr() const {
    return *static_cast<data_type*>(data_position);
  }

  data_type& valuePtr() const {
    return *static_cast<data_type*>(value_position);
  }

  BOOL isAssigned() const {
    return assign_ptr() & shiftOffset;
  }

  DomainInt assignedValue() const {
    D_ASSERT(isAssigned());
    return (bool)(valuePtr() & shiftOffset);
  }

  BOOL inDomain(DomainInt b) const {
    if((checked_cast<SysInt>(b) | 1) != 1)
      return false;
    return (!isAssigned()) || (b == assignedValue());
  }

  BOOL inDomain_noBoundCheck(DomainInt b) const {
    D_ASSERT(b == 0 || b == 1);
    return (!isAssigned()) || (b == assignedValue());
  }

  DomainInt min() const {
    if(!isAssigned())
      return 0;
    return assignedValue();
  }

  DomainInt domSize() const {
    if(isAssigned())
      return 1;
    else
      return 2;
  }

  DomainInt max() const {
    if(!isAssigned())
      return 1;
    return assignedValue();
  }

  DomainInt initialMin() const {
    return 0;
  }

  DomainInt initialMax() const {
    return 1;
  }

  DomainInt getBaseVal(DomainInt v) const {
    D_ASSERT(inDomain(v));
    return v;
  }

  Var getBaseVar() const {
    return Var(VAR_BOOL, varNum);
  }

  vector<Mapper> getMapperStack() const {
    return vector<Mapper>();
  }

  friend std::ostream& operator<<(std::ostream& o, const BoolVarRef_internal& b) {
    return o << "Bool:" << b.varNum;
  }
};

struct GetBoolVarContainer;

#ifdef MORE_SEARCH_INFO
typedef InfoRefType<QuickVarRefType<GetBoolVarContainer, BoolVarRef_internal>, VAR_INFO_BOOL>
    BoolVarRef;
#else
typedef QuickVarRefType<GetBoolVarContainer, BoolVarRef_internal> BoolVarRef;
#endif

/// Container for boolean variables
struct BoolVarContainer {

  BoolVarContainer() : varCount_m(0), trigger_list(false), lock_m(false) {}

  static const SysInt width = 7;
  ExtendableBlock assignOffset;
  void* values_mem;
  vector<vector<AbstractConstraint*>> constraints;
#ifdef WDEG
  vector<DomainInt> wdegs;
#endif
  UnsignedSysInt varCount_m;
  TriggerList trigger_list;
  /// When false, no variable can be altered. When true, no variables can be
  /// created.
  BOOL lock_m;

  data_type* valuePtr() {
    return static_cast<data_type*>(values_mem);
  }

  const data_type* valuePtr() const {
    return static_cast<const data_type*>(values_mem);
  }

  data_type* assign_ptr() {
    return (data_type*)(assignOffset());
  }

  const data_type* assign_ptr() const {
    return (const data_type*)(assignOffset());
  }

  void lock() {
    lock_m = true;
    // Min domain value = 0, max domain val = 1.
    std::vector<std::pair<DomainInt, DomainInt>> doms(varCount_m,
                                                      make_pair(DomainInt(0), DomainInt(1)));
    trigger_list.addVariables(doms);
  }

  /// Returns a new Boolean Variable.
  // BoolVarRef get_new_var();

  void addVariables(SysInt new_bools) {
    D_ASSERT(!lock_m);
    varCount_m += new_bools;

    SysInt required_mem = varCount_m / 8 + 1;
    // Round up to nearest data_type block
    required_mem += sizeof(data_type) - (required_mem % sizeof(data_type));
    if(assignOffset.empty()) {
      assignOffset = getMemory().backTrack().requestBytesExtendable(required_mem);
      values_mem = checked_malloc(10 * 1024 * 1024);
      CHECK(required_mem < 10 * 1024 * 1024, "Bool mem overflow");
    } else {
      getMemory().backTrack().resizeExtendableBlock(assignOffset, required_mem);
    }
    constraints.resize(varCount_m);
#ifdef WDEG
    wdegs.resize(varCount_m);
#endif
  }

  /// Returns a reference to the ith Boolean variable which was previously
  /// created.
  BoolVarRef getVarNum(DomainInt i);

  UnsignedSysInt varCount() {
    return varCount_m;
  }

  void setMax(const BoolVarRef_internal& d, DomainInt i) {
    if(i < 0) {
      getState().setFailed(true);
      return;
    }

    D_ASSERT(i >= 0);
    if(i == 0)
      assign(d, 0);
  }

  void setMin(const BoolVarRef_internal& d, DomainInt i) {
    if(i > 1) {
      getState().setFailed(true);
      return;
    }
    D_ASSERT(i <= 1);
    if(i == 1)
      assign(d, 1);
  }

  void removeFromDomain(const BoolVarRef_internal& d, DomainInt b) {
    D_ASSERT(lock_m && d.varNum < (SysInt)varCount_m);
    if((checked_cast<SysInt>(b) | 1) != 1)
      return;

    if(d.isAssigned()) {
      if(b == d.assignedValue())
        getState().setFailed(true);
    } else
      uncheckedAssign(d, 1 - b);
  }

  void internalAssign(const BoolVarRef_internal& d, DomainInt b) {
    D_ASSERT(lock_m && d.varNum < (SysInt)varCount_m);
    D_ASSERT(!d.isAssigned());
    if((checked_cast<SysInt>(b) | 1) != 1) {
      getState().setFailed(true);
      return;
    }
    assign_ptr()[d.dataOffset()] |= d.shiftOffset;

    trigger_list.push_assign(d.varNum, b);
    trigger_list.pushDomainChanged(d.varNum);
    trigger_list.pushDomain_removal(d.varNum, 1 - b);

    if(b == 1) {
      trigger_list.push_lower(d.varNum, 1);
      valuePtr()[d.dataOffset()] |= d.shiftOffset;
    } else {
      trigger_list.push_upper(d.varNum, 1);
      valuePtr()[d.dataOffset()] &= ~d.shiftOffset;
    }
  }

  void uncheckedAssign(const BoolVarRef_internal& d, DomainInt b) {
    internalAssign(d, b);
  }

  void assign(const BoolVarRef_internal& d, DomainInt b) {
    if(!d.isAssigned())
      internalAssign(d, b);
    else {
      if(d.assignedValue() != b)
        getState().setFailed(true);
    }
  }

  void addDynamicTrigger(BoolVarRef_internal& b, Trig_ConRef t, TrigType type,
                         DomainInt pos = NoDomainValue, TrigOp op = TO_Default) {
    D_ASSERT(pos == NoDomainValue || (type == DomainRemoval && pos != NoDomainValue));
    D_ASSERT(lock_m);

    trigger_list.addDynamicTrigger(b.varNum, t, type, pos, op);
  }

  vector<AbstractConstraint*>* getConstraints(const BoolVarRef_internal& b) {
    return &constraints[b.varNum];
  }

  void addConstraint(const BoolVarRef_internal& b, AbstractConstraint* c) {
    constraints[b.varNum].push_back(c);
#ifdef WDEG
    wdegs[b.varNum] += checked_cast<SysInt>(c->getWdeg()); // add constraint score to base var wdeg
#endif
  }

#ifdef WDEG
  DomainInt getBaseWdeg(const BoolVarRef_internal& b) {
    return wdegs[b.varNum];
  }

  void incWdeg(const BoolVarRef_internal& b) {
    wdegs[b.varNum]++;
  }
#endif
};

inline BoolVarRef BoolVarContainer::getVarNum(DomainInt i) {
  D_ASSERT(i < (SysInt)varCount_m);
  return BoolVarRef(BoolVarRef_internal(i, this));
}

inline BoolVarRef_internal::BoolVarRef_internal(DomainInt value, BoolVarContainer* b_con)
    : varNum(checked_cast<UnsignedSysInt>(value)),
      data_position((char*)(b_con->assignOffset()) + dataOffset() * sizeof(data_type)),
      value_position((char*)(b_con->values_mem) + dataOffset() * sizeof(data_type)) {
  shiftOffset = one << (checked_cast<UnsignedSysInt>(value) % (sizeof(data_type) * 8));
}

#endif
