// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#include "../system/system.h"
#include "SearchManager.h"
#include "TabSearchManager.h"
#include "variable_orderings.h"

namespace Controller {

shared_ptr<VariableOrder> makeSearchOrder(SearchOrder order) {
  // collect the variables in the SearchOrder object
  vector<AnyVarRef> varArray;
  for(SysInt i = 0; i < (SysInt)order.varOrder.size(); i++) {
    varArray.push_back(getAnyVarRefFromVar(order.varOrder[i]));
    // some check here?
  }

  VariableOrder* vo;
  VariableOrder* vo2;

  switch(order.order) // get the VarOrderEnum
  {
  case ORDER_STATIC: vo = new StaticBranch(varArray, order.valOrder); break;
  case ORDER_ORIGINAL: vo = new StaticBranch(varArray, order.valOrder); break;
  case ORDER_SDF: vo = new SDFBranch(varArray, order.valOrder); break;
  case ORDER_SRF: vo = new SRFBranch(varArray, order.valOrder); break;
  case ORDER_LDF: vo = new LDFBranch(varArray, order.valOrder); break;
  case ORDER_CONFLICT:
    // for the time being, just use static as the underlying order
    vo2 = new StaticBranch(varArray, order.valOrder);
    vo = new ConflictBranch(varArray, order.valOrder, vo2);
    break;

#ifdef WDEG
  case ORDER_WDEG: vo = new WdegBranch(varArray, order.valOrder); break;
  case ORDER_DOMOVERWDEG: vo = new DomOverWdegBranch(varArray, order.valOrder); break;
#else
  case ORDER_WDEG:
  case ORDER_DOMOVERWDEG:
    USER_ERROR("This minion was not compiled with support for wdeg or "
               "domoverwdeg orderings (add -WDEG to build options)");
#endif

  default: cout << "Order not found in makeSearchOrder." << endl; abort();
  }
  return shared_ptr<VariableOrder>(vo);
}

shared_ptr<VariableOrder> makeSearchOrder_multiple(const vector<SearchOrder>& order) {
  shared_ptr<VariableOrder> vo;
  bool hasAux = false;

  if(order.size() == 1) {
    return makeSearchOrder(order[0]);
  } else {
    vector<shared_ptr<VariableOrder>> vovector;
    for(SysInt i = 0; i < (SysInt)order.size(); i++) {
      vovector.push_back(makeSearchOrder(order[i]));
      if(order[i].findOneAssignment)
        hasAux = true;
      if(order[i].findOneAssignment && i != (SysInt)order.size() - 1) {
        cout << "Only one VARORDER AUX is allowed, and it must be the final "
                "VARORDER command."
             << endl;
        abort();
      }
    }

    vo = shared_ptr<VariableOrder>(new MultiBranch(vovector, hasAux));
  }

  return vo;
}

shared_ptr<Propagate> make_propagator(PropagationLevel propMethod) {
  shared_ptr<Propagate> p;
  switch(propMethod.type) { // doesn't cover the PropLevel_None case.
  case PropLevel_GAC: p = shared_ptr<Propagate>(new PropGAC(propMethod)); break;
  case PropLevel_SAC: p = shared_ptr<Propagate>(new PropSAC(propMethod)); break;
  case PropLevel_SSAC: p = shared_ptr<Propagate>(new PropSSAC(propMethod)); break;
  case PropLevel_SACBounds: p = shared_ptr<Propagate>(new PropSAC_Bounds(propMethod)); break;
  case PropLevel_SSACBounds: p = shared_ptr<Propagate>(new PropSSAC_Bounds(propMethod)); break;
  default: cout << "Propagation method not found in makeSearch_manager." << endl; abort();
  }
  return p;
}

// returns an instance of SearchManager with the required variable ordering,
// propagator etc.
shared_ptr<SearchManager> makeSearch_manager(PropagationLevel propMethod,
                                              vector<SearchOrder> order) {
  shared_ptr<VariableOrder> vo;

  vo = makeSearchOrder_multiple(order);

  shared_ptr<Propagate> p = make_propagator(propMethod);

  std::function<void(void)> opt_handler;
  if(getState().isOptimisationProblem()) {
    opt_handler = []() {
      const auto& vals = getState().getOptimiseValues();
      auto& vars = getState().getOptimiseVars();
      // vals.size == 0 before an optimisation value found
      D_ASSERT(vals.size() == 0 || vals.size() == vars.size());
      for(int i = 0; i < vals.size(); ++i) {
        vars[i].setMin(vals[i]);
        if(!(vars[i].isAssigned() && vars[i].assignedValue() == vals[i])) {
          return;
        }
      }
    };
  } else {
    opt_handler = []() {};
  }

  // need to switch here for different search algorthms. plain, parallel, group
  
  if(getOptions().tabulationMode) {
    shared_ptr<SearchManager> sm(new TabSearchManager(vo, p, standardTime_ctrlc_checks,
                                                         standard_dealWith_solution, opt_handler));
    return sm;
  }
  else {
    shared_ptr<SearchManager> sm(new StandardSearchManager(vo, p, standardTime_ctrlc_checks,
                                                         standard_dealWith_solution, opt_handler));
    return sm;
  }
}
} // namespace Controller
