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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

// These are just because VC++ sucks.
#define _CRT_SECURE_NO_DEPRECATE 1
#define _CRT_NONSTDC_NO_DEPRECATE 1

#include "minion.h"

#include "BuildVariables.h"

#include "inputfile_parse/inputfile_parse.h"
#include "commandline_parse.h"

#include "system/defined_macros.h"

#include "MILtools/print_CSP.h"

#include "MILtools/sym_output.h"

#ifndef HG_VER
#define HG_VER "0"
#endif

#ifndef HG_DATE
#define HG_DATE Not from a HG checkout
#endif

// The marvels of the C pre-processor...
#define CAJ_EXPAND(x) #x
#define CAJ_STRING(x) CAJ_EXPAND(x)

#define HG_DATE_STRING CAJ_STRING(HG_DATE)
#define HG_VER_STRING CAJ_STRING(HG_VER)


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//Entrance:
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void help(string request);

void print_default_help(char** argv)
{
  cout << "Type '" << argv[0] << " help' for usage." << endl;
  cout << endl << "Usage: " << argv[0] << " {switch}* [input file]" << endl;
  help("switches");
  cout << endl;
  cout << "This version of Minion was built with internal checking " <<
#ifdef NO_DEBUG
  "off" << endl;
#else
  "on" << endl;
#endif
  cout << "The following preprocessor flags were active:" << endl;
  print_macros();
}

void worker()
{
  while(1) {;}
}

template<typename Con>
void munge_container(Con& con, SysInt type)
{
  switch(type)
  {
    case 0: return;
    case 1:
      std::reverse(con.begin(), con.end());
      return;
    case 2:
    {
      Con con2;
      SysInt size = con.size();
      if(size%2==1)
      {
        size--;
        con2.push_back(con[size/2]);
        size--;
      }
      else
        size-=2;

      for(SysInt i = size/2; i >= 0; --i)
      {
        con2.push_back(con[i]);
        con2.push_back(con[con.size() - i - 1]);
      }
      D_ASSERT(con2.size() == con.size());
      con = con2;
      return;
    }
    case 3:
    case 4:
    case 5:
    case 6:
    {
      srand(type);
      std::random_shuffle(con.begin(), con.end());
      return;
    }
    default:
      abort();
  }
}

int main(int argc, char** argv) {
// Wrap main in a try/catch just to stop exceptions leaving main,
// as windows gets really annoyed when that happens.
try {
  StateObj* stateObj = new StateObj();

  getState(stateObj).getOldTimer().startClock();

  if (argc == 1) {
    getOptions(stateObj).printLine("# " + tostring(VERSION));
    getOptions(stateObj).printLine("# HG version: " + tostring(HG_VER_STRING));
    print_default_help(argv);
    return EXIT_SUCCESS;
  }

  if(argv[1] == string("help") || argv[1] == string("--help") || argv[1] == string("-help") || argv[1] == string("-h")) {
    std::string sect("");
    if(argc != 2) {
      for(size_t i = 2; i < argc - 1; i++)
        sect.append(argv[i]).append(" ");
      sect.append(argv[argc - 1]);
    }
    help(sect);
    return EXIT_SUCCESS;
  } else {
  }

  CSPInstance instance;
  SearchMethod args;

  parse_command_line(stateObj, args, argc, argv);

  if(getOptions(stateObj).outputType != -1)
    getState(stateObj).getOldTimer().setOutputType(getOptions(stateObj).outputType);

  getOptions(stateObj).printLine("# " + tostring(VERSION));
  getOptions(stateObj).printLine("# HG version: " + tostring(HG_VER_STRING));

  if (!getOptions(stateObj).silent)
  {

    getOptions(stateObj).printLine("# HG last changed date: " + tostring(HG_DATE_STRING) );

    time_t rawtime;
    time(&rawtime);
    cout << "#  Run at: UTC " << asctime(gmtime(&rawtime)) << endl;
    cout << "#    http://minion.sourceforge.net" << endl;
    cout << "# If you have problems with Minion or find any bugs, please tell us!" << endl;
    cout << "# Mailing list at: https://mailman.cs.st-andrews.ac.uk/mailman/listinfo/mug" << endl;
    cout << "# Input filename: " << getOptions(stateObj).instance_name << endl;
    cout << "# Command line: " ;
    for (SysInt i=0; i < argc; ++i) { cout << argv[i] << " " ; }
    cout << endl;
  }

  if(!getOptions(stateObj).noTimers)
  {
      getState(stateObj).setupAlarm(getOptions(stateObj).timeout_active, getOptions(stateObj).time_limit, getOptions(stateObj).time_limit_is_CPU_time);
      getState(stateObj).setupCtrlc();
  }

  vector<string> files(1, getOptions(stateObj).instance_name);
  readInputFromFiles(instance, files, getOptions(stateObj).parser_verbose, getOptions(stateObj).map_long_short);

  if(getOptions(stateObj).Xvarmunge != -1)
  {
    assert(instance.search_order.size() == 1);
    munge_container(instance.search_order[0].var_order, getOptions(stateObj).Xvarmunge);
  }

  if(getOptions(stateObj).Xsymmunge != -1)
  {
    munge_container(instance.sym_order, getOptions(stateObj).Xsymmunge);
  }

  if(getOptions(stateObj).graph)
  {
    GraphBuilder graph(instance);
    //graph.g.output_graph();
    graph.g.output_nauty_graph(instance);
    exit(0);
  }

  if(getOptions(stateObj).instance_stats)
  {
      InstanceStats s(instance, stateObj);
      s.output_stats();

      // Do the minimal amount of setting up to create the constraint objects.
      getState(stateObj).setTupleListContainer(instance.tupleListContainer);
      getState(stateObj).setShortTupleListContainer(instance.shortTupleListContainer);

      BuildCon::build_variables(stateObj, instance.vars);

      // Create Constraints
      vector<AbstractConstraint*> cons;
      while(!instance.constraints.empty())
      {
         cons.push_back(build_constraint(stateObj, instance.constraints.front()));
         instance.constraints.pop_front();
      }

      s.output_stats_tightness(cons);
      exit(0);
  }

  if(getOptions(stateObj).redump)
  {
    MinionInstancePrinter printer(instance);
    printer.build_instance();
    cout << printer.getInstance();
    exit(0);
  }

  // Copy args into tableout
  getTableOut().set("RandomSeed", tostring(args.random_seed));
  {   const char * b = "";
    switch (args.preprocess) {
      case PropLevel_None:
        b = "None"; break;
      case PropLevel_GAC:
        b = "GAC"; break;
      case PropLevel_SAC:
        b = "SAC"; break;
      case PropLevel_SSAC:
        b = "SSAC"; break;
      case PropLevel_SACBounds:
        b = "SACBounds"; break;
      case PropLevel_SSACBounds:
        b = "SSACBounds"; break;
    }
    getTableOut().set("Preprocess", string(b));
  }
  // should be one for varorder as well.
  getTableOut().set("MinionVersion", HG_VER_STRING);
  getTableOut().set("TimeOut", 0); // will be set to 1 if a timeout occurs.
  getState(stateObj).getOldTimer().maybePrintTimestepStore(cout, Output_Always, "Parsing Time: ", "ParsingTime", getTableOut(), !getOptions(stateObj).silent);

  BuildCSP(stateObj, instance);
  SolveCSP(stateObj, instance, args);

  delete stateObj;

  return 0;

}
catch(...)
{
  cerr << "Minion exited abnormally via an exception." << endl;
  exit(9);
}
}
