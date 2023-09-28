// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef _BUILDCSP_H
#define _BUILDCSP_H

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#endif

struct SearchMethod {
  VarOrderEnum order;
  ValOrder valorder;
  unsigned int limit; // for static limited.
  PropagationLevel preprocess;
  PropagationLevel propMethod;
  UnsignedSysInt randomSeed;
  SearchMethod()
      : order(ORDER_NONE),
        valorder(VALORDER_NONE),
        preprocess(PropLevel_None),
        propMethod(PropLevel_GAC),
        randomSeed(std::random_device{}()) {}
};

void SetupCSPOrdering(CSPInstance& instance, SearchMethod args);
void BuildCSP(ProbSpec::CSPInstance& instance);
bool PreprocessCSP(ProbSpec::CSPInstance& instance, SearchMethod args);
void SolveCSP(ProbSpec::CSPInstance& instance, SearchMethod args);

#endif
