#ifndef MINION_NEIGHBOURHOODCHOOSINGSTRATEGIES_H
#define MINION_NEIGHBOURHOODCHOOSINGSTRATEGIES_H
#include "neighbourhood-def.h"
#include <vector>

struct NeighbourhoodStats {
  int newMinValue;
  u_int64_t timeTaken;
  bool solutionFound;

public:
  NeighbourhoodStats(int newMinValue, u_int64_t timeTaken, bool solutionFound)
      : newMinValue(newMinValue), timeTaken(timeTaken), solutionFound(solutionFound) {}
};

class RandomNeighbourhoodChooser {

  std::vector<bool> neighbourhoodSuccessHistory;
  std::vector<int> successfulNeighbourhoods;

public:
  RandomNeighbourhoodChooser(const NeighbourhoodContainer& neighbourhoodContainer)
      : neighbourhoodSuccessHistory(neighbourhoodContainer.neighbourhoods.size(), true),
        successfulNeighbourhoods(neighbourhoodContainer.neighbourhoods.size()) {}

  void updateNeighbourhoodStats(vector<int>& activatedNeighbourhoods,
                                NeighbourhoodStats& neighbourhoodStats) {
    if(neighbourhoodStats.solutionFound) {
      neighbourhoodSuccessHistory.assign(neighbourhoodSuccessHistory.size(), true);
    } else {
      for(int i = 0; i < neighbourhoodSuccessHistory.size(); i++) {
        neighbourhoodSuccessHistory[activatedNeighbourhoods[i]] = false;
      }
    }
  }

  vector<int> getNeighbourHoodsToActivate(const NeighbourhoodContainer& neighbourhoodContainer) {

    successfulNeighbourhoods.clear();
    for(int i = 0; i < neighbourhoodSuccessHistory.size(); i++) {
      if(neighbourhoodSuccessHistory[i])
        successfulNeighbourhoods.push_back(i);
    }

    int random_variable = static_cast<int>(std::rand() % successfulNeighbourhoods.size());
    return  {successfulNeighbourhoods[random_variable]};
  }
};

#endif // MINION_NEIGHBOURHOODCHOOSINGSTRATEGIES_H
