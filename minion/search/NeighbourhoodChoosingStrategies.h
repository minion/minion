#ifndef MINION_NEIGHBOURHOODCHOOSINGSTRATEGIES_H
#define MINION_NEIGHBOURHOODCHOOSINGSTRATEGIES_H
#include "neighbourhood-def.h"
#include "neighbourhoodSearchStats.h"
#include <cassert>
#include <math.h>
#include <vector>

class RandomNeighbourhoodChooser {

  std::vector<bool> neighbourhoodSuccessHistory;
  std::vector<int> neighbourhoodActivationHistory;

public:
  RandomNeighbourhoodChooser(const NeighbourhoodContainer& neighbourhoodContainer)
      : neighbourhoodSuccessHistory(neighbourhoodContainer.neighbourhoods.size(), true) {}

  void updateStats(const vector<int>& activatedNeighbourhoods,
                   const NeighbourhoodStats& neighbourhoodStats) {}

  vector<int> getNeighbourHoodsToActivate(const NeighbourhoodContainer& neighbourhoodContainer,
                                          double neighbourhoodTimeout) {

    int random = std::rand() % neighbourhoodContainer.neighbourhoods.size();
    int iterations(0);
    while(true) {
      if(!neighbourhoodContainer
              .neighbourhoods[random % neighbourhoodContainer.neighbourhoods.size()]
              .activation.isAssigned()) {
        neighbourhoodActivationHistory.push_back(random);
        return {random};
      }
      random++;
      if(iterations++ == neighbourhoodContainer.neighbourhoods.size()) {
        D_FATAL_ERROR("EMPTY NEIGHBOURHOODS!!!");
        return {};
      }
    }
  }

  void printHistory(NeighbourhoodContainer& nhc) {
    int timeStep(0);
    for(int& n : neighbourhoodActivationHistory) {
      std::cout << "Time Step: " << timeStep++ << " Neighbourhood Activated-> " << n << std::endl;
    }
  }
};

struct NeighbourhoodRewards {
public:
  double reward;
  int numberOfVisits;
  u_int64_t totalTimeTaken;
  bool timeOut;
  double timeOutValue = 500;

  NeighbourhoodRewards() : reward(0), numberOfVisits(0), totalTimeTaken(0), timeOut(false) {}

  friend std::ostream& operator<<(std::ostream& cout, const NeighbourhoodRewards& nr);
};

struct NeighbourhoodHistory {

public:
  std::vector<std::pair<int, DomainInt>> neighbourhoodsActivated;
  std::vector<double> neighbourhoodValues;
  NeighbourhoodSearchStats stats;

  NeighbourhoodHistory(const NeighbourhoodContainer& nhc)
      : neighbourhoodValues(nhc.neighbourhoods.size()) {}

  void addNeighbourhoodUCBValue(double ucbValue, int index) {
    neighbourhoodValues[index] = ucbValue;
  }

  void addActivatedNeighbourhood(int neighbourhood, DomainInt newMinValue) {
    neighbourhoodsActivated.push_back(std::make_pair(neighbourhood, newMinValue));
  }

  void addStats(NeighbourhoodSearchStats currentStats) {
    // stats = currentStats;
  }
  void print(std::ostream& cout, NeighbourhoodContainer& nhc) {
    cout << "Neighbourhood Values " << neighbourhoodValues << std::endl;
    cout << "Activated Neighbourhoods: " << std::endl;
    for(int i = 0; i < neighbourhoodsActivated.size(); i++) {
      cout << nhc.neighbourhoods[neighbourhoodsActivated[i].first].name
           << ": Achieved Min Value -> " << neighbourhoodsActivated[i].second;
    }
    cout << std::endl;
    //  stats.printStats(cout, nhc);
  }
};

std::ostream& operator<<(std::ostream& cout, const NeighbourhoodRewards& nr) {
  cout << "Reward: " << nr.reward << "\n"
       << "Number of Visits: " << nr.numberOfVisits << "\n"
       << "Total Time Taken: " << nr.totalTimeTaken << "\n"
       << "TimeOut: " << nr.timeOut << "\n";
  return cout;
}

class UCBNeighborHoodSelection {
private:
  /**
   * Stores the UCB of each neighbourhood for each timestep -> For Debugging
   */
  std::vector<NeighbourhoodHistory> neighbourhoodRewardHistory;
  // Stores the neighbourhood reward structs
  vector<NeighbourhoodRewards> neighbourhoodRewards;

  int timeStep;
  u_int64_t totalTime;
  int totalNumberOfVisits;
  DomainInt mostRecentMinValue;
  DomainInt highestMinValue;
  static const int TIMEOUT_PENALTY_COST = 1000;

  double ucbValue(double reward, int totalActivations, int totalNeighbourhoodVisits) {
    std::cout << "Reward is " << reward << std::endl;
    std::cout << "Total activations is " << totalActivations << std::endl;
    std::cout << "total neighbourhood visits " << totalNeighbourhoodVisits << std::endl;
    return (reward / totalNeighbourhoodVisits) +
           std::sqrt((2 * std::log(totalActivations)) / (totalNeighbourhoodVisits));
  }

  bool isEnabled(const NeighbourhoodContainer& nhc, int nhIndex) {
    return !(nhc.neighbourhoods[nhIndex].activation.isAssigned());
  }

public:
  UCBNeighborHoodSelection()
      : timeStep(0),
        totalTime(0),
        totalNumberOfVisits(0),
        mostRecentMinValue(getState().getOptimiseVar()->getMin()),
        highestMinValue(getState().getOptimiseVar()->getMin()) {}

  void updateStats(const vector<int>& activatedNeighbourhoods,
                   const NeighbourhoodStats& neighbourhoodStats) {
    debug_log(neighbourhoodStats);
    if(neighbourhoodStats.solutionFound) {
      std::cout << "soltuion found for " << activatedNeighbourhoods[0] << std::endl;
    } else {
      std::cout << "solution not found for " << activatedNeighbourhoods[0] << std::endl;
    }
    neighbourhoodRewardHistory.back().addActivatedNeighbourhood(
        activatedNeighbourhoods[0],
        neighbourhoodStats.solutionFound ? neighbourhoodStats.newMinValue : -1);
  }


  vector<int> getNeighbourHoodsToActivate(const NeighbourhoodContainer& nhc,
                                          NeighbourhoodSearchStats& globalStats) {
    NeighbourhoodHistory currentHistory(nhc);
    double bestUCTValue = -(std::numeric_limits<double>::max());
    int index = -1;
    for(int i = 0; i < globalStats.numberActivations.size(); i++) {
      if(isEnabled(nhc, i)) {
        if(globalStats.numberActivations[i] == 0) {
          currentHistory.addNeighbourhoodUCBValue(0, i);
          neighbourhoodRewardHistory.push_back(currentHistory);
          return {i};
        }
        double currentUCBValue = ucbValue(
            globalStats.numberPositiveSolutions[i] - globalStats.numberNegativeSolutions[i] - globalStats.numberNoSolutions[i],
            globalStats.numberIterations, globalStats.numberActivations[i]);
        // std::cout << "Neighbourhood " << i << " vale is " << currentUCBValue << std::endl;
        if(currentUCBValue > bestUCTValue) {
          bestUCTValue = currentUCBValue;
          index = i;
        }
        currentHistory.addNeighbourhoodUCBValue(currentUCBValue, i);
      } else {
        currentHistory.addNeighbourhoodUCBValue(0, i);
      }
    }
    currentHistory.addStats(globalStats);
    neighbourhoodRewardHistory.push_back(currentHistory);
    return {index};
  }

  void printHistory(NeighbourhoodContainer& nhc) {
    int currentTimeStep = 0;
    for(NeighbourhoodHistory& n : neighbourhoodRewardHistory) {
      std::cout << "Time Step: " << currentTimeStep++ << std::endl;
      std::cout << "--------" << std::endl;
      n.print(std::cout, nhc);
      std::cout << "---------" << std::endl;
    }
    }
};

class InteractiveNeighbourhoodChooser {

public:
  InteractiveNeighbourhoodChooser(const NeighbourhoodContainer& neighbourhoodContainer) {}

  void updateStats(const vector<int>& activatedNeighbourhoods,
                   const NeighbourhoodStats& neighbourhoodStats) {
    std::cout << neighbourhoodStats << std::endl;
  }

  vector<int> getNeighbourHoodsToActivate(const NeighbourhoodContainer& neighbourhoodContainer,
                                          int& neighbourhoodTimeout,
                                          NeighbourhoodSearchStats& globalStats) {
    std::cout << "Global STats: " << std::endl;
    globalStats.printStats(std::cout, neighbourhoodContainer);
    neighbourhoodTimeout = 500;
    for(int i = 0; i < neighbourhoodContainer.neighbourhoods.size(); i++) {
      std::cout << "Neighbourhood " << i << " Is assigned? "
                << neighbourhoodContainer.neighbourhoods[i].activation.isAssigned() << std::endl;
    }
    int activatedNeighbourhood;
    std::cout << "Enter the neighbourhood would you like to activate " << std::endl;
    cin >> activatedNeighbourhood;
    return {activatedNeighbourhood};
  }

  void printHistory(NeighbourhoodContainer& nhc) {}
};

#endif // MINION_NEIGHBOURHOODCHOOSINGSTRATEGIES_H
