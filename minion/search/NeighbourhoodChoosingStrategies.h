#include "neighbourhood-def.h"
#include <vector>
//
// Created by Patrick Spracklen on 3/1/17.
//

#ifndef MINION_NEIGHBOURHOODCHOOSINGSTRATEGIES_H
#define MINION_NEIGHBOURHOODCHOOSINGSTRATEGIES_H

struct NeighbourhoodStats{
  int minValue;
  u_int64_t timeTaken;
};









class RandomNeighbourhoodChooser{

  void updateNeighbourhoodStats(vector<int> &activatedNeighbourhoods, NeighbourhoodStats &neighbourhoodStats){

  }

  vector<int> getNeighbourHoodsToActivate(const NeighbourhoodContainer &neighbourhoodContainer){
    int random_variable = static_cast<int>(std::rand() % neighbourhoodContainer.neighbourhoods.size());
    return vector<int>{random_variable};
  }







};




#endif //MINION_NEIGHBOURHOODCHOOSINGSTRATEGIES_H
