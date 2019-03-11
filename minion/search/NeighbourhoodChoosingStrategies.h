#ifndef MINION_NEIGHBOURHOODCHOOSINGSTRATEGIES_H
#define MINION_NEIGHBOURHOODCHOOSINGSTRATEGIES_H
#include "learningAutomatonNeighbourhoodSelectionStrategy.h"
#include "neighbourhood-def.h"
#include "neighbourhood-search.h"
#include "neighbourhoodSearchStats.h"
#include "ucbNeighbourhoodSelectionStrategy.h"
#include <cassert>
#include <math.h>
#include <vector>

class RandomCombinationChooser {
  std::vector<int> viableCombinations;

public:
  RandomCombinationChooser(const NeighbourhoodContainer&) {}

  void updateStats(int activatedCombination, const NeighbourhoodStats&) {}

  int getCombinationsToActivate(const NeighbourhoodState& nhState) {
    viableCombinations.clear();
    for(size_t i = 0; i < nhState.nhc.neighbourhoodCombinations.size(); i++) {
      if(nhState.nhc.isCombinationEnabled(i)) {
        viableCombinations.push_back(i);
      }
    }
    std::uniform_int_distribution<int> dist(0, viableCombinations.size() - 1);
    return viableCombinations[dist(global_random_gen)];
  }
};

class InteractiveCombinationChooser {

public:
  InteractiveCombinationChooser(const NeighbourhoodContainer&) {}

  void updateStats(int activatedCombination, const NeighbourhoodStats& stats) {
    std::cout << stats << std::endl;
  }

  int getCombinationsToActivate(const NeighbourhoodState& nhState) {
    while(true) {
      std::cout << "Select Combination:\n";
      for(size_t i = 0; i < nhState.nhc.neighbourhoodCombinations.size(); ++i) {
        cout << i << ":\n";
        nhState.globalStats.printCombinationDescription(cout, nhState.nhc, i);
        cout << "\n";
      }
      int index;
      bool readInt = bool(cin >> index);
      if(!readInt || index < -1 || index >= (int)nhState.nhc.neighbourhoodCombinations.size()) {
        cout << "Error, please enter an integer in the range 0.."
             << nhState.nhc.neighbourhoodCombinations.size()
             << " to select a combination or -1 to end search.\n";
      } else if(index == -1) {
        std::cerr << "InteractiveCombinationChooser: ending search...\n";
        throw EndOfSearch();
      } else if(!nhState.nhc.isCombinationEnabled(index)) {
        cout << "Error: This combination cannot be activated, one or more of the neighbourhood "
                "activation variables must be assigned to false.";
      } else {
        return index;
      }
    }
  }
};

#endif // MINION_NEIGHBOURHOODCHOOSINGSTRATEGIES_H
