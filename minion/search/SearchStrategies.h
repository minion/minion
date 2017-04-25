
#ifndef MINION_SEARCHSTRATEGIES_H
#define MINION_SEARCHSTRATEGIES_H

#include <algorithm>
#include <cmath>

struct SearchParams {
  std::vector<int> neighbourhoodsToActivate;
  bool optimiseMode;
  int timeoutInMillis;
  DomainInt initialNeighbourhoodSize;

  SearchParams(std::vector<int> neighbourhoods, bool optimiseMode = true, int timeoutInMillis = 500, DomainInt initialNeighbourhoodSize = 1)
      : neighbourhoodsToActivate(std::move(neighbourhoods)),
        optimiseMode(optimiseMode),
        timeoutInMillis(timeoutInMillis),
        initialNeighbourhoodSize(initialNeighbourhoodSize){}
  friend inline std::ostream& operator<<(std::ostream& os, const SearchParams& searchParams) {
    os << "SearchParams(\nneighbourhoodsToActivate =  " << searchParams.neighbourhoodsToActivate
       << ",\noptimiseMode = " << searchParams.optimiseMode
       << ",\ntimeoutInMillis = " << searchParams.timeoutInMillis <<
        ",\ninitialNeighbourhoodSize = " << searchParams.initialNeighbourhoodSize << ")";
    return os;
  }
};

inline void copyOverIncumbent(NeighbourhoodContainer& nhc, const vector<DomainInt>& solution,
                              std::shared_ptr<Propagate>& prop) {
  if(Controller::get_world_depth() != 1) {
    Controller::world_pop();
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
public:
  friend MetaStrategy<SelectionStrategy>;

  static constexpr double INITIAL_LOCAL_MAX_PROBABILITY = 0.01;
  static constexpr double probabilityIncrementConstant = 0.5;
  DomainInt bestSolutionValue;
  std::vector<DomainInt> bestSolution;
  bool searchComplete = false;
  // The probability that we will enter a random mode of exploration.
  double localMaxProbability = INITIAL_LOCAL_MAX_PROBABILITY;
  SelectionStrategy selectionStrategy;

  std::vector<DomainInt> highestNeighbourhoodValues;



  /*
   * If a solution is found reset the probability of random exploration and
   * copy over the incumbent otherwise increase the probability that we will enter random
   * exploration mode.
   */
  void updateStats(NeighbourhoodContainer& nhc, std::shared_ptr<Propagate>& prop,
                   std::vector<int>& currentActivatedNeighbourhoods, NeighbourhoodStats & stats,
                   std::vector<DomainInt>& solution, NeighbourhoodSearchStats &) {
    debug_log("Hill Climbing Search -- Update Stats " << std::endl);

    selectionStrategy.updateStats(currentActivatedNeighbourhoods, stats);

    if(stats.solutionFound) {
      highestNeighbourhoodValues.assign(highestNeighbourhoodValues.size(), 1);
      bestSolutionValue = stats.newMinValue;
      bestSolution = solution;
      resetLocalMaxProbability();
      copyOverIncumbent(nhc, solution, prop);
      if(stats.newMinValue == getState().getOptimiseVar()->getMax()) {
        searchComplete = true;
        return;
      }
      getState().getOptimiseVar()->setMin(stats.newMinValue + 1);
    } else {
      highestNeighbourhoodValues[currentActivatedNeighbourhoods[0]] = stats.highestNeighbourhoodSize;
      localMaxProbability += (1.0 / nhc.neighbourhoods.size()) * probabilityIncrementConstant;
    }
  }

  void resetLocalMaxProbability() {
    localMaxProbability = INITIAL_LOCAL_MAX_PROBABILITY;
  }

  SearchParams getSearchParams(NeighbourhoodContainer& nhc, NeighbourhoodSearchStats globalStats) {
    std::vector<int> neighbourhoodsToActivate = selectionStrategy.getNeighbourhoodsToActivate(nhc, globalStats);
    if (highestNeighbourhoodValues[neighbourhoodsToActivate[0]] != 1)
      std::cout << "NEIGHBOURHOOD SIZE NOT STARTING AT 1 " << std::endl;
    return SearchParams(neighbourhoodsToActivate, true, 500, highestNeighbourhoodValues[neighbourhoodsToActivate[0]]);
  }

  bool continueSearch(NeighbourhoodContainer&, std::vector<DomainInt>&) {
    return true;
  }

  bool hasFinishedPhase() {
    debug_log("Current probability is " << localMaxProbability << std::endl);
    debug_log("Current random is " << static_cast<double>(std::rand()) / RAND_MAX << std::endl);
    return searchComplete || static_cast<double>(std::rand()) / RAND_MAX < localMaxProbability;
  }

  void initialise(NeighbourhoodContainer& nhc, DomainInt newBestMinValue,
                  const std::vector<DomainInt>& newBestSolution, std::shared_ptr<Propagate>& prop,
                  NeighbourhoodSearchStats &globalStats) {
    resetLocalMaxProbability();
    highestNeighbourhoodValues.assign(nhc.neighbourhoods.size(), 1);
    bestSolutionValue = newBestMinValue;
    bestSolution = newBestSolution;
    getState().getOptimiseVar()->setMin(newBestMinValue + 1);
    searchComplete = false;
    copyOverIncumbent(nhc, bestSolution, prop);
  }
};

typedef std::vector<std::pair<DomainInt, std::vector<DomainInt>>> SolutionBag;

class HolePuncher {
  static const int maxNumberOfSolutions = 5;
  SolutionBag solutionBag;
  std::vector<int> activeNeighbourhoods;

  int currentNeighbourhoodSolutionsCount = 0;
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
                   std::vector<int>& currentActivatedNeighbourhoods, NeighbourhoodStats& stats,
                   std::vector<DomainInt>& solution, NeighbourhoodSearchStats &) {
    finishedPhase = activeNeighbourhoods.empty();
    if(finishedPhase) {
      std::random_shuffle(solutionBag.begin(), solutionBag.end());
    }
  }

  /**
   * Pop a neighbourhood from the vector of active neighbourhoods and return it
   * in the struct SearchParams
   * @param nhc
   * @return Struct SearchParams which contains a neighbourhood to activate
   */
  SearchParams getSearchParams(NeighbourhoodContainer& nhc, NeighbourhoodSearchStats&) {
    assert(!activeNeighbourhoods.empty());
    currentNeighbourhoodSolutionsCount = 0;
    int neighbourhood = activeNeighbourhoods.back();
    activeNeighbourhoods.pop_back();
    return SearchParams({neighbourhood});
  }

  /*
   * Called during Minion Search once a solution has been found. If the number of solutions
   * found for the currently activated neighbourhood equals the max stop search.
   */
  bool continueSearch(NeighbourhoodContainer& nhc, const std::vector<DomainInt>& solution) {
    solutionBag.emplace_back(getState().getOptimiseVar()->getMin(), solution);
    return ++currentNeighbourhoodSolutionsCount != maxNumberOfSolutions;
  }

  /*
   * Finished phase should be set to true when the hole puncher has run out of
   * neighbourhoods to activate.
   */
  bool hasFinishedPhase() {
    return finishedPhase;
  }

  /*
   * Generate the vector of neighbourhoods that can be activated for the inputted size. Can
   * potentially
   * crash if no neighbourhood size from inputSize -> maxNeighbourhoodSize can be set.
   */
  void initialise(NeighbourhoodContainer& nhc, DomainInt,
                  const std::vector<DomainInt>& incumbentSolution,
                  std::shared_ptr<Propagate>& prop,
                  NeighbourhoodSearchStats &globalStats) {
    auto maxElement = std::max_element(nhc.neighbourhoods.begin(), nhc.neighbourhoods.end(),
                                       [](const Neighbourhood& n1, const Neighbourhood& n2) {
                                         return n1.deviation.getMax() < n2.deviation.getMax();
                                       });

    while(neighbourhoodSize <= maxElement->deviation.getMax()) {
      activeNeighbourhoods.clear();
      for(int i = 0; i < nhc.neighbourhoods.size(); i++) {
        if(nhc.neighbourhoods[i].deviation.inDomain(neighbourhoodSize))
          activeNeighbourhoods.push_back(i);
      }
      if(!activeNeighbourhoods.empty()) {
        break;
      } else {
        ++neighbourhoodSize;
      }
    }

    if(activeNeighbourhoods.empty())
      D_FATAL_ERROR("Cannot produce and active neighbourhoods in the hole puncher");

    debug_log("Initialise hole puncher: Neighbourhood size = " << neighbourhoodSize << std::endl);
    solutionBag = {};
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
                   std::vector<int>& currentActivatedNeighbourhoods, NeighbourhoodStats& stats,
                   std::vector<DomainInt>& solution, NeighbourhoodSearchStats &globalStats) {
    debug_log("METASTRATEGY- UPDATE STATS" << std::endl);
    switch(currentPhase) {
    case Phase::HILL_CLIMBING:
     debug_log("IN HILL CLIMBING PHASE " << std::endl);
      hillClimber.updateStats(nhc, prop, currentActivatedNeighbourhoods, stats, solution, globalStats);
      if(hillClimber.hasFinishedPhase()) {
        debug_log("HILL climbing phase has finished " << std::endl);
        if(hillClimber.bestSolutionValue > bestSolutionValue) {
          debug_log("A new best value was found " << std::endl);
          bestSolutionValue = hillClimber.bestSolutionValue;
          bestSolution = hillClimber.bestSolution;

          /*
           * If a better solution has been found we want to punch random holes around this solution
           */
          solutionBag.clear();
          currentPhase = Phase::HOLE_PUNCHING;
          holePuncher.resetNeighbourhoodSize();
          holePuncher.initialise(nhc, bestSolutionValue, bestSolution, prop, globalStats);
          std::cout << "EXPLORATION STARTED" << std::endl;
        } else if(solutionBag.empty()) {
          debug_log("Solution bag is empty move back to hole punching " << std::endl);
          // If there are no solutions left want to generate new random solutions for a larger
          // neighbourhood size
          currentPhase = Phase::HOLE_PUNCHING;
          holePuncher.incrementNeighbourhoodSize();
          holePuncher.initialise(nhc, bestSolutionValue, bestSolution, prop, globalStats);
        } else {
          globalStats.totalNumberOfRandomSolutionsPulled += 1;
          globalStats.numberPulledThisPhase += 1;
          std::cout << "Grabbing random solution" << std::endl;
          // Grab a random solution
          hillClimber.initialise(nhc, solutionBag.back().first, solutionBag.back().second, prop, globalStats);
          solutionBag.pop_back();
        }
      }
      break;
    // In this phase we want to generate random solutions
    case Phase::HOLE_PUNCHING: {
      // D_FATAL_ERROR("IN THE HOLE PUNCHING PHASE");
      holePuncher.updateStats(nhc, prop, currentActivatedNeighbourhoods, stats, solution, globalStats);
      if(holePuncher.hasFinishedPhase()) {
        solutionBag = std::move(holePuncher.getSolutionBag());
        if(!solutionBag.empty()) {
          currentPhase = Phase::HILL_CLIMBING;
          hillClimber.initialise(nhc, solutionBag.back().first, solutionBag.back().second, prop, globalStats);
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
                  const std::vector<DomainInt>& newBestSolution, std::shared_ptr<Propagate>& prop, NeighbourhoodSearchStats &globalStats) {
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
