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

#include <vector>

template <typename VarArrayType>
struct CollectEvents : public AbstractConstraint {
  VarArrayType varArray;

public:
  std::vector<std::pair<int, DomainInt>> assignments;

  CollectEvents(const VarArrayType& _varArray) : varArray(_varArray) {}

  virtual string constraintName() {
    return "collectevents";
  }

  CONSTRAINT_ARG_LIST1(varArray);

  SysInt dynamicTriggerCount() {
    return varArray.size();
  }

  typedef typename VarArrayType::value_type VarRef;

  virtual void propagateDynInt(SysInt trig, DomainDelta) {
    //  Trig is the assigned variable.
    assignments.push_back(std::make_pair(trig, varArray[trig].assignedValue()));
  }

  virtual void fullPropagate() {
    // Set up triggers.
    for(int i = 0; i < (SysInt)varArray.size(); i++) {
      moveTriggerInt(varArray[i], i, Assigned);
    }
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt arraySize) {
    return true;
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> vars;
    vars.reserve(varArray.size());
    for(UnsignedSysInt i = 0; i < varArray.size(); ++i)
      vars.push_back(varArray[i]);
    return vars;
  }

  void liftTriggersLessEqual(int i) {
    //  Get rid of triggers on variables up to i.
    for(int j = 0; j <= i; j++) {
      releaseTriggerInt(j);
    }
  }
};

template <typename VarArray>
AbstractConstraint* BuildCT_COLLECTEVENTS(const VarArray& varArray, ConstraintBlob& b) {
  return new CollectEvents<VarArray>(varArray);
}

// This constraint purposefully has a name which is not legal input, to stop it being used
// in input files

/* JSON
{ "type": "constraint",
  "name": "()()collectevents()()",
  "internal_name": "CT_COLLECTEVENTS",
  "args": [ "read_list" ]
}
*/
