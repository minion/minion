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

#include "../../triggering/constraint_abstract.h"

template <typename VarRef, typename ShiftType>
struct ShiftVar {
  static const BOOL isBool = false;
  static const BoundType isBoundConst = VarRef::isBoundConst;
  VarRef data;

  AnyVarRef popOneMapper() const {
    return data;
  }

  BOOL isBound() const {
    return data.isBound();
  }

  ShiftType shift;
  ShiftVar(const VarRef& _data, ShiftType _shift) : data(_data), shift(_shift) {}

  ShiftVar() : data(), shift() {}

  ShiftVar(const ShiftVar& b) : data(b.data), shift(b.shift) {}

  BOOL isAssigned() const {
    return data.isAssigned();
  }

  DomainInt assignedValue() const {
    return data.assignedValue() + shift;
  }

  BOOL isAssignedValue(DomainInt i) const {
    return data.assignedValue() == i - shift;
  }

  BOOL inDomain(DomainInt i) const {
    return data.inDomain(i - shift);
  }

  BOOL inDomain_noBoundCheck(DomainInt i) const {
    return data.inDomain(i - shift);
  }

  DomainInt domSize() const {
    return data.domSize();
  }

  DomainInt max() const {
    return data.max() + shift;
  }

  DomainInt min() const {
    return data.min() + shift;
  }

  DomainInt initialMax() const {
    return data.initialMax() + shift;
  }

  DomainInt initialMin() const {
    return data.initialMin() + shift;
  }

  void setMax(DomainInt i) {
    data.setMax(i - shift);
  }

  void setMin(DomainInt i) {
    data.setMin(i - shift);
  }

  void uncheckedAssign(DomainInt b) {
    data.uncheckedAssign(b - shift);
  }

  void assign(DomainInt b) {
    data.assign(b - shift);
  }

  void removeFromDomain(DomainInt b) {
    data.removeFromDomain(b - shift);
  }

  void addDynamicTrigger(Trig_ConRef t, TrigType type, DomainInt pos = NoDomainValue,
                         TrigOp op = TO_Default) {
    switch(type) {
    case UpperBound: data.addDynamicTrigger(t, UpperBound, pos, op); break;
    case LowerBound: data.addDynamicTrigger(t, LowerBound, pos, op); break;
    case Assigned:
    case DomainChanged: data.addDynamicTrigger(t, type, pos, op); break;
    case DomainRemoval: data.addDynamicTrigger(t, DomainRemoval, pos - shift, op); break;
    default: D_FATAL_ERROR("Broken dynamic trigger");
    }
  }

  friend std::ostream& operator<<(std::ostream& o, const ShiftVar& sv) {
    return o << "Shift " << sv.data << "+" << sv.shift;
  }

  DomainInt getDomainChange(DomainDelta d) {
    return data.getDomainChange(d);
  }

  vector<AbstractConstraint*>* getConstraints() {
    return data.getConstraints();
  }

  void addConstraint(AbstractConstraint* c) {
    data.addConstraint(c);
  }

  DomainInt getBaseVal(DomainInt v) const {
    return data.getBaseVal(v - shift);
  }

  Var getBaseVar() const {
    return data.getBaseVar();
  }

  vector<Mapper> getMapperStack() const {
    vector<Mapper> v = data.getMapperStack();
    v.push_back(Mapper(MAP_SHIFT, (DomainInt)shift));
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

template <typename T, typename U>
struct ShiftType {
  typedef ShiftVar<T, U> type;
};

template <typename T, typename U>
struct ShiftType<vector<T>, U> {
  typedef vector<ShiftVar<T, U>> type;
};

template <typename T, std::size_t i, typename U>
struct ShiftType<std::array<T, i>, U> {
  typedef std::array<ShiftVar<T, U>, i> type;
};

template <typename VRef, typename Shift>
typename ShiftType<VRef, Shift>::type ShiftVarRef(VRef var_ref, Shift shift) {
  return ShiftVar<VRef, Shift>(var_ref, shift);
}

template <typename VarRef, typename Shift>
vector<ShiftVar<VarRef, Shift>> ShiftVarRef(const vector<VarRef>& var_array, const Shift& shift) {
  vector<ShiftVar<VarRef, Shift>> shift_array(var_array.size());
  for(UnsignedSysInt i = 0; i < var_array.size(); ++i)
    shift_array[i] = ShiftVarRef(var_array[i], shift);
  return shift_array;
}

template <typename VarRef, typename Shift, std::size_t i>
std::array<ShiftVar<VarRef, Shift>, i> ShiftVarRef(const std::array<VarRef, i>& var_array,
                                                   const Shift& shift) {
  std::array<ShiftVar<VarRef, Shift>, i> shift_array;
  for(UnsignedSysInt l = 0; l < i; ++l)
    shift_array[l] = ShiftVarRef(var_array[l], shift);
  return shift_array;
}
