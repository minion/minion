#ifndef MINION_NEIGHBOURHOODCHOOSINGSTRATEGIES_H
#define MINION_NEIGHBOURHOODCHOOSINGSTRATEGIES_H
#include "neighbourhood-def.h"
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

  NeighbourhoodRewards(int id)
      : id(id), reward(0), numberOfVisits(0), totalTimeTaken(0), timeOut(false) {}

  friend std::ostream& operator<<(std::ostream& cout, const NeighbourhoodRewards& nr);
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

  u_int64_t totalTime;
  double ucbValue(NeighbourhoodRewards& neighbourhoodReward) {
    if(nhc.neighbourhoods[neighbourhoodReward.id].activation.isAssigned()) {
      return -(std::numeric_limits<double>::max());
    }
    return neighbourhoodReward.reward +
           ((2 * std::log(totalTime + epsilon)) / (neighbourhoodReward.totalTimeTaken + epsilon));
  }

public:
  vector<NeighbourhoodRewards> neighbourhoodRewards;
  UCBNeighborHoodSelection(const NeighbourhoodContainer nhc) : nhc(nhc), totalTime(0) {
    for(int i = 0; i < nhc.neighbourhoods.size(); i++) {
      neighbourhoodRewards.push_back(NeighbourhoodRewards(i));
    }
  }

  void updateStats(const vector<int>& activatedNeighbourhoods,
                   const NeighbourhoodStats& neighbourhoodStats) {
    // Get the activated neighbourhood
    NeighbourhoodRewards& currentNeighbourhood = neighbourhoodRewards[activatedNeighbourhoods[0]];
    currentNeighbourhood.numberOfVisits++;
    currentNeighbourhood.totalTimeTaken += neighbourhoodStats.timeTaken;
    currentNeighbourhood.reward += neighbourhoodStats.solutionFound ? 1 : -1;
    currentNeighbourhood.timeOut = neighbourhoodStats.timeoutReached;
  }

  vector<int> getNeighbourHoodsToActivate(const NeighbourhoodContainer& neighbourhoodContainer) {
    auto neighbourhoodToActivate =
        std::max_element(neighbourhoodRewards.begin(), neighbourhoodRewards.end(),
                         [this](NeighbourhoodRewards& N1, NeighbourhoodRewards& N2) {
                           return this->ucbValue(N1) < this->ucbValue(N2);
                         });

    return {static_cast<int>(neighbourhoodToActivate - neighbourhoodRewards.begin())};
  }
};

#endif // MINION_NEIGHBOURHOODCHOOSINGSTRATEGIES_H
