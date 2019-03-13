#include "SearchManager.h"

namespace Controller {
struct RestartSearchManager : public Controller::SearchManager {
  PropagationLevel propMethod;
  vector<SearchOrder> initialOrder;

  void doASearch(const vector<SearchOrder>& order, int nodelimit) {
    int depth = Controller::get_world_depth();
    Controller::world_push();

    auto prop = Controller::make_propagator(propMethod);
    auto vo = Controller::makeSearchOrder_multiple(order);

    std::shared_ptr<Controller::StandardSearchManager> sm;

    bool timeout = false;

    int initial_nodes = getState().getNodeCount();

    std::cout << "Starting search\n";

    auto timeoutChecker = [&](const vector<AnyVarRef>& varArray,
                              const vector<Controller::triple>& branches) {
      Controller::standard_time_ctrlc_checks(varArray, branches);
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

    if(Parallel::isCtrlCPressed()
       //||  (getOptions().timeoutActive &&
       //    globalStats.getTotalTimeTaken() >= getOptions().time_limit)
    ) {
      throw EndOfSearch();
    }
    std::cout << "Did a search " << timeout << "\n";
    Controller::world_popToDepth(depth);
  }

  RestartSearchManager(PropagationLevel _propMethod, const vector<SearchOrder>& _order)
      : propMethod(_propMethod), initialOrder(_order) {}

  virtual void search() {
    std::uniform_int_distribution<int> order(1, 7);
    for(int i = 1; i < 10000000; i *= 10) {
      vector<SearchOrder> new_order = initialOrder;
      for(int j = 0; j < new_order.size(); ++j) {
        new_order[j].order = (VarOrderEnum)(order(global_random_gen));
      }
      doASearch(new_order, i);
    }
  }
};

shared_ptr<Controller::SearchManager> make_restart_search_manager(PropagationLevel propMethod,
                                                                  vector<SearchOrder> order) {
  return shared_ptr<Controller::SearchManager>(new RestartSearchManager(propMethod, order));
}
} // namespace Controller
