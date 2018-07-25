#ifndef MINION_NEIGHBOURHOOD_SEARCH_H
#define MINION_NEIGHBOURHOOD_SEARCH_H
#include "SearchManager.h"
#include "inputfile_parse/CSPSpec.h"
#include "neighbourhood-def.h"
#include "neighbourhoodSearchStats.h"
#include "search/nhConfig.h"
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
struct SearchParams {
  enum Mode { STANDARD_SEARCH, NEIGHBOURHOOD_SEARCH, RANDOM_WALK };
  // Only used with RANDOM_WALK
  int random_bias;

  Mode mode;
  int combinationToActivate;
  std::vector<int> neighbourhoodsToActivate;
  bool nhLocalVarsComeFirst;
  bool optimiseMode;
  bool stopAtFirstSolution;

  int timeoutInMillis;
  int backtrackLimit;
  bool backtrackInsteadOfTimeLimit;
  DomainInt initialNeighbourhoodSize;
  bool nhSizeVarAscendVsRandom;

private:
  SearchParams(int random_bias, Mode mode, int combinationToActivate,
               std::vector<int> neighbourhoods, bool nhLocalVarsComeFirst, bool optimiseMode,
               bool stopAtFirstSolution, int timeoutInMillis, int backtrackLimit,
               bool backtrackInsteadOfTimeLimit, DomainInt initialNeighbourhoodSize,
               bool nhSizeVarAscendVsRandom)
      : random_bias(random_bias),
        mode(mode),
        combinationToActivate(combinationToActivate),
        neighbourhoodsToActivate(std::move(neighbourhoods)),
        nhLocalVarsComeFirst(nhLocalVarsComeFirst),
        optimiseMode(optimiseMode),
        stopAtFirstSolution(stopAtFirstSolution),
        timeoutInMillis(timeoutInMillis),
        backtrackLimit(backtrackLimit),
        backtrackInsteadOfTimeLimit(backtrackInsteadOfTimeLimit),
        initialNeighbourhoodSize(initialNeighbourhoodSize),
        nhSizeVarAscendVsRandom(nhSizeVarAscendVsRandom) {}

public:
  static inline SearchParams
  neighbourhoodSearch(int combinationToActivate, const NeighbourhoodContainer& nhc,
                      bool nhLocalVarsComeFirst, bool optimiseMode, bool stopAtFirstSolution,
                      int timeoutInMillis, int backtrackLimit, bool backtrackInsteadOfTimeLimit,
                      DomainInt initialNeighbourhoodSize, bool nhSizeVarAscendVsRandom = true) {
    SearchParams searchParams(0, NEIGHBOURHOOD_SEARCH, combinationToActivate,
                              nhc.neighbourhoodCombinations[combinationToActivate],
                              nhLocalVarsComeFirst, optimiseMode, stopAtFirstSolution,
                              timeoutInMillis, backtrackLimit, backtrackInsteadOfTimeLimit,
                              initialNeighbourhoodSize, nhSizeVarAscendVsRandom);
    if(searchParams.neighbourhoodsToActivate.size() > 1) {
      std::random_shuffle(searchParams.neighbourhoodsToActivate.begin() + 1,
                          searchParams.neighbourhoodsToActivate.end());
    }
    return searchParams;
  }

  static inline SearchParams standardSearch(bool optimiseMode, bool stopAtFirstSolution,
                                            int timeoutInMillis, int backtrackLimit,
                                            bool backtrackInsteadOfTimeLimit) {
    return SearchParams(0, STANDARD_SEARCH, -1, {}, false, optimiseMode, stopAtFirstSolution,
                        timeoutInMillis, backtrackLimit, backtrackInsteadOfTimeLimit, 0, false);
  }
  static inline SearchParams randomWalk(bool optimiseMode, bool stopAtFirstSolution,
                                        int timeoutInMillis, int backtrackLimit,
                                        bool backtrackInsteadOfTimeLimit, int bias) {
    return SearchParams(bias, RANDOM_WALK, -1, {}, false, optimiseMode, stopAtFirstSolution,
                        timeoutInMillis, backtrackLimit, backtrackInsteadOfTimeLimit, 0, false);
  }
  friend inline std::ostream& operator<<(std::ostream& os, const SearchParams& searchParams) {
    os << "SearchParams(";
    switch(searchParams.mode) {
    case NEIGHBOURHOOD_SEARCH: os << "mode=NEIGHBOURHOOD_SEARCH"; break;
    case STANDARD_SEARCH: os << "STANDARD_SEARCH"; break;
    case RANDOM_WALK: os << "RANDOM_WALK"; break;
    }
    os << "\ncombinationToActivate = " << searchParams.combinationToActivate
       << "\nneighbourhoodsToActivate =  " << searchParams.neighbourhoodsToActivate
       << ",\noptimiseMode = " << searchParams.optimiseMode
       << ",\ntimeoutInMillis = " << searchParams.timeoutInMillis
       << ",\ninitialNeighbourhoodSize = " << searchParams.initialNeighbourhoodSize << ")";
    return os;
  }
};

struct NeighbourhoodState {
  struct DefaultContinueSearchFunc {
    bool operator()(vector<DomainInt>&) const {
      return true;
    }
  };
  shared_ptr<Propagate> prop;
  vector<SearchOrder> base_order;
  vector<DomainInt> solution;
  // holds the last solution of any search iteration
  NeighbourhoodContainer nhc;
  NeighbourhoodSearchStats globalStats;
  NeighbourhoodState(shared_ptr<Propagate> _prop, vector<SearchOrder> _base_order,
                     NeighbourhoodContainer _nhc)
      : prop(std::move(_prop)),
        base_order(_base_order),
        nhc(std::move(_nhc)),
        globalStats(
            nhc.neighbourhoodCombinations.size(),
            make_pair(getState().getOptimiseVar()->getMin(), getState().getOptimiseVar()->getMax()),
            nhc.maxNeighbourhoodSize) {}
  template <typename ContinueSearchFunc = DefaultContinueSearchFunc>
  inline NeighbourhoodStats
  searchNeighbourhoods(const SearchParams& searchParams,
                       ContinueSearchFunc&& continueSearch = DefaultContinueSearchFunc()) {
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
        NeighbourhoodStats stats(getState().getOptimiseVar()->getMin(),
                                 getState().getOptimiseVar()->getMin(), 0, false, false);

        globalStats.reportnewStats(searchParams.combinationToActivate, stats);
        Controller::world_pop_to_depth(depth);
        return stats;
      }
    }

    DomainInt highestNeighbourhoodSize, oldMinValue = getState().getOptimiseVar()->getMin(),
                                        newOptMinTarget = oldMinValue, lastOptVal = oldMinValue;
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
      if(searchParams.stopAtFirstSolution || !continueSearch(solution)) {
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
    bool solutionFound = !solution.empty();
    double totalTime = get_cpu_time();
    NeighbourhoodStats stats(lastOptVal, oldMinValue, totalTime - startTime, solutionFound, timeout,
                             highestNeighbourhoodSize);
    globalStats.reportnewStats(searchParams.combinationToActivate, stats);

    if(getState().isCtrlcPressed()) {
      cout << "Ctrl-C pressed----" << std::endl;
      throw EndOfSearch();
    } else if(getOptions().timeout_active && totalTime >= getOptions().time_limit) {
      cout << "Timeout\n";
      throw EndOfSearch();
    }

    Controller::world_pop_to_depth(depth);
    return stats;
  }

  inline void copyOverIncumbent(const vector<DomainInt>& solution) {
    if(Controller::get_world_depth() != 1) {
      Controller::world_pop_to_depth(1);
    }
    Controller::world_push();
    for(int i = 0; i < nhc.shadow_mapping[0].size(); i++) {
      nhc.shadow_mapping[1][i].assign(solution[i]);
    }
    std::vector<AnyVarRef> emptyVars;
    prop->prop(emptyVars);
  }

  void propagate() {
    vector<AnyVarRef> emptyVars;
    prop->prop(emptyVars);
  }
  inline void printBestSolution() {
    if(Controller::get_world_depth() != 1) {
      Controller::world_pop_to_depth(1);
    }
    std::vector<std::pair<AnyVarRef, DomainInt>>& bestAssignment = globalStats.getBestAssignment();
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

private:
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
            auto sizeVarValOrder =
                (searchParams.nhSizeVarAscendVsRandom) ? VALORDER_ASCEND : VALORDER_RANDOM;
            addNhLocalVars(searchOrders, neighbourhood, sizeVarValOrder);
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
};

inline NeighbourhoodStats findRandomSolutionUsingNormalSearch(NeighbourhoodState& nhState) {
  // holds return value of each search iteration

  // try to find initial solution
  int initialBacktrackLimit = getOptions().nhConfig->initialBacktrackLimit;
  int attempt = 0;
  do {
    int bias = 0;
    if(attempt % 5 == 2)
      bias = 90;
    if(attempt % 5 == 3)
      bias = -90;
    nhLog("Searching for random solution, backtrack limit = " << round(initialBacktrackLimit));
    NeighbourhoodStats stats = nhState.searchNeighbourhoods(
        SearchParams::randomWalk(false, true, 0, round(initialBacktrackLimit), true, bias));
    if(stats.solutionFound) {
      return stats;
    }
    initialBacktrackLimit *= getOptions().nhConfig->initialSearchBacktrackLimitMultiplier;
    attempt++;
  } while(true);
}

template <typename SearchStrategy>
struct NeighbourhoodSearchManager : public Controller::SearchManager {
  NeighbourhoodState nhState;
  SearchStrategy searchStrategy;
  NeighbourhoodSearchManager(shared_ptr<Propagate> _prop, vector<SearchOrder> _base_order,
                             NeighbourhoodContainer _nhc)
      : nhState(_prop, std::move(_base_order), std::move(_nhc)), searchStrategy(nhState.nhc) {}

  virtual void search() {
    signal(SIGVTALRM, triggerAlarm);
    cout << getOptions().nhConfig << endl;
    cout.setf(ios::fixed, ios::floatfield);
    cout.precision(3);

    nhState.globalStats.startTimer();
    try {
      NeighbourhoodStats initialStats = findRandomSolutionUsingNormalSearch(nhState);
      debug_log("Stats on initial solution:\n" << initialStats << endl);
      searchStrategy.run(nhState, initialStats.newMinValue, nhState.solution);
    } catch(EndOfSearch&) {}
    nhState.printBestSolution();
    nhState.globalStats.printStats(cout, nhState.nhc);
    cout << endl;
    throw EndOfSearch();
  }
};

#endif
