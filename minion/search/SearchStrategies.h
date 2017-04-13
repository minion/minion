//
// Created by Patrick Spracklen on 3/26/17.
//

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

void copyOverIncumbent(NeighbourhoodContainer& nhc, const vector<DomainInt>& solution) {
  if(Controller::get_world_depth() != 1) {
    Controller::world_pop();
  }
  Controller::world_push();
  for(int i = 0; i < nhc.shadow_mapping[0].size(); i++) {
    nhc.shadow_mapping[1][i].assign(solution[i]);
  }
}



template <typename SelectionStrategy>
class HillClimbingSearch {

  static const double INITIAL_LOCAL_MAX_PROBABILITY;
  bool searchComplete;
  DomainInt minValue;
  // The probability that we will enter a random mode of exploration.
  double localMaxProbability = INITIAL_LOCAL_MAX_PROBABILITY;
  double probabilityIncrementConstant = 0.1;

public:
  std::shared_ptr<SelectionStrategy> selectionStrategy;

  HillClimbingSearch(std::shared_ptr<SelectionStrategy> selectionStrategy)
      : selectionStrategy(std::move(selectionStrategy)), searchComplete(false) {}

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
      resetLocalMaxProbability();
      copyOverIncumbent(nhc, solution);
      if(stats.newMinValue == getState().getOptimiseVar()->getMax()) {
        searchComplete = true;
        return;
      }
      getState().getOptimiseVar()->setMin(stats.newMinValue + 1);
      std::vector<AnyVarRef> emptyVars;
      prop->prop(emptyVars);
    }else {
      localMaxProbability += (1/nhc.neighbourhoods.size()) * probabilityIncrementConstant;
    }
  }

  void resetLocalMaxProbability(){
    localMaxProbability = INITIAL_LOCAL_MAX_PROBABILITY;
  }

  SearchParams getSearchParams(NeighbourhoodContainer& nhc) {
    return SearchParams(selectionStrategy->getNeighbourhoodsToActivate(nhc));
  }

  bool continueSearch(NeighbourhoodContainer& nhc, std::vector<DomainInt> &solution) {
    return true;
  }

  bool hasFinishedPhase() {
    return static_cast<double>(std::rand()) / RAND_MAX < localMaxProbability;
  }

  void startPhase(){
    resetLocalMaxProbability();
  }
};





class HolePuncher{

  std::vector<std::vector<DomainInt>> solutionBag;
  int lastNeighbourhoodIndex = 0;
  int lastSize = 0;
  int currentNeighbourhoodSolutionsCount = 0;
  int neighbourhoodSize = 1;
  bool finishedPhase = false;
  std::vector<int> activeNeighbourhoods;
public:
  int maxNumberOfSolutions = 5;

  /*
   * Check if we have run out of neighbourhoods to activate. If so shuffle the solution bag
   * so that the generated solutions are in a random configuration.
   */
  void updateStats(NeighbourhoodContainer& nhc, std::shared_ptr<Propagate> prop,
                   std::vector<int>& currentActivatedNeighbourhoods, NeighbourhoodStats& stats,
                   std::vector<DomainInt>& solution) {
    finishedPhase = activeNeighbourhoods.empty();
    if(finishedPhase){
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
  bool continueSearch(NeighbourhoodContainer& nhc, std::vector<DomainInt> &solution){
    solutionBag.push_back(solution);
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
  void startPhase(NeighbourhoodContainer& nhc, int& neighbourhoodSize) {
    activeNeighbourhoods.clear();
    auto maxElement = std::max_element(nhc.neighbourhoods.begin(), nhc.neighbourhoods.end(),
                                       [](const Neighbourhood& n1, const Neighbourhood& n2) {
                                         return n1.deviation.getMax() < n2.deviation.getMax();
                                       });

    while(++neighbourhoodSize <= maxElement->deviation.getMax()) {
      for(int i = 0; i < nhc.neighbourhoods.size(); i++) {
        if(nhc.neighbourhoods[i].deviation.inDomain(neighbourhoodSize))
          activeNeighbourhoods.push_back(i);
      }
      if(!activeNeighbourhoods.empty())
        break;
    }

    if (activeNeighbourhoods.empty())
      D_FATAL_ERROR("Cannot produce and active neighbourhoods in the hole puncher");

    solutionBag.clear();
    finishedPhase = false;
  }

  std::vector<std::vector<DomainInt>>& getSolutionBag() {
    return solutionBag;
  }
};

template <typename SelectionStrategy>
class MetaStrategy {

  HillClimbingSearch<SelectionStrategy> hillClimber;
  HolePuncher holePuncher;
  int neighbourhoodSize = 0;
  DomainInt bestSolutionValue;
  std::vector<DomainInt> bestSolution;
  bool betterSolutionFound;
  enum class Phase { HILL_CLIMBING, HOLE_PUNCHING };
  Phase currentPhase = Phase::HILL_CLIMBING;

  std::vector<std::vector<DomainInt>> solutionBag;
  bool searchEnded;

public:
  MetaStrategy()
      : hillClimber(SelectionStrategy()),
        bestSolutionValue(getState().getOptimiseVar()->getMin()) {}

  SearchParams getSearchParams(NeighbourhoodContainer& nhc) {
    switch(currentPhase) {
    case Phase::HILL_CLIMBING: return hillClimber.getSearchParams();
    case Phase::HOLE_PUNCHING: return holePuncher.getSearchParams();
    }
  }

  bool continueSearch(NeighbourhoodContainer& nhc) {
    return true;
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
        if(stats.newMinValue > bestSolutionValue) {
          bestSolutionValue = stats.newMinValue;
          bestSolution = solution;
          betterSolutionFound = true;
        }

        /*
         * If a better solution has been found we want to punch random holes around this solution
         * space
         */
        if(betterSolutionFound) {
          solutionBag.clear();
          currentPhase = Phase::HOLE_PUNCHER;
          neighbourhoodSize = 0;
          holePuncher.startPhase(nhc, neighbourhoodSize);
        } // If there are no solutions left want to generate new random solutions for a larger
          // neighbourhood size
        else if(solutionBag.empty()) {
          currentPhase = Phase::HOLE_PUNCHER;
          neighbourhoodSize++;
          holePuncher.startPhase(nhc, neighbourhoodSize);
        } // Grab a random solution
        else {
          std::vector<DomainInt> randomSolution = solutionBag.back();
          solutionBag.pop_back();
          copyOverIncumbent(nhc, randomSolution);
        }
      }
    }
    // In this phase we want to generate random solutions
    case Phase::HOLE_PUNCHER: {
      holePuncher.updateStats(nhc, prop, currentActivatedNeighbourhoods, stats, solution);
      if(holePuncher.hasFinishedPhase()) {
        currentPhase = Phase::HILL_CLIMBING;
        solutionBag = std::move(holePuncher.getSolutionBag());
      }
      break;
    }
    }
  }
};

#endif // MINION_SEARCHSTRATEGIES_H
