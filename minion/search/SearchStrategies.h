//
// Created by Patrick Spracklen on 3/26/17.
//

#ifndef MINION_SEARCHSTRATEGIES_H
#define MINION_SEARCHSTRATEGIES_H

#include "neighbourhood-search.h"
#include <cmath>

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

  bool searchComplete;
  DomainInt minValue;

public:
  std::shared_ptr<SelectionStrategy> selectionStrategy;
  std::unordered_set<int> activatedNeighbourhoods;

  HillClimbingSearch(NeighbourhoodContainer& nhc,
                     std::shared_ptr<SelectionStrategy> selectionStrategy)
      : selectionStrategy(std::move(selectionStrategy)), searchComplete(false) {}

  void updateStats(NeighbourhoodContainer& nhc, std::shared_ptr<Propagate> prop,
                   std::vector<int>& currentActivatedNeighbourhoods, NeighbourhoodStats& stats,
                   std::vector<DomainInt>& solution) {
    std::cout << "Hill Climbing Search -- Update Stats " << std::endl;

    selectionStrategy->updateStats(currentActivatedNeighbourhoods, stats);

    if(stats.solutionFound) {
      activatedNeighbourhoods.clear();
      copyOverIncumbent(nhc, solution);
      if(stats.newMinValue == getState().getOptimiseVar()->getMax()) {
        searchComplete = true;
        return;
      }
      getState().getOptimiseVar()->setMin(stats.newMinValue + 1);
      std::vector<AnyVarRef> emptyVars;
      prop->prop(emptyVars);
    } else {
      activatedNeighbourhoods.insert(currentActivatedNeighbourhoods.begin(),
                                     currentActivatedNeighbourhoods.end());
    }
  }

  vector<int> getNeighbourHoodsToActivate(NeighbourhoodContainer& nhc,
                                          double& neighbourhoodTimeout) {
    return selectionStrategy->getNeighbourHoodsToActivate(nhc, neighbourhoodTimeout);
  }

  bool continueSearch(NeighbourhoodContainer& nhc) {
    if(activatedNeighbourhoods.size() == nhc.neighbourhoods.size()) {
      std::cout << "Search Exhausted!" << std::endl;
      return false;
    }
    return !searchComplete;
  }
};

template <typename SelectionStrategy>
class SimulatedAnnealing {

  double temperature = 100;
  double coolingParameter = 0.003;
  DomainInt maxValue;
  vector<DomainInt> solution;
  std::shared_ptr<SelectionStrategy> selectionStrategy;
  double time = 500;
  double averageTime;

public:
  SimulatedAnnealing(NeighbourhoodContainer& nhc,
                     std::shared_ptr<SelectionStrategy> selectionStrategy)
      : maxValue(getState().getOptimiseVar()->getMin()),
        selectionStrategy(std::move(selectionStrategy)) {}

  double acceptanceProbability(int previousMax, int currentMax) {
    double pr = pow(M_E, (double)((currentMax - previousMax) / temperature));
    std::cout << "Prob is " << pr << std::endl;
    return pr;
  }
  void updateStats(NeighbourhoodContainer& nhc, std::shared_ptr<Propagate> prop,
                   std::vector<int>& currentActivatedNeighbourhoods, NeighbourhoodStats& stats,
                   std::vector<DomainInt>& solution) {

    std::cout << " Min value found is " << stats.newMinValue << std::endl;
    std::cout << " max value found is " << maxValue << std::endl;
    if(stats.newMinValue > maxValue) {
      maxValue = stats.newMinValue;
      copyOverIncumbent(nhc, solution);
    } else if(stats.solutionFound &&
              acceptanceProbability(checked_cast<int>(maxValue),
                                    checked_cast<int>(stats.newMinValue)) >
                  (std::rand() / RAND_MAX)) {
      std::cout << "Moving to a worse solution " << std::endl;

      // Save old solution
      this->solution = solution;
      copyOverIncumbent(nhc, solution);
    }

    // Propogate vars
    std::vector<AnyVarRef> emptyVars;
    prop->prop(emptyVars);
    // Update temperature
    temperature *= 1 - coolingParameter;
    selectionStrategy->updateStats(currentActivatedNeighbourhoods, stats);
  }

  vector<int> getNeighbourHoodsToActivate(NeighbourhoodContainer& nhc, int& neighbourhoodTimeout) {
    return selectionStrategy->getNeighbourHoodsToActivate(nhc, neighbourhoodTimeout);
  }

  bool continueSearch(NeighbourhoodContainer& nhc) {
    return temperature > 1 && (maxValue != getState().getOptimiseVar()->getMax());
  }

  void printHistory(NeighbourhoodContainer& nhc) {
    selectionStrategy->printHistory(nhc);
  }
};

#endif // MINION_SEARCHSTRATEGIES_H
