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
  VarArrayType var_array;

public:
  std::vector<std::pair<int, int>> assignments;

  CollectEvents(const VarArrayType& _var_array) : var_array(_var_array) {}

  virtual string constraint_name() {
    return "collectevents";
  }

  CONSTRAINT_ARG_LIST1(var_array);

  SysInt dynamic_trigger_count() {
    return var_array.size();
  }

  typedef typename VarArrayType::value_type VarRef;

  virtual void propagateDynInt(SysInt trig, DomainDelta) {
    //  Trig is the assigned variable.
    assignments.push_back(std::make_pair(trig, var_array[trig].assignedValue()));
  }

  virtual void full_propagate() {
    // Set up triggers.
    for(int i = 0; i < (SysInt)var_array.size(); i++) {
      moveTriggerInt(var_array[i], i, Assigned);
    }
  }

  virtual BOOL check_assignment(DomainInt* v, SysInt array_size) {
    return true;
  }

  virtual vector<AnyVarRef> get_vars() {
    vector<AnyVarRef> vars;
    vars.reserve(var_array.size());
    for(UnsignedSysInt i = 0; i < var_array.size(); ++i)
      vars.push_back(var_array[i]);
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
AbstractConstraint* BuildCT_COLLECTEVENTS(const VarArray& var_array, ConstraintBlob& b) {
  return new CollectEvents<VarArray>(var_array);
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
