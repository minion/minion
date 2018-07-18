
#ifndef MINION_SEARCH_NHCONFIG_H_
#define MINION_SEARCH_NHCONFIG_H_

#include <memory>

struct NhConfig {
  // values here are not necessarily optimal (tuned)
  // they are defaults until tuning info can be given
  // can be overridden by commandline

  bool backtrackInsteadOfTimeLimit = true;
  int iterationSearchTime = 500;
  double initialSearchBacktrackLimitMultiplier =
      2.0; // NGUYEN: test (2.0 is the best one return by
           // tuning-initialise-only), original value: 1.5
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
  double ucbExplorationBias = 2;
  double learningAutomatonRate = 0.1;
  int holePuncherSolutionBagSizeConstant = 5;
};

std::shared_ptr<NhConfig> makeNhConfig() {
  return make_shared<NhConfig>();
}

inline std::ostream& operator<<(std::ostream& os, const NhConfig& config) {
  os << "NhConfig {";
  if(config.backtrackInsteadOfTimeLimit) {
    os << "Using backtracks,\n";
  } else {
    os << "Using timelimit,\n";
  }
  os << "search Backtrack limit multiplier:" << config.initialSearchBacktrackLimitMultiplier
     << ",\n";
  os << "hill climber Backtrack limit multiplier:" << config.backtrackLimitMultiplier << ",\n";
  os << "hill climber Backtrack limit increment:" << config.backtrackLimitIncrement << ",\n";
  os << "hole puncher Backtrack limit multiplier:" << config.holePuncherBacktrackLimitMultiplier
     << ",\n";
  os << "hill climber Increase backtrack only on failure: " << config.increaseBacktrackOnlyOnFailure
     << ",\n";
  os << "iterationSearchTime:" << config.iterationSearchTime << ",\n";
  os << "hillClimberMinIterationsToSpendAtPeak: " << config.hillClimberMinIterationsToSpendAtPeak
     << ",\n";
  os << "hillClimberInitialLocalMaxProbability : " << config.hillClimberInitialLocalMaxProbability
     << ",\n";
  os << "hillClimberProbabilityIncrementMultiplier: "
     << config.hillClimberProbabilityIncrementMultiplier << "\n}";
  return os;
}

#endif /* MINION_SEARCH_NHCONFIG_H_ */
