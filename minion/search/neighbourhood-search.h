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
std::ostream& operator<<(std::ostream& os, const SearchOptions::NHConfig& config);
void triggerAlarm(int) {
  alarmTriggered = true;
}
class TimeoutException : public std::exception {};

template <typename SearchStrategy>
struct NeighbourhoodSearchManager : public Controller::SearchManager {

  shared_ptr<Propagate> prop;
  vector<SearchOrder> base_order;
  NeighbourhoodContainer nhc;
  SearchStrategy searchStrategy;

  NeighbourhoodSearchManager(shared_ptr<Propagate> _prop, vector<SearchOrder> _base_order,
                             NeighbourhoodContainer _nhc)
      : prop(std::move(_prop)), base_order(_base_order), nhc(std::move(_nhc)), searchStrategy(nhc) {
    signal(SIGVTALRM, triggerAlarm);
  }

  inline NeighbourhoodStats searchNeighbourhoods(vector<DomainInt>& solution,
                                                 const SearchParams& searchParams,
                                                 NeighbourhoodSearchStats& globalStats) {
    // Save state of the world
    int depth = Controller::get_world_depth();
    Controller::world_push();
    vector<SearchOrder> searchOrder;
    vector<SearchOrder>* chosenSearchOrder = NULL;
    int bottomOfPrimaryNhIndex;
    if(searchParams.mode != SearchParams::NEIGHBOURHOOD_SEARCH) {
      nhc.shadow_disable.assign(1);
      switchOffAllNeighbourhoods();
      if(searchParams.mode == SearchParams::STANDARD_SEARCH) {
        chosenSearchOrder = &base_order;
      } else if(searchParams.mode == SearchParams::RANDOM_WALK) {
        searchOrder = makeRandomWalkSearchOrder(searchParams.random_bias);
        chosenSearchOrder = &searchOrder;
      }
    } else {
      switchOnNeighbourhoods(searchParams, solution);
      auto indexSearchOrderPair =
          makeNeighbourhoodSearchOrder(searchParams, base_order.front().order);
      searchOrder = std::move(indexSearchOrderPair.second);
      bottomOfPrimaryNhIndex = indexSearchOrderPair.first;
      chosenSearchOrder = &searchOrder;
    }

    solution.clear();
    auto vo = Controller::make_search_order_multiple(*chosenSearchOrder);

    prop->prop(vo->getVars());

    if(getState().isFailed()) {
      if(searchParams.mode == SearchParams::STANDARD_SEARCH) {
        D_FATAL_ERROR("Problem unsatisfiable with all neighbourhoods turned off");
      } else {
        NeighbourhoodStats stats(getState().getOptimiseVar()->getMin(), 0, false, false);

        globalStats.reportnewStats(searchParams.combinationToActivate, stats);
        Controller::world_pop_to_depth(depth);
        return stats;
      }
    }

    DomainInt highestNeighbourhoodSize, lastOptVal = getState().getOptimiseVar()->getMin(),
                                        newOptMinTarget = getState().getOptimiseVar()->getMin();
    std::shared_ptr<Controller::StandardSearchManager> sm;
    auto timeoutChecker = [&](const vector<AnyVarRef>& var_array,
                              const vector<Controller::triple>& branches) {
      Controller::standard_time_ctrlc_checks(var_array, branches);
      if(alarmTriggered) {
        if(searchParams.mode == SearchParams::NEIGHBOURHOOD_SEARCH) {
          highestNeighbourhoodSize =
              nhc.neighbourhoods[searchParams.neighbourhoodsToActivate[0]].deviation.getMin();
        }
        throw TimeoutException();
      }
    };

    auto solutionHandler = [&]() {
      lastOptVal = getState().getOptimiseVar()->getMin();
      solution.clear();
      for(const auto& var : this->nhc.shadow_mapping[0]) {
        solution.push_back(var.getAssignedValue());
      }
      globalStats.foundSolution(getState().getOptimiseVar()->getMin());
      if(searchParams.stopAtFirstSolution || !searchStrategy.continueSearch(nhc, solution)) {
        throw EndOfSearch();
      }
      if(searchParams.optimiseMode) {
        newOptMinTarget = getState().getOptimiseVar()->getMin() + 1;
      }
      if(searchParams.mode == SearchParams::NEIGHBOURHOOD_SEARCH) {
        jumpBacktToPrimaryNeighbourhood(*sm, *((MultiBranch*)vo.get()), bottomOfPrimaryNhIndex);
      }
    };
    auto backtrackCountAtStart = getState().getBacktrackCount();

    auto optimisationHandler = [&]() {

      getState().getOptimiseVar()->setMin(newOptMinTarget);
      if(searchParams.backtrackInsteadOfTimeLimit && searchParams.backtrackLimit > 0 &&
         (getState().getBacktrackCount() - backtrackCountAtStart) > searchParams.backtrackLimit) {
        throw TimeoutException();
      }
    };
    sm = make_shared<Controller::StandardSearchManager>(vo, prop, timeoutChecker, solutionHandler,
                                                        optimisationHandler);
    if(!searchParams.backtrackInsteadOfTimeLimit && searchParams.timeoutInMillis > 0) {
      setTimeout(searchParams.timeoutInMillis);
    }

    double startTime = get_cpu_time();
    bool timeout = false;
    try {
      sm->search();
    } catch(EndOfSearch&) {
    } catch(TimeoutException&) { timeout = true; }
    if(!searchParams.backtrackInsteadOfTimeLimit && searchParams.timeoutInMillis > 0) {
      clearTimeout();
    }
    if(getState().isCtrlcPressed() ||
       (getOptions().timeout_active &&
        globalStats.getTotalTimeTaken() >= getOptions().time_limit)) {
      throw EndOfSearch();
    }
    bool solutionFound = !solution.empty();
    NeighbourhoodStats stats(lastOptVal, getTimeTaken(startTime), solutionFound, timeout,
                             highestNeighbourhoodSize);
    globalStats.reportnewStats(searchParams.combinationToActivate, stats);
    Controller::world_pop_to_depth(depth);
    return stats;
  }

  inline void jumpBacktToPrimaryNeighbourhood(Controller::StandardSearchManager& sm,
                                              MultiBranch& varOrder, int bottomOfPrimaryNhIndex) {
    while(!sm.branches.empty() && varOrder.pos > bottomOfPrimaryNhIndex) {
      if(sm.branches.back().isLeft) {
        Controller::world_pop();
        Controller::maybe_print_right_backtrack();
        sm.depth--;
      }
      sm.branches.pop_back();
    }
  }

  virtual void search() {
    cout << getOptions().nhConfig << endl;
    cout.setf(ios::fixed, ios::floatfield);
    cout.precision(3);

    int maxSize = nhc.getMaxNeighbourhoodSize();

    NeighbourhoodSearchStats globalStats(
        nhc.neighbourhoodCombinations.size(),
        make_pair(getState().getOptimiseVar()->getMin(), getState().getOptimiseVar()->getMax()),
        maxSize);

    globalStats.startTimer();

    vector<DomainInt> solution;
    // holds the last solution of any search iteration

    NeighbourhoodStats stats(0, 0, false, false);
    // holds return value of each search iteration

    // try to find initial solution
    try {
      int initialSearchTimeout = 100;
      double multiplier = 1.5;
      int initialBacktrackLimit = 1;
      int attempt = 0;
      do {
        int bias = 0;
        if(attempt % 5 == 2)
          bias = 90;
        if(attempt % 5 == 3)
          bias = -90;
        nhLog("Searching for initial solution");
        if(getOptions().nhConfig.backtrackInsteadOfTimeLimit) {
          cout << "backtrackLimit=" << round(initialBacktrackLimit) << endl;
        } else {
          cout << "timeout=" << initialSearchTimeout << ":\n";
        }
        stats = searchNeighbourhoods(
            solution,
            SearchParams::randomWalk(false, true, initialSearchTimeout,
                                     round(initialBacktrackLimit),
                                     getOptions().nhConfig.backtrackInsteadOfTimeLimit, bias),
            globalStats);
        if(!stats.solutionFound) {
          initialSearchTimeout = (int)(initialSearchTimeout * multiplier);
          initialBacktrackLimit =
              initialBacktrackLimit * getOptions().nhConfig.backtrackLimitMultiplier +
              getOptions().nhConfig.backtrackLimitIncrement;
        }
        attempt++;
      } while(!stats.solutionFound);
    } catch(EndOfSearch&) {
      if(getState().isCtrlcPressed()) {
        cout << "Ctrl-C pressed----" << std::endl;
      }
      cout << "Initial solution not found\n";
      cout << endl;
      throw EndOfSearch();
    }
    searchStrategy.initialise(nhc, stats.newMinValue, solution, prop, globalStats);
    debug_log("Stats on initial solution:\n" << stats << endl);
    try {
      while(!searchStrategy.hasFinishedPhase()) {
        SearchParams searchParams = searchStrategy.getSearchParams(nhc, globalStats);
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
   * Create a search order where for each activated neighbourhood, first come the neighbourhood's
   * size and
   * local variables in a static ordering, then the primary variables that the neighbourhood
   * operates on in the ordering specified by minion..
   */
  pair<int, vector<SearchOrder>> makeNeighbourhoodSearchOrder(const SearchParams& searchParams,
                                                              VarOrderEnum defaultOrdering) {
    int bottomOfPrimaryNhIndex = 0;
    if(searchParams.nhLocalVarsComeFirst) {
      bottomOfPrimaryNhIndex = 1;
    }
    vector<SearchOrder> searchOrders;
    vector<bool> neighbourhoodSet(nhc.neighbourhoods.size());
    for(size_t i = 0; i < searchParams.neighbourhoodsToActivate.size(); ++i) {
      const int& nhIndex = searchParams.neighbourhoodsToActivate[i];
      neighbourhoodSet[nhIndex] = true;
      Neighbourhood& neighbourhood = nhc.neighbourhoods[nhIndex];
      if(neighbourhood.type != Neighbourhood::CLOSED) {
        searchOrders.emplace_back();
        searchOrders.back().order = defaultOrdering;
        if(neighbourhood.type == Neighbourhood::STANDARD) {
          if(searchParams.nhLocalVarsComeFirst) {
            searchOrders.back().order = ORDER_STATIC;
            addNhLocalVars(searchOrders, neighbourhood, VALORDER_ASCEND);
            searchOrders.emplace_back();
            searchOrders.back().order = defaultOrdering;
          } else {
            addNhLocalVars(searchOrders, neighbourhood, VALORDER_RANDOM);
          }
        }
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
    // also add the local vars for neighbourhoods not activated just in case they are not
    // dontcared
    for(int i = 0; i < nhc.neighbourhoods.size(); i++) {
      if(nhc.neighbourhoods[i].type == Neighbourhood::STANDARD && !neighbourhoodSet[i]) {
        addNhLocalVars(searchOrders, nhc.neighbourhoods[i], VALORDER_RANDOM);
      }
    }
    return pair<int, vector<SearchOrder>>(bottomOfPrimaryNhIndex, move(searchOrders));
  }

  void addNhLocalVars(vector<SearchOrder>& searchOrders, Neighbourhood& neighbourhood,
                      ValOrder nhSizeOrder) {
    searchOrders.back().var_order.push_back(neighbourhood.deviation.getBaseVar());
    searchOrders.back().val_order.push_back(nhSizeOrder);
    for(AnyVarRef& nhLocalVar : neighbourhood.vars) {
      searchOrders.back().var_order.push_back(nhLocalVar.getBaseVar());
      searchOrders.back().val_order.push_back(VALORDER_RANDOM);
    }
  }

  vector<SearchOrder> makeRandomWalkSearchOrder(int bias) {
    vector<SearchOrder> searchOrder(base_order.begin(), base_order.end());
    for(auto& so : searchOrder) {
      for(int i = 0; i < so.val_order.size(); i++) {
        so.val_order[i] = ValOrder(VALORDER_RANDOM, bias);
      }
    }
    return searchOrder;
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

  inline double getTimeTaken(double startTime) {
    return get_cpu_time() - startTime;
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
  default: assert(false); abort();
  }
}

shared_ptr<Controller::SearchManager> MakeNeighbourhoodSearch(PropagationLevel prop_method,
                                                              vector<SearchOrder> base_order,
                                                              NeighbourhoodContainer nhc) {
  switch(getOptions().neighbourhoodSelectionStrategy) {
  case SearchOptions::NeighbourhoodSelectionStrategy::RANDOM:
    return MakeNeighbourhoodSearchHelper<RandomCombinationChooser>(prop_method, base_order, nhc);
  case SearchOptions::NeighbourhoodSelectionStrategy::UCB:
    return MakeNeighbourhoodSearchHelper<UCBNeighbourhoodSelection>(prop_method, base_order, nhc);
  case SearchOptions::NeighbourhoodSelectionStrategy::LEARNING_AUTOMATON:
    return MakeNeighbourhoodSearchHelper<LearningAutomatonNeighbourhoodSelection>(prop_method,
                                                                                  base_order, nhc);
  case SearchOptions::NeighbourhoodSelectionStrategy::INTERACTIVE:
    return MakeNeighbourhoodSearchHelper<InteractiveCombinationChooser>(prop_method, base_order,
                                                                        nhc);
  default: assert(false); abort();
  }
}

inline std::ostream& operator<<(std::ostream& os, const SearchOptions::NHConfig& config) {
  os << "NHConfig {";
  if(config.backtrackInsteadOfTimeLimit) {
    os << "Using backtracks,\n";
  } else {
    os << "Using timelimit,\n";
  }
  os << "Backtrack limit multiplier:" << config.backtrackLimitMultiplier << ",\n";
  os << "Backtrack limit increment:" << config.backtrackLimitIncrement << ",\n";
  os << "reset backtrack limit after hill climb: " << config.resetBacktrackAfterHillClimb << ",\n";
  os << "iterationSearchTime:" << config.iterationSearchTime << ",\n";
  os << "hillClimberMinIterationsToSpendAtPeak: " << config.hillClimberMinIterationsToSpendAtPeak
     << ",\n";
  os << "hillClimberInitialLocalMaxProbability : " << config.hillClimberInitialLocalMaxProbability
     << ",\n";
  os << "hillClimberProbabilityIncrementMultiplier: "
     << config.hillClimberProbabilityIncrementMultiplier << "\n}";
  return os;
}
#endif
