// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

/*
 * Functions for using minion as a library.
 */

#ifndef _WRAPPER_H
#define _WRAPPER_H

/*
 * void resetMinion:
 *
 *  Reset all global variables in minion to their initial values.
 *
 */
#include "minion.h"
void resetMinion();

enum ReturnCodes {
  OK,
  INVALID_INSTANCE,
  UNKNOWN_ERROR=255
};

/*
 * int runMinion:
 *
 *   Run Minion.
 *
 *   Minion is reset on each invokation of this function, so it is safe to call
 *   sequentially.
 *
 *   An error code is returned. These are described in ReturnCodes.
 */
ReturnCodes runMinion(SearchOptions& options, SearchMethod& args, ProbSpec::CSPInstance& instance);


/* 
 * void newVar:
 *  
 *  Add a variable to `instance`.
 *
 *  Once created, the variable can be accessed by calling 
 *  `instance.vars.getSymbol(name)`.
 *
 * Throws parse_exception if the variable is invalid, or the name is already
 * used.
 */
void newVar(CSPInstance& instance, string name, VariableType type, vector<DomainInt> bounds);

/*
 * ConstraintDef* lib_getConstraint:
 *  
 *  Get the constraint definition for the given ConstraintType.
 *  This is mainly required to define a ConstraintBlob for a model.
 *
 *  Example:
 *    for the constraint: x + y <= z:
 *
 *    ```
 *    ConstraintBlob leq(lib_getConstraint(ConstraintType::CT_GEQSUM));

 *    geq.vars.push_back({instance.vars.getSymbol("x"),instance.vars.getSymbol("y")});
 *    geq.vars.push_back({instance.vars.getSymbol("z")});
 *    instance.constraints.push_back(leq);
 *    ```
 */
ConstraintDef* lib_getConstraint(ConstraintType t);


#endif

// vim: cc=80 tw=80
