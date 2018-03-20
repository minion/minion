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

private:
  SearchParams(int random_bias, Mode mode, int combinationToActivate,
               std::vector<int> neighbourhoods, bool nhLocalVarsComeFirst, bool optimiseMode,
               bool stopAtFirstSolution, int timeoutInMillis, int backtrackLimit,
               bool backtrackInsteadOfTimeLimit, DomainInt initialNeighbourhoodSize)
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
        initialNeighbourhoodSize(initialNeighbourhoodSize) {}

public:
  static inline SearchParams
  neighbourhoodSearch(int combinationToActivate, const NeighbourhoodContainer& nhc,
                      bool nhLocalVarsComeFirst, bool optimiseMode, bool stopAtFirstSolution,
                      int timeoutInMillis, int backtrackLimit, bool backtrackInsteadOfTimeLimit,
                      DomainInt initialNeighbourhoodSize = 1) {
    SearchParams searchParams(0, NEIGHBOURHOOD_SEARCH, combinationToActivate,
                              nhc.neighbourhoodCombinations[combinationToActivate],
                              nhLocalVarsComeFirst, optimiseMode, stopAtFirstSolution,
                              timeoutInMillis, backtrackLimit, backtrackInsteadOfTimeLimit,
                              initialNeighbourhoodSize);
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
                        timeoutInMillis, backtrackLimit, backtrackInsteadOfTimeLimit, 0);
  }
  static inline SearchParams randomWalk(bool optimiseMode, bool stopAtFirstSolution,
                                        int timeoutInMillis, int backtrackLimit,
                                        bool backtrackInsteadOfTimeLimit, int bias) {
    return SearchParams(bias, RANDOM_WALK, -1, {}, false, optimiseMode, stopAtFirstSolution,
                        timeoutInMillis, backtrackLimit, backtrackInsteadOfTimeLimit, 0);
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

template <typename SelectionStrategy>
class HillClimbingSearch {
  friend MetaStrategy<SelectionStrategy>;
  int iterationsSpentAtPeak = 0;
  bool searchComplete = false;
  // The probability that we will enter a random mode of exploration.
  double localMaxProbability = getOptions().nhConfig.hillClimberInitialLocalMaxProbability;
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

    if(stats.solutionFound) {
      highestNeighbourhoodSizes.assign(nhc.neighbourhoodCombinations.size(), 1);
      bestSolutionValue = stats.newMinValue;
      bestSolution = solution;
      resetLocalMaxProbability();
      copyOverIncumbent(nhc, solution, prop);
      if(stats.newMinValue == getState().getOptimiseVar()->getMax()) {
        searchComplete = true;
        std::cout << "HillClimber: achieved max possible opt value : " << stats.newMinValue
                  << std::endl;
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
        getOptions().nhConfig.backtrackLimit, getOptions().nhConfig.backtrackInsteadOfTimeLimit,
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
      std::cout << "HillClimber: completed search at opt value: " << bestSolutionValue << std::endl;
      std::cout << "Number iterations spent at peak: " << iterationsSpentAtPeak << std::endl;
    }
    return completed;
  }

  void initialise(NeighbourhoodContainer& nhc, DomainInt newBestMinValue,
                  const std::vector<DomainInt>& newBestSolution, std::shared_ptr<Propagate>& prop,
                  NeighbourhoodSearchStats& globalStats) {
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

    std::cout << "HillClimber: Hill climbing from opt value: " << bestSolutionValue << std::endl;
  }
};

typedef std::vector<std::pair<DomainInt, std::vector<DomainInt>>> SolutionBag;

class HolePuncher {
  SolutionBag solutionBag;
  std::vector<int> activeCombinations;
  int currentNeighbourhoodSolutionsCount = 0;
  int maxSolutionsPerCombination = 1;
  bool finishedPhase = false;
  bool randomWalk = false;

public:
  int minNeighbourhoodSize = 1;
  int neighbourhoodSizeOffset = 0;

  void resetNeighbourhoodSize() {
    minNeighbourhoodSize = 1;
    neighbourhoodSizeOffset = 0;
    randomWalk = false;
  }

  void nextNeighbourhoodSize() {
    minNeighbourhoodSize *= (1 + !randomWalk);
  }

  /*
   * Check if we have run out of neighbourhoods to activate. If so shuffle the solution bag
   * so that the generated solutions are in a random configuration.
   */
  void updateStats(NeighbourhoodContainer& nhc, std::shared_ptr<Propagate>& prop,
                   int currentActivatedCombination, NeighbourhoodStats& stats,
                   std::vector<DomainInt>& solution, NeighbourhoodSearchStats&) {
    finishedPhase = activeCombinations.empty() || randomWalk;
    if(finishedPhase) {
      if(randomWalk) {
        if(stats.solutionFound) {
          solutionBag.emplace_back(stats.newMinValue, solution);
        } else {
          cout << "HolePuncher: unable to find any random solutions.\n";
          throw EndOfSearch();
        }
      } else {
        std::random_shuffle(solutionBag.begin(), solutionBag.end());
      }
      std::cout << "HolePuncher: search complete, solutionBag size = " << solutionBag.size()
                << std::endl;
    }
  }

  /**
   * Pop a neighbourhood combination from the vector of active neighbourhoods and return it
   * in the struct SearchParams.  Unless in Random walk mode.
   * @param nhc
   * @return Struct SearchParams which contains a neighbourhood to activate
   */
  SearchParams getSearchParams(NeighbourhoodContainer& nhc, NeighbourhoodSearchStats&) {
    if(randomWalk) {
      return SearchParams::randomWalk(false, true, 0, 0, 0,
                                      getOptions().nhConfig.backtrackInsteadOfTimeLimit);
    }
    assert(!activeCombinations.empty());
    currentNeighbourhoodSolutionsCount = 0;
    int combination = activeCombinations.back();
    activeCombinations.pop_back();
    return SearchParams::neighbourhoodSearch(
        combination, nhc, true, false, false, getOptions().nhConfig.iterationSearchTime,
        getOptions().nhConfig.backtrackLimit, getOptions().nhConfig.backtrackInsteadOfTimeLimit,
        currentNeighbourhoodSize());
  }

  /*
   * Called during Minion Search once a solution has been found. If the number of solutions
   * found for the currently activated neighbourhood equals the max stop search.
   */
  bool continueSearch(NeighbourhoodContainer& nhc, const std::vector<DomainInt>& solution) {
    solutionBag.emplace_back(getState().getOptimiseVar()->getMin(), solution);
    return ++currentNeighbourhoodSolutionsCount <= maxSolutionsPerCombination;
  }

  bool hasFinishedPhase() {
    return finishedPhase;
  }

  inline int currentNeighbourhoodSize() const {
    return minNeighbourhoodSize + neighbourhoodSizeOffset;
  }

  /*
   * Generate the vector of neighbourhoods that can be activated.
   */
  void initialise(NeighbourhoodContainer& nhc, DomainInt,
                  const std::vector<DomainInt>& incumbentSolution, std::shared_ptr<Propagate>& prop,
                  NeighbourhoodSearchStats& globalStats) {
    if(randomWalk) {
      cout << "HolePuncher: fetching another random solution:\n";
      Controller::world_pop_to_depth(1);
    } else {
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

      if(activeCombinations.empty()) {
        std::cout << "HolePuncher: there are no neighbourhood combinations that may be "
                     "activated.  Fetching a random solution:\n";
        randomWalk = true;
        Controller::world_pop_to_depth(1);
      } else {
        std::cout << "HolePuncher: initialised search starting at neighbourhood size: "
                  << currentNeighbourhoodSize() << std::endl;
        maxSolutionsPerCombination =
            (int)ceil(((double)getOptions().nhConfig.holePuncherSolutionBagSizeConstant) /
                      activeCombinations.size());
        globalStats.startExploration(currentNeighbourhoodSize());
        copyOverIncumbent(nhc, incumbentSolution, prop);
      }
    }
    solutionBag = {};
    finishedPhase = false;
  }

  SolutionBag& getSolutionBag() {
    return solutionBag;
  }
};

template <typename SelectionStrategy>
class MetaStrategy {
  enum class Phase { HILL_CLIMBING, HOLE_PUNCHING };
  HillClimbingSearch<SelectionStrategy> hillClimber;
  HolePuncher holePuncher;
  DomainInt bestSolutionValue;
  std::vector<DomainInt> bestSolution;
  Phase currentPhase = Phase::HILL_CLIMBING;
  SolutionBag solutionBag;
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
        if(hillClimber.bestSolutionValue > bestSolutionValue) {
          bestSolutionValue = hillClimber.bestSolutionValue;
          bestSolution = std::move(hillClimber.bestSolution);
          std::cout << "MetaStrategy: new best value achieved, caching solution\n";
          /*
           * If a better solution has been found we want to punch random holes around this
           * solution
           */
          solutionBag.clear();
          currentPhase = Phase::HOLE_PUNCHING;
          holePuncher.resetNeighbourhoodSize();
          holePuncher.initialise(nhc, bestSolutionValue, bestSolution, prop, globalStats);
        } else if(solutionBag.empty()) {
          std::cout << "MetaStrategy: new best value not achieved, solutionBag empty\n";
          // If there are no solutions left want to generate new random solutions for a larger
          // neighbourhood size
          currentPhase = Phase::HOLE_PUNCHING;
          holePuncher.nextNeighbourhoodSize();
          holePuncher.initialise(nhc, bestSolutionValue, bestSolution, prop, globalStats);
        } else {
          std::cout << "MetaStrategy: new best value not achieved, trying hill climbing from next "
                       "solution in solution bag\n";
          globalStats.totalNumberOfRandomSolutionsPulled += 1;
          globalStats.numberPulledThisPhase += 1;
          // Grab a random solution
          hillClimber.initialise(nhc, solutionBag.back().first, solutionBag.back().second, prop,
                                 globalStats);
          solutionBag.pop_back();
        }
      }
      break;
    // In this phase we want to generate random solutions
    case Phase::HOLE_PUNCHING: {
      holePuncher.updateStats(nhc, prop, currentActivatedCombination, stats, solution, globalStats);
      if(holePuncher.hasFinishedPhase()) {
        solutionBag = std::move(holePuncher.getSolutionBag());
        if(solutionBag.size() > nhc.neighbourhoodCombinations.size()) {
          solutionBag.resize(nhc.neighbourhoodCombinations.size());
        }
        std::cout << "MetaStrategy: trimmed solution bag to size " << solutionBag.size()
                  << std::endl;
        if(!solutionBag.empty()) {
          currentPhase = Phase::HILL_CLIMBING;
          hillClimber.initialise(nhc, solutionBag.back().first, solutionBag.back().second, prop,
                                 globalStats);
          globalStats.totalNumberOfRandomSolutionsPulled += 1;
          globalStats.numberPulledThisPhase += 1;
          solutionBag.pop_back();
        } else {
          holePuncher.nextNeighbourhoodSize();
          holePuncher.initialise(nhc, bestSolutionValue, bestSolution, prop, globalStats);
        }
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
