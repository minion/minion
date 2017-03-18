#ifndef MINION_NEIGHBOURHOODCHOOSINGSTRATEGIES_H
#define MINION_NEIGHBOURHOODCHOOSINGSTRATEGIES_H
#include "neighbourhood-def.h"
#include <vector>

struct NeighbourhoodStats {
  DomainInt newMinValue;
  u_int64_t timeTaken;
  bool solutionFound;
  bool timeoutReached;

public:
  NeighbourhoodStats(DomainInt newMinValue, u_int64_t timeTaken, bool solutionFound,
                     bool timeoutReached)
      : newMinValue(newMinValue),
        timeTaken(timeTaken),
        solutionFound(solutionFound),
        timeoutReached(timeoutReached) {}
};

class RandomNeighbourhoodChooser {

  std::vector<bool> neighbourhoodSuccessHistory;
  std::vector<int> successfulNeighbourhoods;

public:
  RandomNeighbourhoodChooser(const NeighbourhoodContainer& neighbourhoodContainer)
      : neighbourhoodSuccessHistory(neighbourhoodContainer.neighbourhoods.size(), true),
        successfulNeighbourhoods(neighbourhoodContainer.neighbourhoods.size()) {}

  void updateStats(const vector<int>& activatedNeighbourhoods,
                   const NeighbourhoodStats& neighbourhoodStats) {
    if(neighbourhoodStats.solutionFound) {
      cout << "Solution found, assiging all neighbourhoods to true" << endl;
      neighbourhoodSuccessHistory.assign(neighbourhoodSuccessHistory.size(), true);
    } else {
      for(int neighbourhoodIndex : activatedNeighbourhoods) {
        cout << "Setting neighbourdhood " << neighbourhoodIndex << " to false" << endl;
        neighbourhoodSuccessHistory[neighbourhoodIndex] = false;
      }
    }
  }

  vector<int> getNeighbourHoodsToActivate(const NeighbourhoodContainer& neighbourhoodContainer) {

    successfulNeighbourhoods.clear();
    for(int i = 0; i < neighbourhoodSuccessHistory.size(); i++) {
      if(neighbourhoodSuccessHistory[i])
        successfulNeighbourhoods.push_back(i);
    }
    if(successfulNeighbourhoods.empty()) {
      return {};
    } else {
      int random_variable = std::rand() % successfulNeighbourhoods.size();
      cout << "Attempting Neighbourhood: " << random_variable << endl;
      return {successfulNeighbourhoods[random_variable]};
    }
  }
};

#endif // MINION_NEIGHBOURHOODCHOOSINGSTRATEGIES_H
