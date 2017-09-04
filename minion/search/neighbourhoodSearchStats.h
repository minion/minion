
#ifndef MINION_SEARCH_NEIGHBOURHOODSEARCHSTATS_H_
#define MINION_SEARCH_NEIGHBOURHOODSEARCHSTATS_H_
#include "neighbourhood-def.h"
#include <utility>
static const std::string indent = "    ";

struct NeighbourhoodStats {
  DomainInt newMinValue;
  double timeTaken;
  bool solutionFound;
  bool timeoutReached;
  DomainInt highestNeighbourhoodSize;

public:
  NeighbourhoodStats(DomainInt newMinValue, double timeTaken, bool solutionFound,
                     bool timeoutReached, DomainInt highestNeighbourhoodSize = 0)
      : newMinValue(newMinValue),
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

struct ExplorationPhase {
  int neighbourhoodSize;
  double startExplorationTime;
  double endExplorationTime;
  int numberOfRandomSolutionsPulled;
};

struct NeighbourhoodSearchStats {

  const std::pair<DomainInt, DomainInt> initialOptVarRange;
  DomainInt valueOfInitialSolution;
  DomainInt bestOptVarValue;
  DomainInt optValueAchievedByLastNHCombination;
  vector<pair<DomainInt, double>> bestValueTimes;
  std::vector<std::pair<AnyVarRef, DomainInt>> bestCompleteSolutionAssignment;

  int numberIterations = 0;
  vector<int> numberActivations; // mapping from combinations index to number of times activated
  vector<double> totalTime;
  vector<int> numberPositiveSolutions;
  vector<int> numberNegativeSolutions;
  vector<int> numberNoSolutions;
  vector<int> numberTimeouts;

  int numberOfExplorationPhases = 0;
  int numberOfBetterSolutionsFoundFromExploration = 0;

  vector<int> numberExplorationsByNHCombinationSize;
  vector<int> numberSuccessfulExplorationsByNHCombinationSize;
  vector<double> neighbourhoodExplorationTimes;
  vector<ExplorationPhase> explorationPhases;

  int totalNumberOfRandomSolutionsPulled = 0;
  int numberPulledThisPhase = 0;

  double startTime;
  double startExplorationTime;
  bool currentlyExploring = false;
  int currentNeighbourhoodSize;
  double totalTimeToBestSolution;

  NeighbourhoodSearchStats(int numberCombinations,
                           const std::pair<DomainInt, DomainInt>& initialOptVarRange,
                           int maxNeighbourhoodSize)
      : initialOptVarRange(initialOptVarRange),
        valueOfInitialSolution(initialOptVarRange.first),
        bestOptVarValue(initialOptVarRange.first),
        optValueAchievedByLastNHCombination(initialOptVarRange.first),
        numberActivations(numberCombinations, 0),
        totalTime(numberCombinations, 0),
        numberPositiveSolutions(numberCombinations, 0),
        numberNegativeSolutions(numberCombinations, 0),
        numberNoSolutions(numberCombinations, 0),
        numberTimeouts(numberCombinations, 0),
        numberExplorationsByNHCombinationSize(maxNeighbourhoodSize, 0),
        numberSuccessfulExplorationsByNHCombinationSize(maxNeighbourhoodSize, 0),
        neighbourhoodExplorationTimes(maxNeighbourhoodSize) {
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
      if(stats.newMinValue > optValueAchievedByLastNHCombination) {
        ++numberPositiveSolutions[activatedCombination];
      } else {
        ++numberNegativeSolutions[activatedCombination];
      }
      optValueAchievedByLastNHCombination = stats.newMinValue;
    } else {
      ++numberNoSolutions[activatedCombination];
    }
    ++numberIterations;
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
    if(currentlyExploring && solutionValue > bestOptVarValue) {

      neighbourhoodExplorationTimes[currentNeighbourhoodSize - 1] +=
          get_cpu_time() - startExplorationTime;
      currentlyExploring = false;
      numberSuccessfulExplorationsByNHCombinationSize[currentNeighbourhoodSize - 1] += 1;
      explorationPhases.back().endExplorationTime = getTotalTimeTaken();
      explorationPhases.back().numberOfRandomSolutionsPulled = numberPulledThisPhase;
      numberPulledThisPhase = 0;
    }
    saveCurrentAssignmentIfBest(solutionValue);
  }

  inline void startExploration(int neighbourhoodSize) {
    if(currentlyExploring) {
      neighbourhoodExplorationTimes[currentNeighbourhoodSize - 1] +=
          get_cpu_time() - startExplorationTime;
      explorationPhases.back().endExplorationTime = getTotalTimeTaken();
      explorationPhases.back().numberOfRandomSolutionsPulled = numberPulledThisPhase;
      numberPulledThisPhase = 0;
    }
    currentlyExploring = true;
    startExplorationTime = get_cpu_time();
    numberExplorationsByNHCombinationSize[neighbourhoodSize - 1] += 1;
    currentNeighbourhoodSize = neighbourhoodSize;
    ExplorationPhase currentPhase;
    currentPhase.neighbourhoodSize = neighbourhoodSize;
    currentPhase.startExplorationTime = getTotalTimeTaken();
    explorationPhases.push_back(currentPhase);
  }

  inline std::vector<std::pair<AnyVarRef, DomainInt>>& getBestAssignment() {
    return bestCompleteSolutionAssignment;
  }

  inline void printStats(std::ostream& os, const NeighbourhoodContainer& nhc) {
    os << "Search Stats:\n";
    os << "Number iterations: " << numberIterations << "\n";
    os << "Initial optimise var range: " << initialOptVarRange << "\n";
    os << "Value achieved by last neighbourhood: " << optValueAchievedByLastNHCombination << "\n";
    os << "Best optimise var value: " << bestOptVarValue << "\n";
    os << "Time till best solution: " << totalTimeToBestSolution << " (ms)\n";
    os << "Total time: " << getTotalTimeTaken() << " (ms)\n";
    os << "Average number of random solutions pulled: "
       << (((double)totalNumberOfRandomSolutionsPulled) / explorationPhases.size()) << "\n";
    os << "Total Number of random solutions pulled : " << totalNumberOfRandomSolutionsPulled
       << "\n";
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

    os << "Stats of Explorations:\n";
    ;
    os << "---------------\n";
    for(int i = 0; i < numberExplorationsByNHCombinationSize.size(); i++) {
      os << "NeighbourhoodSize " << (i + 1) << ":\n";
      os << indent << "Activations: " << numberExplorationsByNHCombinationSize[i] << "\n";
      os << indent << "Number successful: " << numberSuccessfulExplorationsByNHCombinationSize[i]
         << "\n";
      os << indent << "Time Spent: " << neighbourhoodExplorationTimes[i] << "\n";
    }
    os << "---------------"
       << "\n";

    os << "Exploration Phases: "
       << "\n";
    for(int i = 0; i < explorationPhases.size(); i++) {
      os << "Phase " << (i + 1) << "\n";
      os << "------------"
         << "\n";
      os << "Start Time: " << explorationPhases[i].startExplorationTime << "\n";
      os << "End Time: " << explorationPhases[i].endExplorationTime << "\n";
      os << "Neighbourhood Size: " << explorationPhases[i].neighbourhoodSize << "\n";
      os << "Number of random solutions PUlled "
         << explorationPhases[i].numberOfRandomSolutionsPulled << "\n";
      os << "-----------------"
         << "\n";
    }
  }

  inline void printCombinationDescription(std::ostream& os, const NeighbourhoodContainer& nhc,
                                          int combIndex) {
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
