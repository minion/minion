// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0


#include "ConstraintEnum.h"
#include "inputfile_parse/CSPSpec.h"
#include "minion.h"
#include "solver.h"
/*
 * Functions for using minion as a library.
 *
 * Primarily for use by the Rust bindings - the library abstractions provided
 * here are neither complete nor safe.
 *
 * The Rust bindings (github.com/conjure-cp/conjure-oxide) should provide a safe
 * but low-level way to use Minion as a library.
 */

/* MODEL BUILDING - AN EXAMPLE
 * ===========================
 *
 * This encodes the following minion file into a C++ minion model:
 *
 * MINION 3
 * **VARIABLES**
 * DISCRETE x #
 * {1..3}
 * DISCRETE y #
 * {2..4}
 * DISCRETE z #
 * {1..5}
 * **SEARCH**
 * PRINT[[x],[y],[z]]
 * VARORDER STATIC [x, y, z]
 * **CONSTRAINTS**
 * sumleq([x,y,z],4)
 * ineq(x, y, -1)
 * **EOF**
 *
 *  
 * CSPInstance instance;
 * 
 * std::vector<DomainInt> domainx = {1,3};
 * std::vector<DomainInt> domainy = {2,4};
 * std::vector<DomainInt> domainz = {1,5};
 * 
 * // **VARIABLES**
 * newVar(instance,"x",VAR_DISCRETE,domainx);
 * newVar(instance,"y",VAR_DISCRETE,domainy);
 * newVar(instance,"z",VAR_DISCRETE,domainz);
 *
 * Var x = instance.vars.getSymbol("x");
 * Var y = instance.vars.getSymbol("y");
 * Var z = instance.vars.getSymbol("z");
 *
 * // **SEARCH**
 * 
 * // PRINT
 * instance.print_matrix.push_back({x});
 * instance.print_matrix.push_back({y});
 * instance.print_matrix.push_back({z});
 *
 *
 * // VARORDER STATIC [x,y,z]
 * bool find_one_sol = false;
 * SearchOrder searchOrder({x,y,z}, VarOrderEnum::ORDER_STATIC,find_one_sol);
 * instance.searchOrder.push_back(searchOrder);


 * // **CONSTRAINTS**
 * 
 * ConstraintBlob leq(lib_getConstraint(ConstraintType::CT_LEQSUM));
 * ConstraintBlob geq(lib_getConstraint(ConstraintType::CT_GEQSUM));
 * ConstraintBlob ineq(lib_getConstraint(ConstraintType::CT_INEQ));

 * 
 * // leq: [var] var
 * leq.vars.push_back({x,y,z});
 * leq.vars.push_back({constantAsVar(4)});
 *                     
 * geq.vars.push_back({x,y,z});
 * geq.vars.push_back({constantAsVar(4)});

 * // ineq: var var const
 * ineq.vars.push_back({x});
 * ineq.vars.push_back({y});
 * ineq.constants.push_back({-1});

 * instance.constraints.push_back(geq);
 * instance.constraints.push_back(leq);
 * instance.constraints.push_back(ineq);
 */

/* ADDING CONSTRAINTS
 * ==================
 *
 * You need to know the types of the arguments to the constraints being used 
 * and add things to the right vectors, or Minion will segfault.
 *
 * (types can be found in bin/src/build_constraints.h).
 
 * Note the different vectors used to add different arguments based on type in
 * the above example.
 *
 * Also note that a constant can be used in place of a variable by using
 * constantAsVar.
 */

/* ADDING VARIABLES
 * ===============
 *
 * Use newVar provided by this header file.
 * Variables added this way are in the symbol table and can be referred to by
 * using: instance.vars.getSymbol("x");
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
#include <vector>
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
ReturnCodes runMinion(SearchOptions& options, SearchMethod& args, ProbSpec::CSPInstance& instance, bool(*callback)(void));


/* 
 * void newVar:
 *  
 *  Add a named variable to `instance`.
 *
 *  Once created, the variable can be accessed by calling 
 *  `instance.vars.getSymbol(name)`.
 *
 * Throws parse_exception if the variable is invalid, or the name is already
 * used.
 */
void newVar(CSPInstance& instance, string name, VariableType type, vector<DomainInt> bounds);

/* 
 * Var constantAsVar(DomainInt constant):
 *   
 *   Returns a constant as an anomynous variable, for use in constraints which
 *   take variables as arguments.
 *
 *   For example, the SUMLEQ constraint has arguments of types [Var],Var.
 *
 *   Despite this, we can encode SUMLEQ([x,y,z],4] using constantAsVar.
 *
 *   ```c++
 *    ...
 *
 *    ConstraintBlob leq(lib_getConstraint(ConstraintType::CT_LEQSUM));
 *
 *    Var x = instance.vars.getSymbol("x");
 *    Var y = instance.vars.getSymbol("y");
 *    Var z = instance.vars.getSymbol("z");
 *    
 *    leq.vars.push_back({x,y,z});
 *    leq.vars.push_back(constantAsVar(4));
 *
 *    ....
 *
 *    Unlike variables created using newVar, these do not appear anywhere else
 *    in the instance (such as the symbol table, or allVars).
 */
Var constantAsVar(DomainInt n);



/*******************************************************/
/*                    FFI FUNCTIONS                    */
/*******************************************************/
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

SearchOptions* newSearchOptions();
SearchMethod* newSearchMethod();
CSPInstance* newInstance();
ConstraintBlob* newConstraintBlob(ConstraintType contraint_type);
SearchOrder* newSearchOrder(std::vector<Var>& vars,VarOrderEnum orderEnum, bool findOneSol);

void searchOptions_free(SearchOptions* searchOptions);
void searchMethod_free(SearchMethod* searchMethod);
void instance_free(CSPInstance* instance);
void constraint_free(ConstraintBlob* constraint);
void searchOrder_free(SearchOrder* searchOrder);

Var getVarByName(CSPInstance& instance, char* name);
void newVar_ffi(CSPInstance& instance, char* name, VariableType type, int bound1, int bound2);

void instance_addSearchOrder(CSPInstance& instance, SearchOrder& searchOrder);
void instance_addConstraint(CSPInstance& instance, ConstraintBlob& constraint);

// Both these assume print matrix of form [[x],[y],[z]]
void printMatrix_addVar(CSPInstance& instance, Var var);

// Should be called only when minion has results in a callback
int printMatrix_getValue(int idx);

void constraint_addVarList(ConstraintBlob& constraint, std::vector<Var>& vars);
void constraint_addConstantList(ConstraintBlob& constraint, std::vector<DomainInt>& constants);

std::vector<Var>* vec_var_new();
void vec_var_push_back(std::vector<Var>* vec, Var var);
void vec_var_free(std::vector<Var>* vec);

std::vector<DomainInt>* vec_int_new();
void vec_int_push_back(std::vector<DomainInt>* vec, int n);
void vec_int_free(std::vector<DomainInt>* vec);

#endif

// vim: cc=80 tw=80
