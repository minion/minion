// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef CHECK_GSA_H_HIUO
#define CHECK_GSA_H_HIUO

#include "../get_info/get_info.h"
#include "../memory_management/reversible_vals.h"
#include "../queue/standard_queue.h"
#include "../triggering/constraint_abstract.h"

#ifdef P
#undef P
#endif

//#define P(x) cout << x << endl
#define P(x)

struct Check_GSA : public AbstractConstraint {
  virtual string extendedName() {
    return constraintName() + ":" + child->extendedName();
  }

  virtual string constraintName() {
    return "check[gsa]";
  }

  CONSTRAINT_ARG_LIST1(child);

  AbstractConstraint* child;

  Check_GSA(AbstractConstraint* _con) : child(_con) {}

  virtual ~Check_GSA() {
    delete child;
  }

  virtual AbstractConstraint* reverseConstraint() {
    return new Check_GSA(child->reverseConstraint());
  }

  virtual SysInt dynamicTriggerCount() {
    return child->getVarsSingleton()->size() * 2;
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    return child->getSatisfyingAssignment(assignment);
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    return child->checkAssignment(v, vSize);
  }

  virtual vector<AnyVarRef> getVars() {
    return child->getVars();
  }

  virtual void propagateDynInt(SysInt, DomainDelta) {
    bool flag = false;
    GET_ASSIGNMENT(assignment, child);
    if(!flag) {
      getState().setFailed();
    } else {
      watch_assignment(assignment, *(child->getVarsSingleton()), 0);
    }
  }

  template <typename T, typename Vars>
  void watch_assignment(const T& assignment, Vars& vars, DomainInt trig) {
    for(SysInt i = 0; i < (SysInt)assignment.size(); ++i) {
      D_ASSERT(vars[assignment[i].first].inDomain(assignment[i].second));
      if(vars[assignment[i].first].isBound()) {
        moveTriggerInt(vars[assignment[i].first], trig + i, DomainChanged);
      } else {
        moveTriggerInt(vars[assignment[i].first], trig + i, DomainRemoval, assignment[i].second);
      }
    }
  }

  virtual void fullPropagate() {
    propagateDynInt(0, DomainDelta::empty());
  }
};

AbstractConstraint* checkGSACon(AbstractConstraint* c) {
  return new Check_GSA(c);
}

inline AbstractConstraint* BuildCT_CHECK_GSA(ConstraintBlob& bl) {
  D_ASSERT(bl.internal_constraints.size() == 1);
  return checkGSACon(build_constraint(bl.internal_constraints[0]));
}

/* JSON
{ "type": "constraint",
  "name": "check[gsa]",
  "internal_name": "CT_CHECK_GSA",
  "args": [ "read_constraint" ]
}
*/

#endif
