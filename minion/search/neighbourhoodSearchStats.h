
#ifndef MINION_SEARCH_NEIGHBOURHOODSEARCHSTATS_H_
#define MINION_SEARCH_NEIGHBOURHOODSEARCHSTATS_H_
#include "neighbourhood-def.h"
#include <utility>
static const std::string indent = "    ";

struct NeighbourhoodStats {
  DomainInt newMinValue;
  u_int64_t timeTaken;
  bool solutionFound;
  bool timeoutReached;

  NeighbourhoodStats(DomainInt newMinValue, u_int64_t timeTaken, bool solutionFound,
                     bool timeoutReached)
      : newMinValue(newMinValue),
        timeTaken(timeTaken),
        solutionFound(solutionFound),
        timeoutReached(timeoutReached) {}

  friend std::ostream& operator<<(std::ostream& cout, const NeighbourhoodStats& stats) {
    cout << "New Min Value: " << stats.newMinValue << "\n"
         << "Time Taken: " << stats.timeTaken << "\n"
         << "Solution Found: " << stats.solutionFound << "\n"
         << "Timeout Reached: " << stats.timeoutReached << "\n";
    return cout;
  }
};

class NeighbourhoodSearchStats {
  int numberIterations;
  vector<int> numberActivations; // mapping from nh index to number of times activated
  vector<u_int64_t> totalTime;
  vector<int> numberPositiveSolutions;
  vector<int> numberNegativeSolutions;
  vector<int> numberNoSolutions;
  vector<int> numberTimeouts;
  const std::pair<DomainInt, DomainInt> initialOptVarRange;
  DomainInt valueOfInitialSolution;
  DomainInt lastOptVarValue;

public:
  NeighbourhoodSearchStats(int numberNeighbourhoods,
                           const std::pair<DomainInt, DomainInt>& initialOptVarRange)
      : numberIterations(0),
        numberActivations(numberNeighbourhoods, 0),
        totalTime(numberNeighbourhoods, 0),
        numberPositiveSolutions(numberNeighbourhoods, 0),
        numberNegativeSolutions(numberNeighbourhoods, 0),
        numberNoSolutions(numberNeighbourhoods, 0),
        numberTimeouts(numberNeighbourhoods, 0),
        initialOptVarRange(initialOptVarRange),
        valueOfInitialSolution(initialOptVarRange.first),
        lastOptVarValue(initialOptVarRange.first) {}

  inline void setValueOfInitialSolution(DomainInt valueOfInitialSolution) {
    this->valueOfInitialSolution = valueOfInitialSolution;
  }

  inline void reportnewStats(const vector<int>& activatedNeighbourhoods,
                             const NeighbourhoodStats& stats) {
    ++numberIterations;
    for(int nhIndex : activatedNeighbourhoods) {
      ++numberActivations[nhIndex];
      totalTime[nhIndex] += stats.timeTaken;
      numberTimeouts[nhIndex] += stats.timeoutReached;
      if(stats.solutionFound) {
        if(stats.newMinValue > lastOptVarValue) {
          ++numberPositiveSolutions[nhIndex];
        } else {
          ++numberNegativeSolutions[nhIndex];
        }
      } else {
        ++numberNoSolutions[nhIndex];
      }
      lastOptVarValue = stats.newMinValue;
    }
  }

  inline void printStats(std::ostream& os, const NeighbourhoodContainer& nhc) {
    os << "Search Stats:\n";
    os << "Number iterations: " << numberIterations << "\n";
    os << "Initial optimise var range: " << initialOptVarRange << "\n";
    os << "Final optimise var value: " << lastOptVarValue << "\n";

    for(int i = 0; i < (int)nhc.neighbourhoods.size(); i++) {
      os << "Neighbourhood: " << nhc.neighbourhoods[i].name << "\n";
      os << indent << "Number activations: " << numberActivations[i] << "\n";
      os << indent << "Total time: " << totalTime[i] << "\n";
      os << indent << "Average time per activation: " << (totalTime[i] / numberActivations[i])
         << "\n";
      os << indent << "Number positive solutions: " << numberPositiveSolutions[i] << "\n";
      os << indent << "Number negative solutions: " << numberNegativeSolutions[i] << "\n";
      os << indent << "Number no solutions: " << numberNoSolutions[i] << "\n";
      os << indent << "Number timeouts: " << numberTimeouts[i] << "\n";
    }
  }
};

#endif /* MINION_SEARCH_NEIGHBOURHOODSEARCHSTATS_H_ */
