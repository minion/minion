/*
 *  BuildCSP.h
 *  Minion
 *
 */

#ifndef _BUILDCSP_H
#define _BUILDCSP_H

//#include "solver.h"
#include "preprocess.h"
#include "CSPSpec.h"



void BuildCSP(StateObj* stateObj, ProbSpec::CSPInstance& instance);
void SolveCSP(StateObj* stateObj, ProbSpec::CSPInstance& instance, MinionArguments args);

#endif
