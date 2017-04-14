
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
public:
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
  DomainInt bestOptVarValue;
  std::chrono::high_resolution_clock::time_point startTime;
  u_int64_t totalTimeToBestSolution;
  NeighbourhoodSearchStats() {}

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
        lastOptVarValue(initialOptVarRange.first),
        bestOptVarValue(initialOptVarRange.first) {}

  inline u_int64_t getTotalTimeTaken() {
    return std::chrono::high_resolution_clock::now() - startTime).count();
  }

  inline void setValueOfInitialSolution(DomainInt valueOfInitialSolution) {
    this->valueOfInitialSolution = valueOfInitialSolution;
    totalTimeToBestSolution = getTotalTimeTaken();
    std::chrono::duration_cast<std::chrono::milliseconds>(
  }
  inline void startTimer() {
    startTime = std::chrono::high_resolution_clock::now();
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
        lastOptVarValue = stats.newMinValue;
      } else {
        ++numberNoSolutions[nhIndex];
      }

      if(lastOptVarValue > bestOptVarValue) {
        bestOptVarValue = lastOptVarValue;
        totalTimeToBestSolution = getTotalTimeTaken();
      }
    }
  }

  inline void printStats(std::ostream& os, const NeighbourhoodContainer& nhc) {
    os << "Search Stats:\n";
    os << "Number iterations: " << numberIterations << "\n";
    os << "Initial optimise var range: " << initialOptVarRange << "\n";
    os << "Most recent optimise var value: " << lastOptVarValue << "\n";
    os << "Best optimise var value: " << lastOptVarValue << "\n";
    os << "Time till best solution: " << totalTimeToBestSolution << " (ms)\n";
    os << "Total time: " << getTotalTimeTaken() << " (ms)\n";
    for(int i = 0; i < (int)nhc.neighbourhoods.size(); i++) {
      os << "Neighbourhood: " << nhc.neighbourhoods[i].name << "\n";
      os << indent << "Number activations: " << numberActivations[i] << "\n";
      u_int64_t averageTime = (numberActivations[i] > 0) ? totalTime[i] / numberActivations[i] : 0;
      os << indent << "Total time: " << totalTime[i] << "\n";
      os << indent << "Average time per activation: " << averageTime << "\n";
      os << indent << "Number positive solutions: " << numberPositiveSolutions[i] << "\n";
      os << indent << "Number negative solutions: " << numberNegativeSolutions[i] << "\n";
      os << indent << "Number no solutions: " << numberNoSolutions[i] << "\n";
      os << indent << "Number timeouts: " << numberTimeouts[i] << "\n";
    }
  }
};

#endif /* MINION_SEARCH_NEIGHBOURHOODSEARCHSTATS_H_ */
