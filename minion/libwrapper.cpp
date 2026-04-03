// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

/*
 * Functions for using minion as a library.
 */

#include "libwrapper.h"
#include "command_search.h"
#include "info_dumps.h"
#include "inputfile_parse/CSPSpec.h"
#include "minion.h"
#include "parallel/parallel.h"
#include "solver.h"
#include "system/minlib/exceptions.hpp"
#include "tuple_container.h"
#include <iomanip>
#include <memory>
#include <cstdlib>

#ifdef LIBMINION

// from minion.cpp
void doStandardSearch(CSPInstance& instance, SearchMethod args);
void finaliseModel(CSPInstance& instance);

extern thread_local Globals* globals;

// RAII guard: sets globals on construction, clears on destruction.
// If globals is already set to ctx (e.g. callback re-entry), this is a no-op.
// Asserts if globals is set to a *different* context (re-entrant call with wrong ctx).
struct ContextGuard {
  bool isOuterEntry;
  ContextGuard(MinionContext* ctx) {
    if(globals == nullptr) {
      globals = ctx;
      isOuterEntry = true;
    } else if(globals == ctx) {
      // Re-entrant call from callback with same context — allowed
      isOuterEntry = false;
    } else {
      // globals is set to a different context — this is a bug
      throw std::runtime_error("Re-entrant minion API call with different context");
    }
  }
  ~ContextGuard() {
    if(isOuterEntry) {
      globals = nullptr;
    }
  }
};

static void resetContextState(MinionContext* ctx)
{
  // Delete sub-objects to reset state for a fresh run, but keep the context alive
  delete ctx->bools_m;     ctx->bools_m = NULL;
  delete ctx->state_m;     ctx->state_m = NULL;
  delete ctx->queues_m;    ctx->queues_m = NULL;
  delete ctx->options_m;   ctx->options_m = NULL;
  delete ctx->varContainer_m; ctx->varContainer_m = NULL;
  delete ctx->searchMem_m; ctx->searchMem_m = NULL;
  delete ctx->tableOut_m;  ctx->tableOut_m = NULL;
  ctx->callback = NULL;
  ctx->callbackUserdata = NULL;
}

MinionContext* minion_newContext()
{
  return new MinionContext();
}

void minion_freeContext(MinionContext* ctx)
{
  delete ctx;
}

void minion_activateContext(MinionContext* ctx)
{
  assert(globals == nullptr && "Cannot activate context: another context is already active on this thread");
  globals = ctx;
}

void minion_deactivateContext()
{
  globals = nullptr;
}

ReturnCodes runMinion(MinionContext* ctx, SearchOptions& options, SearchMethod& args,
                      ProbSpec::CSPInstance& instance,
                      bool (*callback)(MinionContext* ctx, void* userdata),
                      void* userdata)
{
  ContextGuard guard(ctx);
  ReturnCodes returnCode = ReturnCodes::OK;

  /*
   * Adapted from minion_main.
   * Whereas minion_main takes in command line arguments, we take in Minion
   * objects.
   */

  resetContextState(ctx);

  // Redirect cout
  // https://stackoverflow.com/questions/49462524/controlling-output-from-external-libraries
  // https://stackoverflow.com/questions/4810516/c-redirecting-stdout

  streambuf* oldCoutStreamBuf = cout.rdbuf();
  ifstream logOutStream;
  time_t rawtime;
  time(&rawtime);

  // enable logging if LIBMINION_LOG is set
  if (std::getenv("LIBMINION_LOG")) {
    stringstream filenameStream;
    filenameStream << "minion";
    filenameStream << put_time(gmtime(&rawtime), "%Y-%m-%d-%H:%M:%S");
    filenameStream << ".log";

    logOutStream.open(filenameStream.str(), ios_base::app);
    cout.rdbuf(logOutStream.rdbuf());
  } else {
    // silence cout
    cout.rdbuf(NULL);
  }

  // Pass error codes across FFI boundaries, not exceptions.
  try {

    // No parallel minion in library usage for now
    //getParallelData();

    getState().getOldTimer().startClock();

    globals->callback = callback;
    globals->callbackUserdata = userdata;
    globals->options_m = new SearchOptions(options);
    globals->options_m->findAllSolutions();

    getOptions().printLine("# " + std::string(MinionVersion));
    getOptions().printLine("# Git version: \"" + tostring(GIT_VER) + "\"");

    GET_GLOBAL(global_random_gen).seed(args.randomSeed);
    if(!getOptions().silent) {
      cout << "#  Run at: UTC " << asctime(gmtime(&rawtime)) << endl;
      cout << "# Input filename: " << getOptions().instance_name << endl;
      getOptions().printLine("Using seed: " + tostring(args.randomSeed));
    }

    Parallel::setupAlarm(getOptions().timeoutActive, getOptions().time_limit,
                         getOptions().time_limit_is_CPUTime);

    finaliseModel(instance);

    // Output graphs, stats, or redump (will not return in these cases)
    infoDumps(instance);

    // Copy args into tableout
    getTableOut().set("RandomSeed", tostring(args.randomSeed));
    getTableOut().set("Preprocess", tostring(args.preprocess));

    getTableOut().set("MinionVersion", -1);
    getTableOut().set("TimeOut", 0); // will be set to 1 if a timeout occurs.
    getState().getOldTimer().maybePrintTimestepStore(cout, "Parsing Time: ", "ParsingTime",
                                                     getTableOut(), !getOptions().silent);

    SetupCSPOrdering(instance, args);
    BuildCSP(instance);

    // TODO (nd60): how to replace this??
    if(getOptions().commandlistIn != "") {
      doCommandSearch(instance, args);
    } else {
      doStandardSearch(instance, args);
    }

  }

  catch(const parse_exception& e) {
    cout << "Invalid instance: " << e.what() << endl;
    returnCode = ReturnCodes::INVALID_INSTANCE;
  } catch(const std::bad_alloc&) {
    returnCode = ReturnCodes::MEMORY_ERROR;
  } catch(...) {
    returnCode = ReturnCodes::UNKNOWN_ERROR;
  }

  // Detect timeout: doStandardSearch sets TableOut "TimeOut" to 1
  if(returnCode == ReturnCodes::OK && ctx->tableOut_m) {
    try {
      if(getTableOut().get("TimeOut") == "1")
        returnCode = ReturnCodes::TIMEOUT;
    } catch(...) {}
  }

  Parallel::endParallelMinion();

  // Restore old cout
  cout.rdbuf(oldCoutStreamBuf);

  // Don't reset context here - caller may still query results via
  // printMatrix_getValue or TableOut_get

  return returnCode;
}

/*********************************************************************/
/*                    Instance building functions                    */
/*********************************************************************/

void newVar(CSPInstance& instance, string name, VariableType type, vector<DomainInt> bounds)
{
  Var v = instance.vars.getNewVar(type, bounds);
  instance.vars.addSymbol(name, v);
  instance.allVars_list.push_back(makeVec(v));
}

Var constantAsVar(int constant)
{
  return Var(VAR_CONSTANT, (DomainInt)constant);
}

// Export of inline function get_constraint as bindings dont like inlines!
ConstraintDef* lib_getConstraint(ConstraintType t)
{
  return get_constraint(t);
}

/************************************************************/
/*                    Internal Functions                    */
/************************************************************/

void finaliseModel(CSPInstance& instance)
{
  /* Add final touches to model and fill in missing defaults.
   *
   * largely copied from MinionThreeInputReader::finalise, but without
   * gadget stuff.
   */

  // Fill in any missing defaults
  if(instance.searchOrder.empty()) {
    instance.searchOrder.push_back(instance.vars.getAllVars());
  }

  vector<Var> allVars = instance.vars.getAllVars();
  set<Var> unusedVars(allVars.begin(), allVars.end());
  for(SysInt i = 0; i < (SysInt)instance.searchOrder.size(); ++i) {
    const vector<Var>& vars_ref = instance.searchOrder[i].varOrder;
    for(vector<Var>::const_iterator it = vars_ref.begin(); it != vars_ref.end(); ++it) {
      unusedVars.erase(*it);
    }
  }

  for(SysInt i = 0; i < (SysInt)instance.searchOrder.size(); ++i)
    instance.searchOrder[i].setupValueOrder();

  if(instance.symOrder.empty())
    instance.symOrder = instance.vars.getAllVars();

  if(instance.symOrder.size() != instance.vars.getAllVars().size()) {
    // MAYBE_PARSER_INFO("Extending symmetry order with auxillery variables");
    vector<Var> allVars = instance.vars.getAllVars();
    for(typename vector<Var>::iterator i = allVars.begin(); i != allVars.end(); ++i) {
      if(find(instance.symOrder.begin(), instance.symOrder.end(), *i) == instance.symOrder.end())
        instance.symOrder.push_back(*i);
    }
  }

  //
  if(instance.symOrder.size() !=
     set<Var>(instance.symOrder.begin(), instance.symOrder.end()).size())
    throw parse_exception("SYMORDER cannot contain any variable more than once");

  if(instance.symOrder.size() != instance.vars.getAllVars().size())
    throw parse_exception("SYMORDER must contain every variable");
}

/*********************************************************************/
/*                    REXPORTING INLINE FUNCTIONS                    */
/*********************************************************************/

/***** Variable *****/

Var getVarByName(CSPInstance& instance, char* name)
{

  return instance.vars.getSymbol(string(name));
}

void newVar_ffi(CSPInstance& instance, char* name, VariableType type, int bound1, int bound2)
{
  newVar(instance, string(name), type, std::vector<DomainInt>({bound1, bound2}));
}

/***** Tuple *****/
TupleList* tupleList_new(vector<vector<DomainInt>>& tupleList)
{
  return new TupleList(tupleList);
}

void tupleList_free(TupleList* tupleList)
{
  delete tupleList;
}

/***** Instance *****/

CSPInstance* instance_new()
{
  return new CSPInstance();
}

void instance_free(CSPInstance* instance)
{
  delete instance;
}

void instance_addSearchOrder(CSPInstance& instance, SearchOrder& searchOrder)
{
  instance.searchOrder.push_back(searchOrder);
}

void instance_addConstraint(CSPInstance& instance, ConstraintBlob& constraint)
{
  instance.constraints.push_back(constraint);
}

bool instance_addConstraintMidsearch(MinionContext* ctx, CSPInstance& instance, ConstraintBlob& constraint)
{
  ContextGuard guard(ctx);
  // Keep a stable copy of the blob alive for the lifetime of `instance`.
  // Some built constraints may retain references to blob-owned argument storage.
  instance.constraints.push_back(constraint);

  AbstractConstraint* c = build_constraint(instance.constraints.back());
  return getState().addConstraintMidsearch(c);
}

void instance_addTupleTableSymbol(CSPInstance& instance, char* name, TupleList* tuplelist)
{
  instance.addTableSymbol(name, std::shared_ptr<TupleList>(tuplelist));
}

TupleList* instance_getTupleTableSymbol(CSPInstance& instance, char* name)
{
  return instance.getTableSymbol(name).get();
}

void instance_addShortTupleTableSymbol(CSPInstance& instance, char* name,
                                       ShortTupleList* shorttuplelist)
{
  instance.addShortTableSymbol(name, std::shared_ptr<ShortTupleList>(shorttuplelist));
}

ShortTupleList* instance_getShortTupleTableSymbol(CSPInstance& instance, char* name)
{
  return instance.getShortTableSymbol(name).get();
}

void printMatrix_addVar(CSPInstance& instance, Var var)
{
  instance.print_matrix.push_back({var});
}

int printMatrix_getValue(MinionContext* ctx, int idx)
{
  ContextGuard guard(ctx);
  return checked_cast<int>(globals->state_m->getPrintMatrix()[idx][0].assignedValue());
}

int printMatrix_getValueByName(MinionContext* ctx, CSPInstance& instance, const char* varname)
{
  ContextGuard guard(ctx);
  Var target = instance.vars.getSymbol(string(varname));
  for(SysInt i = 0; i < (SysInt)instance.print_matrix.size(); ++i) {
    if(instance.print_matrix[i].size() == 1 && instance.print_matrix[i][0] == target) {
      return checked_cast<int>(globals->state_m->getPrintMatrix()[i][0].assignedValue());
    }
  }
  throw parse_exception("Variable '" + string(varname) + "' not found in print matrix");
}

/***** SearchOptions *****/

SearchOptions* searchOptions_new()
{
  return new SearchOptions();
}

void searchOptions_free(SearchOptions* searchOptions)
{
  delete searchOptions;
}

/***** SearchMethod *****/

SearchMethod* searchMethod_new()
{
  return new SearchMethod();
}

void searchMethod_free(SearchMethod* searchMethod)
{
  delete searchMethod;
}

/***** SearchOrder *****/

SearchOrder* searchOrder_new(std::vector<Var>& vars, VarOrderEnum orderEnum, bool findOneSol)
{
  return new SearchOrder(vars, orderEnum, findOneSol);
}

void searchOrder_free(SearchOrder* searchOrder)
{
  delete searchOrder;
}

/***** ConstraintBlob *****/

ConstraintBlob* constraint_new(ConstraintType constraint_type)
{
  return new ConstraintBlob(lib_getConstraint(constraint_type));
}

void constraint_free(ConstraintBlob* constraint)
{
  delete constraint;
}

// mirrors MinionThreeInputReader::readGeneralConstraint, but over FFI.
// look there for the why/how

void constraint_addList(ConstraintBlob& constraint, std::vector<Var>& vars)
{
  constraint.vars.push_back(vars);
}

void constraint_addVar(ConstraintBlob& constraint, Var& var)
{
  constraint.vars.push_back(makeVec(var));
}

void constraint_addTwoVars(ConstraintBlob& constraint, Var& var1, Var& var2)
{
  vector<Var> vars(2);
  vars[0] = std::move(var1);
  vars[1] = std::move(var2);
  constraint.vars.push_back(std::move(vars));
}

void constraint_addConstant(ConstraintBlob& constraint, int constant)
{
  constraint.constants.push_back(makeVec((DomainInt)constant));
}

void constraint_addConstantList(ConstraintBlob& constraint, std::vector<DomainInt>& constants)
{
  constraint.constants.push_back(constants);
}

void constraint_addConstraint(ConstraintBlob& constraint, ConstraintBlob& internal_constraint)
{
  constraint.internal_constraints.push_back(internal_constraint);
}

void constraint_addConstraintList(ConstraintBlob& constraint,
                                  vector<ConstraintBlob>& internal_constraints)
{
  constraint.internal_constraints = std::move(internal_constraints);
}

void constraint_setTuples(ConstraintBlob& constraint, TupleList* tupleList)
{
  constraint.tuples = std::shared_ptr<TupleList>(tupleList);
}

/***** Vector Rexports *****/

std::vector<Var>* vec_var_new()
{
  return new std::vector<Var>();
}

void vec_var_push_back(std::vector<Var>* vec, Var var)
{
  vec->push_back(var);
}

void vec_var_free(std::vector<Var>* vec)
{
  delete vec;
}

std::vector<DomainInt>* vec_int_new()
{
  return new std::vector<DomainInt>();
}

void vec_int_push_back(std::vector<DomainInt>* vec, int n)
{
  vec->push_back(n);
}

void vec_int_free(std::vector<DomainInt>* vec)
{
  delete vec;
}

std::vector<ConstraintBlob>* vec_constraints_new()
{
  return new std::vector<ConstraintBlob>();
}

void vec_constraints_push_back(std::vector<ConstraintBlob>* vec, ConstraintBlob& constraint)
{
  // TODO: how to memory manage this?
  // move?
  vec->push_back(std::move(constraint));
}

void vec_constraints_free(std::vector<ConstraintBlob>* vec)
{
  delete vec;
}

std::vector<std::vector<DomainInt>>* vec_vec_int_new()
{
  return new std::vector<std::vector<DomainInt>>();
}
void vec_vec_int_push_back(std::vector<std::vector<DomainInt>>* vec,
                           std::vector<DomainInt> new_elem)
{
  vec->push_back(new_elem);
}

void vec_vec_int_push_back_ptr(std::vector<std::vector<DomainInt>>* vec,
                               std::vector<DomainInt>* new_elem)
{
  vec->push_back(*new_elem);
}

void vec_vec_int_free(std::vector<std::vector<DomainInt>>* vec)
{
  delete vec;
}

char* TableOut_get(MinionContext* ctx, char* key) {
  ContextGuard guard(ctx);
  try {
    /*
     * .data() doesn't copy, it just returns a ptr to the internal
     * representation of the string . As we are interfacing with C and using 
     * char*, I will just malloc strcpy here (even though it might not be 
     * idiomatic C++?)
     *
     * It needs this many temporary variables due to memory shenanigans!
     */

    string val_str = getTableOut().get(key);
    const char* val = val_str.data();

    char* heaped_val = (char*) std::malloc(strlen(val) +1);
    strcpy(heaped_val,val);
    return heaped_val;

  } catch(const std::out_of_range&) {
    return NULL;
  }
}

#endif

// vim: cc=80 tw=80
