
#ifndef MINION_SEARCHSTRATEGIES_H
#define MINION_SEARCHSTRATEGIES_H

#include "neighbourhood-search.h"
#include <algorithm>
#include <cmath>

struct SearchParams {
  std::vector<int> neighbourhoodsToActivate;
  bool optimiseMode;
  int timeOut;
  SearchParams(std::vector<int> neighbourhoods, bool optimiseMode = true, int timeOut = 500)
      : neighbourhoodsToActivate(std::move(neighbourhoods)),
        optimiseMode(optimiseMode),
        timeOut(timeOut) {}
};

inline void copyOverIncumbent(NeighbourhoodContainer& nhc, const vector<DomainInt>& solution) {
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
  friend MetaSearch<SelectionStrategy>;

  static const double INITIAL_LOCAL_MAX_PROBABILITY = 0.01;
  static double probabilityIncrementConstant = 0.1;
  DomainInt bestSolutionValue;
  std::vector<DomainInt> bestSolution;
  bool searchComplete = false;
  // The probability that we will enter a random mode of exploration.
  double localMaxProbability = INITIAL_LOCAL_MAX_PROBABILITY;
  SelectionStrategy selectionStrategy;

public:
  /*
   * If a solution is found reset the probability of random exploration and
   * copy over the incumbent otherwise increase the probability that we will enter random
   * exploration mode.
   */
  void updateStats(NeighbourhoodContainer& nhc, std::shared_ptr<Propagate> prop,
                   std::vector<int>& currentActivatedNeighbourhoods, NeighbourhoodStats& stats,
                   std::vector<DomainInt>& solution) {
    std::cout << "Hill Climbing Search -- Update Stats " << std::endl;

    selectionStrategy->updateStats(currentActivatedNeighbourhoods, stats);

    if(stats.solutionFound) {
      bestSolutionValue = stats.newMinValue;
      bestSolution = solution;
      resetLocalMaxProbability();
      copyOverIncumbent(nhc, solution);
      if(stats.newMinValue == getState().getOptimiseVar()->getMax()) {
        searchComplete = true;
        return;
      }
      getState().getOptimiseVar()->setMin(stats.newMinValue + 1);
    } else {
      localMaxProbability += (1 / nhc.neighbourhoods.size()) * probabilityIncrementConstant;
    }
  }

  void resetLocalMaxProbability() {
    localMaxProbability = INITIAL_LOCAL_MAX_PROBABILITY;
  }

  SearchParams getSearchParams(NeighbourhoodContainer& nhc) {
    return SearchParams(selectionStrategy->getNeighbourhoodsToActivate(nhc));
  }

  bool continueSearch(NeighbourhoodContainer& nhc, std::vector<DomainInt>& solution) {
    return true;
  }

  bool hasFinishedPhase() {
    return searchComplete || static_cast<double>(std::rand()) / RAND_MAX < localMaxProbability;
  }

  void initialise(NeighbourhoodContainer&, DomainInt newBestMinValue,
                  const std::vector<DomainInt>& newBestSolution) {
    resetLocalMaxProbability();
    bestSolutionValue = newBestMinValue;
    bestSolution = newBestSolution;
    getState().getOptimiseVar().setMin(newBestMinValue + 1);
    searchComplete = false;
  }
};

typedef std::vector<std::pair<DomainInt, std::vector<DomainInt>>> SolutionBag;

class HolePuncher {
  static const int maxNumberOfSolutions = 5;
  SolutionBag solutionBag;
  std::vector<int> activeNeighbourhoods;

  int currentNeighbourhoodSolutionsCount = 0;
  int neighbourhoodSize = 1;
  bool finishedPhase = false;

public:
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
  void updateStats(NeighbourhoodContainer& nhc, std::shared_ptr<Propagate> prop,
                   std::vector<int>& currentActivatedNeighbourhoods, NeighbourhoodStats& stats,
                   std::vector<DomainInt>& solution) {
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
  SearchParams getSearchParams(NeighbourhoodContainer& nhc) {
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
                  const std::vector<DomainInt>& incumbentSolution) {
    copyOverIncumbent(nhc, incumbentSolution);
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

    solutionBag = {};
    finishedPhase = false;
  }

  std::vector<std::vector<DomainInt>>& getSolutionBag() {
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
  SearchParams getSearchParams(NeighbourhoodContainer& nhc) {
    switch(currentPhase) {
    case Phase::HILL_CLIMBING: return hillClimber.getSearchParams();
    case Phase::HOLE_PUNCHING: return holePuncher.getSearchParams();
    }
  }

  bool continueSearch(NeighbourhoodContainer& nhc) {
    switch(currentPhase) {
    case Phase::HILL_CLIMBING: return hillClimber.continueSearch(nhc, solution);
    case Phase::HOLE_PUNCHING: return holePuncher.continueSearch(nhc, solution);
    }
  }

  void updateStats(NeighbourhoodContainer& nhc, std::shared_ptr<Propagate> prop,
                   std::vector<int>& currentActivatedNeighbourhoods, NeighbourhoodStats& stats,
                   std::vector<DomainInt>& solution) {
    std::cout << "METASTRATEGY- UPDATE STATS" << std::endl;
    switch(currentPhase) {
    case Phase::HILL_CLIMBING: {
      std::cout << "IN HILL CLIMBING PHASE " << std::endl;
      hillClimber.updateStats(nhc, prop, currentActivatedNeighbourhoods, stats, solution);
      if(hillClimber.hasFinishedPhase()) {
        if(hillClimber.bestSolutionValue > bestSolutionValue) {
          bestSolutionValue = hillClimber.bestSolutionValue;
          bestSolution = hilllClimber.bestSolution;
          /*
           * If a better solution has been found we want to punch random holes around this solution
           */
          solutionBag.clear();
          currentPhase = Phase::HOLE_PUNCHER;
          holePuncher.resetNeighbourhoodSize();
          holePuncher.initialise(nhc, bestSolutionValue, bestSolution);
        } else if(solutionBag.empty()) {
          // If there are no solutions left want to generate new random solutions for a larger
          // neighbourhood size
          currentPhase = Phase::HOLE_PUNCHER;
          holePuncher.incrementNeighbourhoodSize();
          holePuncher.initialise(nhc, bestSolutionValue, bestSolution);
        } else {
          // Grab a random solution
          hillClimber.initialise(nhc, solutionBag.back().first, solutionBag.back().second);
          solutionBag.pop_back();
        }
      }
    }

    // In this phase we want to generate random solutions
    case Phase::HOLE_PUNCHER: {
      holePuncher.updateStats(nhc, prop, currentActivatedNeighbourhoods, stats, solution);
      if(holePuncher.hasFinishedPhase()) {
        solutionBag = std::move(holePuncher.getSolutionBag());
        if(!solutionBag.empty()) {
          currentPhase = Phase::HILL_CLIMBING;
          hillClimber.initialise(nhc, solutionBag.back().first, solutionBag.back().second);
          solutionBag.pop_back();
        } else {
          holePuncher.incrementNeighbourhoodSize();
          holePuncher.initialise(nhc, bestSolution);
        }
      }
      break;
    }
    }
  }
  void initialise(NeighbourhoodContainer&, DomainInt newBestMinValue,
                  const std::vector<DomainInt>& newBestSolution) {
    bestSolutionValue = newBestMinValue;
    bestSolution = newBestSolution;
    currentPhase = Phase::HILL_CLIMBING;
    hillClimber.initialise(newBestMinValue, newBestSolution);
  }
};

#endif // MINION_SEARCHSTRATEGIES_H
