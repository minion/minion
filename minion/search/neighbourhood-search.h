
#ifndef MINION_NEIGHBOURHOOD_SEARCH_H
#define MINION_NEIGHBOURHOOD_SEARCH_H
#include "NeighbourhoodChoosingStrategies.h"
#include "SearchManager.h"
#include "SearchStrategies.h"
#include "inputfile_parse/CSPSpec.h"
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
  SearchStrategy searchStrategy;

  NeighbourhoodSearchManager(shared_ptr<Propagate> _prop, vector<SearchOrder> _base_order,
                             NeighbourhoodContainer _nhc)
      : prop(std::move(_prop)), base_order(_base_order), nhc(std::move(_nhc)) {
    signal(SIGVTALRM, triggerAlarm);
  }

  inline NeighbourhoodStats searchNeighbourhoods(vector<DomainInt>& solution,
                                                 const SearchParams& searchParams,
                                                 NeighbourhoodSearchStats& globalStats,
                                                 bool restrictToFirstSolution = true) {
    // Save state of the world
    int depth = Controller::get_world_depth();

    Controller::world_push();
    vector<SearchOrder> searchOrder;
    if(searchParams.neighbourhoodsToActivate.empty()) {
      nhc.shadow_disable.assign(1);
      switchOffAllNeighbourhoods();
    } else {
      switchOnNeighbourhoods(searchParams.neighbourhoodsToActivate, solution);
      searchOrder = makeNeighbourhoodSearchOrder(searchParams, base_order.front().order);
    }
    searchOrder.insert(searchOrder.end(), base_order.begin(), base_order.end());
    solution.clear();
    auto vo = Controller::make_search_order_multiple(searchOrder);

    prop->prop(vo->getVars());

    if(getState().isFailed()) {
      if(searchParams.neighbourhoodsToActivate.empty()) {
        D_FATAL_ERROR("Problem unsatisfiable with all neighbourhoods turned off");
      } else {
        Controller::world_pop_to_depth(depth);
        return NeighbourhoodStats(getState().getOptimiseVar()->getMin(), 0, false, false);
      }
    }

    DomainInt highestNeighbourhoodSize;
    DomainInt optimisationValueCache = getState().getOptimiseVar()->getMin() - 1;
    // minus 1 is used as the optimisationHandler always sets it to optimisationValueCache +1
    auto timeoutChecker = [&](const vector<AnyVarRef>& var_array,
                              const vector<Controller::triple>& branches) {
      Controller::standard_time_ctrlc_checks(var_array, branches);
      if(alarmTriggered) {
        if(!searchParams.neighbourhoodsToActivate.empty()) {
          highestNeighbourhoodSize =
              nhc.neighbourhoods[searchParams.neighbourhoodsToActivate[0]].deviation.getMin();
        }
        throw TimeoutException();
      }
    };

    auto solutionHandler = [&]() {
      solution.clear();
      for(const auto& var : this->nhc.shadow_mapping[0]) {
        solution.push_back(var.getAssignedValue());
      }
      globalStats.foundSolution(getState().getOptimiseVar()->getMin());
      if(restrictToFirstSolution || !searchStrategy.continueSearch(nhc, solution)) {
        throw EndOfSearch();
      }
      if(searchParams.optimiseMode) {
        optimisationValueCache = getState().getOptimiseVar()->getMin();
      }
    };
    auto optimisationHandler = [&]() {
      getState().getOptimiseVar()->setMin(optimisationValueCache + 1);
    };
    auto sm = make_shared<Controller::StandardSearchManager>(vo, prop, timeoutChecker,
                                                             solutionHandler, optimisationHandler);

    if(searchParams.timeoutInMillis > 0) {
      setTimeout(searchParams.timeoutInMillis);
    }

    auto startTime = std::chrono::high_resolution_clock::now();
    bool timeout = false;
    try {
      sm->search();
    } catch(EndOfSearch&) { clearTimeout(); } catch(TimeoutException&) {
      timeout = true;
    }

    if(getState().isCtrlcPressed() ||
       (getOptions().timeout_active &&
        (globalStats.getTotalTimeTaken() / 1000) >= getOptions().time_limit)) {
      throw EndOfSearch();
    }

    bool solutionFound = !solution.empty();
    DomainInt bestOptimisation =
        (solutionFound) ? optimisationValueCache : optimisationValueCache + 1;
    NeighbourhoodStats stats(bestOptimisation, getTimeTaken(startTime), solutionFound, timeout,
                             highestNeighbourhoodSize);
    Controller::world_pop_to_depth(depth);
    return stats;
  }

  virtual void search() {
    int maxNeighbourhoodSize =
        checked_cast<int>(std::max_element(nhc.neighbourhoods.begin(), nhc.neighbourhoods.end(),
                                           [](const Neighbourhood& n1, const Neighbourhood& n2) {
                                             return n1.deviation.getMax() < n2.deviation.getMax();
                                           })
                              ->deviation.getMax());
    NeighbourhoodSearchStats globalStats(
        nhc.neighbourhoods.size(),
        make_pair(getState().getOptimiseVar()->getMin(), getState().getOptimiseVar()->getMax()),
        maxNeighbourhoodSize);
    globalStats.startTimer();
    vector<DomainInt> solution;
    std::cout << "Searching for initial solution:\n";
    NeighbourhoodStats stats =
        searchNeighbourhoods(solution, SearchParams({}, true, 0), globalStats);
    if(!stats.solutionFound) {
      cout << "Initial solution not found\n";
      return;
    } else {
      globalStats.setValueOfInitialSolution(stats.newMinValue);
      searchStrategy.initialise(nhc, stats.newMinValue, solution, prop, globalStats);
      debug_log("Stats on initial solution:\n" << stats << endl);
    }
    try {
      while(!searchStrategy.hasFinishedPhase()) {
        SearchParams searchParams = searchStrategy.getSearchParams(nhc, globalStats);
        debug_log("Searching with params  " << searchParams);
        stats = searchNeighbourhoods(solution, searchParams, globalStats, false);
        debug_log("Stats on last search: " << stats << endl);
        searchStrategy.updateStats(nhc, prop, searchParams.neighbourhoodsToActivate, stats,
                                   solution, globalStats);
        debug_log("Global stats:\n");
        globalStats.reportnewStats(searchParams.neighbourhoodsToActivate, stats);

        debug_code(globalStats.printStats(cout, nhc));
      }
    } catch(EndOfSearch&) {}

    if(getState().isCtrlcPressed()) {
      cout << "Ctrl-C pressed----" << std::endl;
    }
    globalStats.printStats(cout, nhc);
    cout << endl;
    throw EndOfSearch();
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

    for(int i = 0; i < nhc.neighbourhoods.size(); i++) {
      cout << "Neighbourhood " << i << " With domain: " << nhc.neighbourhoods[i].activation.getMin()
           << " -> " << nhc.neighbourhoods[i].activation.getMax() << endl;
    }
    cout << "---------------" << endl;
  }
  /**
   * Create a search order where the first variables to branch on are the activated neighbourhoods
   * sizes with ascending value ordering.  Followed by the primary variabels referenced by the
   * activated neighbourhoods with random ordering.
   */
  vector<SearchOrder> makeNeighbourhoodSearchOrder(const SearchParams& searchParams,
                                                   VarOrderEnum varOrder) {
    nhc.neighbourhoods[0].deviation.setMin(searchParams.initialNeighbourhoodSize);
    vector<SearchOrder> searchOrders(2);
    for(int neighbourhoodIndex : searchParams.neighbourhoodsToActivate) {
      searchOrders[0].var_order.push_back(
          nhc.neighbourhoods[neighbourhoodIndex].deviation.getBaseVar());
      searchOrders[0].val_order.push_back(VALORDER_ASCEND);
    }
    for(int neighbourhoodIndex : searchParams.neighbourhoodsToActivate) {
      for(const auto& primaryVar : nhc.neighbourhoods[neighbourhoodIndex].vars) {
        searchOrders[1].var_order.push_back(primaryVar.getBaseVar());
        searchOrders[1].val_order.push_back(VALORDER_RANDOM);
      }
    }
    searchOrders[1].order = varOrder;
    return searchOrders;
  }

  /**assign all neighbourhood activation variables to false
   *
   */
  inline void switchOffAllNeighbourhoods() {
    for(auto& nh : nhc.neighbourhoods) {
      nh.activation.assign(0);
    }
  }

  /**
   * Switch on the neighbourhood activation vars
   * Find the set of primary variables not contained in any neighbourhoods and assign them to the
   * incumbent solution
   * @param neighbourHoodIndexes
   */
  void switchOnNeighbourhoods(const vector<int>& neighbourHoodIndexes,
                              const vector<DomainInt>& solution) {
    assert(neighbourHoodIndexes.size() == 1);
    std::unordered_set<AnyVarRef> shadowVariables;

    for(int i = 0; i < nhc.neighbourhoods.size(); i++) {
      if(i != neighbourHoodIndexes[0])
        nhc.neighbourhoods[i].activation.assign(0);
    }

    Neighbourhood& neighbourhood = nhc.neighbourhoods[neighbourHoodIndexes[0]];
    neighbourhood.activation.assign(1);

    debug_log("Switching on neighbourhood "
              << i << " With domain: " << nhc.neighbourhoods[i].activation.getMin() << " -> "
              << nhc.neighbourhoods[i].activation.getMax());

    for(auto& n : neighbourhood.vars) {
      shadowVariables.insert(n);
    }

    debug_code(for(int i = 0; i < nhc.neighbourhoods.size(); i++) {
      cout << "Neighbourhood " << i << " With domain: " << nhc.neighbourhoods[i].activation.getMin()
           << " -> " << nhc.neighbourhoods[i].activation.getMax() << endl;
    });

    /*
     * If the primary variable is not contained in any shadow neighbourhoods, assign its incumbent
     * solution
     */
    for(int i = 0; i < solution.size(); i++) {
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
    return std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
  }
};

template <typename NhSelectionStrategy>
shared_ptr<Controller::SearchManager>
MakeNeighbourhoodSearchHelper(PropagationLevel& prop_method, vector<SearchOrder>& base_order,
                              NeighbourhoodContainer& nhc,
                              CSPInstance::NeighbourhoodSearchStrategy searchStrategy) {
  shared_ptr<Propagate> prop = Controller::make_propagator(prop_method);
  switch(searchStrategy) {
  case CSPInstance::NeighbourhoodSearchStrategy::HILL_CLIMBING:
    return std::make_shared<NeighbourhoodSearchManager<HillClimbingSearch<NhSelectionStrategy>>>(
        prop, base_order, std::move(nhc));
  case CSPInstance::NeighbourhoodSearchStrategy::META_STRATEGY:
    return std::make_shared<NeighbourhoodSearchManager<MetaStrategy<NhSelectionStrategy>>>(
        prop, base_order, nhc);
  }
}

shared_ptr<Controller::SearchManager>
MakeNeighbourhoodSearch(PropagationLevel prop_method, vector<SearchOrder> base_order,
                        NeighbourhoodContainer nhc,
                        CSPInstance::NeighbourhoodSearchStrategy searchStrategy,
                        CSPInstance::NeighbourhoodSelectionStrategy selectionStrategy) {
  searchStrategy = CSPInstance::NeighbourhoodSearchStrategy::META_STRATEGY;
  selectionStrategy = CSPInstance::NeighbourhoodSelectionStrategy::UCB;
  switch(selectionStrategy) {
  case CSPInstance::NeighbourhoodSelectionStrategy::RANDOM:
    D_FATAL_ERROR("Dont instantiate this please");
  //   return MakeNeighbourhoodSearchHelper<RandomNeighbourhoodChooser>(prop_method, base_order,
  //   nhc,
  // searchStrategy);
  case CSPInstance::NeighbourhoodSelectionStrategy::UCB:
    return MakeNeighbourhoodSearchHelper<UCBNeighborHoodSelection>(prop_method, base_order, nhc,
                                                                   searchStrategy);
  }
}

#endif
