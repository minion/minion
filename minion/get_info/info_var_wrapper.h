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

#include "../triggering/constraint_abstract.h"

#define VAR_INFO_PRINT_0(X, Y)
#define VAR_INFO_PRINT_1(X, Y, Z)

template <typename WrapType, Info_VarType VAR_TYPE>
struct InfoRefType {
  WrapType data;

  static const BOOL isBool = WrapType::isBool;
  static const BoundType isBoundConst = WrapType::isBoundConst;

  BOOL isBound() const {
    return data.isBound();
  }

  // Here we just pass through, as we don't know if this is the bottom or not.
  AnyVarRef popOneMapper() const {
    return data.popOneMapper();
  }

  InfoRefType(const WrapType& _data) : data(_data) {
    VAR_INFO_ADDONE(VAR_TYPE, copy);
  }

  InfoRefType() {
    VAR_INFO_ADDONE(VAR_TYPE, construct);
  }

  InfoRefType(const InfoRefType& b) : data(b.data) {
    VAR_INFO_ADDONE(VAR_TYPE, copy);
  }

  bool isAssigned() const {
    VAR_INFO_ADDONE(VAR_TYPE, isAssigned);
    bool assign = data.isAssigned();
    VAR_INFO_PRINT_0("Assigned", assign);
    return assign;
  }

  DomainInt getAssignedValue() const {
    VAR_INFO_ADDONE(VAR_TYPE, getAssignedValue);
    DomainInt assignValue = data.getAssignedValue();
    VAR_INFO_PRINT_0("isAssignedValue", assignValue);
    return assignValue;
  }

  bool isAssignedValue(DomainInt i) const {
    VAR_INFO_ADDONE(VAR_TYPE, isAssignedValue);
    bool isAssignValue = data.isAssignedValue(i);
    VAR_INFO_PRINT_1("is assigned ", i, isAssignValue);
    return isAssignValue;
  }

  bool inDomain(DomainInt b) const {
    VAR_INFO_ADDONE(VAR_TYPE, inDomain);
    bool indom = data.inDomain(b);
    VAR_INFO_PRINT_1("in domain", b, indom);
    return indom;
  }

  bool inDomain_noBoundCheck(DomainInt b) const {
    VAR_INFO_ADDONE(VAR_TYPE, inDomain_noBoundCheck);
    bool indom_noBC = data.inDomain_noBoundCheck(b);
    VAR_INFO_PRINT_1("in domain, no bound check", b, indom_noBC);
    return indom_noBC;
  }

  DomainInt getDomSize() const {
    VAR_INFO_ADDONE(VAR_TYPE, getDomSize);
    DomainInt domval = data.getDomSize();
    VAR_INFO_PRINT_0("GetDomSize", domval);
    return domval;
  }

  DomainInt getMax() const {
    VAR_INFO_ADDONE(VAR_TYPE, getMax);
    DomainInt maxval = data.getMax();
    VAR_INFO_PRINT_0("GetMax", maxval);
    return maxval;
  }

  DomainInt getMin() const {
    VAR_INFO_ADDONE(VAR_TYPE, getMin);
    DomainInt minval = data.getMin();
    VAR_INFO_PRINT_0("GetMin", minval);
    return minval;
  }

  DomainInt getInitialMax() const {
    VAR_INFO_ADDONE(VAR_TYPE, getInitialMax);
    DomainInt initialMax = data.getInitialMax();
    VAR_INFO_PRINT_0("InitialMax", initialMax);
    return initialMax;
  }

  DomainInt getInitialMin() const {
    VAR_INFO_ADDONE(VAR_TYPE, getInitialMin);
    DomainInt initialMin = data.getInitialMin();
    VAR_INFO_PRINT_0("InitialMin", initialMin);
    return initialMin;
  }

  void setMax(DomainInt i) {
    VAR_INFO_ADDONE(VAR_TYPE, setMax);
    VAR_INFO_PRINT_0("SetMax", i);
    data.setMax(i);
  }

  void setMin(DomainInt i) {
    VAR_INFO_ADDONE(VAR_TYPE, setMin);
    VAR_INFO_PRINT_0("SetMin", i);
    data.setMin(i);
  }

  void uncheckedAssign(DomainInt b) {
    VAR_INFO_ADDONE(VAR_TYPE, uncheckedAssign);
    VAR_INFO_PRINT_0("uncheckedAssign", b);
    data.uncheckedAssign(b);
  }

  void assign(DomainInt b) {
    VAR_INFO_ADDONE(VAR_TYPE, assign);
    VAR_INFO_PRINT_0("assign", b);
    data.assign(b);
  }

  void removeFromDomain(DomainInt b) {
    VAR_INFO_ADDONE(VAR_TYPE, RemoveFromDomain);
    VAR_INFO_PRINT_0("removeFromDomain", b);
    data.removeFromDomain(b);
  }

  vector<AbstractConstraint*>* getConstraints() {
    VAR_INFO_ADDONE(VAR_TYPE, getConstraints);
    return data.getConstraints();
  }

  void addConstraint(AbstractConstraint* c) {
    VAR_INFO_ADDONE(VAR_TYPE, addConstraint);
    data.addConstraint(c);
  }

  DomainInt getBaseVal(DomainInt v) const {
    VAR_INFO_ADDONE(VAR_TYPE, getBaseVal);
    return data.getBaseVal(v);
  }

  vector<Mapper> getMapperStack() const {
    VAR_INFO_ADDONE(VAR_TYPE, getMapperStack);
    return data.getMapperStack();
  }

  Var getBaseVar() const {
    VAR_INFO_ADDONE(VAR_TYPE, getBaseVar);
    return data.getBaseVar();
  }

#ifdef WDEG
  DomainInt getBaseWdeg() {
    VAR_INFO_ADDONE(VAR_TYPE, getBaseWdeg);
    return data.getBaseWdeg();
  }

  void incWdeg() {
    VAR_INFO_ADDONE(VAR_TYPE, incWdeg);
    data.incWdeg();
  }
#endif

  friend std::ostream& operator<<(std::ostream& o, const InfoRefType& ir) {
    return o << "InfoRef " << ir.data;
  }

  DomainInt getDomainChange(DomainDelta d) {
    VAR_INFO_ADDONE(VAR_TYPE, getDomainChange);
    return d.XXX_get_domain_diff();
  }

  void addDynamicTrigger(Trig_ConRef tcr, TrigType type, DomainInt pos = NoDomainValue,
                         TrigOp op = TO_Default) {
    VAR_INFO_ADDONE(VAR_TYPE, addDynamicTrigger);
    data.addDynamicTrigger(tcr, type, pos, op);
  }
};
