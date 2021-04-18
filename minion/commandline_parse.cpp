/*
 * Minion http://minion.sourceforge.net
 * Copyright (C) 2006-09
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */









/* @help switches;-varorder Example
To use smallest domain first ordering (probably the most sensible of
the available orderings) do:

   minion -varorder sdf myinput.minion
*/



#include "commandline_parse.h"
#include "search_dump.hpp"

#define INCREMENT_i(flag)                                                                          \
  {                                                                                                \
    ++i;                                                                                           \
    if(i == argc || argv[i][0] == '-') {                                                           \
      cerr << #flag << " requires a value\n";                                                      \
      exit(1);                                                                                     \
    }                                                                                              \
  }

void parseCommandLine(SearchMethod& args, SysInt argc, char** argv) {
  for(SysInt i = 1; i < argc; ++i) {
    const string command(argv[i]);
    if(command == string("-findallsols")) {
      getOptions().findAllSolutions();
    }



    else if(command == string("-quiet")) {
      getOptions().parserVerbose = false;
    }

    else if(command == string("-redump")) {
      getOptions().redump = true;
    }

    else if(command == string("-outputCompressedDomains")) {
      getOptions().outputCompressedDomains = true;
    }



    else if(command == string("-outputCompressed")) {
      INCREMENT_i(-outputCompressed);
      getOptions().outputCompressed = argv[i];
    } else if(command == string("-instancestats") || command == string("-X-instancestats")) {
      getOptions().instance_stats = true;
    } else if(command == string("-Xgraph")) {
      getOptions().graph = true;
      getOptions().silent = true;
    }

    else if(command == string("-printsols")) {
      getOptions().print_solution = true;
    }

    else if(command == string("-noprintsols")) {
      getOptions().print_solution = false;
    }

    else if(command == string("-printsolsonly")) {
      getOptions().silent = true;
    }

    else if(command == string("-printonlyoptimal")) {
      getOptions().printonlyoptimal = true;
    }



    else if(command == string("-verbose")) {
      getOptions().parserVerbose = true;
    }





    else if(command == string("-prop-node") || command == string("-X-prop-node")) {
      INCREMENT_i(-X - prop - node);
      string prop_mode(argv[i]);
      args.propMethod = GetPropMethodFromString(prop_mode);
      if(args.propMethod.type == PropLevel_None) {
        outputFatalError("Cannot use 'None' for -prop-node, must propagate at each node!");
      }
    }

    else if(command == string("-map-long-short")) {
      INCREMENT_i(-X - prop - node);
      string prop_mode(argv[i]);
      if(prop_mode == "none")
        getOptions().map_long_short = MLTTS_NoMap;
      else if(prop_mode == "keeplong")
        getOptions().map_long_short = MLTTS_KeepLong;
      else if(prop_mode == "eager")
        getOptions().map_long_short = MLTTS_Eager;
      else if(prop_mode == "lazy")
        getOptions().map_long_short = MLTTS_Lazy;
      else {
        outputFatalError(" -X-map-long-short <none|keeplong|eager|lazy>");
      }
    }





    else if(command == string("-preprocess")) {
      INCREMENT_i(-preprocess);
      string prop_mode(argv[i]);
      args.preprocess = GetPropMethodFromString(prop_mode);
    }



    else if(command == string("-nocheck")) {
      getOptions().nocheck = true;
    }



    else if(command == string("-check")) {
      getOptions().nocheck = false;
    }

    else if(command == string("-dumptree")) {
      getOptions().dumptree = true;
    }

    else if(command == string("-dumptreejson")) {
      INCREMENT_i(-dumptreejson);
      if(getOptions().dumptree) {
        outputFatalError("Only one tree dumper active at once");
      }
      std::ostream* outfile = new std::ofstream(argv[i]);
      if(!outfile || !(*outfile)) {
        outputFatalError("Could not open '" + std::string(argv[i]) + "' for writing");
      }
      getOptions().dumptreeobj = makeDumpTreeJson(outfile);
    } else if(command == string("-dumptreesql")) {
      if(getOptions().dumptree) {
        outputFatalError("Only one tree dumper active at once");
      }
      getOptions().dumptreeobj = makeDumpTreeSQL();
    }



    else if(command == string("-nodelimit")) {
      INCREMENT_i(-nodelimit);
      try {
        getOptions().nodelimit = fromstring<long long int>(argv[i]);
        if(getOptions().nodelimit < 0)
          throw "Invalid lower bound";
      } catch(...) {
        cout << "Did not understand parameter to nodelimit:" << argv[i] << endl;
        exit(1);
      }
    }



    else if(command == string("-sollimit")) {
      INCREMENT_i(-sollimit);
      try {
        getOptions().sollimit = fromstring<SysInt>(argv[i]);
        if(getOptions().sollimit <= 0)
          throw "Invalid lower bound";
      } catch(...) {
        cout << "Did not understand the parameter to sollimit:" << argv[i] << endl;
        exit(1);
      }
    }



    else if(command == string("-timelimit")) {
      INCREMENT_i(-timelimit);
      if(getOptions().timeoutActive) {
        cout << "Only one '-cpulimit' or '-timelimit' per instance" << endl;
        exit(1);
      }
      getOptions().timeoutActive = true;
      try {
        getOptions().time_limit = fromstring<SysInt>(argv[i]);
        getOptions().time_limit_is_CPUTime = false;
      } catch(...) {
        cout << "Did not understand the parameter to timelimit:" << argv[i] << endl;
        exit(1);
      }
    }

    else if(command == string("-skipautoaux")) {
      cout << "# WARNING: -skipautoaux can lead to incorrect solutions being "
              "produced\n";
      cout << "# WARNING: This is by design, but use this option with extreme "
              "care\n";
      getOptions().ensureBranchOnAllVars = false;
    }



    else if(command == string("-cpulimit")) {
      INCREMENT_i(-cpulimit);
      if(getOptions().timeoutActive) {
        cout << "Only one '-cpulimit', or '-timelimit' per instance" << endl;
        exit(1);
      }
      getOptions().timeoutActive = true;
      try {
        getOptions().time_limit = fromstring<SysInt>(argv[i]);
        getOptions().time_limit_is_CPUTime = true;
      } catch(...) {
        cout << "Did not understand the parameter to cpulimit:" << argv[i] << endl;
        exit(1);
      }
    } // TODO : Should remove -varorder for beta orderings.
    else if(command == string("-varorder")) {
      INCREMENT_i(-varorder);

      string order(argv[i]);

      if(order == "static")
        args.order = ORDER_STATIC;
      else if(order == "srf")
        args.order = ORDER_SRF;
      else if(order == "staticlimited") {
        args.order = ORDER_STATIC_LIMITED;
        INCREMENT_i(staticlimited);
        // Parse an integer for the limit.
        unsigned int tmp;
        std::istringstream iss(argv[i]);
        if(!(iss >> tmp)) {
          outputFatalError("-varorder staticlimited requires a positive integer value");
        }
        args.limit = tmp;
      } else if(order == "srf-random") {
        args.order = ORDER_SRF;
        getOptions().randomiseValvarorder = true;
      } else if(order == "sdf")
        args.order = ORDER_SDF;
      else if(order == "sdf-random") {
        args.order = ORDER_SDF;
        getOptions().randomiseValvarorder = true;
      } else if(order == "ldf")
        args.order = ORDER_LDF;
      else if(order == "ldf-random") {
        args.order = ORDER_LDF;
        getOptions().randomiseValvarorder = true;
      } else if(order == "random")
        getOptions().randomiseValvarorder = true;
      else if(order == "conflict")
        args.order = ORDER_CONFLICT;
      else if(order == "wdeg") {
        args.order = ORDER_WDEG;
      } else if(order == "domoverwdeg") {
        args.order = ORDER_DOMOVERWDEG;
      } else {
        ostringstream oss;
        oss << "I do not understand the order:" << order;
        outputFatalError(oss.str());
      }
    } else if(command == string("-valorder")) {
      INCREMENT_i(-valorder);

      string order(argv[i]);

      if(order == "ascend") {
        args.valorder = VALORDER_ASCEND;
      } else if(order == "descend") {
        args.valorder = VALORDER_DESCEND;
      } else if(order == "random") {
        args.valorder = VALORDER_RANDOM;
      }
    }


    else if(command == string("-randomiseorder")) {
      getOptions().randomiseValvarorder = true;
    }

    else if(command == string("-randomseed")) {
      INCREMENT_i(-randomseed);
      args.randomSeed = atoi(argv[i]);
    }



    else if(command == string("-tableout") || command == string("-tableout0")) {
      getOptions().tableout = true;
      INCREMENT_i(-tableout);
      getTableOut().set_table_filename(argv[i]);
    } else if(command == string("-jsontableout")) {
      getOptions().tableout = true;
      INCREMENT_i(-jsontableout);
      getTableOut().set_json_filename(argv[i]);
    }



    else if(command == string("-solsout") || command == string("-solsout0")) {
      if(getOptions().solsoutWrite) {
        outputFatalError("Cannot give two of -jsonsolsout and -solsout");
      }
      getOptions().solsoutWrite = true;
      INCREMENT_i(-solsout);
      solsoutFile.open(argv[i], ios::app);
      if(!solsoutFile) {
        ostringstream oss;
        oss << "Cannot open '" << argv[i] << "' for writing.";
        outputFatalError(oss.str());
      }
    }


    else if(command == string("-jsonsolsout")) {
      if(getOptions().solsoutWrite) {
        outputFatalError("Cannot give two of -jsonsolsout and -solsout");
      }
      getOptions().solsoutWrite = true;
      getOptions().solsoutJson = true;
      INCREMENT_i(-jsonsolsout);
      solsoutFile.open(argv[i], ios::app);
      if(!solsoutFile) {
        ostringstream oss;
        oss << "Cannot open '" << argv[i] << "' for writing.";
        outputFatalError(oss.str());
      }
    }

    else if(command == string("-makeresume")) {
      getOptions().noresumefile = false;
    }

    else if(command == string("-noresume")) {
      getOptions().noresumefile = true;
    }

    else if(command == string("-gap")) {
      INCREMENT_i(-gap);
      getOptions().gapname = argv[i];
    } else if(command == string("-parallel")) {
      std::cerr << "Warning: parallel is beta\n";
      std::cerr << "Use -solsout to store the solutions";
      getOptions().parallel = true;
    } else if(command == string("-X-AMO")) {
      getOptions().gatherAMOs = true;
    } else if(command == string("-cores")) {
      INCREMENT_i(-cores);
      getOptions().parallelcores = atoi(argv[i]);
    } else if(command == string("-steallow")) {
      getOptions().parallelStealHigh = false;
    }

    else if(command == string("-split")) {
      getOptions().split = true;
      getOptions().noresumefile = false;
      getOptions().splitstderr = false;
    }

    else if(command == string("-split-stderr")) {
      getOptions().split = true;
      getOptions().noresumefile = false;
      getOptions().splitstderr = true;
    }
    else if(command == string("-command-list")) {
      INCREMENT_i(-command-list);
      getOptions().commandlistIn = argv[i];
      INCREMENT_i(-command-list);
      getOptions().commandlistOut = argv[i];
    }
    else if(command == string("-restarts")) {
      getOptions().restart.active = true;
    } else if(command == string("-restarts-multiplier")) {
      INCREMENT_i("restarts multiplier");
      getOptions().restart.multiplier = fromstring<double>(argv[i]);
    } else if(command == string("-no-restarts-bias")) {
      getOptions().restart.bias = false;
    } else if(command[0] == '-' && command != string("--")) {
      cout << "I don't understand '" << command << "'. Sorry. " << endl;
      exit(1);
    } else {
      if(getOptions().instance_name == "")
        getOptions().instance_name = command;
      else {
        cout << "I was confused by '" << command << "'. Sorry." << endl;
        cout << "You can only give one instance file." << endl;
        exit(1);
      }
    }
  }
  // bundle all options together and store
  string s = string("");
  for(SysInt i = 1; i < argc; ++i) {
    if(i < argc - 1)
      s = s + argv[i] + ",";
    else
      s = s + argv[i];
  }
  getTableOut().set("CommandLineArguments", s);
}
