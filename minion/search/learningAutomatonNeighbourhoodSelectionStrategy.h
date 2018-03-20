#ifndef MINION_SEARCH_LEARNINGAUTOMATONNEIGHBOURHOODSELECTIONSTRATEGY_H_
#define MINION_SEARCH_LEARNINGAUTOMATONNEIGHBOURHOODSELECTIONSTRATEGY_H_
#include "neighbourhood-def.h"
#include "neighbourhoodSearchStats.h"
#include <math.h>

#include <cassert>
#include <fstream>
#include <iostream>
#include <vector>
class LearningAutomatonNeighbourhoodSelection {
  DomainInt lastSolutionValue;

  std::vector<double> combinationProbabilities;

public:
  LearningAutomatonNeighbourhoodSelection(const NeighbourhoodContainer& nhc)
      : combinationProbabilities(nhc.neighbourhoodCombinations.size(),
                                 ((double)1) / nhc.neighbourhoodCombinations.size()) {}

  void updateStats(int activatedCombination, const NeighbourhoodStats& combinationStats) {
    bool improved = combinationStats.newMinValue > lastSolutionValue;
    double rate = getOptions().nhConfig.learningAutomatonRate;
    double& currentProb = combinationProbabilities[activatedCombination];
    currentProb =
        currentProb + rate * improved * (1 - currentProb) - rate * (1 - improved) * currentProb;
    for(size_t i = 0; i < combinationProbabilities.size(); ++i) {
      if(i == activatedCombination) {
        continue;
      }
      double& otherProb = combinationProbabilities[i];
      otherProb =
          otherProb - rate * improved * otherProb +
          rate * (1 - improved) * (((double)1) / (combinationProbabilities.size() - 1) - otherProb);
    }
  }

  int getCombinationsToActivate(const NeighbourhoodContainer& nhc,
                                NeighbourhoodSearchStats& globalStats,
                                DomainInt currentSolutionValue) {
    lastSolutionValue = currentSolutionValue;
    double randomNumber = ((double)rand()) / RAND_MAX;
    double totalProb = 0;
    for(size_t i = 0; i < combinationProbabilities.size(); i++) {
      totalProb += combinationProbabilities[i];
      if(totalProb > randomNumber) {
        return i;
      }
    }
    std::cerr << "Error, could not choose neighbourhood, should never be reached. totalProb="
              << totalProb << " and randomNumber=" << randomNumber << std::endl;
    abort();
  }
};

#endif /* MINION_SEARCH_LEARNINGAUTOMATONNEIGHBOURHOODSELECTIONSTRATEGY_H_ */
