// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#include "minion.h"

#include "dump_state.hpp"

#include "BuildVariables.h"

#include "commandline_parse.h"
#include "inputfile_parse/inputfile_parse.h"

#include "info_dumps.h"

#include "command_search.h"

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Entrance:
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void help();

void print_default_help(char** argv) {
  cout << "Type '" << argv[0] << " help' for usage." << endl;
  cout << endl << "Usage: " << argv[0] << " {switch}* [input file]" << endl;
  cout << endl;
  cout << "This version of Minion was built with internal checking "
       <<
#ifdef NO_DEBUG
      "off";
#else
      "on";
#endif

#ifdef WDEG
  cout << " and wdeg on";
#endif

  cout << endl;
}

void doStandardSearch(CSPInstance& instance, SearchMethod args) {
  bool preprocess = PreprocessCSP(instance, args);

  getState().getOldTimer().maybePrintTimestepStore(cout, "Preprocess Time: ", "PreprocessTime",
                                                   getTableOut(), !getOptions().silent);

  if(getOptions().outputCompressed != "" || getOptions().outputCompressedDomains)
    dumpSolver(getOptions().outputCompressed, getOptions().outputCompressedDomains);

  // This has to happen here, so the dumpSolver knows if the solver failed or not
  if(!preprocess) {
    getState().setFailed(true);
  }

  SolveCSP(instance, args);

  getState().getOldTimer().maybePrintFinaltimestepStore(cout, "Solve Time: ", "SolveTime",
                                                        getTableOut(), !getOptions().silent);
  getOptions().printLine("Total Nodes: " + tostring(getState().getNodeCount()));

  getOptions().printLine("Solutions Found: " + tostring(getState().getSolutionCount()));

  getTableOut().set("Nodes", tostring(getState().getNodeCount()));
  getTableOut().set("Satisfiable", (getState().getSolutionCount() == 0 ? 0 : 1));
  getTableOut().set("SolutionsFound", getState().getSolutionCount());

  if(getOptions().tableout && !Parallel::isAChildProcess()) {
    getTableOut().print_line(); // Outputs a line to the table file.
  }

#ifdef MORE_SEARCH_INFO
  if(!getOptions().silent)
    printSearchInfo();
#endif
}

int minion_main(int argc, char** argv) {
  // Wrap main in a try/catch just to stop exceptions leaving main,
  // as windows gets really annoyed when that happens.
  try {

    // Force parallel data to be created
    getParallelData();

    getState().getOldTimer().startClock();

    getOptions().printLine("# " + std::string(MinionVersion));
    getOptions().printLine("# Git version: \"" + tostring(GIT_VER) + "\"");

    if(argc == 1) {
      print_default_help(argv);
      return EXIT_SUCCESS;
    }

    if(argv[1] == string("help") || argv[1] == string("--help") || argv[1] == string("-help") ||
       argv[1] == string("-h")) {
      help();
      return EXIT_SUCCESS;
    }
  
    CSPInstance instance;
    SearchMethod args;

    parseCommandLine(args, argc, argv);

    GET_GLOBAL(global_random_gen).seed(args.randomSeed);

    if(!getOptions().silent) {
      time_t rawtime;
      time(&rawtime);
      cout << "#  Run at: UTC " << asctime(gmtime(&rawtime)) << endl;
      cout << "# Input filename: " << getOptions().instance_name << endl;
      cout << "# Command line: ";
      for(SysInt i = 0; i < argc; ++i) {
        cout << argv[i] << " ";
      }
      cout << endl;
      getOptions().printLine("Using seed: " + tostring(args.randomSeed));
    }

    Parallel::setupAlarm(getOptions().timeoutActive, getOptions().time_limit,
                         getOptions().time_limit_is_CPUTime);

    vector<string> files(1, getOptions().instance_name);
    readInputFromFiles(instance, files, getOptions().parserVerbose, getOptions().map_long_short,
                       getOptions().ensureBranchOnAllVars);

    // Output graphs, stats, or redump (will not return in these cases)
    infoDumps(instance);

    // Copy args into tableout
    getTableOut().set("RandomSeed", tostring(args.randomSeed));
    getTableOut().set("Preprocess", tostring(args.preprocess));
    // should be one for varorder as well.
    getTableOut().set("MinionVersion", -1);
    getTableOut().set("TimeOut", 0); // will be set to 1 if a timeout occurs.
    getState().getOldTimer().maybePrintTimestepStore(cout, "Parsing Time: ", "ParsingTime",
                                                     getTableOut(), !getOptions().silent);

    SetupCSPOrdering(instance, args);
    BuildCSP(instance);

    if(getOptions().commandlistIn != "") {
      doCommandSearch(instance, args);
    } else {
      doStandardSearch(instance, args);
    }

    return 0;

  } catch(...) {
    cerr << "Minion exited abnormally via an exception." << endl;
    exit(9);
  }
}

