#ifndef MINION_NEIGHBOURHOODCHOOSINGSTRATEGIES_H
#define MINION_NEIGHBOURHOODCHOOSINGSTRATEGIES_H
#include "neighbourhood-def.h"
#include "neighbourhoodSearchStats.h"
#include <cassert>
#include <math.h>
#include <vector>

class RandomCombinationChooser {
  std::vector<int> viableCombinations;

public:
  RandomCombinationChooser() {}

  void updateStats(int activatedCombination, const NeighbourhoodStats&) {}

  int getCombinationsToActivate(const NeighbourhoodContainer& nhc, NeighbourhoodSearchStats&) {
    viableCombinations.clear();
    for(size_t i = 0; i < nhc.neighbourhoodCombinations.size(); i++) {
      if(nhc.isCombinationEnabled(i)) {
        viableCombinations.push_back(i);
      }
    }
    return viableCombinations[std::rand() % viableCombinations.size()];
  }
};

class UCBNeighborHoodSelection {
private:
  static const int TIMEOUT_PENALTY_COST = 1000;

  inline double ucbValue(double reward, int totalActivations, int totalCombinationVisits) {
    return (reward / totalCombinationVisits) +
           std::sqrt((2 * std::log(totalActivations)) / (totalCombinationVisits));
  }

public:
  UCBNeighborHoodSelection() {}

  void updateStats(int activatedCombination, const NeighbourhoodStats& combinationStats) {}

  int getCombinationsToActivate(const NeighbourhoodContainer& nhc,
                                NeighbourhoodSearchStats& globalStats) {
    double bestUCTValue = -(std::numeric_limits<double>::max());
    int index = -1;
    for(int i = 0; i < (int)globalStats.numberActivations.size(); i++) {
      if(nhc.isCombinationEnabled(i)) {
        if(globalStats.numberActivations[i] == 0) {
          return i;
        }
        double currentUCBValue =
            ucbValue(globalStats.numberPositiveSolutions[i] -
                         globalStats.numberNegativeSolutions[i] - globalStats.numberNoSolutions[i],
                     globalStats.numberIterations, globalStats.numberActivations[i]);
        if(currentUCBValue > bestUCTValue) {
          bestUCTValue = currentUCBValue;
          index = i;
        }
      }
    }
    if(index == -1) {
      std::cout << "UCBNeighborHoodSelection: could not activate a combination.\n";
      throw EndOfSearch();
    }
    return index;
  }
};

class InteractiveCombinationChooser {

public:
  InteractiveCombinationChooser() {}

  void updateStats(int activatedCombination, const NeighbourhoodStats& stats) {
    std::cout << stats << std::endl;
  }

  int getCombinationsToActivate(const NeighbourhoodContainer& nhc,
                                NeighbourhoodSearchStats& globalStats) {
    while(true) {
      std::cout << "Select Combination:\n";
      for(size_t i = 0; i < nhc.neighbourhoodCombinations.size(); ++i) {
        cout << i << ":\n";
        globalStats.printCombinationDescription(cout, nhc, i);
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
