
#ifndef MINION_NEIGHBOURHOOD_SEARCH_H
#define MINION_NEIGHBOURHOOD_SEARCH_H
#include "NeighbourhoodChoosingStrategies.h"
#include "SearchManager.h"
#include "SearchStrategies.h"
#include "neighbourhood-def.h"
#include <atomic>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <memory>
#include <signal.h>
#include <sys/time.h>


static std::atomic<bool> alarmTriggered(false);

void triggerAlarm(int) {
  alarmTriggered = true;
}
class TimeoutException : public std::exception {};

template <typename SearchStrategy>
struct NeighbourhoodSearchManager : public Controller::SearchManager {
  typedef std::chrono::high_resolution_clock::time_point timePoint;

  shared_ptr<Propagate> prop;
  vector<SearchOrder> base_order;
  NeighbourhoodContainer nhc;
  shared_ptr<SearchStrategy> searchStrategy;

  NeighbourhoodSearchManager(shared_ptr<Propagate> _prop, vector<SearchOrder> _base_order,
                             NeighbourhoodContainer _nhc,
                             shared_ptr<SearchStrategy> searchStrategy)
      : prop(std::move(_prop)),
        base_order(_base_order),
        nhc(_nhc),
        searchStrategy(std::move(searchStrategy)) {
    signal(SIGVTALRM, triggerAlarm);
  }

  inline NeighbourhoodStats searchNeighbourhoods(vector<DomainInt>& solution,
                                                 vector<int>& activatedNeighbourhoods,
                                                 int timeoutInMillis = 0,
                                                 bool restrictToFirstSolution = true) {
    // Save state of the world
    int depth = Controller::get_world_depth();

    debug_log("---In search function---");
    debug_log("Depth is: " << depth);
    Controller::world_push();
    auto vo = Controller::make_search_order_multiple(base_order);
    if(activatedNeighbourhoods.empty()) {
      nhc.shadow_disable.assign(1);
    } else {
      switchOnNeighbourhoods(activatedNeighbourhoods, solution);
    }

    solution.clear();
    prop->prop(vo->getVars());

    if(getState().isFailed()) {
      if(activatedNeighbourhoods.empty()) {
        D_FATAL_ERROR("Problem unsatisfiable with all neighbourhoods turned off");
      } else {
        debug_log("---No Search was carried out---");
        Controller::world_pop_to_depth(depth);
        return NeighbourhoodStats(getState().getOptimiseVar()->getMin(), 0, false, false);
      }
    }

    DomainInt optimisationValueCache = getState().getOptimiseVar()->getMin() - 1;
    // minus 1 is used as the optimisationHandler always sets it to optimisationValueCache +1
    auto timeoutChecker = [](const vector<AnyVarRef>& var_array,
                             const vector<Controller::triple>& branches) {
      Controller::standard_time_ctrlc_checks(var_array, branches);
      if(alarmTriggered) {
        throw TimeoutException();
      }
    };
    auto solutionHandler = [&]() {
      solution.clear();
      for(const auto& var : this->nhc.shadow_mapping[0]) {
        solution.push_back(var.getAssignedValue());
      }
      optimisationValueCache = getState().getOptimiseVar()->getMin();
      if(restrictToFirstSolution) {
        throw EndOfSearch();
      }
    };
    auto optimisationHandler = [&]() {
      getState().getOptimiseVar()->setMin(optimisationValueCache + 1);
    };
    auto sm = make_shared<Controller::StandardSearchManager>(vo, prop, timeoutChecker,
                                                             solutionHandler, optimisationHandler);

    if(timeoutInMillis) {
      setTimeout(timeoutInMillis);
    }

    auto startTime = std::chrono::high_resolution_clock::now();
    bool timeout = false;
    try {
      sm->search();
    } catch(EndOfSearch&) { clearTimeout(); } catch(TimeoutException&) {
      timeout = true;
    }
    bool solutionFound = !solution.empty();
    DomainInt bestOptimisation =
        (solutionFound) ? optimisationValueCache : optimisationValueCache + 1;
    NeighbourhoodStats stats(bestOptimisation, getTimeTaken(startTime), solutionFound, timeout);
    Controller::world_pop_to_depth(depth);
    return stats;
  }



  virtual void search() {
    vector<DomainInt> solution;
    vector<int> activatedNeighbourhoods;
    NeighbourhoodStats stats = searchNeighbourhoods(solution, activatedNeighbourhoods);
    if(!stats.solutionFound) {
      return;
    }
    int numberOfSearches = 0;
    while(searchStrategy->continueSearch(nhc)) {
      activatedNeighbourhoods = searchStrategy->getNeighbourHoodsToActivate(nhc);
      stats = searchNeighbourhoods(solution, activatedNeighbourhoods, 500);
      searchStrategy->updateStats(nhc,prop,activatedNeighbourhoods, stats, solution);

      cout << "Optimsae Variable Bound " <<  getState().getOptimiseVar()->getMin() << "->"
                                             << getState().getOptimiseVar()->getMax() << endl;
    }
  }

  void printWorld() {
    cout << "---------------" << endl;
    cout << "Optimise Variable bound: " << getState().getOptimiseVar()->getMin() << "->"
         << getState().getOptimiseVar()->getMax() << endl;
    for(int i = 0; i < nhc.shadow_mapping[0].size(); i++) {
      cout << "Variable " << nhc.shadow_mapping[0][i] << " : " << nhc.shadow_mapping[0][i].getMin()
           << "->" << nhc.shadow_mapping[0][i].getMax() << endl;
    }

    for(int i = 0; i < nhc.shadow_mapping[0].size(); i++) {
      cout << "Shadow Variable " << nhc.shadow_mapping[1][i] << " : "
           << nhc.shadow_mapping[1][i].getMin() << "->" << nhc.shadow_mapping[0][i].getMax()
           << endl;
    }

    for(int i = 0; i < nhc.neighbourhoods.size(); i++){
      cout << "Neighbourhood " << i << " With domain: " << nhc.neighbourhoods[i].activation.getMin()
           << " -> " << nhc.neighbourhoods[i].activation.getMax() << endl;
    }
    cout << "---------------" << endl;
  }

  /**
   * Switch on the neighbourhood activation vars
   * Find the set of primary variables not contained in any neighbourhoods and assign them to the
   * incumbent solution
   * @param neighbourHoodIndexes
   */
  void switchOnNeighbourhoods(const vector<int>& neighbourHoodIndexes, const vector<DomainInt> &solution) {
    std::unordered_set<AnyVarRef> shadowVariables;
    debug_log("Switiching on neighbourhoods with depth " << Controller::get_world_depth());

    for(int i : neighbourHoodIndexes) {
      debug_log("Switching on neighbourhood " << i << " With domain: " << nhc.neighbourhoods[i].activation.getMin()
                                                 << " -> " << nhc.neighbourhoods[i].activation.getMax());
      Neighbourhood& neighbourhood = nhc.neighbourhoods[i];
      neighbourhood.activation.assign(1);

      for(auto& n : neighbourhood.vars) {
        shadowVariables.insert(n);
      }
    }


    debug_code(for (int i = 0; i < nhc.neighbourhoods.size(); i++){
      cout << "Neighbourhood " << i << " With domain: " << nhc.neighbourhoods[i].activation.getMin()
           << " -> " << nhc.neighbourhoods[i].activation.getMax() << endl;
    });

    /*
     * If the primary variable is not contained in any shadow neighbourhoods, assign its incumbent
     * solution
     */
    for(int i = 0; i < solution.size(); i++){
      if(shadowVariables.count(nhc.shadow_mapping[0][i]) == 0) {
        nhc.shadow_mapping[0][i].assign(solution[i]);
      }
    }
  }


  inline void setTimeout(int numberMillis) {
    struct itimerval timer;
    // numberSeconds
    timer.it_value.tv_sec = numberMillis / 1000;
    // remaining micro seconds
    timer.it_value.tv_usec = (numberMillis % 1000) * 1000;
    // prevent intervals
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    alarmTriggered = false;
    setitimer(ITIMER_VIRTUAL, &timer, NULL);
  }

  inline void clearTimeout() {
    struct itimerval timer;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    alarmTriggered = false;
    setitimer(ITIMER_VIRTUAL, &timer, NULL);
  }

  inline u_int64_t getTimeTaken(timePoint startTime) {
    auto endTime = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
  }
};

shared_ptr<Controller::SearchManager> MakeNeighbourhoodSearch(PropagationLevel prop_method,
                                                              vector<SearchOrder> base_order,
                                                              NeighbourhoodContainer nhc) {
  shared_ptr<Propagate> prop = Controller::make_propagator(prop_method);
  return std::make_shared<NeighbourhoodSearchManager<HillClimbingSearch<UCBNeighborHoodSelection>>>(
      prop, base_order, nhc, std::make_shared<HillClimbingSearch<UCBNeighborHoodSelection>>(nhc, std::make_shared<UCBNeighborHoodSelection>(nhc)));
}

#endif
