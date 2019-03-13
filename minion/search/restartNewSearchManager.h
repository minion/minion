#include "SearchManager.h"

namespace Controller {
struct RestartNewSearchManager : public Controller::SearchManager {
  PropagationLevel propMethod;
  vector<SearchOrder> initialOrder;

  void doASearch(const vector<SearchOrder>& order, int backtracklimit) {
    bool timeout = false;

    int depth = Controller::get_world_depth();
    Controller::world_push();

    auto prop = Controller::make_propagator(propMethod);
    auto vo = Controller::makeSearchOrder_multiple(order);

    std::shared_ptr<Controller::StandardSearchManager> sm;

    int initial_backtracks = getState().getBacktrackCount();

    // std::cout << "Starting search\n";

    auto timeoutChecker = [&](const vector<AnyVarRef>& varArray,
                              const vector<Controller::triple>& branches) {
      try {
        Controller::standard_time_ctrlc_checks(varArray, branches);
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
      throw EndOfSearch();
    };

    auto optimisationHandler = [&]() { /* insert optimisation handling here */ };

    sm = make_shared<Controller::StandardSearchManager>(vo, prop, timeoutChecker, solutionHandler,
                                                        optimisationHandler);

    try {
      sm->search();
    } catch(EndOfSearch&) {}

    // double cputime = get_cpu_time();
    // double timelimit = getOptions().time_limit;

    if(solutionFound) {
      // cout << "Solution found, stop the search" << endl;
      throw EndOfSearch();
    } else if(timeout) {
      if(getOptions().timeout_active && get_cpu_time() > getOptions().time_limit)
        cout << "Time limit is reached, stop the search" << endl;
      else
        cout << "Node limit is reached, stop the search" << endl;
      throw EndOfSearch();
    }

    Controller::world_popToDepth(depth);
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

    for(int i = 10; i < 10000000; i *= multiplier) {
      cout << "Increasing backtrack limit to " << i << endl;
      int bias = 0;
      if(useBias)
        bias = random() % 200 - 100;
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
