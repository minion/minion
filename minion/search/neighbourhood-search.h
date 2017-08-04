
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
                                                 NeighbourhoodSearchStats& globalStats) {
    // Save state of the world
    int depth = Controller::get_world_depth();

    Controller::world_push();
    vector<SearchOrder> searchOrder;

    if(searchParams.isStandardSearchOnly()) {
      nhc.shadow_disable.assign(1);
      switchOffAllNeighbourhoods();
    } else {
      switchOnNeighbourhoods(searchParams, solution);
      searchOrder = makeNeighbourhoodSearchOrder(searchParams, base_order.front().order);
    }
    if(searchParams.isStandardSearchOnly()) {
      searchOrder.insert(searchOrder.end(), base_order.begin(), base_order.end());
    }
    solution.clear();
    auto vo = Controller::make_search_order_multiple(searchOrder);

    prop->prop(vo->getVars());

    if(getState().isFailed()) {
      if(searchParams.isStandardSearchOnly()) {
        D_FATAL_ERROR("Problem unsatisfiable with all neighbourhoods turned off");
      } else {
        NeighbourhoodStats stats(getState().getOptimiseVar()->getMin(), 0, false, false);
        globalStats.reportnewStats(searchParams.combinationToActivate, stats);
        Controller::world_pop();
        Controller::world_pop_to_depth(depth);
        return stats;
      }
    }

    DomainInt highestNeighbourhoodSize;
    DomainInt optimisationValueCache = getState().getOptimiseVar()->getMin() - 1;
    std::shared_ptr<Controller::StandardSearchManager> sm;
    // minus 1 is used because the optimisationHandler always sets it to optimisationValueCache +1
    auto timeoutChecker = [&](const vector<AnyVarRef>& var_array,
                              const vector<Controller::triple>& branches) {
      Controller::standard_time_ctrlc_checks(var_array, branches);
      if(alarmTriggered) {
        if(!searchParams.isStandardSearchOnly()) {
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
      if(searchParams.restrictToFirstSolution || !searchStrategy.continueSearch(nhc, solution)) {
        throw EndOfSearch();
      }
      if(searchParams.optimiseMode) {
        optimisationValueCache = getState().getOptimiseVar()->getMin();
      }
      if(!searchParams.isStandardSearchOnly()) {
        jumpBacktToPrimaryNeighbourhood(*sm, *((MultiBranch*)vo.get()));
      }
    };
    auto optimisationHandler = [&]() {
      getState().getOptimiseVar()->setMin(optimisationValueCache + 1);
    };
    sm = make_shared<Controller::StandardSearchManager>(vo, prop, timeoutChecker, solutionHandler,
                                                        optimisationHandler);

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
    globalStats.reportnewStats(searchParams.combinationToActivate, stats);
    Controller::world_pop_to_depth(depth);
    return stats;
  }

  inline void jumpBacktToPrimaryNeighbourhood(Controller::StandardSearchManager& sm,
                                              MultiBranch& varOrder) {
    while(varOrder.pos > 1) {
      if(sm.branches.back().isLeft) {
        Controller::world_pop();
        Controller::maybe_print_right_backtrack();
        sm.depth--;
      }
      sm.branches.pop_back();
    }
  }

  virtual void search() {
    int maxSize = nhc.getMaxNeighbourhoodSize();

    NeighbourhoodSearchStats globalStats(
        nhc.neighbourhoodCombinations.size(),
        make_pair(getState().getOptimiseVar()->getMin(), getState().getOptimiseVar()->getMax()),
        maxSize);

    globalStats.startTimer();
    vector<DomainInt> solution;
    std::cout << "Searching for initial solution:\n";
    SearchParams searchParams = SearchParams::standardSearch(false, 0);
    searchParams.restrictToFirstSolution = true;
    NeighbourhoodStats stats = searchNeighbourhoods(solution, searchParams, globalStats);
    if(!stats.solutionFound) {
      cout << "Initial solution not found\n";
      return;
    } else {
      searchStrategy.initialise(nhc, stats.newMinValue, solution, prop, globalStats);
      debug_log("Stats on initial solution:\n" << stats << endl);
    }
    try {
      while(!searchStrategy.hasFinishedPhase()) {
        searchParams = searchStrategy.getSearchParams(nhc, globalStats);
        debug_log("Searching with params  " << searchParams);
        stats = searchNeighbourhoods(solution, searchParams, globalStats);
        debug_log("Stats on last search: " << stats << endl);
        searchStrategy.updateStats(nhc, prop, searchParams.combinationToActivate, stats, solution,
                                   globalStats);
      }
    } catch(EndOfSearch&) {}

    if(getState().isCtrlcPressed()) {
      cout << "Ctrl-C pressed----" << std::endl;
    }
    printBestSolution(globalStats);
    globalStats.printStats(cout, nhc);
    cout << endl;
    throw EndOfSearch();
  }

  inline void printBestSolution(NeighbourhoodSearchStats& stats) {
    if(Controller::get_world_depth() != 1) {
      Controller::world_pop_to_depth(1);
    }
    std::vector<std::pair<AnyVarRef, DomainInt>>& bestAssignment = stats.getBestAssignment();
    for(auto& varAssignmentPair : bestAssignment) {
      varAssignmentPair.first.assign(varAssignmentPair.second);
    }
    Controller::check_sol_is_correct();
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
      cout << "Neighbourhood " << i;
      if(nhc.neighbourhoods[i].type != Neighbourhood::STANDARD) {
        cout << endl;
        continue;
      }
      cout << " With domain: " << nhc.neighbourhoods[i].activation.getMin() << " -> "
           << nhc.neighbourhoods[i].activation.getMax() << endl;
    }
    cout << "---------------" << endl;
  }

  /**
   * Create a search order where the first variables to branch on are the activated neighbourhoods
   * sizes with ascending value ordering.  Followed by the primary variabels referenced by the
   * activated neighbourhoods with random ordering.
   */
  vector<SearchOrder> makeNeighbourhoodSearchOrder(const SearchParams& searchParams,
                                                   VarOrderEnum defaultOrdering) {
    vector<SearchOrder> searchOrders;
    for(int nhIndex : searchParams.neighbourhoodsToActivate) {
      Neighbourhood& neighbourhood = nhc.neighbourhoods[nhIndex];
      if(neighbourhood.type == Neighbourhood::STANDARD) {
        searchOrders.emplace_back();
        searchOrders.back().order = ORDER_STATIC;

        searchOrders.back().var_order.push_back(neighbourhood.deviation.getBaseVar());
        searchOrders.back().val_order.push_back(VALORDER_ASCEND);
        for(AnyVarRef& nhLocalVar : neighbourhood.vars) {
          searchOrders.back().var_order.push_back(nhLocalVar.getBaseVar());
          searchOrders.back().val_order.push_back(VALORDER_RANDOM);
        }
      }
      if(neighbourhood.type != Neighbourhood::CLOSED) {
        searchOrders.emplace_back();
        searchOrders.back().order = defaultOrdering;
        for(AnyVarRef& varRef : neighbourhood.group->vars) {
          searchOrders.back().var_order.push_back(varRef.getBaseVar());
          searchOrders.back().val_order.push_back(VALORDER_RANDOM);
        }
      }
    }
    searchOrders.emplace_back();
    searchOrders.back().order = defaultOrdering;
    for(AnyVarRef& v : nhc.variablesOutOfNeighbourhoods) {
      searchOrders.back().var_order.push_back(v.getBaseVar());
      searchOrders.back().val_order.push_back(VALORDER_RANDOM);
    }
    return searchOrders;
  }

  /**assign all neighbourhood activation variables to false
   *
   */
  inline void switchOffAllNeighbourhoods() {
    for(auto& nh : nhc.neighbourhoods) {
      if(nh.type == Neighbourhood::STANDARD) {
        nh.activation.assign(0);
      }
    }
  }

  /**
   * Switch on the neighbourhood activation vars
   * If a closed neighbourhood is found, assign its group vars to their incumbent.
   *  Disable all other neighbourhoods.
   */
  void switchOnNeighbourhoods(const SearchParams& searchParams, const vector<DomainInt>& solution) {

    for(int i = 0; i < (int)searchParams.neighbourhoodsToActivate.size(); i++) {
      Neighbourhood& nh = nhc.neighbourhoods[searchParams.neighbourhoodsToActivate[i]];
      if(nh.type == Neighbourhood::STANDARD) {
        nh.activation.assign(1);
        if(i == 0 && searchParams.initialNeighbourhoodSize > 1) {
          nh.deviation.setMin(searchParams.initialNeighbourhoodSize);
        }
      } else if(nh.type == Neighbourhood::CLOSED) {
        for(AnyVarRef& varRef : nh.group->vars) {
          varRef.assign(nhc.shadowLookup[varRef].getAssignedValue());
        }
      }
    }

    for(auto& nh : nhc.neighbourhoods) {
      if(nh.type == Neighbourhood::STANDARD && !nh.activation.isAssigned()) {
        nh.activation.assign(0);
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
shared_ptr<Controller::SearchManager> MakeNeighbourhoodSearchHelper(PropagationLevel& prop_method,
                                                                    vector<SearchOrder>& base_order,
                                                                    NeighbourhoodContainer& nhc) {
  shared_ptr<Propagate> prop = Controller::make_propagator(prop_method);
  switch(getOptions().neighbourhoodSearchStrategy) {
  case SearchOptions::NeighbourhoodSearchStrategy::HILL_CLIMBING:
    return std::make_shared<NeighbourhoodSearchManager<HillClimbingSearch<NhSelectionStrategy>>>(
        prop, base_order, std::move(nhc));
  case SearchOptions::NeighbourhoodSearchStrategy::META_STRATEGY:
    return std::make_shared<NeighbourhoodSearchManager<MetaStrategy<NhSelectionStrategy>>>(
        prop, base_order, nhc);
  }
}

shared_ptr<Controller::SearchManager> MakeNeighbourhoodSearch(PropagationLevel prop_method,
                                                              vector<SearchOrder> base_order,
                                                              NeighbourhoodContainer nhc) {
  switch(getOptions().neighbourhoodSelectionStrategy) {
  case SearchOptions::NeighbourhoodSelectionStrategy::RANDOM:
    return MakeNeighbourhoodSearchHelper<RandomCombinationChooser>(prop_method, base_order, nhc);
  case SearchOptions::NeighbourhoodSelectionStrategy::UCB:
    return MakeNeighbourhoodSearchHelper<UCBNeighborHoodSelection>(prop_method, base_order, nhc);
  case SearchOptions::NeighbourhoodSelectionStrategy::INTERACTIVE:
    return MakeNeighbourhoodSearchHelper<InteractiveCombinationChooser>(prop_method, base_order,
                                                                        nhc);
  }
}

#endif
