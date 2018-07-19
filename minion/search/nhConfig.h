
#ifndef MINION_SEARCH_NHCONFIG_H_
#define MINION_SEARCH_NHCONFIG_H_

#include <memory>

struct NhConfig {
  // values here are not necessarily optimal (tuned)
  // they are defaults until tuning info can be given
  // can be overridden by commandline

  bool backtrackInsteadOfTimeLimit = true;
  int iterationSearchTime = 500;
  double initialSearchBacktrackLimitMultiplier = 2.0;
  int initialBacktrackLimit = 22;
  double backtrackLimitMultiplier = 1.1;
  double backtrackLimitIncrement = 0;
  int lahcQueueSize = 100;
  double lahcStoppingLimitRatio = 1.0;
  double holePuncherBacktrackLimitMultiplier = 1.1;
  bool increaseBacktrackOnlyOnFailure = true;
  int hillClimberMinIterationsToSpendAtPeak = 4;
  double hillClimberInitialLocalMaxProbability = 0.001;
  double hillClimberProbabilityIncrementMultiplier = 1.0 / 16;
  double simulatedAnnealingTemperatureCoolingFactor = 0.9;
  int simulatedAnnealingIterationsBetweenCool = 5;
  double simulatedAnnealingTargetProbabilityForInitialTemperature = 0.8;
  double ucbExplorationBias = 2;
  double learningAutomatonRate = 0.1;
  int holePuncherSolutionBagSizeConstant = 5;
};
inline std::ostream& operator<<(std::ostream& os, NhConfig& nhConfig) {
  os << "NhConfig {" << std::endl;
  os << "backtrackInsteadOfTimeLimit = " << nhConfig.backtrackInsteadOfTimeLimit << ",\n";
  os << "iterationSearchTime = " << nhConfig.iterationSearchTime << ",\n";
  os << "initialSearchBacktrackLimitMultiplier = " << nhConfig.initialSearchBacktrackLimitMultiplier
     << ",\n";
  os << "initialBacktrackLimit = " << nhConfig.initialBacktrackLimit << ",\n";
  os << "backtrackLimitMultiplier = " << nhConfig.backtrackLimitMultiplier << ",\n";
  os << "backtrackLimitIncrement = " << nhConfig.backtrackLimitIncrement << ",\n";
  os << "lahcQueueSize = " << nhConfig.lahcQueueSize << ",\n";
  os << "lahcStoppingLimitRatio = " << nhConfig.lahcStoppingLimitRatio << ",\n";
  os << "holePuncherBacktrackLimitMultiplier = " << nhConfig.holePuncherBacktrackLimitMultiplier
     << ",\n";
  os << "increaseBacktrackOnlyOnFailure = " << nhConfig.increaseBacktrackOnlyOnFailure << ",\n";
  os << "hillClimberMinIterationsToSpendAtPeak = " << nhConfig.hillClimberMinIterationsToSpendAtPeak
     << ",\n";
  os << "hillClimberInitialLocalMaxProbability = " << nhConfig.hillClimberInitialLocalMaxProbability
     << ",\n";
  os << "hillClimberProbabilityIncrementMultiplier = "
     << nhConfig.hillClimberProbabilityIncrementMultiplier << ",\n";
  os << "simulatedAnnealingTemperatureCoolingFactor = "
     << nhConfig.simulatedAnnealingTemperatureCoolingFactor << ",\n";
  os << "simulatedAnnealingIterationsBetweenCool = "
     << nhConfig.simulatedAnnealingIterationsBetweenCool << ",\n";
  os << "simulatedAnnealingTargetProbabilityForInitialTemperature = "
     << nhConfig.simulatedAnnealingTargetProbabilityForInitialTemperature << ",\n";
  os << "ucbExplorationBias = " << nhConfig.ucbExplorationBias << ",\n";
  os << "learningAutomatonRate = " << nhConfig.learningAutomatonRate << ",\n";
  os << "holePuncherSolutionBagSizeConstant = " << nhConfig.holePuncherSolutionBagSizeConstant
     << "\n";
  os << "}\n";
  return os;
}

std::shared_ptr<NhConfig> makeNhConfig() {
  return make_shared<NhConfig>();
}

#endif /* MINION_SEARCH_NHCONFIG_H_ */
