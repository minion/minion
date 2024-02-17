// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0
#include "ConstraintEnum.h"
#include "inputfile_parse/CSPSpec.h"
#include "minion.h"
#include "solver.h"
#include "tuple_container.h"

/*
 * Functions for using minion as a library.
 *
 * Primarily for use by the Rust bindings - the library abstractions provided
 * here are neither complete nor safe.
 *
 * The Rust bindings (https://github.com/conjure-cp/conjure-oxide/tree/main/solvers/minion)
 * should provide a safe but low-level way to use Minion as a library.
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

void resetMinion();

enum ReturnCodes
{
  OK,
  INVALID_INSTANCE,
  UNKNOWN_ERROR = 255
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
ReturnCodes runMinion(SearchOptions& options, SearchMethod& args, ProbSpec::CSPInstance& instance,
                      bool (*callback)(void));

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
Var constantAsVar(int n);

/*******************************************************/
/*                    FFI FUNCTIONS                    */
/*******************************************************/

/*
 * The below functions are primarily for use by FFI wrappers of Minion.
 *
 * This means that:
 *   - Complex structures that other languages do not understand are returned as
 *     opaque pointers.
 *
 *   - Use of char* instead of string.
 *
 *   - Various inline functions are re-exported so they can be seen.
 *
 *   - Manual constructors and deconstructors are provided for languages that
 *     do not use them.
 */

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

/***** Variable *****/
Var getVarByName(CSPInstance& instance, char* name);
void newVar_ffi(CSPInstance& instance, char* name, VariableType type, int bound1, int bound2);

/***** Tuple *****/
TupleList* tupleList_new(vector<vector<DomainInt>>& tupleList);
void tupleList_free(TupleList* tupleList);

/***** Instance *****/
CSPInstance* instance_new();
void instance_free(CSPInstance* instance);

void instance_addSearchOrder(CSPInstance& instance, SearchOrder& searchOrder);
void instance_addConstraint(CSPInstance& instance, ConstraintBlob& constraint);
void instance_addTupleTableSymbol(CSPInstance& instance, char* name, TupleList* tuplelist);
TupleList* instance_getTupleTableSymbol(CSPInstance& instance, char* name);

/*
 * printMatrix_* functions assume the print matrix is of the form
 * [[x],[y],[z],...].
 */
void printMatrix_addVar(CSPInstance& instance, Var var);

// Should be called only when minion has results in a callback.
int printMatrix_getValue(int idx);

/***** Constraint argument parse types ****/

// as per MinionThreeInput.h and build/src/Constraintdefs.h.

ConstraintBlob* constraint_new(ConstraintType contraint_type);
void constraint_free(ConstraintBlob* constraint);

ConstraintDef* lib_getConstraint(ConstraintType t);
void constraint_addList(ConstraintBlob& constraint, std::vector<Var>& vars);
void constraint_addVar(ConstraintBlob& constraint, Var& var);
void constraint_addTwoVars(ConstraintBlob& constraint, Var& var1, Var& var2);

void constraint_addConstant(ConstraintBlob& constraint, int constant);
void constraint_addConstantList(ConstraintBlob& constraint, std::vector<DomainInt>& constants);

void constraint_addConstraint(ConstraintBlob& constraint, ConstraintBlob& internal_constraint);
void constraint_addConstraintList(ConstraintBlob& constraint,
                                  vector<ConstraintBlob>& internal_constraints);

// Note that adding tuples to the symbol table is optional, but useful!
// A constraint only has one tuple list.
void constraint_setTuples(ConstraintBlob& constraint, TupleList* tupleList);

/***** Small misc useful types *****/

SearchOptions* searchOptions_new();
SearchMethod* searchMethod_new();
SearchOrder* searchOrder_new(std::vector<Var>& vars, VarOrderEnum orderEnum, bool findOneSol);

void searchOptions_free(SearchOptions* searchOptions);
void searchMethod_free(SearchMethod* searchMethod);
void searchOrder_free(SearchOrder* searchOrder);

/***** std::vector Wrappers *****/

std::vector<Var>* vec_var_new();
void vec_var_push_back(std::vector<Var>* vec, Var var);
void vec_var_free(std::vector<Var>* vec);

std::vector<DomainInt>* vec_int_new();
void vec_int_push_back(std::vector<DomainInt>* vec, int n);
void vec_int_free(std::vector<DomainInt>* vec);

std::vector<ConstraintBlob>* vec_constraints_new();
void vec_constraints_push_back(std::vector<ConstraintBlob>* vec, ConstraintBlob& constraint);
void vec_constraints_free(std::vector<ConstraintBlob>* vec);

std::vector<std::vector<DomainInt>>* vec_vec_int_new();
void vec_vec_int_push_back(std::vector<std::vector<DomainInt>>* vec,
                           std::vector<DomainInt> new_elem);
void vec_vec_int_free(std::vector<std::vector<DomainInt>>* vec);

#endif

// vim: cc=80 tw=80
