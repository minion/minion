#ifndef MINION_NEIGHBOURHOODCHOOSINGSTRATEGIES_H
#define MINION_NEIGHBOURHOODCHOOSINGSTRATEGIES_H
#include "neighbourhood-def.h"
#include <cassert>
#include <math.h>
#include <vector>

struct NeighbourhoodStats {

public:
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

  friend std::ostream& operator<<(std::ostream& cout, const NeighbourhoodStats& stats);
};

std::ostream& operator<<(std::ostream& cout, const NeighbourhoodStats& stats) {
  cout << "New Min Value: " << stats.newMinValue << "\n"
       << "Time Taken: " << stats.timeTaken << "\n"
       << "Solution Found: " << stats.solutionFound << "\n"
       << "Timeout Reached: " << stats.timeoutReached << "\n";
  return cout;
}

class RandomNeighbourhoodChooser {

  std::vector<bool> neighbourhoodSuccessHistory;
  std::vector<int> successfulNeighbourhoods;

public:
  RandomNeighbourhoodChooser(const NeighbourhoodContainer& neighbourhoodContainer)
      : neighbourhoodSuccessHistory(neighbourhoodContainer.neighbourhoods.size(), true),
        successfulNeighbourhoods(neighbourhoodContainer.neighbourhoods.size()) {}

  void updateStats(const vector<int>& activatedNeighbourhoods,
                   const NeighbourhoodStats& neighbourhoodStats) {
    if(neighbourhoodStats.solutionFound) {
      cout << "Solution found, assiging all neighbourhoods to true" << endl;
      neighbourhoodSuccessHistory.assign(neighbourhoodSuccessHistory.size(), true);
    } else {
      for(int neighbourhoodIndex : activatedNeighbourhoods) {
        cout << "Setting neighbourdhood " << neighbourhoodIndex << " to false" << endl;
        neighbourhoodSuccessHistory[neighbourhoodIndex] = false;
      }
    }
  }

  vector<int> getNeighbourHoodsToActivate(const NeighbourhoodContainer& neighbourhoodContainer) {

    successfulNeighbourhoods.clear();
    for(int i = 0; i < neighbourhoodSuccessHistory.size(); i++) {
      if(neighbourhoodSuccessHistory[i])
        successfulNeighbourhoods.push_back(i);
    }
    if(successfulNeighbourhoods.empty()) {
      return {};
    } else {
      int random_variable = std::rand() % successfulNeighbourhoods.size();
      cout << "Attempting Neighbourhood: " << random_variable << endl;
      return {successfulNeighbourhoods[random_variable]};
    }
  }
};

struct NeighbourhoodRewards {
public:
  int id;
  double reward;
  int numberOfVisits;
  u_int64_t totalTimeTaken;
  bool timeOut;
  double ucbValue;

  NeighbourhoodRewards(int id)
      : id(id), reward(0), numberOfVisits(0), totalTimeTaken(0), timeOut(false), ucbValue(0) {}

  friend std::ostream& operator<<(std::ostream& cout, const NeighbourhoodRewards& nr);
};

struct NeighbourhoodHistory {

public:
  bool activated;
  double ucbValue;

  NeighbourhoodHistory(bool activated, double ucbValue)
      : activated(activated), ucbValue(ucbValue) {}
};

std::ostream& operator<<(std::ostream& cout, const NeighbourhoodRewards& nr) {
  cout << "Neighbourhood ID: " << nr.id << "\n"
       << "Reward: " << nr.reward << "\n"
       << "Number of Visits: " << nr.numberOfVisits << "\n"
       << "Total Time Taken: " << nr.totalTimeTaken << "\n"
       << "TimeOut: " << nr.timeOut << "\n";
  return cout;
}

class UCBNeighborHoodSelection {

private:
  double epsilon = 0.0000001;
  const NeighbourhoodContainer nhc;

  std::vector<vector<NeighbourhoodHistory>> neighbourhoodRewardHistory;
  std::vector<int> neighbourhoodActivationHistory;
  int timeStep;
  u_int64_t totalTime;
  bool ucbValue(NeighbourhoodRewards& neighbourhoodReward) {
    if(nhc.neighbourhoods[neighbourhoodReward.id].activation.isAssigned()) {
      std::cout << "Neighbourhood " << neighbourhoodReward.id << " is assigned a value of "
                << nhc.neighbourhoods[neighbourhoodReward.id].activation.getAssignedValue()
                << std::endl;
      return false;
    }
    neighbourhoodReward.ucbValue =
        neighbourhoodReward.reward +
        ((2 * std::log(totalTime + epsilon)) / (neighbourhoodReward.totalTimeTaken + epsilon));
    return true;
  }

public:
  vector<NeighbourhoodRewards> neighbourhoodRewards;
  UCBNeighborHoodSelection(const NeighbourhoodContainer nhc) : nhc(nhc), totalTime(0), timeStep(0) {
    std::cout << "Number of neighbour hoods is " << nhc.neighbourhoods.size() << std::endl;
    for(int i = 0; i < nhc.neighbourhoods.size(); i++) {
      neighbourhoodRewards.push_back(NeighbourhoodRewards(i));
    }
    std::cout << "Number of neighbour hoods rewards is " << neighbourhoodRewards.size()
              << std::endl;
  }

  void updateStats(const vector<int>& activatedNeighbourhoods,
                   const NeighbourhoodStats& neighbourhoodStats) {
    std::cout << neighbourhoodStats << std::endl;
    // Get the activated neighbourhood
    NeighbourhoodRewards& currentNeighbourhood = neighbourhoodRewards[activatedNeighbourhoods[0]];
    currentNeighbourhood.numberOfVisits++;
    currentNeighbourhood.totalTimeTaken +=
        neighbourhoodStats.timeTaken == 0 ? 1000 : neighbourhoodStats.timeTaken;
    currentNeighbourhood.reward += neighbourhoodStats.solutionFound ? 1 : -1;
    currentNeighbourhood.timeOut = neighbourhoodStats.timeoutReached;
    totalTime += neighbourhoodStats.timeTaken == 0 ? 1000 : neighbourhoodStats.timeTaken;
    ;
  }

  vector<int> getNeighbourHoodsToActivate(const NeighbourhoodContainer& neighbourhoodContainer) {
    /* auto neighbourhoodToActivate =
         std::max_element(neighbourhoodRewards.begin(), neighbourhoodRewards.end(),
                          [this](NeighbourhoodRewards& N1, NeighbourhoodRewards& N2) {
                            return this->ucbValue(N1) < this->ucbValue(N2);
                          });
                          */
    neighbourhoodRewardHistory.push_back({});
    double bestUCTValue = -(std::numeric_limits<double>::max());
    int index = -1;
    for(int i = 0; i < neighbourhoodRewards.size(); i++) {
      if(ucbValue(neighbourhoodRewards[i])) {
        std::cout << "Neighbourhood " << i << " vale is " << neighbourhoodRewards[i].ucbValue
                  << std::endl;
        if(neighbourhoodRewards[i].ucbValue > bestUCTValue) {
          bestUCTValue = neighbourhoodRewards[i].ucbValue;
          index = i;
        }
        neighbourhoodRewardHistory[timeStep].push_back(
            NeighbourhoodHistory(true, neighbourhoodRewards[i].ucbValue));
      } else {
        neighbourhoodRewardHistory[timeStep].push_back(NeighbourhoodHistory(false, 0));
      }
    }
    timeStep++;
    neighbourhoodActivationHistory.push_back(index);
    assert(index >= 0);
    return {index};
    // return {1};
    // return {static_cast<int>(neighbourhoodToActivate - neighbourhoodRewards.begin())};
  }

  void printHistory() {
    int currentTimeStep = 0;
    for(vector<NeighbourhoodHistory>& timeStepHistory : neighbourhoodRewardHistory) {
      std::cout << "Current time step: " << currentTimeStep << std::endl;
      std::cout << "[";
      for(NeighbourhoodHistory& currentHistory : timeStepHistory) {
        if(currentHistory.activated)
          std::cout << currentHistory.ucbValue << ",";
        else
          std::cout << "False"
                    << ",";
      }
      std::cout << "]";
      std::cout << " Activated Neighbourhood -> " << neighbourhoodActivationHistory[currentTimeStep]
                << ": " << nhc.neighbourhoods[neighbourhoodActivationHistory[currentTimeStep]].name;
      currentTimeStep++;
    }
    std::cout << std::endl;
  }
};

#endif // MINION_NEIGHBOURHOODCHOOSINGSTRATEGIES_H
