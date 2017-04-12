//
// Created by Patrick Spracklen on 3/26/17.
//

#ifndef MINION_SEARCHSTRATEGIES_H
#define MINION_SEARCHSTRATEGIES_H

#include "neighbourhood-search.h"
#include <cmath>

void copyOverIncumbent(NeighbourhoodContainer& nhc, const vector<DomainInt>& solution) {
  if(Controller::get_world_depth() != 1) {
    Controller::world_pop();
  }
  Controller::world_push();
  for(int i = 0; i < nhc.shadow_mapping[0].size(); i++) {
    nhc.shadow_mapping[1][i].assign(solution[i]);
  }
}

template<typename SelectionStrategy>
class MetaStrategy{

  HillClimbingSearch<SelectionStrategy> hillClimbingSearch;
  HolePuncher holePuncher;
  int neighbourhoodSize = 0;
  DomainInt bestSolutionValue;
  std::vector<DomainInt> bestSolution;
  bool betterSolutionFound;
  enum class PHASE {HILL_CLIMBING, HOLE_PUNCHING, HILL_TESTING};
  PHASE currentPhase = HILL_CLIMBING;

public:
  MetaStrategy():
    hillClimbingSearch(SelectionStrategy()){}


  void updateStats(NeighbourhoodContainer &nhc, std::shared_ptr<Propagate> prop,
                   std::vector<int> &currentActivatedNeighbourhoods, NeighbourhoodStats &stats,
                   std::vector<DomainInt> & solution) {

      switch{
        case HILL_CLIMBING:{
          if (!hillClimbingSearch.finishedPhase()){
            hillClimbingSearch.updateStats(nhc, prop, currentActivatedNeighbourhoods, stats, solution);
          }else {
            if (stats.newMinValue > bestSolutionFound){
              bestSolutionFound = stats.newMinValue;
              bestSolution = solution;
              betterSolutionFound = true;
            }

            //This will check whether hole puncher has run out of actie neighbourhoods or if the
            // new solution is better than the current one we have at that point it should
            // clear all active neighbourhoods and move to the hole puncher and generate new solutions
            if (holePuncher.shouldFinishAndCleanup(betterSolutionFound, currentNeighbourhoodSize)){
              currentPhase = HOLE_PUNCHER;
            }else {
              std::vector<DomainInt> randomSolution = holePuncher.getRandomSolution();
              copyOverIncumbent(nhc, randomSolution);
            }

          }
        }
        case HOLE_PUNCHER:{
          holePuncher.updateStats(nhc, prop, currentActivatedNeighbourhoods, stats, solution);
          if (holePuncher.finishedPhase()){
            currentPhase = HILL_CLIMBING;
          }


        }


      }


  }



  void startHillTesting(NeighbourhoodContainer &nhc){
    if (holePuncher.hasNext()){
      std::vector<DomainInt> randomSolution = holePuncher.getRandomSolution();
      copyOverIncumbent(nhc, randomSolution);
    }else {

    }
  }



  std::vector<int> getNeighbourhoodsToActivate(NeighbourhoodContainer &nhc,
                    int &neighbourhoodTimeout){
    switch PHASE {
        case HILL_CLIMBING:
          return hillClimbing.getNeighbourhoodsToActivate(nhc, neighbourhoodTimeout);
        case HOLE_PUNCHING:
          return holePuncher.getNeighbourhoodsToActivate(nhc, neighbourhoodTimeout);
        case HILL_TESTING:
          return hillClimbing.getNeighbourhoodsToActivate(nhc, neighbourhoodTimeout);
      }
  }

};





template <typename SelectionStrategy>
class HillClimbingSearch {

  static const double INITIAL_LOCAL_MAX_PROBABILITY;
  bool searchComplete;
  DomainInt minValue;
  double localMaxProbability = INITIAL_LOCAL_MAX_PROBABILITY;
  double probabilityIncrementConstant = 0.1;

public:
  std::shared_ptr<SelectionStrategy> selectionStrategy;

  HillClimbingSearch(std::shared_ptr<SelectionStrategy> selectionStrategy)
      : selectionStrategy(std::move(selectionStrategy)), searchComplete(false) {}

  void updateStats(NeighbourhoodContainer& nhc, std::shared_ptr<Propagate> prop,
                   std::vector<int>& currentActivatedNeighbourhoods, NeighbourhoodStats& stats,
                   std::vector<DomainInt>& solution) {
    std::cout << "Hill Climbing Search -- Update Stats " << std::endl;

    selectionStrategy->updateStats(currentActivatedNeighbourhoods, stats);

    if(stats.solutionFound) {
      resetLocalMaxProbability();
      copyOverIncumbent(nhc, solution);
      if(stats.newMinValue == getState().getOptimiseVar()->getMax()) {
        searchComplete = true;
        return;
      }
      getState().getOptimiseVar()->setMin(stats.newMinValue + 1);
      std::vector<AnyVarRef> emptyVars;
      prop->prop(emptyVars);
    }else {
      localMaxProbability += (1/nhc.neighbourhoods.size()) * probabilityIncrementConstant;
    }
  }

  void resetLocalMaxProbability(){
    localMaxProbability = INITIAL_LOCAL_MAX_PROBABILITY;
  }

  vector<int> getNeighbourHoodsToActivate(NeighbourhoodContainer& nhc,
                                          int& neighbourhoodTimeout) {
    return selectionStrategy->getNeighbourHoodsToActivate(nhc, neighbourhoodTimeout);
  }

  bool continueSearch(NeighbourhoodContainer& nhc, std::vector<DomainInt> &solution) {
      return true;
  }

  bool finishedPhase(){
    return (double std::rand()) / RAND_MAX < localMaxProbability;
  }

  void startPhase(){
    resetLocalMaxProbability();
  }
};





class HolePuncher{

  std::vector<std::vector<DomainInt>> solutionBag;
  int lastNeighbourhoodIndex = 0;
  int lastSize = 0;
  int currentNeighbourhoodSolutionsCount = 0;
  int neighbourhoodSize = 1;
  bool finishedPhase = false;
  std::vector<int> activeNeighbourhoods;
public:

  int maxNumberOfSolutions;

  void updateStats(NeighbourhoodContainer &nhc, std::shared_ptr<Propagate> prop,
                std::vector<int> &currentActivatedNeighbourhoods, NeighbourhoodStats &stats,
                std::vector<DomainInt> & solution){
    finishedPhase = activeNeighbourhoods.empty();
    if(finishedPhase){
      std::shuffle(solutionBag.begin(), solutionBag.end());
    }
  }

  vector<int> getNeighbourhoodsToActivate(NeighbourhoodContainer &nhc, int &neighbourhoodTimeout){
    currentNeighbourhoodSolutionsCount = 0;
    int neighbourhood = activeNeighbourhoods.back();
    activeNeighbourhoods.pop_back();
    return neighbourhood;
  }

  bool continueSearch(NeighbourhoodContainer& nhc, std::vector<DomainInt> &solution){
    solutionBag.push_back(solution);
    return ++currentNeighbourhoodSolutionsCount != maxNumberOfSolutions;
  }

  bool finishedPhase(){
    return finishedPhase;
  }

  bool shouldFinish(bool betterSolutionFound, int &neighbourhoodSize){
    if(betterSolutionFound){
      solutionBag.clear();
      activeNeighbourhoods.clear();
      neighbourhoodSize = 1;
    }else {
      neighbourhoodSize++;
    }
  }

  bool startPhase(NeighbourhoodContainer &nhc, int neighbourhoodSize, int maxNumberOfSolutions, bool betterSolutionFound){
    activeNeighbourhoods.clear();
    for (int i = 0; i < nhc.neighbourhoods.size(); i++){
      if (nhc.neighbourhoods[i].deviation.inDomain(neighbourhoodSize))
        activeNeighbourhoods.push_back(i);
    }

    if (activeNeighbourhoods.empty())
      return false;

    solutionBag.clear();
    this->neighbourhoodSize = neighbourhoodSize;
    this->maxNumberOfSolutions = maxNumberOfSolutions;
    finishedPhase = false;
  }

  bool hasNext(){
    return !solutionBag.empty();
  }

  std::vector<DomainInt> getRandomSolution(){
    assert(!solutionBag.empty());
    std::vector<DomainInt> solution = solutionBag.back();
    solution.pop_back();
    return solution;
  }


};





#endif // MINION_SEARCHSTRATEGIES_H
