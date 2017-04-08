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
  bool activated;
  double ucbValue;

  NeighbourhoodHistory(bool activated, double ucbValue)
      : activated(activated), ucbValue(ucbValue) {}
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
  std::vector<vector<NeighbourhoodHistory>> neighbourhoodRewardHistory;
  /**
   * Stores which neighbourhood was chosen at each timestep
   */
  std::vector<int> neighbourhoodActivationHistory;
  /**
   * The neighbourhoods which have been activated on the current incumbent solution
   */
  std::vector<bool> activatedNeighbourhoods;
  vector<NeighbourhoodRewards> neighbourhoodRewards;

  int timeStep;
  u_int64_t totalTime;
  int totalNumberOfVisits;
  DomainInt lastMinValue;
  DomainInt bestMinValue;

  double ucbValue(NeighbourhoodRewards& neighbourhoodReward) {
    return neighbourhoodReward.reward +
           std::sqrt((2 * std::log(totalTime)) / (neighbourhoodReward.totalTimeTaken));
  }

  bool isEnabled(const NeighbourhoodContainer& nhc, int nhIndex) {
    return !(nhc.neighbourhoods[nhIndex].activation.isAssigned());
  }

public:
  UCBNeighborHoodSelection(const NeighbourhoodContainer& nhc)
      : activatedNeighbourhoods(nhc.neighbourhoods.size(), false),
        neighbourhoodRewards(nhc.neighbourhoods.size()),
        timeStep(0),
        totalTime(0),
        totalNumberOfVisits(0),
        lastMinValue(getState().getOptimiseVar()->getMin()),
        bestMinValue(getState().getOptimiseVar()->getMin()) {}

  void updateStats(const vector<int>& activatedNeighbourhoods,
                   const NeighbourhoodStats& neighbourhoodStats) {
    debug_log(neighbourhoodStats);
    bestMinValue = neighbourhoodStats.newMinValue > bestMinValue ? neighbourhoodStats.newMinValue
                                                                 : bestMinValue;
    totalTime += neighbourhoodStats.timeTaken == 0 ? 1000 : neighbourhoodStats.timeTaken;

    if(neighbourhoodStats.solutionFound) {
      this->activatedNeighbourhoods.assign(this->activatedNeighbourhoods.size(), false);
    }
    for(int nhIndex : activatedNeighbourhoods) {
      NeighbourhoodRewards& currentNeighbourhood = neighbourhoodRewards[nhIndex];
      currentNeighbourhood.numberOfVisits++;
      currentNeighbourhood.totalTimeTaken +=
          neighbourhoodStats.timeTaken == 0 ? 1000 : neighbourhoodStats.timeTaken;
      currentNeighbourhood.timeOut = neighbourhoodStats.timeoutReached;
      totalNumberOfVisits++;
      if(neighbourhoodStats.solutionFound) {
        currentNeighbourhood.reward += (neighbourhoodStats.newMinValue > lastMinValue) ? 1 : -1;
        currentNeighbourhood.timeOutValue = 500;
      } else {
        this->activatedNeighbourhoods[activatedNeighbourhoods[0]] = true;
        currentNeighbourhood.reward--;
      }
    }
  }

  vector<int> getNeighbourHoodsToActivate(const NeighbourhoodContainer& neighbourhoodContainer,
                                          int& neighbourhoodTimeout) {
    neighbourhoodRewardHistory.push_back({});
    double bestUCTValue = -(std::numeric_limits<double>::max());
    int index = -1;
    for(int i = 0; i < neighbourhoodRewards.size(); i++) {
      if(isEnabled(neighbourhoodContainer, i)) {
        if(neighbourhoodRewards[i].numberOfVisits == 0) {
          neighbourhoodActivationHistory.push_back(i);
          timeStep++;
          return {i};
        }
        double currentUCBValue = ucbValue(neighbourhoodRewards[i]);
        // std::cout << "Neighbourhood " << i << " vale is " << currentUCBValue << std::endl;
        if(currentUCBValue > bestUCTValue) {
          bestUCTValue = currentUCBValue;
          index = i;
        }
        neighbourhoodRewardHistory[timeStep].push_back(NeighbourhoodHistory(true, currentUCBValue));
      } else {
        neighbourhoodRewardHistory[timeStep].push_back(NeighbourhoodHistory(false, 0));
      }
    }

    timeStep++;
    neighbourhoodActivationHistory.push_back(index);
    if(activatedNeighbourhoods[index]) {
      // neighbourhoodTimeout = neighbourhoodRewards[index].timeOutValue * 2;
      // neighbourhoodRewards[index].timeOutValue *= 2;
      //std::cout << "Neighbourhood  " << index << " has already been tried! " << std::endl;
      // D_FATAL_ERROR("CALLING A NEIGHBOURHOOD TWICE on the same solution. Should implement logic
      // to change timeout"
      //               "or something!!");
    }

    activatedNeighbourhoods[index] = true;
    assert(index >= 0);
    neighbourhoodTimeout = 500;
    return {index};
  }
  /*
    void printHistory(NeighbourhoodContainer& nhc) {
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
        std::cout << " Activated Neighbourhood -> " <<
    neighbourhoodActivationHistory[currentTimeStep]
                  << ": " <<
    nhc.neighbourhoods[neighbourhoodActivationHistory[currentTimeStep]].name;
        std::cout << " Value found here is -> " << valuesFound[currentTimeStep];
        currentTimeStep++;
        std::cout << std::endl;
      }
      std::cout << std::endl;
    }
    */
};

class InteractiveNeighbourhoodChooser {

public:
  InteractiveNeighbourhoodChooser(const NeighbourhoodContainer& neighbourhoodContainer) {}

  void updateStats(const vector<int>& activatedNeighbourhoods,
                   const NeighbourhoodStats& neighbourhoodStats) {
    std::cout << neighbourhoodStats << std::endl;
  }

  vector<int> getNeighbourHoodsToActivate(const NeighbourhoodContainer& neighbourhoodContainer,
                                          double& neighbourhoodTimeout) {
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
