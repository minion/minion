#ifndef MINION_NEIGHBOURHOODCHOOSINGSTRATEGIES_H
#define MINION_NEIGHBOURHOODCHOOSINGSTRATEGIES_H
#include "neighbourhood-def.h"
#include <vector>

struct NeighbourhoodStats {
  int minValue;
  u_int64_t timeTaken;
};

class RandomNeighbourhoodChooser {
public:
  void updateNeighbourhoodStats(vector<int>& activatedNeighbourhoods,
                                NeighbourhoodStats& neighbourhoodStats) {}

  vector<int> getNeighbourHoodsToActivate(const NeighbourhoodContainer& neighbourhoodContainer) {
    int random_variable =
        static_cast<int>(std::rand() % neighbourhoodContainer.neighbourhoods.size());
    return vector<int>{random_variable};
  }
};

#endif // MINION_NEIGHBOURHOODCHOOSINGSTRATEGIES_H
