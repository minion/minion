#ifndef MINION_SEARCH_UCBNEIGHBOURHOODSELECTIONSTRATEGY_H_
#define MINION_SEARCH_UCBNEIGHBOURHOODSELECTIONSTRATEGY_H_
#include "neighbourhood-def.h"
#include "neighbourhoodSearchStats.h"
#include <math.h>

#include <cassert>
#include <fstream>
#include <iostream>
#include <vector>
static const char* NUMBER_ITERATIONS_KEY = "numberIterations:";
static const char* NUMBER_POSITIVE_KEY = "numberPositive:";
static const char* NUMBER_ACTIVATIONS_KEY = "numberActivations:";

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
           std::sqrt((getOptions().nhConfig.ucbExplorationBias * std::log(totalActivations)) /
                     (totalCombinationVisits));
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
  /*
    inline void readInitialValuesFromFile(std::istream& is, int numberNeighbourhoods) {
      checkName(NUMBER_ITERATIONS_KEY, is);
      if(!is >> initialNumberIterations) {
        std::cerr << "Error when reading " << NUMBER_ITERATIONS_KEY << " expected integer after.\n";
        exit(1);
      }
      std::ws(is);
      checkName(NUMBER_ACTIVATIONS_KEY, is);
      readVector()
    }
    inline void checkName(const std::string& name, std::istream& is) {
      for(const char i : name) {
        checkChar(i, name, is);
      }
    }

    inline void readVector(size_t expectedVectorLength, const std::string& name, std::istream& is,
                           std::vector<u_int64_t>& vec) {
      checkChar('[', name, is);
      for(size_t i = 0; i < expectedVectorLength; i++) {
        int read;
        if(!(is >> read)) {
          std::cerr << "Error when reading numbers after key " << name << ".  Expected "
                    << expectedVectorLength
                    << " integers but something else was found after only reading " << i
                    << " integers.\n";
          exit(1);
        }
        vec.push_back(read);
      }
    }

    inline void checkChar(const char i, const std::string& name, std::istream& is) {
      char read;
      if(!(is >> read) || read != i) {
        std::cerr << "Error when reading key " << name << std::endl;
        std::cerr << "Expected '" << i << "' but found '" << read << "'\n";
        exit(1);
      }
      */
};

#endif /* MINION_SEARCH_UCBNEIGHBOURHOODSELECTIONSTRATEGY_H_ */
