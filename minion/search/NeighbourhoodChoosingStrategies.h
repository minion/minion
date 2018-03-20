#ifndef MINION_NEIGHBOURHOODCHOOSINGSTRATEGIES_H
#define MINION_NEIGHBOURHOODCHOOSINGSTRATEGIES_H
#include "learningAutomatonNeighbourhoodSelectionStrategy.h"
#include "neighbourhood-def.h"
#include "neighbourhoodSearchStats.h"
#include "ucbNeighbourhoodSelectionStrategy.h"
#include <cassert>
#include <math.h>
#include <vector>

class RandomCombinationChooser {
  std::vector<int> viableCombinations;

public:
  RandomCombinationChooser(const NeighbourhoodContainer&) {}
  RandomCombinationChooser() {}

  void updateStats(int activatedCombination, const NeighbourhoodStats&) {}

  int getCombinationsToActivate(const NeighbourhoodContainer& nhc, NeighbourhoodSearchStats&,
                                DomainInt) {
    viableCombinations.clear();
    for(size_t i = 0; i < nhc.neighbourhoodCombinations.size(); i++) {
      if(nhc.isCombinationEnabled(i)) {
        viableCombinations.push_back(i);
      }
    }
    return viableCombinations[std::rand() % viableCombinations.size()];
  }
};

class InteractiveCombinationChooser {

public:
  InteractiveCombinationChooser(const NeighbourhoodContainer&) {}

  void updateStats(int activatedCombination, const NeighbourhoodStats& stats) {
    std::cout << stats << std::endl;
  }

  int getCombinationsToActivate(const NeighbourhoodContainer& nhc,
                                NeighbourhoodSearchStats& globalStats, DomainInt) {
    while(true) {
      std::cout << "Select Combination:\n";
      for(size_t i = 0; i < nhc.neighbourhoodCombinations.size(); ++i) {
        cout << i << ":\n";
        globalStats.printCombinationDescription(cout, nhc, i);
        cout << "\n";
      }
      int index;
      bool readInt = bool(cin >> index);
      if(!readInt || index < -1 || index >= (int)nhc.neighbourhoodCombinations.size()) {
        cout << "Error, please enter an integer in the range 0.."
             << nhc.neighbourhoodCombinations.size()
             << " to select a combination or -1 to end search.\n";
      } else if(index == -1) {
        std::cerr << "InteractiveCombinationChooser: ending search...\n";
        throw EndOfSearch();
      } else if(!nhc.isCombinationEnabled(index)) {
        cout << "Error: This combination cannot be activated, one or more of the neighbourhood "
                "activation variables must be assigned to false.";
      } else {
        return index;
      }
    }
  }
};

#endif // MINION_NEIGHBOURHOODCHOOSINGSTRATEGIES_H
