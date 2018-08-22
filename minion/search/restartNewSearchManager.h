#include "SearchManager.h"

namespace Controller {
struct RestartNewSearchManager : public Controller::SearchManager {
  PropagationLevel prop_method;
  vector<SearchOrder> initial_order;

  void doASearch(const vector<SearchOrder>& order, int backtracklimit) {
    bool timeout = false;

    int depth = Controller::get_world_depth();
    Controller::world_push();


    auto prop = Controller::make_propagator(prop_method);
    auto vo = Controller::make_search_order_multiple(order);

    std::shared_ptr<Controller::StandardSearchManager> sm;

    int initial_backtracks = getState().getBacktrackCount();

    //std::cout << "Starting search\n";

    auto timeoutChecker = [&](const vector<AnyVarRef>& var_array,
                              const vector<Controller::triple>& branches) {
      try{
        Controller::standard_time_ctrlc_checks(var_array, branches);
      } catch(EndOfSearch&){
        timeout = true;
        throw EndOfSearch();
      }
      
      if(getState().getBacktrackCount() - initial_backtracks > backtracklimit)
        throw EndOfSearch();
    };

    bool solutionFound = false;

    auto solutionHandler = [&]() { solutionFound=true; throw EndOfSearch();};

    auto optimisationHandler = [&]() { /* insert optimisation handling here */ };

    sm = make_shared<Controller::StandardSearchManager>(vo, prop, timeoutChecker, solutionHandler,
                                                        optimisationHandler);

    try {
      sm->search();
    } catch(EndOfSearch&) {}


    double cputime = get_cpu_time();
    double timelimit = getOptions().time_limit;

    if(solutionFound) {
        //cout << "Solution found, stop the search" << endl;
         throw EndOfSearch();
    } else if (timeout){
        if (getOptions().timeout_active && get_cpu_time() > getOptions().time_limit)
            cout << "Time limit is reached, stop the search" << endl;
        else        
            cout << "Node limit is reached, stop the search" << endl;
        throw EndOfSearch();
    }

    Controller::world_pop_to_depth(depth);
  }

  RestartNewSearchManager(PropagationLevel _prop_method, const vector<SearchOrder>& _order)
      : prop_method(_prop_method), initial_order(_order) {}

vector<SearchOrder> makeRandomWalkSearchOrder(int bias) {
    vector<SearchOrder> searchOrder(initial_order);
    for(auto& so : searchOrder) {
      for(int i = 0; i < so.val_order.size(); i++) {
        so.val_order[i] = ValOrder(VALORDER_RANDOM, bias);
      }
    }
    return searchOrder;
  }

  virtual void search() {
    for(int i = 10; i < 10000000; i *= 1.5) {
       cout << "Increasing backtrack limit to " << i << endl;
        int bias = random()%200 - 100;
        vector<SearchOrder> new_order = makeRandomWalkSearchOrder(bias);
        doASearch(new_order, i);
    }
  }
};

shared_ptr<Controller::SearchManager> make_restart_new_search_manager(PropagationLevel prop_method,
                                                      vector<SearchOrder> order) {
  return shared_ptr<Controller::SearchManager>(new RestartNewSearchManager(prop_method, order));
}

} // namespace Controller
