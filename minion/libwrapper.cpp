// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

/*
 * Functions for using minion as a library.
 */

#include "libwrapper.h"
#include "minion.h"
#include "command_search.h"
#include "info_dumps.h"
#include "system/minlib/exceptions.hpp"
#include <iomanip>

#ifdef LIBMINION

// from minion.cpp
void doStandardSearch(CSPInstance& instance, SearchMethod args);
void finaliseModel(CSPInstance& instance);


extern Globals* globals;

void resetMinion() {
  delete globals;
  globals = new Globals();
}

ReturnCodes runMinion(SearchOptions& options, SearchMethod& args, ProbSpec::CSPInstance& instance) {

  ReturnCodes returnCode = ReturnCodes::OK;

  /*
   * Adapted from minion_main.
   * Whereas minion_main takes in command line arguments, we take in Minion
   * objects.
   */

  resetMinion();


  // Redirect cout
  // https://stackoverflow.com/questions/49462524/controlling-output-from-external-libraries
  // https://stackoverflow.com/questions/4810516/c-redirecting-stdout

  streambuf* oldCoutStreamBuf = cout.rdbuf();
  ifstream logOutStream;
  time_t rawtime;
  time(&rawtime);
  char time[30];

  stringstream filenameStream;
  filenameStream << "minion";
  filenameStream << put_time(gmtime(&rawtime), "%Y-%m-%d-%H:%M:%S");
  filenameStream << ".log";

  logOutStream.open(filenameStream.str(),ios_base::app);
  
  cout.rdbuf(logOutStream.rdbuf());

  // Pass error codes across FFI boundaries, not exceptions.
  try {

    // No parallel minion in library usage for now
    // getParallelData();

    getState().getOldTimer().startClock();

    globals->options_m = new SearchOptions(options);

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

    //TODO (nd60): how to replace this??
    if(getOptions().commandlistIn != "") {
       doCommandSearch(instance, args);
     } else {
       doStandardSearch(instance, args);
     }
  }

  catch(parse_exception e) {
    cout << "Invalid instance: " << e.what() << endl;
    returnCode =  ReturnCodes::INVALID_INSTANCE;
  }
  catch(...){
    returnCode =  ReturnCodes::UNKNOWN_ERROR;
  }


  // Restore old cout
  cout.rdbuf( oldCoutStreamBuf );
  
  return returnCode;
}

/*********************************************************************/
/*                    Instance building functions                    */
/*********************************************************************/

void newVar(CSPInstance& instance, string name, VariableType type, vector<DomainInt> bounds) {
  Var v = instance.vars.getNewVar(VAR_DISCRETE, bounds);
  instance.vars.addSymbol(name, v);
  instance.allVars_list.push_back(makeVec(v));
}


Var constantAsVar(DomainInt constant) {
  return Var(VAR_CONSTANT,constant);
}


// Export of inline function get_constraint as bindings dont like inlines!
ConstraintDef* lib_getConstraint(ConstraintType t) {
  return get_constraint(t);
}

/************************************************************/
/*                    Internal Functions                    */
/************************************************************/
void finaliseModel(CSPInstance& instance) {
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
    //MAYBE_PARSER_INFO("Extending symmetry order with auxillery variables");
    vector<Var> allVars = instance.vars.getAllVars();
    for(typename vector<Var>::iterator i = allVars.begin(); i != allVars.end(); ++i) {
      if(find(instance.symOrder.begin(), instance.symOrder.end(), *i) ==
         instance.symOrder.end())
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


#endif


// vim: cc=80 tw=80
