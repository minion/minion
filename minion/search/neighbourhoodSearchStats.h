
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
  DomainInt highestNeighbourhoodSize;

  NeighbourhoodStats(DomainInt newMinValue, u_int64_t timeTaken, bool solutionFound,
                     bool timeoutReached, DomainInt highestNeighbourhoodSize = 0)
      : newMinValue(newMinValue),
        timeTaken(timeTaken),
        solutionFound(solutionFound),
        timeoutReached(timeoutReached),
        highestNeighbourhoodSize(highestNeighbourhoodSize){}

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
  vector<pair<DomainInt, u_int64_t>> bestSolutions;
  int numberOfExplorationPhases;
  int numberOfBetterSolutionsFoundFromExploration;


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
        numberOfExplorationPhases(0),
        numberOfBetterSolutionsFoundFromExploration(0),
        initialOptVarRange(initialOptVarRange),
        valueOfInitialSolution(initialOptVarRange.first),
        lastOptVarValue(initialOptVarRange.first),
        bestOptVarValue(initialOptVarRange.first) {}

  inline u_int64_t getTotalTimeTaken() {
    auto endTime = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
  }

  inline void setValueOfInitialSolution(DomainInt valueOfInitialSolution) {
    this->valueOfInitialSolution = valueOfInitialSolution;
    totalTimeToBestSolution = getTotalTimeTaken();
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
        bestSolutions.emplace_back(lastOptVarValue, getTotalTimeTaken());
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
    os << "Best optimise var value: " << bestOptVarValue << "\n";
    os << "Time till best solution: " << totalTimeToBestSolution << " (ms)\n";
    os << "Total time: " << getTotalTimeTaken() << " (ms)\n";
    os << "Number of explorations " << numberOfExplorationPhases << "\n";
    os << "Number of better solutions found through exploration " << numberOfBetterSolutionsFoundFromExploration << "\n";
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
    os << "History of best solutions found " << "\n";
    for (auto &currentPair: bestSolutions){
      os << "Value : " << currentPair.first << " Time : " << currentPair.second << " \n";
    }
  }
};

/*
  I guess it would be nice to know the following:
  1. How much time do we spend in hole punching mode?
    So we can estimate this by looking at the number of neighbourhoods. As we know each neighbourhood hole punch is capped to 500 ms then we know
    at most we spend 500ms * |numberOfNeighbourhoods| per hole punch.



  2. The problem with UCB is that it will not respond to quick changes in performance very quickly so for tsp if one arm is doing really well even when we hit the peak
  it will continue to select that arm for a bit until the holepuncher kicks in.



  3. Should we update UCB after the fact?



  Questions:
  1. Is there any point in pulling the same arm twice??
  -> Dont we start at the same value ordering for the size of the neighbourhood?
  -> If we give it the same amount of time is it going to find out anything different?

  Maybe we could do something whereby we include the size of the neighbourhood that the neighbourhood arrived at in part of the calculation of whether or not to stop.
  The higher it managed to climb and if it fails then we take that as more of an indication that we should stop.

*/


#endif /* MINION_SEARCH_NEIGHBOURHOODSEARCHSTATS_H_ */
