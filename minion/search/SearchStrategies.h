#ifndef MINION_SEARCHSTRATEGIES_H
#define MINION_SEARCHSTRATEGIES_H

#include <algorithm>
#include <cmath>

/**
 * Struct containing hardcoded parameters that need to be tuned
 */
struct TunableParams {
  int iterationSearchTime = 500;
  int hillClimberMinIterationsToSpendAtPeak = 4;
  double hillClimberInitialLocalMaxProbability = 0.001;
  double hillClimberProbabilityIncrementMultiplier = 1.0 / 16;
  int holePuncherSolutionBagSizeConstant = 5;
  TunableParams() {}
};

static const TunableParams tunableParams;

struct SearchParams {
  static const int STANDARD_SEARCH = -1;
  int combinationToActivate;

  std::vector<int> neighbourhoodsToActivate;
  bool optimiseMode;
  int timeoutInMillis;
  DomainInt initialNeighbourhoodSize;
  bool restrictToFirstSolution = false;

private:
  SearchParams(int combinationToActivate, std::vector<int> neighbourhoods, bool optimiseMode,
               int timeoutInMillis, DomainInt initialNeighbourhoodSize)
      : combinationToActivate(combinationToActivate),
        neighbourhoodsToActivate(std::move(neighbourhoods)),
        optimiseMode(optimiseMode),
        timeoutInMillis(timeoutInMillis),
        initialNeighbourhoodSize(initialNeighbourhoodSize) {}

public:
  SearchParams(int combinationToActivate, const NeighbourhoodContainer& nhc, bool optimiseMode,
               int timeoutInMillis, DomainInt initialNeighbourhoodSize = 1)
      : SearchParams(combinationToActivate, nhc.neighbourhoodCombinations[combinationToActivate],
                     optimiseMode, timeoutInMillis, initialNeighbourhoodSize) {
    if(neighbourhoodsToActivate.size() > 1) {
      std::random_shuffle(neighbourhoodsToActivate.begin() + 1, neighbourhoodsToActivate.end());
    }
  }
  inline bool isStandardSearchOnly() const {
    return combinationToActivate == STANDARD_SEARCH;
  }
  static inline SearchParams standardSearch(bool optimiseMode, int timeoutInMillis) {
    return SearchParams(STANDARD_SEARCH, {}, optimiseMode, timeoutInMillis, 1);
  }
  friend inline std::ostream& operator<<(std::ostream& os, const SearchParams& searchParams) {
    os << "SearchParams(\ncombinationToActivate = " << searchParams.combinationToActivate
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
  double localMaxProbability = tunableParams.hillClimberInitialLocalMaxProbability;
  SelectionStrategy selectionStrategy;

  std::vector<DomainInt> highestNeighbourhoodSizes;

public:
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
      getState().getOptimiseVar()->setMin(stats.newMinValue + 1);
      std::vector<AnyVarRef> emptyVars;
      prop->prop(emptyVars);
    } else {
      highestNeighbourhoodSizes[currentActivatedCombination] = stats.highestNeighbourhoodSize;
      localMaxProbability +=
          (1.0 / nhc.neighbourhoodCombinations.size()) *
          tunableParams.hillClimberProbabilityIncrementMultiplier *
          (int)(iterationsSpentAtPeak > tunableParams.hillClimberMinIterationsToSpendAtPeak);
      ++iterationsSpentAtPeak;
    }
  }

  void resetLocalMaxProbability() {
    localMaxProbability = tunableParams.hillClimberInitialLocalMaxProbability;
    iterationsSpentAtPeak = 0;
  }

  SearchParams getSearchParams(NeighbourhoodContainer& nhc, NeighbourhoodSearchStats globalStats) {
    int combinationToActivate = selectionStrategy.getCombinationsToActivate(nhc, globalStats);
    return SearchParams(combinationToActivate, nhc, true, tunableParams.iterationSearchTime,
                        highestNeighbourhoodSizes[combinationToActivate]);
  }
  bool continueSearch(NeighbourhoodContainer&, std::vector<DomainInt>&) {
    return true;
  }

  bool hasFinishedPhase() {
    bool completed = searchComplete ||
                     (iterationsSpentAtPeak > tunableParams.hillClimberMinIterationsToSpendAtPeak &&
                      static_cast<double>(std::rand()) / RAND_MAX < localMaxProbability);
    if(completed) {
      std::cout << "HillClimber: completed search at opt value: "
                << getState().getOptimiseVar()->getMin() << std::endl;
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
    getState().getOptimiseVar()->setMin(newBestMinValue + 1);
    searchComplete = false;
    copyOverIncumbent(nhc, bestSolution, prop);
    std::cout << "HillClimber: Hill climbing from opt value: " << bestSolutionValue << std::endl;
  }
};

typedef std::vector<std::pair<DomainInt, std::vector<DomainInt>>> SolutionBag;

class HolePuncher {
  SolutionBag solutionBag;
  std::vector<int> activeCombinations;
  int currentNeighbourhoodSolutionsCount = 0;
  int maxSolutionsPerNeighbourhood = 1;
  bool finishedPhase = false;

public:
  int neighbourhoodSize = 0;
  void resetNeighbourhoodSize() {
    neighbourhoodSize = 1;
  }

  void incrementNeighbourhoodSize() {
    ++neighbourhoodSize;
  }

  /*
   * Check if we have run out of neighbourhoods to activate. If so shuffle the solution bag
   * so that the generated solutions are in a random configuration.
   */
  void updateStats(NeighbourhoodContainer& nhc, std::shared_ptr<Propagate>& prop,
                   int currentActivatedCombination, NeighbourhoodStats& stats,
                   std::vector<DomainInt>& solution, NeighbourhoodSearchStats&) {
    finishedPhase = activeCombinations.empty();
    if(finishedPhase) {
      std::random_shuffle(solutionBag.begin(), solutionBag.end());
      std::cout << "HolePuncher: search complete, solutionBag size = " << solutionBag.size()
                << std::endl;
    }
  }

  /**
   * Pop a neighbourhood from the vector of active neighbourhoods and return it
   * in the struct SearchParams
   * @param nhc
   * @return Struct SearchParams which contains a neighbourhood to activate
   */
  SearchParams getSearchParams(NeighbourhoodContainer& nhc, NeighbourhoodSearchStats&) {
    assert(!activeCombinations.empty());
    currentNeighbourhoodSolutionsCount = 0;
    int combination = activeCombinations.back();
    activeCombinations.pop_back();
    return SearchParams(combination, nhc, false, tunableParams.iterationSearchTime);
  }

  /*
   * Called during Minion Search once a solution has been found. If the number of solutions
   * found for the currently activated neighbourhood equals the max stop search.
   */
  bool continueSearch(NeighbourhoodContainer& nhc, const std::vector<DomainInt>& solution) {
    solutionBag.emplace_back(getState().getOptimiseVar()->getMin(), solution);
    return ++currentNeighbourhoodSolutionsCount <= maxSolutionsPerNeighbourhood;
  }

  bool hasFinishedPhase() {
    return finishedPhase;
  }

  /*
   * Generate the vector of neighbourhoods that can be activated.
   */
  void initialise(NeighbourhoodContainer& nhc, DomainInt,
                  const std::vector<DomainInt>& incumbentSolution, std::shared_ptr<Propagate>& prop,
                  NeighbourhoodSearchStats& globalStats) {
    int maxNHSize = nhc.getMaxNeighbourhoodSize();
    while(neighbourhoodSize <= maxNHSize) {
      activeCombinations.clear();
      for(int i = 0; i < nhc.neighbourhoodCombinations.size(); ++i) {
        if(nhc.isCombinationEnabled(i) &&
           nhc.neighbourhoods[nhc.neighbourhoodCombinations[i][0]].deviation.inDomain(
               neighbourhoodSize))
          activeCombinations.push_back(i);
      }
      if(!activeCombinations.empty()) {
        break;
      } else {
        ++neighbourhoodSize;
      }
    }

    if(activeCombinations.empty()) {
      std::cout << "HolePuncher: there are no neighbourhood combinations that may be "
                   "activated.\nThrowing "
                   "EndOfSearch."
                << std::endl;
      throw EndOfSearch();
    }

    std::cout << "HolePuncher: initialised search starting at neighbourhood size: "
              << neighbourhoodSize << std::endl;
    solutionBag = {};
    maxSolutionsPerNeighbourhood = (int)ceil(
        ((double)tunableParams.holePuncherSolutionBagSizeConstant) / activeCombinations.size());
    finishedPhase = false;
    globalStats.startExploration(neighbourhoodSize);
    copyOverIncumbent(nhc, incumbentSolution, prop);
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
  SearchParams getSearchParams(NeighbourhoodContainer& nhc, NeighbourhoodSearchStats& globalStats) {
    switch(currentPhase) {
    case Phase::HILL_CLIMBING: return hillClimber.getSearchParams(nhc, globalStats);
    case Phase::HOLE_PUNCHING: return holePuncher.getSearchParams(nhc, globalStats);
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
          holePuncher.incrementNeighbourhoodSize();
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
          holePuncher.incrementNeighbourhoodSize();
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
