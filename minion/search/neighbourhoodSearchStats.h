
#ifndef MINION_SEARCH_NEIGHBOURHOODSEARCHSTATS_H_
#define MINION_SEARCH_NEIGHBOURHOODSEARCHSTATS_H_
#include "neighbourhood-def.h"
#include <utility>
static const std::string indent = "    ";

struct NeighbourhoodStats {
  DomainInt newMinValue;
  DomainInt oldMinValue;
  double timeTaken;
  bool solutionFound;
  bool timeoutReached;
  DomainInt highestNeighbourhoodSize;

public:
  NeighbourhoodStats(DomainInt newMinValue, DomainInt oldMinValue, double timeTaken,
                     bool solutionFound, bool timeoutReached,
                     DomainInt highestNeighbourhoodSize = 0)
      : newMinValue(newMinValue),
        oldMinValue(oldMinValue),
        timeTaken(timeTaken),
        solutionFound(solutionFound),
        timeoutReached(timeoutReached),
        highestNeighbourhoodSize(highestNeighbourhoodSize) {}

  friend std::ostream& operator<<(std::ostream& cout, const NeighbourhoodStats& stats) {
    cout << "New Min Value: " << stats.newMinValue << "\n"
         << "Time Taken: " << stats.timeTaken << "\n"
         << "Solution Found: " << stats.solutionFound << "\n"
         << "Timeout Reached: " << stats.timeoutReached << "\n";
    return cout;
  }
};

struct NeighbourhoodSearchStats {

  const std::pair<DomainInt, DomainInt> initialOptVarRange;
  DomainInt valueOfInitialSolution;
  DomainInt bestOptVarValue;
  vector<pair<DomainInt, double>> bestValueTimes;
  std::vector<std::pair<AnyVarRef, DomainInt>> bestCompleteSolutionAssignment;

  u_int64_t numberIterations = 0;
  vector<u_int64_t>
      numberActivations; // mapping from combinations index to number of times activated
  vector<double> totalTime;
  vector<u_int64_t> numberPositiveSolutions;
  vector<u_int64_t> numberNegativeSolutions;
  vector<u_int64_t> numberNoSolutions;
  vector<u_int64_t> numberTimeouts;
  double startTime;
  double totalTimeToBestSolution;
  double hillClimberStartTime;
  double totalHillClimberTime = 0;
  int numberHillClimbs = 0;
  double explorationStartTime;
  double totalExplorationTime = 0;
  int numberExplorations = 0;
  int numberTimesSolutionBagExhausted = 0;
  double randomSearchStartTime;
  double totalRandomSearchTime = 0;
  int numberRandomSearches = 0;

  NeighbourhoodSearchStats(int numberCombinations,
                           const std::pair<DomainInt, DomainInt>& initialOptVarRange,
                           int maxNeighbourhoodSize)
      : initialOptVarRange(initialOptVarRange),
        valueOfInitialSolution(initialOptVarRange.first),
        bestOptVarValue(initialOptVarRange.first),
        numberActivations(numberCombinations, 0),
        totalTime(numberCombinations, 0),
        numberPositiveSolutions(numberCombinations, 0),
        numberNegativeSolutions(numberCombinations, 0),
        numberNoSolutions(numberCombinations, 0),
        numberTimeouts(numberCombinations, 0) {
    getVars().forAllVars(
        [&](const AnyVarRef& v) { bestCompleteSolutionAssignment.emplace_back(v, 0); });
  }

  inline double getTotalTimeTaken() {
    return get_cpu_time() - startTime;
  }

  inline void startTimer() {
    startTime = get_cpu_time();
  }

  inline void reportnewStats(const int activatedCombination, const NeighbourhoodStats& stats) {
    if(activatedCombination < 0) {
      return;
    }
    ++numberActivations[activatedCombination];
    totalTime[activatedCombination] += stats.timeTaken;
    numberTimeouts[activatedCombination] += stats.timeoutReached;
    if(stats.solutionFound) {
      if(stats.newMinValue > stats.oldMinValue) {
        ++numberPositiveSolutions[activatedCombination];
      } else {
        ++numberNegativeSolutions[activatedCombination];
      }
    } else {
      ++numberNoSolutions[activatedCombination];
    }
    ++numberIterations;
    if(stats.solutionFound && stats.newMinValue == initialOptVarRange.second) {
      nhLog("achieved max possible opt value : " << stats.newMinValue);
      throw EndOfSearch();
      return;
    }
  }

  inline void saveCurrentAssignmentIfBest(DomainInt currentAssignmentOptValue) {
    if(numberIterations == 0 || currentAssignmentOptValue > bestOptVarValue) {
      bestOptVarValue = currentAssignmentOptValue;
      totalTimeToBestSolution = getTotalTimeTaken();
      cout << "Best solution: " << bestOptVarValue << " " << totalTimeToBestSolution << endl;
      bestValueTimes.emplace_back(bestOptVarValue, totalTimeToBestSolution);
      // save assignment
      for(auto& varValuePair : bestCompleteSolutionAssignment) {
        assert(varValuePair.first.isAssigned());
        varValuePair.second = varValuePair.first.getAssignedValue();
      }
    }
  }

  inline void foundSolution(DomainInt solutionValue) {
    saveCurrentAssignmentIfBest(solutionValue);
  }

  inline std::vector<std::pair<AnyVarRef, DomainInt>>& getBestAssignment() {
    return bestCompleteSolutionAssignment;
  }

  inline void notifyStartClimb() {
    numberHillClimbs += 1;
    hillClimberStartTime = getTotalTimeTaken();
    nhLog("Start climb from value " << getState().getOptimiseVar()->getMin()
                                    << ", number iterations = " << numberIterations);
  }

  inline void notifyEndClimb() {
    totalHillClimberTime += (getTotalTimeTaken() - hillClimberStartTime);
    nhLog("End climb at value " << getState().getOptimiseVar()->getMin()
                                << ", number iterations = " << numberIterations);
  }

  inline void notifyStartExploration() {
    numberExplorations += 1;
    explorationStartTime = getTotalTimeTaken();
  }

  inline void notifyEndExploration() {
    totalExplorationTime += (getTotalTimeTaken() - explorationStartTime);
  }

  inline void notifyStartRandomSearch() {
    numberRandomSearches += 1;
    randomSearchStartTime = getTotalTimeTaken();
  }

  inline void notifyEndRandomSearch() {
    totalRandomSearchTime += (getTotalTimeTaken() - randomSearchStartTime);
  }

  inline void printStats(std::ostream& os, const NeighbourhoodContainer& nhc) {
    os << "Search Stats:\n";
    os << "Number iterations: " << numberIterations << "\n";
    os << "Initial optimise var range: " << initialOptVarRange << "\n";
    os << "Best optimise var value: " << bestOptVarValue << "\n";
    os << "Time till best solution: " << totalTimeToBestSolution << "s\n";
    os << "Total time: " << getTotalTimeTaken() << "s\n";
    os << "Total hill climbing time: " << totalHillClimberTime << "s\n";
    os << "Number hill climbs: " << numberHillClimbs << "\n";
    os << "Total exploration time: " << totalExplorationTime << "s\n";
    os << "Number explorations: " << numberExplorations << "\n";
    os << "Total random search time: " << totalRandomSearchTime << "s\n";
    os << "Number random searches: " << numberRandomSearches << "\n";
    os << "Number of times solution bag exhausted: " << numberTimesSolutionBagExhausted << "\n";
    for(int i = 0; i < (int)nhc.neighbourhoodCombinations.size(); i++) {
      printCombinationDescription(os, nhc, i);
      os << "\n";
      os << indent << "Number activations: " << numberActivations[i] << "\n";
      double averageTime = (numberActivations[i] > 0) ? totalTime[i] / numberActivations[i] : 0;
      os << indent << "Total time: " << totalTime[i] << "\n";
      os << indent << "Average time per activation: " << averageTime << "\n";
      os << indent << "Number positive solutions: " << numberPositiveSolutions[i] << "\n";
      os << indent << "Number negative solutions: " << numberNegativeSolutions[i] << "\n";
      os << indent << "Number no solutions: " << numberNoSolutions[i] << "\n";
      os << indent << "Number timeouts: " << numberTimeouts[i] << "\n";
    }
    os << "History of best solutions found:\n";
    for(const auto& valueTimePair : bestValueTimes) {
      os << indent << "Value: " << valueTimePair.first << ", Time: " << valueTimePair.second
         << "\n";
    }
  }

  inline void printCombinationDescription(std::ostream& os, const NeighbourhoodContainer& nhc,
                                          int combIndex) const {
    const std::vector<int>& combination = nhc.neighbourhoodCombinations[combIndex];
    os << "Neighbourhood Combination:\n    Primary nh: " << nhc.neighbourhoods[combination[0]].name
       << "\n";
    os << "    Secondary nhs (random ordering): [";
    bool first = true;
    for(int i = 1; i < combination.size(); ++i) {
      if(first) {
        first = false;
      } else {
        os << ",";
      }
      os << nhc.neighbourhoods[combination[i]].name;
    }
    os << "]";
  }
};

#endif /* MINION_SEARCH_NEIGHBOURHOODSEARCHSTATS_H_ */
