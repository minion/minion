#ifndef MINION_NEIGHBOURHOODCHOOSINGSTRATEGIES_H
#define MINION_NEIGHBOURHOODCHOOSINGSTRATEGIES_H
#include "neighbourhood-def.h"
#include "neighbourhoodSearchStats.h"
#include <cassert>
#include <math.h>
#include <vector>

class RandomNeighbourhoodChooser {
  std::vector<int> viableNeighbourhoods;

public:
  RandomNeighbourhoodChooser() {}

  void updateStats(const vector<int>& activatedNeighbourhoods, const NeighbourhoodStats&) {}

  vector<int> getNeighbourhoodsToActivate(const NeighbourhoodContainer& nhc,
                                          NeighbourhoodSearchStats&) {
    viableNeighbourhoods.clear();
    for(size_t i = 0; i < nhc.neighbourhoods.size(); i++) {
      if(nhc.neighbourhoods[i].activation.inDomain(1)) {
        viableNeighbourhoods.push_back(i);
      }
    };
    return {viableNeighbourhoods[std::rand() % viableNeighbourhoods.size()]};
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

  DomainInt mostRecentMinValue;
  DomainInt highestMinValue;
  static const int TIMEOUT_PENALTY_COST = 1000;

  double ucbValue(double reward, int totalActivations, int totalNeighbourhoodVisits) {
    debug_log("Reward is " << reward << std::endl);
    debug_log("Total activations is " << totalActivations << std::endl);
    debug_log("total neighbourhood visits " << totalNeighbourhoodVisits << std::endl);
    return (reward / totalNeighbourhoodVisits) +
           std::sqrt((2 * std::log(totalActivations)) / (totalNeighbourhoodVisits));
  }

  bool isEnabled(const NeighbourhoodContainer& nhc, int nhIndex) {
    return !(nhc.neighbourhoods[nhIndex].activation.isAssigned());
  }

public:
  UCBNeighborHoodSelection()
      : mostRecentMinValue(getState().getOptimiseVar()->getMin()),
        highestMinValue(getState().getOptimiseVar()->getMin()) {}

  void updateStats(const vector<int>& activatedNeighbourhoods,
                   const NeighbourhoodStats& neighbourhoodStats) {
    debug_log(neighbourhoodStats);
    if(neighbourhoodStats.solutionFound) {
      debug_log("soltuion found for " << activatedNeighbourhoods[0] << std::endl);
    } else {
      debug_log("solution not found for " << activatedNeighbourhoods[0] << std::endl);
    }
    neighbourhoodRewardHistory.back().addActivatedNeighbourhood(
        activatedNeighbourhoods[0],
        neighbourhoodStats.solutionFound ? neighbourhoodStats.newMinValue : -1);
  }

  vector<int> getNeighbourhoodsToActivate(const NeighbourhoodContainer& nhc,
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
        double currentUCBValue =
            ucbValue(globalStats.numberPositiveSolutions[i] -
                         globalStats.numberNegativeSolutions[i] - globalStats.numberNoSolutions[i],
                     globalStats.numberIterations, globalStats.numberActivations[i]);
        if(currentUCBValue > bestUCTValue) {
          bestUCTValue = currentUCBValue;
          index = i;
        }
        currentHistory.addNeighbourhoodUCBValue(currentUCBValue, i);
      } else {
        currentHistory.addNeighbourhoodUCBValue(0, i);
      }
    }
    if(index == -1) {
      std::cout << "UCBNeighborHoodSelection: could not activate a neighbourhood.\n";
      throw EndOfSearch();
    }
    currentHistory.addStats(globalStats);
    neighbourhoodRewardHistory.push_back(currentHistory);
    return {index};
  }

  void printHistory(NeighbourhoodContainer& nhc) {
    debug_code(for(NeighbourhoodHistory& n
                   : neighbourhoodRewardHistory) {
      int currentTimeStep = 0;
      debug_log("Time Step: " << currentTimeStep++ << std::endl);
      debug_log("--------" << std::endl);
      debug_code(n.print(std::cout, nhc));
      debug_log("---------" << std::endl);
    });
  }
};

class InteractiveNeighbourhoodChooser {

public:
  InteractiveNeighbourhoodChooser() {}

  void updateStats(const vector<int>& activatedNeighbourhoods, const NeighbourhoodStats& stats) {
    std::cout << stats << std::endl;
  }

  vector<int> getNeighbourhoodsToActivate(const NeighbourhoodContainer& nhc,
                                          NeighbourhoodSearchStats&) {
    while(true) {
      std::cout << "Select Neighbourhood:\n";
      for(size_t i = 0; i < nhc.neighbourhoods.size(); ++i) {
        cout << i << ": " << nhc.neighbourhoods[i].name << endl;
      }
      int index;
      bool readInt = bool(cin >> index);
      if(!readInt || index < -1 || index >= (int)nhc.neighbourhoods.size()) {
        cout << "Error, please enter an integer in the range 0.." << nhc.neighbourhoods.size()
             << " to select a neighbourhood or -1 to end search.\n";
      } else if(index == -1) {
        std::cerr << "InteractiveNeighbourhoodChooser: ending search...\n";
        throw EndOfSearch();
      } else {
        return {index};
      }
    }
  }
};

#endif // MINION_NEIGHBOURHOODCHOOSINGSTRATEGIES_H
