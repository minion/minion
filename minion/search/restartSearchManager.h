#include "SearchManager.h"

namespace Controller {
struct RestartSearchManager : public Controller::SearchManager {
  PropagationLevel prop_method;
  vector<SearchOrder> initial_order;

  void doASearch(const vector<SearchOrder>& order, int nodelimit) {
    int depth = Controller::get_world_depth();
    Controller::world_push();

    auto prop = Controller::make_propagator(prop_method);
    auto vo = Controller::make_search_order_multiple(order);

    std::shared_ptr<Controller::StandardSearchManager> sm;

    bool timeout = false;

    int initial_nodes = getState().getNodeCount();

    std::cout << "Starting search\n";

    auto timeoutChecker = [&](const vector<AnyVarRef>& var_array,
                              const vector<Controller::triple>& branches) {
      Controller::standard_time_ctrlc_checks(var_array, branches);
      if(alarmTriggered) {
        throw TimeoutException();
      }
      if(getState().getNodeCount() - initial_nodes > nodelimit)
        throw EndOfSearch();
    };

    auto solutionHandler = [&]() { std::cout << "Found a solution!\n"; };

    auto optimisationHandler = [&]() { /* insert optimisation handling here */ };

    sm = make_shared<Controller::StandardSearchManager>(vo, prop, timeoutChecker, solutionHandler,
                                                        optimisationHandler);

    try {
      sm->search();
    } catch(EndOfSearch&) {
    } catch(TimeoutException&) { timeout = true; }

    if(getState().isCtrlcPressed()
       //||  (getOptions().timeout_active &&
       //    globalStats.getTotalTimeTaken() >= getOptions().time_limit)
    ) {
      throw EndOfSearch();
    }
    std::cout << "Did a search " << timeout << "\n";
    Controller::world_pop_to_depth(depth);
  }

  RestartSearchManager(PropagationLevel _prop_method, const vector<SearchOrder>& _order)
      : prop_method(_prop_method), initial_order(_order) {}

  virtual void search() {
    for(int i = 1; i < 10000000; i *= 10) {
        vector<SearchOrder> new_order = initial_order;
        for(int j = 0; j < new_order.size(); ++j) {
            new_order[j].order = (VarOrderEnum)(1 + rand() % 7);
        }
      doASearch(new_order, i);
    }
  }
};

shared_ptr<Controller::SearchManager> make_restart_search_manager(PropagationLevel prop_method,
                                                      vector<SearchOrder> order) {
  return shared_ptr<Controller::SearchManager>(new RestartSearchManager(prop_method, order));
}
}