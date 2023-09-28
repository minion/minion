// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

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
