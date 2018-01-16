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
  RandomCombinationChooser(const NeighbourhoodContainer&) {}
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
  std::vector<u_int64_t> initialNumberActivations;
  std::vector<u_int64_t> initialNumberPositiveSolutions;
  u_int64_t initialNumberIterations = 0;

  inline u_int64_t numberPositiveSolutions(const NeighbourhoodSearchStats& globalStats, int index) {
    return (!initialNumberPositiveSolutions.empty())
               ? initialNumberPositiveSolutions[index] + globalStats.numberPositiveSolutions[index]
               : globalStats.numberPositiveSolutions[index];
  }

  inline u_int64_t numberActivations(const NeighbourhoodSearchStats& globalStats, int index) {
    return (!initialNumberActivations.empty())
               ? initialNumberActivations[index] + globalStats.numberActivations[index]
               : globalStats.numberActivations[index];
  }

  inline u_int64_t numberIterations(const NeighbourhoodSearchStats& globalStats) {
    return initialNumberIterations + globalStats.numberIterations;
  }

private:
  static const int TIMEOUT_PENALTY_COST = 1000;

  inline double ucbValue(double reward, u_int64_t totalActivations,
                         u_int64_t totalCombinationVisits) {
    return (reward / totalCombinationVisits) +
           std::sqrt((2 * std::log(totalActivations)) / (totalCombinationVisits));
  }

public:
  UCBNeighborHoodSelection(const NeighbourhoodContainer&) {}

  void updateStats(int activatedCombination, const NeighbourhoodStats& combinationStats) {}

  int getCombinationsToActivate(const NeighbourhoodContainer& nhc,
                                NeighbourhoodSearchStats& globalStats) {
    double bestUCTValue = -(std::numeric_limits<double>::max());
    bool allCombinationsTryed = true;
    std::vector<int> bestCombinations;
    // changed so that if multiple neighbourhoods have the same best UCb value, a random one is
    // selected

    for(int i = 0; i < (int)globalStats.numberActivations.size(); i++) {
      if(nhc.isCombinationEnabled(i)) {
        if(globalStats.numberActivations[i] == 0) {
          if(allCombinationsTryed) {
            allCombinationsTryed = false;
            bestCombinations.clear();
          }
          bestCombinations.push_back(i);
        }
        if(allCombinationsTryed) {
          double currentUCBValue =
              ucbValue(numberPositiveSolutions(globalStats, i), numberIterations(globalStats),
                       numberActivations(globalStats, i));
          if(currentUCBValue > bestUCTValue) {
            bestUCTValue = currentUCBValue;
            bestCombinations.clear();
            bestCombinations.push_back(i);
          } else if(currentUCBValue == bestUCTValue) {
            bestCombinations.push_back(i);
          }
        }
      }
    }
    if(bestCombinations.empty()) {
      std::cout << "UCBNeighborHoodSelection: could not activate a combination.\n";
      throw EndOfSearch();
    }
    return bestCombinations[std::rand() % bestCombinations.size()];
  }
};

class InteractiveCombinationChooser {

public:
  InteractiveCombinationChooser(const NeighbourhoodContainer&) {}

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
