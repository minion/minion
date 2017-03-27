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
      debug_log("Solution found, assiging all neighbourhoods to true");
      neighbourhoodSuccessHistory.assign(neighbourhoodSuccessHistory.size(), true);
    } else {
      for(int neighbourhoodIndex : activatedNeighbourhoods) {
        debug_log("Setting neighbourdhood " << neighbourhoodIndex << " to false");
        neighbourhoodSuccessHistory[neighbourhoodIndex] = false;
      }
    }
  }

  vector<int> getNeighbourHoodsToActivate(const NeighbourhoodContainer& neighbourhoodContainer) {

    successfulNeighbourhoods.clear();
    for(int i = 0; i < neighbourhoodSuccessHistory.size(); i++) {
      if(neighbourhoodSuccessHistory[i] &&
         !neighbourhoodContainer.neighbourhoods[i].activation.isAssigned()) {
        successfulNeighbourhoods.push_back(i);
      }
    }
    if(successfulNeighbourhoods.empty()) {
      return {};
    } else {
      int random_variable = std::rand() % successfulNeighbourhoods.size();
      debug_log("Attempting Neighbourhood: " << random_variable);
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

  NeighbourhoodRewards(int id)
      : id(id), reward(0), numberOfVisits(0), totalTimeTaken(0), timeOut(false) {}

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
  int timeStep;
  u_int64_t totalTime;
  int totalNumberOfVisits;

  double ucbValue(NeighbourhoodRewards& neighbourhoodReward) {
    return neighbourhoodReward.reward +
        std::sqrt((2 * std::log(totalNumberOfVisits)) / (neighbourhoodReward.numberOfVisits ));
  }

  bool isEnabled(const NeighbourhoodContainer &nhc, NeighbourhoodRewards &neighbourhoodReward){
    return !(nhc.neighbourhoods[neighbourhoodReward.id].activation.isAssigned());
  }

public:
  vector<NeighbourhoodRewards> neighbourhoodRewards;
  UCBNeighborHoodSelection(const NeighbourhoodContainer nhc) :
    totalTime(0),
    timeStep(0),
    totalNumberOfVisits(0)
    {
      activatedNeighbourhoods.assign(nhc.neighbourhoods.size(), false);
    std::cout << "Number of neighbour hoods is " << nhc.neighbourhoods.size() << std::endl;
    for(int i = 0; i < nhc.neighbourhoods.size(); i++) {
      neighbourhoodRewards.push_back(NeighbourhoodRewards(i));
    }
    std::cout << "Number of neighbour hoods rewards is " << neighbourhoodRewards.size()
              << std::endl;
  }

  void updateStats(const vector<int>& activatedNeighbourhoods,
                   const NeighbourhoodStats& neighbourhoodStats) {
    debug_log(neighbourhoodStats);
    // Get the activated neighbourhood
    NeighbourhoodRewards& currentNeighbourhood = neighbourhoodRewards[activatedNeighbourhoods[0]];
    currentNeighbourhood.numberOfVisits++;
    currentNeighbourhood.totalTimeTaken +=
        neighbourhoodStats.timeTaken == 0 ? 1000 : neighbourhoodStats.timeTaken;
    currentNeighbourhood.reward += neighbourhoodStats.solutionFound ? 1 : -1;
    currentNeighbourhood.timeOut = neighbourhoodStats.timeoutReached;
    totalTime += neighbourhoodStats.timeTaken == 0 ? 1000 : neighbourhoodStats.timeTaken;
    totalNumberOfVisits++;

    if (neighbourhoodStats.solutionFound) {
      this->activatedNeighbourhoods.assign(this->activatedNeighbourhoods.size(), false);
    }
    else {
      this->activatedNeighbourhoods[activatedNeighbourhoods[0]] = true;
    }

  }

  vector<int> getNeighbourHoodsToActivate(const NeighbourhoodContainer& neighbourhoodContainer) {
    neighbourhoodRewardHistory.push_back({});
    double bestUCTValue = -(std::numeric_limits<double>::max());
    int index = -1;
    for(int i = 0; i < neighbourhoodRewards.size(); i++) {
      if(isEnabled(neighbourhoodContainer, neighbourhoodRewards[i])){
        if(neighbourhoodRewards[i].numberOfVisits == 0){
          neighbourhoodActivationHistory.push_back(i);
          timeStep++;
          return {i};
        }
        double currentUCBValue= ucbValue(neighbourhoodRewards[i]);
        debug_log("Neighbourhood " << i << " vale is " << currentUCBValue);
        if(currentUCBValue> bestUCTValue) {
          bestUCTValue = currentUCBValue;
          index = i;
        }
        neighbourhoodRewardHistory[timeStep].push_back(
            NeighbourhoodHistory(true, currentUCBValue));
      } else {
        neighbourhoodRewardHistory[timeStep].push_back(NeighbourhoodHistory(false, 0));
      }
    }

    timeStep++;
    neighbourhoodActivationHistory.push_back(index);
    if (activatedNeighbourhoods.at(index)){
      std::cout << "Neighbourhood has already been tried! " << std::endl;
      D_FATAL_ERROR("CALLING A NEIGHBOURHOOD TWICE on the same solution. Should implement logic to change timeout"
                      "or something!!");
    }

    activatedNeighbourhoods[index] = true;
    assert(index >= 0);
    return {index};
  }

  void printHistory(NeighbourhoodContainer &nhc) {
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

class InteractiveNeighbourhoodChooser{

public:
  InteractiveNeighbourhoodChooser(const NeighbourhoodContainer& neighbourhoodContainer)
  {}

  void updateStats(const vector<int>& activatedNeighbourhoods,
                   const NeighbourhoodStats& neighbourhoodStats) {
    std::cout << neighbourhoodStats << std::endl;
  }



  vector<int> getNeighbourHoodsToActivate(const NeighbourhoodContainer& neighbourhoodContainer) {
    for (int i = 0 ; i< neighbourhoodContainer.neighbourhoods.size(); i++){
      std::cout << "Neighbourhood " << i << " Is assigned? " << neighbourhoodContainer.neighbourhoods[i].activation.isAssigned() << std::endl;
    }
    int activatedNeighbourhood;
    std::cout << "Enter the neighbourhood would you like to activate " << std::endl;
    cin >> activatedNeighbourhood;
    return {activatedNeighbourhood};
  }




};







#endif // MINION_NEIGHBOURHOODCHOOSINGSTRATEGIES_H
