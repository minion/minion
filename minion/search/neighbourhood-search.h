
#ifndef MINION_NEIGHBOURHOOD_SEARCH_H
#define MINION_NEIGHBOURHOOD_SEARCH_H
#include "NeighbourhoodChoosingStrategies.h"
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

template <typename NeighbourhoodSelectionStrategy>
struct NeighbourhoodSearchManager : public Controller::SearchManager {
  typedef std::chrono::high_resolution_clock::time_point timePoint;

  shared_ptr<Propagate> prop;
  vector<SearchOrder> base_order;
  NeighbourhoodContainer nhc;
  shared_ptr<NeighbourhoodSelectionStrategy> selectionStrategy;

  NeighbourhoodSearchManager(shared_ptr<Propagate> _prop, vector<SearchOrder> _base_order,
                             NeighbourhoodContainer _nhc,
                             shared_ptr<NeighbourhoodSelectionStrategy> selectionStrategy)
      : prop(std::move(_prop)),
        base_order(_base_order),
        nhc(_nhc),
        selectionStrategy(std::move(selectionStrategy)) {
    signal(SIGVTALRM, triggerAlarm);
  }

  inline NeighbourhoodStats searchNeighbourhoods(vector<DomainInt>& solution,
                                                 vector<int>& activatedNeighbourhoods,
                                                 int timeoutInMillis = 0,
                                                 bool restrictToFirstSolution = true) {
    // Save state of the world
    int depth = Controller::get_world_depth();

    cout << "---In search function---" << endl;
    cout << "Depth is: " << depth << endl;
    Controller::world_push();
    auto vo = Controller::make_search_order_multiple(base_order);
    if(activatedNeighbourhoods.empty()) {
      nhc.shadow_disable.assign(1);
    } else {
      switchOnNeighbourhoods(activatedNeighbourhoods, solution);
    }

    solution.clear();

    prop->prop(vo->getVars());

    // printWorld();
    if(getState().isFailed()) {
      if(activatedNeighbourhoods.empty()) {
        D_FATAL_ERROR("Problem unsatisfiable with all neighbourhoods turned off");
      } else {
        cout << "---No Search was carried out---" << endl;
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
    while(true) {
      if(stats.solutionFound) {
        cout << "Found solution with op min " << stats.newMinValue << "\nTrying op min "
             << (stats.newMinValue + 1) << endl;
        copyOverIncumbent(solution);
        getState().getOptimiseVar()->setMin(stats.newMinValue + 1);
        std::vector<AnyVarRef> emptyVars;
        prop->prop(emptyVars);
      }

      if((activatedNeighbourhoods = selectionStrategy->getNeighbourHoodsToActivate(nhc)).empty() ||
         getState().getOptimiseVar()->getDomSize() == 0)
        return;

      // cout << "----Performing Search here---" << endl;
      // cout << "Depth is " << Controller::get_world_depth() << endl;
      stats = searchNeighbourhoods(solution, activatedNeighbourhoods, 500);
      // cout << "After -----" << endl;
      // cout << "Depth is " << Controller::get_world_depth() << endl;
      printWorld();
      selectionStrategy->updateStats(activatedNeighbourhoods, stats);
      numberOfSearches++;
      cout << "Number of searches: " << numberOfSearches << endl;
      cout << "Optimise Variable bound: " << getState().getOptimiseVar()->getMin() << "->"
           << getState().getOptimiseVar()->getMax() << endl;

      if(numberOfSearches == 10)
        break;
    }
    cout << "Neighbourhood History: " << endl;
    selectionStrategy->printHistory();
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
    cout << "Switiching on neighbourhoods with depth " << Controller::get_world_depth() << endl;
    for(int i : neighbourHoodIndexes) {
      cout << "Switching on neighbourhood " << i << endl;
      Neighbourhood& neighbourhood = nhc.neighbourhoods[i];
      neighbourhood.activation.assign(1);

      for(auto& n : neighbourhood.vars) {
        shadowVariables.insert(n);
      }
    }

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

  void copyOverIncumbent(const vector<DomainInt>& solution) {
    if (Controller::get_world_depth() != 1) {
      Controller::world_pop();
    }
    Controller::world_push();
    for(int i = 0; i < nhc.shadow_mapping[0].size(); i++) {
      nhc.shadow_mapping[1][i].assign(solution[i]);
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
  return std::make_shared<NeighbourhoodSearchManager<UCBNeighborHoodSelection>>(
      prop, base_order, nhc, std::make_shared<UCBNeighborHoodSelection>(nhc));
}

#endif
