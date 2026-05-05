#include <stdlib.h>

#include "SearchManager.h"

namespace Controller {
struct RestartNewSearchManager : public Controller::SearchManager {
  PropagationLevel propMethod;
  vector<SearchOrder> initialOrder;

  void doASearch(const vector<SearchOrder>& order, int backtracklimit) {
    bool timeout = false;
    // For optimisation problems each restart attempt is a complete
    // depth-first search under the current best bound (kept across
    // attempts via getState().getOptimiseValues()). Solutions found
    // during a restart tighten the bound exactly as in the standard
    // search; the restart loop only contributes the per-attempt
    // backtrack cap and the per-attempt random ordering shuffle.
    bool isOpt = getState().isOptimisationProblem();

    int depth = Controller::getWorldDepth();
    Controller::worldPush();

    auto prop = Controller::make_propagator(propMethod);
    auto vo = Controller::makeSearchOrder_multiple(order);

    std::shared_ptr<Controller::StandardSearchManager> sm;

    int initial_backtracks = getState().getBacktrackCount();

    auto timeoutChecker = [&](const vector<AnyVarRef>& varArray,
                              const vector<Controller::triple>& branches) {
      try {
        Controller::standardTime_ctrlc_checks(varArray, branches);
      } catch(EndOfSearch&) {
        timeout = true;
        throw EndOfSearch();
      }

      if(getState().getBacktrackCount() - initial_backtracks > backtracklimit)
        throw EndOfSearch();
    };

    bool solutionFound = false;

    auto solutionHandler = [&]() {
      solutionFound = true;
      if(isOpt) {
        // Run the standard post-solution bookkeeping: tightens the
        // bound via setOptimiseValue, fires the LIBMINION callback,
        // checks sollimit (which is -1 for optimisation, so never
        // fires here). Don't throw — let StandardSearchManager
        // backtrack and continue exploring with the new bound, which
        // optimisationHandler picks up at every right-branch step.
        Controller::standard_dealWith_solution();
        return;
      }
      // SAT mode: stop on first solution.
      throw EndOfSearch();
    };

    // Mirror search_control.h's opt_handler — apply the running best
    // bound and the cross-worker shared bound (when running under
    // -X-parallelWorkSteal / -X-parallelThreads).
    auto optimisationHandler = [&]() {
      if(!isOpt) return;
      const auto& vals = getState().getOptimiseValues();
      auto& vars = getState().getOptimiseVars();
      D_ASSERT(vals.size() == 0 || vals.size() == vars.size());
      for(size_t i = 0; i < vals.size(); ++i) {
        vars[i].setMin(vals[i]);
        if(!(vars[i].isAssigned() && vars[i].assignedValue() == vals[i])) {
          return;
        }
      }
      if(getOptions().parallelBoundChannel != nullptr && vars.size() == 1) {
        auto* shared = static_cast<std::atomic<long long>*>(
            getOptions().parallelBoundChannel);
        long long sentinel = std::numeric_limits<long long>::min();
        long long s = shared->load(std::memory_order_relaxed);
        if(s != sentinel && DomainInt(checked_cast<SysInt>(s)) > vars[0].min()) {
          vars[0].setMin(DomainInt(checked_cast<SysInt>(s)));
        }
      }
    };

    sm = make_shared<Controller::StandardSearchManager>(vo, prop, timeoutChecker, solutionHandler,
                                                        optimisationHandler);

    bool exhausted = false;
    try {
      sm->search();
      // sm->search returned naturally → the entire tree has been
      // explored (under the bound that was active by the last node).
      exhausted = true;
    } catch(EndOfSearch&) {
      // Either timeoutChecker fired (timeout=true), or solutionHandler
      // threw in SAT mode (solutionFound=true), or some other early
      // termination (LIBMINION callback / ctrl-C).
    }

    if(isOpt) {
      // Optimisation: each restart's tree is searched from scratch
      // under the running bound, so a natural exhaustion means no
      // further improving solution exists — the bound recorded in
      // State (and lastOptimumValues) is the optimum. Tell the outer
      // loop we're done.
      if(exhausted) {
        Controller::worldPopToDepth(depth);
        throw EndOfSearch();
      }
      // Otherwise the inner search hit the backtrack cap (or an
      // external stop signal). For backtrack cap, fall through and
      // let the outer loop restart with a bigger limit. For external
      // stops, propagate.
      if(timeout) {
        // The "timeout" flag here covers both the backtrack-limit
        // hit AND a real -timelimit/-cpulimit/ctrl-C. Distinguish
        // by checking whether standardTime_ctrlc_checks set the
        // shared TimeOut flag (the real-time path also writes
        // TableOut "TimeOut"=1 and prints "Time out.").
        // Backtrack-cap hits don't set TableOut TimeOut.
        try {
          // Re-run the time/ctrl-C check; if it would still raise
          // EndOfSearch, we know it was a real timeout (or ctrl-C),
          // not just our backtrack cap.
          vector<AnyVarRef> empty;
          vector<Controller::triple> empty2;
          Controller::standardTime_ctrlc_checks(empty, empty2);
          // No real timeout — backtrack-cap hit. Fall through to
          // restart loop.
        } catch(EndOfSearch&) {
          Controller::worldPopToDepth(depth);
          throw EndOfSearch();
        }
      }
      Controller::worldPopToDepth(depth);
      return;
    }

    // SAT mode (existing behaviour).
    if(solutionFound) {
      throw EndOfSearch();
    } else if(timeout) {
      if(getOptions().timeoutActive && get_cpuTime() > getOptions().time_limit)
        cout << "Time limit is reached, stop the search" << endl;
      else
        cout << "Node limit is reached, stop the search" << endl;
      throw EndOfSearch();
    }

    Controller::worldPopToDepth(depth);
  }

  RestartNewSearchManager(PropagationLevel _propMethod, const vector<SearchOrder>& _order)
      : propMethod(_propMethod), initialOrder(_order) {}

  vector<SearchOrder> makeRandomWalkSearchOrder(int bias) {
    vector<SearchOrder> searchOrder(initialOrder);
    for(auto& so : searchOrder) {
      for(int i = 0; i < so.valOrder.size(); i++) {
        so.valOrder[i] = ValOrder(VALORDER_RANDOM, bias);
      }
    }
    return searchOrder;
  }

  virtual void search() {
    bool useBias = getOptions().restart.bias;
    double multiplier = getOptions().restart.multiplier;

    unsigned long long i = 10;
    while(true) {
      i *= multiplier;
      if(i > (1LL << 60)) {
        i = 1LL << 60;
      }
      if(!getOptions().silent) {
        cout << "Increasing backtrack limit to " << i << endl;
      }
      int bias = 0;
      if(useBias)
        bias = rand() % 200 - 100;
      vector<SearchOrder> new_order = makeRandomWalkSearchOrder(bias);
      doASearch(new_order, i);
    }
  }
};

shared_ptr<Controller::SearchManager> make_restart_new_search_manager(PropagationLevel propMethod,
                                                                      vector<SearchOrder> order) {
  return shared_ptr<Controller::SearchManager>(new RestartNewSearchManager(propMethod, order));
}

} // namespace Controller
