#ifndef MINION_SEARCHSTRATEGIES_H
#define MINION_SEARCHSTRATEGIES_H

#include "NeighbourhoodChoosingStrategies.h"
#include "neighbourhood-search.h"
#include "search/neighbourhood-search.h"
#include "search/nhConfig.h"
#include <algorithm>
#include <cmath>

template <typename Integer>
class ExponentialIncrementer {
  double value;
  const double multiplier;
  const double increment;

public:
  ExponentialIncrementer(double initialValue, double multiplier, double increment)
      : value(initialValue), multiplier(multiplier), increment(increment) {}
  void increase() {
    value *= multiplier;
    value += increment;
  }

  Integer getValue() {
    return std::round(value);
  }
};

template <typename SelectionStrategy>
class HillClimbingSearch {

  SelectionStrategy selectionStrategy;

public:
  DomainInt bestSolutionValue;
  std::vector<DomainInt> bestSolution;

private:
  pair<size_t, NeighbourhoodStats>
  runNeighbourhood(NeighbourhoodState& nhState, ExponentialIncrementer<int> backtrackLimit,
                   const vector<size_t>& highestNeighbourhoodSizes) {
    int combinationToActivate = selectionStrategy.getCombinationsToActivate(nhState);
    SearchParams params = SearchParams::neighbourhoodSearch(
        combinationToActivate, nhState.nhc, true, true, false,
        getOptions().nhConfig->iterationSearchTime, backtrackLimit.getValue(),
        getOptions().nhConfig->backtrackInsteadOfTimeLimit,
        highestNeighbourhoodSizes[combinationToActivate]);
    NeighbourhoodStats stats = nhState.searchNeighbourhoods(params);
    selectionStrategy.updateStats(combinationToActivate, stats);

    return make_pair(combinationToActivate, stats);
  }
  void handleBetterSolution(NeighbourhoodState& nhState, NeighbourhoodStats& stats,
                            int& iterationsSpentAtPeak, double& localMaxProbability,
                            vector<size_t> highestNeighbourhoodSizes) {
    iterationsSpentAtPeak = 0;
    localMaxProbability = getOptions().nhConfig->hillClimberInitialLocalMaxProbability;
    highestNeighbourhoodSizes.assign(nhState.nhc.neighbourhoodCombinations.size(), 1);
    bestSolutionValue = stats.newMinValue;
    bestSolution = std::move(nhState.solution);
    nhState.solution = {};
    nhState.copyOverIncumbent(bestSolution);
    getState().getOptimiseVar()->setMin(stats.newMinValue + 1);
    nhState.propagate();
  }

public:
  HillClimbingSearch(const NeighbourhoodContainer& nhc) : selectionStrategy(nhc) {}

  void run(NeighbourhoodState& nhState, DomainInt initSolutionValue,
           std::vector<DomainInt>& initSolution) {

    int iterationsSpentAtPeak = 0;
    size_t numberIterationsAtStart = nhState.globalStats.numberIterations;
    // The probability that we will enter a random mode of exploration.
    double localMaxProbability = getOptions().nhConfig->hillClimberInitialLocalMaxProbability;
    auto& nhConfig = getOptions().nhConfig;
    ExponentialIncrementer<int> backtrackLimit(nhConfig->initialBacktrackLimit,
                                               nhConfig->backtrackLimitMultiplier,
                                               nhConfig->backtrackLimitIncrement);
    vector<size_t> highestNeighbourhoodSizes(nhState.nhc.neighbourhoodCombinations.size(), 1);

    bestSolutionValue = initSolutionValue;
    bestSolution = initSolution;
    nhState.copyOverIncumbent(bestSolution);
    getState().getOptimiseVar()->setMin(bestSolutionValue);
    nhState.propagate();
    nhState.globalStats.notifyStartClimb();
    while(true) {
      auto nhInfo = runNeighbourhood(nhState, backtrackLimit, highestNeighbourhoodSizes);
      NeighbourhoodStats& stats = nhInfo.second;
      if(!getOptions().nhConfig->increaseBacktrackOnlyOnFailure || !stats.solutionFound) {
        backtrackLimit.increase();
      }
      if(stats.solutionFound) {
        handleBetterSolution(nhState, stats, iterationsSpentAtPeak, localMaxProbability,
                             highestNeighbourhoodSizes);
      } else {
        highestNeighbourhoodSizes[nhInfo.first] =
            checked_cast<size_t>(stats.highestNeighbourhoodSize);
        localMaxProbability += (1.0 / nhState.nhc.neighbourhoodCombinations.size()) *
                               getOptions().nhConfig->hillClimberProbabilityIncrementMultiplier;
        ++iterationsSpentAtPeak;
        uniform_real_distribution<double> dist(0.0, 1.0);
        double random_number = dist(global_random_gen);
        if(iterationsSpentAtPeak > getOptions().nhConfig->hillClimberMinIterationsToSpendAtPeak &&
           random_number < localMaxProbability) {
          nhState.globalStats.notifyEndClimb();
          cout << "numberIterations: "
               << (nhState.globalStats.numberIterations - numberIterationsAtStart) << std::endl;
          return;
        }
      }
    }
  }
};

template <typename SelectionStrategy>
class LateAcceptanceHillClimbingSearch {

  SelectionStrategy selectionStrategy;
  DomainInt currentSolutionValue;
  std::vector<DomainInt> currentSolution;
  std::deque<DomainInt> recentSolutionValueQueue;

public:
  DomainInt bestSolutionValue;
  std::vector<DomainInt> bestSolution;

private:
  pair<size_t, NeighbourhoodStats>
  runNeighbourhood(NeighbourhoodState& nhState, ExponentialIncrementer<int> backtrackLimit,
                   const vector<size_t>& highestNeighbourhoodSizes) {
    int combinationToActivate = selectionStrategy.getCombinationsToActivate(nhState);
    SearchParams params = SearchParams::neighbourhoodSearch(
        combinationToActivate, nhState.nhc, true, true, false,
        getOptions().nhConfig->iterationSearchTime, backtrackLimit.getValue(),
        getOptions().nhConfig->backtrackInsteadOfTimeLimit,
        highestNeighbourhoodSizes[combinationToActivate]);
    NeighbourhoodStats stats = nhState.searchNeighbourhoods(params);
    selectionStrategy.updateStats(combinationToActivate, stats);

    return make_pair(combinationToActivate, stats);
  }

  void handleSolutionFound(NeighbourhoodState& nhState, NeighbourhoodStats& stats,
                           int& iterationsSpentAtPeak, vector<size_t> highestNeighbourhoodSizes) {
    highestNeighbourhoodSizes.assign(nhState.nhc.neighbourhoodCombinations.size(), 1);
    currentSolutionValue = stats.newMinValue;

    currentSolution = nhState.solution;
    recentSolutionValueQueue.push_back(stats.newMinValue);
    recentSolutionValueQueue.pop_front();

    if(stats.newMinValue > bestSolutionValue) {
      iterationsSpentAtPeak = 0;

      bestSolutionValue = stats.newMinValue;
      bestSolution = std::move(nhState.solution);
      nhState.solution = {};
    }
  }
  bool hasFinished(size_t iterationsSpentAtPeak) {
    size_t iterationLimit =
        round(getOptions().nhConfig->lahcQueueSize * getOptions().nhConfig->lahcStoppingLimitRatio);
    return iterationsSpentAtPeak > iterationLimit;
  }

public:
  LateAcceptanceHillClimbingSearch(const NeighbourhoodContainer& nhc) : selectionStrategy(nhc) {}

  void run(NeighbourhoodState& nhState, DomainInt initSolutionValue,
           std::vector<DomainInt>& initSolution) {

    int iterationsSpentAtPeak = 0;
    size_t numberIterationsAtStart = nhState.globalStats.numberIterations;
    auto& nhConfig = getOptions().nhConfig;
    ExponentialIncrementer<int> backtrackLimit(nhConfig->initialBacktrackLimit,
                                               nhConfig->backtrackLimitMultiplier,
                                               nhConfig->backtrackLimitIncrement);
    vector<size_t> highestNeighbourhoodSizes(nhState.nhc.neighbourhoodCombinations.size(), 1);

    bestSolutionValue = initSolutionValue;
    bestSolution = initSolution;
    currentSolutionValue = bestSolutionValue;
    currentSolution = bestSolution;
    recentSolutionValueQueue.assign(getOptions().nhConfig->lahcQueueSize, currentSolutionValue);
    nhState.globalStats.notifyStartClimb();
    while(true) {
      nhState.copyOverIncumbent(currentSolution);
      getState().getOptimiseVar()->setMin(
          std::min(currentSolutionValue, recentSolutionValueQueue.front()));
      nhState.propagate();
      auto nhInfo = runNeighbourhood(nhState, backtrackLimit, highestNeighbourhoodSizes);
      NeighbourhoodStats& stats = nhInfo.second;
      if(!getOptions().nhConfig->increaseBacktrackOnlyOnFailure || !stats.solutionFound) {
        backtrackLimit.increase();
      }
      if(!stats.solutionFound || stats.newMinValue <= bestSolutionValue) {
        ++iterationsSpentAtPeak;
        if(hasFinished(iterationsSpentAtPeak)) {
          nhState.globalStats.notifyEndClimb();
          cout << "numberIterations: "
               << (nhState.globalStats.numberIterations - numberIterationsAtStart) << std::endl;
          return;
        }
      }

      if(stats.solutionFound) {
        handleSolutionFound(nhState, stats, iterationsSpentAtPeak, highestNeighbourhoodSizes);
      } else {
        recentSolutionValueQueue.push_back(currentSolutionValue);
        recentSolutionValueQueue.pop_front();
        highestNeighbourhoodSizes[nhInfo.first] =
            checked_cast<size_t>(stats.highestNeighbourhoodSize);
      }
    }
  }
};

template <typename SelectionStrategy>
class SimulatedAnnealingSearch {

  SelectionStrategy selectionStrategy;
  DomainInt currentSolutionValue;
  std::vector<DomainInt> currentSolution;
  double temperature;

public:
  DomainInt bestSolutionValue;
  std::vector<DomainInt> bestSolution;

private:
  pair<size_t, NeighbourhoodStats> runNeighbourhood(NeighbourhoodState& nhState,
                                                    ExponentialIncrementer<int> backtrackLimit) {
    int combinationToActivate = selectionStrategy.getCombinationsToActivate(nhState);
    SearchParams params = SearchParams::neighbourhoodSearch(
        combinationToActivate, nhState.nhc, true, false, true,
        getOptions().nhConfig->iterationSearchTime, backtrackLimit.getValue(),
        getOptions().nhConfig->backtrackInsteadOfTimeLimit, 1, false);
    NeighbourhoodStats stats = nhState.searchNeighbourhoods(params);
    selectionStrategy.updateStats(combinationToActivate, stats);

    return make_pair(combinationToActivate, stats);
  }

  void handleSolutionFound(NeighbourhoodState& nhState, NeighbourhoodStats& stats,
                           int& iterationsSpentAtPeak, int delta) {
    bool solutionAccepted;
    if(stats.newMinValue > currentSolutionValue) {
      solutionAccepted = true;
    } else {
      double acceptanceProb = exp(delta / temperature);
      uniform_real_distribution<double> dist(0.0, 1.0);
      solutionAccepted = dist(global_random_gen) <= acceptanceProb;
    }
    if(solutionAccepted) {
      currentSolutionValue = stats.newMinValue;
      currentSolution = nhState.solution;
    }

    if(stats.newMinValue > bestSolutionValue) {
      iterationsSpentAtPeak = 0;

      bestSolutionValue = stats.newMinValue;
      bestSolution = std::move(nhState.solution);
      nhState.solution = {};
    }
  }
  bool hasFinished(size_t iterationsSpentAtPeak) {
    size_t iterationLimit = round(getOptions().nhConfig->simulatedAnnealingIterationsBetweenCool *
                                  getOptions().nhConfig->simulatedAnnealingRatioForStopping);
    return iterationsSpentAtPeak > iterationLimit;
  }
  void setInitialTemperature(NeighbourhoodState& nhState, vector<DomainInt>& initSolution) {
    nhState.solution = initSolution;
    vector<int> solutionDeltas;
    auto& nhConfig = getOptions().nhConfig;
    ExponentialIncrementer<int> backtrackLimit(nhConfig->initialBacktrackLimit,
                                               nhConfig->backtrackLimitMultiplier,
                                               nhConfig->backtrackLimitIncrement);
    size_t i = 0;
    while(i < 10) {
      nhState.copyOverIncumbent(nhState.solution);
      nhState.propagate();
      NeighbourhoodStats stats = runNeighbourhood(nhState, backtrackLimit).second;
      if(!stats.solutionFound) {
        // nguyen what to do in this case?
        // maybe backtrackLimit.increase and just don't count this run.
        // I am not incrementing i in this case so that we get exactly 10 deltas
        backtrackLimit.increase();
      } else {
        int delta = checked_cast<int>(stats.newMinValue - stats.oldMinValue);
        if(delta < 0) {
          solutionDeltas.push_back(delta);
          ++i;
        }
      }
    }

    // do something with the deltas
    double meanDelta = 0;
    for(int i = 0; i < solutionDeltas.size(); i++)
      meanDelta += solutionDeltas[i];
    meanDelta /= solutionDeltas.size();

    // when finished, set the class member called temperature
    temperature =
        meanDelta / log(nhConfig->simulatedAnnealingTargetProbabilityForInitialTemperature);
    // abort();
  }

public:
  SimulatedAnnealingSearch(const NeighbourhoodContainer& nhc) : selectionStrategy(nhc) {}

  void run(NeighbourhoodState& nhState, DomainInt initSolutionValue,
           std::vector<DomainInt>& initSolution) {

    int iterationsSpentAtPeak = 0;
    size_t numberIterationsAtStart = nhState.globalStats.numberIterations;
    auto& nhConfig = getOptions().nhConfig;
    ExponentialIncrementer<int> backtrackLimit(nhConfig->initialBacktrackLimit,
                                               nhConfig->backtrackLimitMultiplier,
                                               nhConfig->backtrackLimitIncrement);
    size_t numberIterationsSinceLastCool = 0;

    bestSolutionValue = initSolutionValue;
    bestSolution = initSolution;
    currentSolutionValue = bestSolutionValue;
    currentSolution = bestSolution;
    setInitialTemperature(nhState, currentSolution);

    nhState.globalStats.notifyStartClimb();
    while(true) {
      nhState.copyOverIncumbent(currentSolution);
      nhState.propagate();
      auto nhInfo = runNeighbourhood(nhState, backtrackLimit);
      NeighbourhoodStats& stats = nhInfo.second;
      int delta =
          (stats.solutionFound) ? checked_cast<int>(stats.newMinValue - currentSolutionValue) : 0;

      if(!getOptions().nhConfig->increaseBacktrackOnlyOnFailure || !stats.solutionFound ||
         delta < 0) {

        backtrackLimit.increase();
      }
      if(!stats.solutionFound || stats.newMinValue <= bestSolutionValue) {
        ++iterationsSpentAtPeak;
        if(hasFinished(iterationsSpentAtPeak)) {
          nhState.globalStats.notifyEndClimb();
          cout << "numberIterations: "
               << (nhState.globalStats.numberIterations - numberIterationsAtStart) << std::endl;
          return;
        }
      }

      if(stats.solutionFound) {
        handleSolutionFound(nhState, stats, iterationsSpentAtPeak, delta);
      }
      if(++numberIterationsSinceLastCool >
         getOptions().nhConfig->simulatedAnnealingIterationsBetweenCool) {
        temperature *= getOptions().nhConfig->simulatedAnnealingTemperatureCoolingFactor;
        numberIterationsSinceLastCool = 0;
      }
    }
  }
};
template <typename SearchStrategy>
class MetaSearch {
  SearchStrategy searchStrategy;
  int minNeighbourhoodSize = 0;
  int neighbourhoodSizeOffset = 0;
  ExponentialIncrementer<int> backtrackLimit;

public:
  std::vector<DomainInt> bestSolution;
  DomainInt bestSolutionValue;
  MetaSearch(const NeighbourhoodContainer& nhc)
      : searchStrategy(nhc),
        backtrackLimit(getOptions().nhConfig->holePuncherInitialBacktrackLimit,
                       getOptions().nhConfig->holePuncherBacktrackLimitMultiplier, 0) {}

  void run(NeighbourhoodState& nhState, DomainInt initSolutionValue,
           std::vector<DomainInt>& initSolution) {
    searchStrategy.run(nhState, initSolutionValue, initSolution);
    bestSolution = std::move(searchStrategy.bestSolution);
    bestSolutionValue = searchStrategy.bestSolutionValue;
    resetNeighbourhoodSize();
    while(true) {
      nhState.copyOverIncumbent(bestSolution);
      nhState.propagate();

      bool success = false;
      auto availableNHCombinations = findNextNeighbourhoodSizeWithActiveCombinations(nhState.nhc);
      nhLog("Exploration with neighbourhood of ssize "
            << currentNeighbourhoodSize()
            << ", number of available neighbourhoods: " << availableNHCombinations.size());
      if(availableNHCombinations.empty()) {
        randomClimbUntilBetter(nhState);
        resetNeighbourhoodSize();
        continue;
      }
      for(int nhIndex : availableNHCombinations) {
        NeighbourhoodStats stats = runNeighbourhood(nhState, nhIndex);
        if(!stats.solutionFound) {
          continue;
        }
        searchStrategy.run(nhState, stats.newMinValue, nhState.solution);
        if(searchStrategy.bestSolutionValue > bestSolutionValue) {
          bestSolutionValue = searchStrategy.bestSolutionValue;
          bestSolution = searchStrategy.bestSolution;
          nhState.copyOverIncumbent(bestSolution);
          nhState.propagate();

          resetNeighbourhoodSize();
          success = true;
          nhLog("New best solution found");
          break;
        } else {
          nhLog("New best solution not found");
          nhState.copyOverIncumbent(bestSolution);
          nhState.propagate();
        }
      }
      if(!success) {
        minNeighbourhoodSize *= 2;
      }
    }
  }

  void randomClimbUntilBetter(NeighbourhoodState& nhState) {
    while(true) {
      nhLog("Exploring from random solution");
      nhState.popToBaseDepth();

      NeighbourhoodStats stats = findRandomSolutionUsingNormalSearch(nhState);
      searchStrategy.run(nhState, stats.newMinValue, nhState.solution);
      if(searchStrategy.bestSolutionValue > bestSolutionValue) {
        bestSolutionValue = searchStrategy.bestSolutionValue;
        bestSolution = nhState.solution;
        nhLog("New best solution found");
        resetNeighbourhoodSize();
        return;
      } else {
        nhLog("New best solution not found");
      }
    }
  }

  NeighbourhoodStats runNeighbourhood(NeighbourhoodState& nhState, size_t nhIndex) {
    SearchParams params = SearchParams::neighbourhoodSearch(
        nhIndex, nhState.nhc, true, false, true, getOptions().nhConfig->iterationSearchTime,
        backtrackLimit.getValue(), getOptions().nhConfig->backtrackInsteadOfTimeLimit,
        currentNeighbourhoodSize());
    NeighbourhoodStats stats = nhState.searchNeighbourhoods(params);
    if(!stats.solutionFound) {
      backtrackLimit.increase();
    }
    return stats;
  }
  void resetNeighbourhoodSize() {
    minNeighbourhoodSize = 1;
    neighbourhoodSizeOffset = 0;
  }
  inline int currentNeighbourhoodSize() const {
    return minNeighbourhoodSize + neighbourhoodSizeOffset;
  }

  inline std::vector<int>
  findNextNeighbourhoodSizeWithActiveCombinations(const NeighbourhoodContainer& nhc) {
    std::vector<int> activeCombinations;
    int maxNHSize = nhc.getMaxNeighbourhoodSize();
    while(currentNeighbourhoodSize() <= maxNHSize) {
      activeCombinations.clear();
      for(int i = 0; i < nhc.neighbourhoodCombinations.size(); ++i) {
        if(nhc.isCombinationEnabled(i) &&
           nhc.neighbourhoods[nhc.neighbourhoodCombinations[i][0]].deviation.inDomain(
               currentNeighbourhoodSize()))
          activeCombinations.push_back(i);
      }
      if(!activeCombinations.empty()) {
        break;
      } else {
        ++neighbourhoodSizeOffset;
      }
    }
    if(!activeCombinations.empty()) {
      std::random_shuffle(activeCombinations.begin(), activeCombinations.end());
    }
    return activeCombinations;
  }
};

template <typename NhSelectionStrategy>
shared_ptr<Controller::SearchManager> MakeNeighbourhoodSearchHelper(PropagationLevel& prop_method,
                                                                    vector<SearchOrder>& base_order,
                                                                    NeighbourhoodContainer& nhc) {
  shared_ptr<Propagate> prop = Controller::make_propagator(prop_method);
  switch(getOptions().neighbourhoodSearchStrategy) {
  case SearchOptions::NeighbourhoodSearchStrategy::META_WITH_HILLCLIMBING:
    return std::make_shared<
        NeighbourhoodSearchManager<MetaSearch<HillClimbingSearch<NhSelectionStrategy>>>>(
        prop, base_order, nhc);
  case SearchOptions::NeighbourhoodSearchStrategy::META_WITH_LAHC:
    return std::make_shared<NeighbourhoodSearchManager<
        MetaSearch<LateAcceptanceHillClimbingSearch<NhSelectionStrategy>>>>(prop, base_order, nhc);
  case SearchOptions::NeighbourhoodSearchStrategy::META_WITH_SIMULATED_ANNEALING:
    return std::make_shared<
        NeighbourhoodSearchManager<MetaSearch<SimulatedAnnealingSearch<NhSelectionStrategy>>>>(
        prop, base_order, nhc);
  case SearchOptions::NeighbourhoodSearchStrategy::HILL_CLIMBING:
    return std::make_shared<NeighbourhoodSearchManager<HillClimbingSearch<NhSelectionStrategy>>>(
        prop, base_order, std::move(nhc));
  case SearchOptions::NeighbourhoodSearchStrategy::LAHC:
    return std::make_shared<
        NeighbourhoodSearchManager<LateAcceptanceHillClimbingSearch<NhSelectionStrategy>>>(
        prop, base_order, nhc);
  case SearchOptions::NeighbourhoodSearchStrategy::SIMULATED_ANNEALING:
    return std::make_shared<
        NeighbourhoodSearchManager<SimulatedAnnealingSearch<NhSelectionStrategy>>>(prop, base_order,
                                                                                   nhc);
  default: assert(false); abort();
  }
}

shared_ptr<Controller::SearchManager> MakeNeighbourhoodSearch(PropagationLevel prop_method,
                                                              vector<SearchOrder> base_order,
                                                              NeighbourhoodContainer nhc) {
  switch(getOptions().neighbourhoodSelectionStrategy) {
  case SearchOptions::NeighbourhoodSelectionStrategy::RANDOM:
    return MakeNeighbourhoodSearchHelper<RandomCombinationChooser>(prop_method, base_order, nhc);
  case SearchOptions::NeighbourhoodSelectionStrategy::UCB:
    return MakeNeighbourhoodSearchHelper<UCBNeighbourhoodSelection>(prop_method, base_order, nhc);
  case SearchOptions::NeighbourhoodSelectionStrategy::LEARNING_AUTOMATON:
    return MakeNeighbourhoodSearchHelper<LearningAutomatonNeighbourhoodSelection>(prop_method,
                                                                                  base_order, nhc);
  case SearchOptions::NeighbourhoodSelectionStrategy::INTERACTIVE:
    return MakeNeighbourhoodSearchHelper<InteractiveCombinationChooser>(prop_method, base_order,
                                                                        nhc);
  default: assert(false); abort();
  }
}

#endif
