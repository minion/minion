#ifndef MINION_SEARCHSTRATEGIES_H
#define MINION_SEARCHSTRATEGIES_H

#include <algorithm>
#include <cmath>

/**
 * Struct containing hardcoded parameters that need to be tuned
 */

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
               bool backtrackInsteadOfTimeLimit, DomainInt initialNeighbourhoodSize, bool nhSizeVarAscendVsRandom)
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
        initialNeighbourhoodSize(initialNeighbourhoodSize), nhSizeVarAscendVsRandom(nhSizeVarAscendVsRandom) {}

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

inline void copyOverIncumbent(NeighbourhoodContainer& nhc, const vector<DomainInt>& solution,
                              std::shared_ptr<Propagate>& prop) {
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

template <typename>
class MetaStrategy;

void incrementBacktrackLimit(double& backtrackLimit) {
  backtrackLimit *= getOptions().nhConfig.hillClimberBacktrackLimitMultiplier;
  backtrackLimit += getOptions().nhConfig.hillClimberBacktrackLimitIncrement;
}

template <typename SelectionStrategy>
class HillClimbingSearch {
  friend MetaStrategy<HillClimbingSearch<SelectionStrategy>>;
  int iterationsSpentAtPeak = 0;
  int numberIterationsAtStart;
  bool searchComplete = false;
  // The probability that we will enter a random mode of exploration.
  double localMaxProbability = getOptions().nhConfig.hillClimberInitialLocalMaxProbability;
  double backtrackLimit;
  SelectionStrategy selectionStrategy;

public:
  HillClimbingSearch(const NeighbourhoodContainer& nhc) : selectionStrategy(nhc) {}
  std::vector<DomainInt> highestNeighbourhoodSizes;

  DomainInt bestSolutionValue;
  std::vector<DomainInt> bestSolution;

  /*
   * If a solution is found reset the probability of random exploration and
   * copy over the incumbent otherwise increase the probability that we will enter random
   * exploration mode.
   */
  void updateStats(NeighbourhoodContainer& nhc, std::shared_ptr<Propagate>& prop,
                   int currentActivatedCombination, NeighbourhoodStats& stats,
                   std::vector<DomainInt>& solution, NeighbourhoodSearchStats&) {

    selectionStrategy.updateStats(currentActivatedCombination, stats);
    if(!getOptions().nhConfig.hillClimberIncreaseBacktrackOnlyOnFailure || !stats.solutionFound) {
      incrementBacktrackLimit(backtrackLimit);
    }
    if(stats.solutionFound) {
      highestNeighbourhoodSizes.assign(nhc.neighbourhoodCombinations.size(), 1);
      bestSolutionValue = stats.newMinValue;
      bestSolution = solution;
      resetLocalMaxProbability();
      copyOverIncumbent(nhc, solution, prop);
      if(stats.newMinValue == getState().getOptimiseVar()->getMax()) {
        searchComplete = true;
        nhLog("HillClimber: achieved max possible opt value : " << stats.newMinValue);
        throw EndOfSearch();
        return;
      }
      getState().getOptimiseVar()->setMin(stats.newMinValue);
      std::vector<AnyVarRef> emptyVars;
      prop->prop(emptyVars);
    } else {
      highestNeighbourhoodSizes[currentActivatedCombination] = stats.highestNeighbourhoodSize;
      localMaxProbability += (1.0 / nhc.neighbourhoodCombinations.size()) *
                             getOptions().nhConfig.hillClimberProbabilityIncrementMultiplier *
                             (int)(iterationsSpentAtPeak >
                                   getOptions().nhConfig.hillClimberMinIterationsToSpendAtPeak);
      ++iterationsSpentAtPeak;
    }
  }

  void resetLocalMaxProbability() {
    localMaxProbability = getOptions().nhConfig.hillClimberInitialLocalMaxProbability;
    iterationsSpentAtPeak = 0;
  }

  SearchParams getSearchParams(NeighbourhoodContainer& nhc, NeighbourhoodSearchStats globalStats) {
    int combinationToActivate = selectionStrategy.getCombinationsToActivate(
        nhc, globalStats, getState().getOptimiseVar()->getMin());
    return SearchParams::neighbourhoodSearch(
        combinationToActivate, nhc, true, true, false, getOptions().nhConfig.iterationSearchTime,
        round(backtrackLimit), getOptions().nhConfig.backtrackInsteadOfTimeLimit,
        highestNeighbourhoodSizes[combinationToActivate]);
  }
  bool continueSearch(NeighbourhoodContainer&, std::vector<DomainInt>&) {
    return true;
  }

  bool hasFinishedPhase() {
    bool completed =
        searchComplete ||
        (iterationsSpentAtPeak > getOptions().nhConfig.hillClimberMinIterationsToSpendAtPeak &&
         static_cast<double>(std::rand()) / RAND_MAX < localMaxProbability);
    if(completed) {
      nhLog("HillClimber: completed search at opt value: " << bestSolutionValue << endl
                                                           << "Number iterations spent at peak: "
                                                           << iterationsSpentAtPeak);
      cout << "HillClimber: backtrack limit = " << round(backtrackLimit) << endl;
    }
    return completed;
  }

  void initialise(NeighbourhoodContainer& nhc, DomainInt newBestMinValue,
                  const std::vector<DomainInt>& newBestSolution, std::shared_ptr<Propagate>& prop,
                  NeighbourhoodSearchStats& globalStats) {
    backtrackLimit = getOptions().nhConfig.initialBacktrackLimit;
    cout << "HillClimber: backtrack limit = " << round(backtrackLimit) << endl;
    numberIterationsAtStart = globalStats.numberIterations;
    globalStats.notifyStartHillClimb();
    resetLocalMaxProbability();
    highestNeighbourhoodSizes.assign(nhc.neighbourhoodCombinations.size(), 1);
    iterationsSpentAtPeak = 0;

    bestSolutionValue = newBestMinValue;
    bestSolution = newBestSolution;
    searchComplete = false;
    copyOverIncumbent(nhc, bestSolution, prop);
    getState().getOptimiseVar()->setMin(newBestMinValue);
    std::vector<AnyVarRef> emptyVars;
    prop->prop(emptyVars);

    nhLog("HillClimber: Hill climbing from opt value: " << bestSolutionValue);
  }
};

template <typename SelectionStrategy>
class LateAcceptanceHillClimbingSearch {
public:
  friend MetaStrategy<LateAcceptanceHillClimbingSearch<SelectionStrategy>>;
  int iterationsSpentAtPeak = 0;
  int numberIterationsAtStart;
  bool searchComplete = false;
  double backtrackLimit;
  SelectionStrategy selectionStrategy;
  std::vector<DomainInt> highestNeighbourhoodSizes;

  DomainInt bestSolutionValue;
  DomainInt currentSolutionValue;
  std::vector<DomainInt> currentSolution;
  std::vector<DomainInt> bestSolution;
  std::deque<DomainInt> recentSolutionValueQueue;

  LateAcceptanceHillClimbingSearch(const NeighbourhoodContainer& nhc) : selectionStrategy(nhc) {}

  void updateStats(NeighbourhoodContainer& nhc, std::shared_ptr<Propagate>& prop,
                   int currentActivatedCombination, NeighbourhoodStats& stats,
                   std::vector<DomainInt>& solution, NeighbourhoodSearchStats&) {

    selectionStrategy.updateStats(currentActivatedCombination, stats);
    if(!getOptions().nhConfig.hillClimberIncreaseBacktrackOnlyOnFailure || !stats.solutionFound) {
      incrementBacktrackLimit(backtrackLimit);
    }
    if(stats.solutionFound && stats.newMinValue > bestSolutionValue) {
      iterationsSpentAtPeak = 0;
      bestSolutionValue = stats.newMinValue;
      bestSolution = solution;
    } else {
      ++iterationsSpentAtPeak;
    }

    if(stats.solutionFound) {
      if(stats.newMinValue == getState().getOptimiseVar()->getMax()) {
        searchComplete = true;
        nhLog("lahc: achieved max possible opt value : " << stats.newMinValue);
        throw EndOfSearch();
        return;
      }
      highestNeighbourhoodSizes.assign(nhc.neighbourhoodCombinations.size(), 1);
      recentSolutionValueQueue.push_back(stats.newMinValue);
      recentSolutionValueQueue.pop_front();
      currentSolutionValue = stats.newMinValue;

      currentSolution = solution;
    } else {
      recentSolutionValueQueue.push_back(currentSolutionValue);
      recentSolutionValueQueue.pop_front();
      highestNeighbourhoodSizes[currentActivatedCombination] = stats.highestNeighbourhoodSize;
    }

    copyOverIncumbent(nhc, currentSolution, prop);
    getState().getOptimiseVar()->setMin(
        std::min(currentSolutionValue, recentSolutionValueQueue.front()));
    std::vector<AnyVarRef> emptyVars;
    prop->prop(emptyVars);
  }

  SearchParams getSearchParams(NeighbourhoodContainer& nhc, NeighbourhoodSearchStats globalStats) {
    int combinationToActivate = selectionStrategy.getCombinationsToActivate(
        nhc, globalStats, getState().getOptimiseVar()->getMin());
    // warning, needs to be updated to support updating backtrack limit rather than using constant.
    return SearchParams::neighbourhoodSearch(combinationToActivate, nhc, true, true, false,
                                             getOptions().nhConfig.iterationSearchTime, 22,
                                             getOptions().nhConfig.backtrackInsteadOfTimeLimit,
                                             highestNeighbourhoodSizes[combinationToActivate]);
  }

  bool continueSearch(NeighbourhoodContainer&, std::vector<DomainInt>&) {
    return true;
  }

  bool hasFinishedPhase() {
    int iterationLimit =
        round(getOptions().nhConfig.lahcQueueSize * getOptions().nhConfig.lahcStoppingLimitRatio);
    bool completed = searchComplete || iterationsSpentAtPeak > iterationLimit;
    if(completed) {
      nhLog("lahc: completed search at opt value: " << bestSolutionValue << endl
                                                    << "Number iterations spent at peak: "
                                                    << iterationsSpentAtPeak);
    }
    return completed;
  }

  void initialise(NeighbourhoodContainer& nhc, DomainInt newBestMinValue,
                  const std::vector<DomainInt>& newBestSolution, std::shared_ptr<Propagate>& prop,
                  NeighbourhoodSearchStats& globalStats) {
    backtrackLimit = getOptions().nhConfig.initialBacktrackLimit;
    numberIterationsAtStart = globalStats.numberIterations;
    globalStats.notifyStartHillClimb();
    highestNeighbourhoodSizes.assign(nhc.neighbourhoodCombinations.size(), 1);
    iterationsSpentAtPeak = 0;

    bestSolutionValue = newBestMinValue;
    bestSolution = newBestSolution;

    currentSolutionValue = bestSolutionValue;
    currentSolution = newBestSolution;
    searchComplete = false;
    recentSolutionValueQueue.assign(getOptions().nhConfig.lahcQueueSize, currentSolutionValue);

    copyOverIncumbent(nhc, bestSolution, prop);
    getState().getOptimiseVar()->setMin(newBestMinValue);
    std::vector<AnyVarRef> emptyVars;
    prop->prop(emptyVars);
    nhLog("lahc: lahcQueueSize = " << getOptions().nhConfig.lahcQueueSize); // NGUYEN: DEBUG
    nhLog("lahc: lahcStoppingLimitRatio = "
          << getOptions().nhConfig.lahcStoppingLimitRatio); // NGUYEN: DEBUG
    nhLog("lahc: Hill climbing from opt value: " << bestSolutionValue);
  }
};

template <typename SelectionStrategy>
class SimulatedAnnealingSearch {
public:
  friend MetaStrategy<LateAcceptanceHillClimbingSearch<SelectionStrategy>>;
  int iterationsSpentAtPeak = 0;
  int numberIterationsAtStart;
  bool searchComplete = false;
  double backtrackLimit;
  SelectionStrategy selectionStrategy;
  double temperature;
  int numberIterationsSinceLastCool = 0;

  DomainInt currentSolutionValue;
  DomainInt bestSolutionValue;
  std::vector<DomainInt> currentSolution;
  std::vector<DomainInt> bestSolution;

  SimulatedAnnealingSearch(const NeighbourhoodContainer& nhc) : selectionStrategy(nhc) {}

  void updateStats(NeighbourhoodContainer& nhc, std::shared_ptr<Propagate>& prop,
                   int currentActivatedCombination, NeighbourhoodStats& stats,
                   std::vector<DomainInt>& solution, NeighbourhoodSearchStats&) {
    // selection strategies use solution found to mean an improvement.  So give the selection
    // strategy altered stats where solution found is true only if the solution was actually
    // improved.
    int delta = (stats.solutionFound) ? checked_cast<int>(stats.newMinValue - currentSolutionValue) : 0;
    NeighbourhoodStats alteredStats = stats;
    alteredStats.solutionFound = stats.solutionFound && delta > 0;
    selectionStrategy.updateStats(currentActivatedCombination, alteredStats);

    if(!getOptions().nhConfig.hillClimberIncreaseBacktrackOnlyOnFailure || !stats.solutionFound ||
       delta < 0) {
      incrementBacktrackLimit(backtrackLimit);
    }
    if(stats.solutionFound && stats.newMinValue > bestSolutionValue) {
      iterationsSpentAtPeak = 0;
      bestSolutionValue = stats.newMinValue;
      bestSolution = solution;
    } else {
      ++iterationsSpentAtPeak;
    }
    if(stats.newMinValue == getState().getOptimiseVar()->getMax()) {
      searchComplete = true;
      nhLog("sa: achieved max possible opt value : " << stats.newMinValue);
      throw EndOfSearch();
      return;
    }
    bool solutionAccepted;
    if(!stats.solutionFound) {
      solutionAccepted = false;
    } else if(delta >= 0) {
      solutionAccepted = true;
    } else {
      double acceptanceProb = exp(delta / temperature);
      solutionAccepted = rand() <= acceptanceProb;
    }
    if(solutionAccepted) {
      currentSolutionValue = stats.newMinValue;
      currentSolution = solution;
    }

    copyOverIncumbent(nhc, currentSolution, prop);
    std::vector<AnyVarRef> emptyVars;
    prop->prop(emptyVars);
    if (++numberIterationsSinceLastCool > getOptions().nhConfig.simulatedAnnealingIterationsBetweenCool) {
        temperature *= getOptions().nhConfig.simulatedAnnealingTemperatureCoolingFactor;
    }
  }

  SearchParams getSearchParams(NeighbourhoodContainer& nhc, NeighbourhoodSearchStats globalStats) {
    int combinationToActivate = selectionStrategy.getCombinationsToActivate(
        nhc, globalStats, getState().getOptimiseVar()->getMin());
    return SearchParams::neighbourhoodSearch(combinationToActivate, nhc, true, true, false,
                                             getOptions().nhConfig.iterationSearchTime, round(backtrackLimit),
                                             getOptions().nhConfig.backtrackInsteadOfTimeLimit,
                                             1, false);
  }

  bool continueSearch(NeighbourhoodContainer&, std::vector<DomainInt>&) {
    return true;
  }

  bool hasFinishedPhase() {
    int iterationLimit = 10; // todo
    bool completed = searchComplete || iterationsSpentAtPeak > iterationLimit;
    if(completed) {
      nhLog("lahc: completed search at opt value: " << bestSolutionValue << endl
                                                    << "Number iterations spent at peak: "
                                                    << iterationsSpentAtPeak);
    }
    return completed;
  }

  void initialise(NeighbourhoodContainer& nhc, DomainInt newBestMinValue,
                  const std::vector<DomainInt>& newBestSolution, std::shared_ptr<Propagate>& prop,
                  NeighbourhoodSearchStats& globalStats) {
    backtrackLimit = getOptions().nhConfig.initialBacktrackLimit;
    numberIterationsAtStart = globalStats.numberIterations;
    globalStats.notifyStartHillClimb();
    iterationsSpentAtPeak = 0;

    bestSolutionValue = newBestMinValue;
    bestSolution = newBestSolution;

    currentSolutionValue = bestSolutionValue;
    currentSolution = newBestSolution;
    searchComplete = false;

    copyOverIncumbent(nhc, bestSolution, prop);
    getState().getOptimiseVar()->setMin(newBestMinValue);
    std::vector<AnyVarRef> emptyVars;
    prop->prop(emptyVars);
    nhLog("sa: climbing from opt value: " << bestSolutionValue);
  }
};

class HolePuncher {
  std::vector<int> activeCombinations;
  bool randomWalk = false;
  int minNeighbourhoodSize = 0;
  int neighbourhoodSizeOffset = 0;
  double backtrackLimit = getOptions().nhConfig.initialBacktrackLimit;

public:
  bool solutionFound = false;
  std::pair<DomainInt, std::vector<DomainInt>> solution;

  void resetNeighbourhoodSize() {
    minNeighbourhoodSize = 0;
    neighbourhoodSizeOffset = 0;
    randomWalk = false;
  }

  void increaseMinNeighbourhoodSize() {
    if(minNeighbourhoodSize == 0) {
      minNeighbourhoodSize = 1;
    } else {
      minNeighbourhoodSize *= (1 + !randomWalk);
    }
    neighbourhoodSizeOffset = 0;
  }

  void updateStats(NeighbourhoodContainer& nhc, std::shared_ptr<Propagate>& prop,
                   int currentActivatedCombination, NeighbourhoodStats& stats,
                   std::vector<DomainInt>& solution, NeighbourhoodSearchStats& globalStats) {

    solutionFound = stats.solutionFound;
    if(solutionFound) {
      nhLog("HolePuncher: solution found.");
      this->solution = make_pair(stats.newMinValue, solution);
    } else {
      nhLog("HolePuncher: solution not found.");
      backtrackLimit *= getOptions().nhConfig.holePuncherBacktrackLimitMultiplier;
    }

    if(randomWalk) {
      globalStats.notifyEndRandomSearch();
      if(!stats.solutionFound) {
        nhLog("HolePuncher: unable to find any random solutions.");
        throw EndOfSearch();
      }
    } else {
      globalStats.notifyEndExploration();
    }
  }

  SearchParams getSearchParams(NeighbourhoodContainer& nhc, NeighbourhoodSearchStats&) {
    if(randomWalk) {
      return SearchParams::randomWalk(false, true, 0, 0, 0,
                                      getOptions().nhConfig.backtrackInsteadOfTimeLimit);
    }
    assert(!activeCombinations.empty());
    int combination = activeCombinations.back();
    activeCombinations.pop_back();
    return SearchParams::neighbourhoodSearch(
        combination, nhc, true, false, true, getOptions().nhConfig.iterationSearchTime,
        round(backtrackLimit), getOptions().nhConfig.backtrackInsteadOfTimeLimit,
        currentNeighbourhoodSize());
  }

  bool continueSearch(NeighbourhoodContainer& nhc, const std::vector<DomainInt>& solution) {
    return false;
  }

  inline bool hasFinishedPhase() {
    return true;
  }

  inline int currentNeighbourhoodSize() const {
    return minNeighbourhoodSize + neighbourhoodSizeOffset;
  }

  void initialise(NeighbourhoodContainer& nhc, DomainInt,
                  const std::vector<DomainInt>& incumbentSolution, std::shared_ptr<Propagate>& prop,
                  NeighbourhoodSearchStats& globalStats) {
    solutionFound = false;
    if(!randomWalk && activeCombinations.empty()) {
      increaseMinNeighbourhoodSize();
      findNextNeighbourhoodSizeWithActiveCombinations(nhc);
      if(activeCombinations.empty()) {
        randomWalk = true;
        nhLog("HolePuncher: reached neighbourhood size limit.");
      } else {
        nhLog("HolePuncher: increasing neighbourhood size to " << currentNeighbourhoodSize());
      }
    }

    if(!randomWalk) {
      globalStats.notifyStartExploration();
      nhLog("HolePuncher: searching for solution in neighbourhood with size "
            << currentNeighbourhoodSize());
      copyOverIncumbent(nhc, incumbentSolution, prop);
    } else {
      nhLog("HolePuncher: fetching an entirely random solution");
      globalStats.notifyStartRandomSearch();
      Controller::world_pop_to_depth(1);
    }
  }

  inline void findNextNeighbourhoodSizeWithActiveCombinations(const NeighbourhoodContainer& nhc) {
    int maxNHSize = nhc.getMaxNeighbourhoodSize();
    while(currentNeighbourhoodSize() <= maxNHSize) {
      activeCombinations.clear();
      for(int i = 0; i < nhc.neighbourhoodCombinations.size(); ++i) {
        if(nhc.isCombinationEnabled(i) &&
           nhc.neighbourhoods[nhc.neighbourhoodCombinations[i][0]].deviation.inDomain(
               currentNeighbourhoodSize()))
          activeCombinations.push_back(i);
      }
      if(!activeCombinations.empty()) {
        break;
      } else {
        ++neighbourhoodSizeOffset;
      }
    }
    if(!activeCombinations.empty()) {
      std::random_shuffle(activeCombinations.begin(), activeCombinations.end());
    }
  }
};

template <typename SearchStrategy>
class MetaStrategy {
  enum class Phase { HILL_CLIMBING, HOLE_PUNCHING };
  SearchStrategy hillClimber;
  HolePuncher holePuncher;
  DomainInt bestSolutionValue;
  std::vector<DomainInt> bestSolution;
  Phase currentPhase = Phase::HILL_CLIMBING;
  bool searchEnded = false;

public:
  MetaStrategy(const NeighbourhoodContainer& nhc) : hillClimber(nhc){};

  SearchParams getSearchParams(NeighbourhoodContainer& nhc, NeighbourhoodSearchStats& globalStats) {
    switch(currentPhase) {
    case Phase::HILL_CLIMBING: return hillClimber.getSearchParams(nhc, globalStats);
    case Phase::HOLE_PUNCHING: return holePuncher.getSearchParams(nhc, globalStats);
    default: assert(false); abort();
    }
  }

  bool continueSearch(NeighbourhoodContainer& nhc, std::vector<DomainInt>& solution) {
    switch(currentPhase) {
    case Phase::HILL_CLIMBING: return hillClimber.continueSearch(nhc, solution);
    case Phase::HOLE_PUNCHING: return holePuncher.continueSearch(nhc, solution);
    }
  }

  int numberOfRandomSolutionsPulled = 0;

  void updateStats(NeighbourhoodContainer& nhc, std::shared_ptr<Propagate>& prop,
                   int currentActivatedCombination, NeighbourhoodStats& stats,
                   std::vector<DomainInt>& solution, NeighbourhoodSearchStats& globalStats) {
    switch(currentPhase) {
    case Phase::HILL_CLIMBING:
      hillClimber.updateStats(nhc, prop, currentActivatedCombination, stats, solution, globalStats);
      if(hillClimber.hasFinishedPhase()) {
        globalStats.notifyEndHillClimb();
        cout << "Number iterations spent during this hill climb: "
             << (globalStats.numberIterations - hillClimber.numberIterationsAtStart) << endl;
        if(hillClimber.bestSolutionValue > bestSolutionValue) {
          bestSolutionValue = hillClimber.bestSolutionValue;
          bestSolution = std::move(hillClimber.bestSolution);
          nhLog("MetaStrategy: new best value achieved, caching solution");
          holePuncher.resetNeighbourhoodSize();
        } else {
          nhLog("MetaStrategy: new best value not achieved");
        }
        currentPhase = Phase::HOLE_PUNCHING;
        holePuncher.initialise(nhc, bestSolutionValue, bestSolution, prop, globalStats);
      }
      break;
    // In this phase we want to generate random solutions
    case Phase::HOLE_PUNCHING: {
      holePuncher.updateStats(nhc, prop, currentActivatedCombination, stats, solution, globalStats);
      if(holePuncher.solutionFound) {
        currentPhase = Phase::HILL_CLIMBING;
        hillClimber.initialise(nhc, holePuncher.solution.first, holePuncher.solution.second, prop,
                               globalStats);
      } else {
        holePuncher.initialise(nhc, bestSolutionValue, bestSolution, prop, globalStats);
      }
      break;
    }
    }
  }

  void initialise(NeighbourhoodContainer& nhc, DomainInt newBestMinValue,
                  const std::vector<DomainInt>& newBestSolution, std::shared_ptr<Propagate>& prop,
                  NeighbourhoodSearchStats& globalStats) {
    bestSolutionValue = newBestMinValue;
    bestSolution = newBestSolution;
    currentPhase = Phase::HILL_CLIMBING;
    hillClimber.initialise(nhc, newBestMinValue, newBestSolution, prop, globalStats);
  }
  bool hasFinishedPhase() {
    return false; // tbc
  }
};

#endif // MINION_SEARCHSTRATEGIES_H
