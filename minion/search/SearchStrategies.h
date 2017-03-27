//
// Created by Patrick Spracklen on 3/26/17.
//

#ifndef MINION_SEARCHSTRATEGIES_H
#define MINION_SEARCHSTRATEGIES_H

#include "neighbourhood-search.h"

void copyOverIncumbent(NeighbourhoodContainer &nhc, const vector<DomainInt>& solution) {
  if (Controller::get_world_depth() != 1) {
    Controller::world_pop();
  }
  Controller::world_push();
  for(int i = 0; i < nhc.shadow_mapping[0].size(); i++) {
    nhc.shadow_mapping[1][i].assign(solution[i]);
  }
}


template<typename SelectionStrategy>
class HillClimbingSearch{

  bool searchComplete;
  DomainInt minValue;


public:

  std::shared_ptr<SelectionStrategy> selectionStrategy;
  std::unordered_set<int> activatedNeighbourhoods;

  HillClimbingSearch(NeighbourhoodContainer &nhc, std::shared_ptr<SelectionStrategy> selectionStrategy)
    :selectionStrategy(std::move(selectionStrategy)), searchComplete(false)
  {}

  void updateStats(NeighbourhoodContainer &nhc, std::shared_ptr<Propagate> prop, std::vector<int> &currentActivatedNeighbourhoods,
                   NeighbourhoodStats &stats, std::vector<DomainInt> &solution){
    std::cout << "Hill Climbing Search -- Update Stats " << std::endl;

    selectionStrategy->updateStats(currentActivatedNeighbourhoods, stats);

    if (stats.solutionFound){
      activatedNeighbourhoods.clear();
      copyOverIncumbent(nhc, solution);
      if (stats.newMinValue == getState().getOptimiseVar()->getMax()){
        searchComplete = true;
        return;
      }
      getState().getOptimiseVar()->setMin(stats.newMinValue + 1);
      std::vector<AnyVarRef> emptyVars;
      prop->prop(emptyVars);
    }else {
      activatedNeighbourhoods.insert(currentActivatedNeighbourhoods.begin(), currentActivatedNeighbourhoods.end());
    }

  }

  vector<int> getNeighbourHoodsToActivate(NeighbourhoodContainer &nhc, double &neighbourhoodTimeout){
    return selectionStrategy->getNeighbourHoodsToActivate(nhc, neighbourhoodTimeout);
  }


  bool continueSearch(NeighbourhoodContainer &nhc){
    if (activatedNeighbourhoods.size() == nhc.neighbourhoods.size()){
      std::cout << "Search Exhausted!" << std::endl;
      return false;
    }
    return !searchComplete;
  }

};




#endif //MINION_SEARCHSTRATEGIES_H
