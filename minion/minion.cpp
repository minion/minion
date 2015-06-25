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
        con2.push_back(con[(SysInt)con.size() - i - 1]);
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

  getState().getOldTimer().startClock();

  if (argc == 1) {
    getOptions().printLine("# " + tostring(VERSION));
    getOptions().printLine("# HG version: " + tostring(HG_VER_STRING));
    print_default_help(argv);
    return EXIT_SUCCESS;
  }

  if(argv[1] == string("help") || argv[1] == string("--help") || argv[1] == string("-help") || argv[1] == string("-h")) {
    std::string sect("");
    if(argc != 2) {
      for(int i = 2; i < argc - 1; i++)
        sect.append(argv[i]).append(" ");
      sect.append(argv[argc - 1]);
    }
    help(sect);
    return EXIT_SUCCESS;
  } else {
  }

  CSPInstance instance;
  SearchMethod args;

  parse_command_line(args, argc, argv);

  if(getOptions().outputType != -1)
    getState().getOldTimer().setOutputType(getOptions().outputType);

  getOptions().printLine("# " + tostring(VERSION));
  getOptions().printLine("# HG version: " + tostring(HG_VER_STRING));

  if (!getOptions().silent)
  {

    getOptions().printLine("# HG last changed date: " + tostring(HG_DATE_STRING) );

    time_t rawtime;
    time(&rawtime);
    cout << "#  Run at: UTC " << asctime(gmtime(&rawtime)) << endl;
    cout << "#    http://minion.sourceforge.net" << endl;
    cout << "# If you have problems with Minion or find any bugs, please tell us!" << endl;
    cout << "# Mailing list at: https://mailman.cs.st-andrews.ac.uk/mailman/listinfo/mug" << endl;
    cout << "# Input filename: " << getOptions().instance_name << endl;
    cout << "# Command line: " ;
    for (SysInt i=0; i < argc; ++i) { cout << argv[i] << " " ; }
    cout << endl;
  }

  if(!getOptions().noTimers)
  {
      getState().setupAlarm(getOptions().timeout_active, getOptions().time_limit, getOptions().time_limit_is_CPU_time);
      getState().setupCtrlc();
  }

  vector<string> files(1, getOptions().instance_name);
  readInputFromFiles(instance, files, getOptions().parser_verbose,
                     getOptions().map_long_short, getOptions().ensure_branch_on_all_vars);

  if(getOptions().Xvarmunge != -1)
  {
    assert(instance.search_order.size() == 1);
    munge_container(instance.search_order[0].var_order, getOptions().Xvarmunge);
  }

  if(getOptions().Xsymmunge != -1)
  {
    munge_container(instance.sym_order, getOptions().Xsymmunge);
  }
/* XXX
  if(getOptions().graph)
  {
    GraphBuilder graph(instance);
    //graph.g.output_graph();
    graph.g.output_nauty_graph(instance);
    exit(0);
  }

  if(getOptions().instance_stats)
  {
      InstanceStats s(instance);
      s.output_stats();

      // Do the minimal amount of setting up to create the constraint objects.
      getState().setTupleListContainer(instance.tupleListContainer);
      getState().setShortTupleListContainer(instance.shortTupleListContainer);

      BuildCon::build_variables(instance.vars);

      // Create Constraints
      vector<AbstractConstraint*> cons;
      while(!instance.constraints.empty())
      {
         cons.push_back(build_constraint(instance.constraints.front()));
         instance.constraints.pop_front();
      }

      s.output_stats_tightness(cons);
      exit(0);
  }
*/
  if(getOptions().redump)
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
  getState().getOldTimer().maybePrintTimestepStore(cout, Output_Always, "Parsing Time: ", "ParsingTime", getTableOut(), !getOptions().silent);

  BuildCSP(instance);
  SolveCSP(instance, args);

  return 0;

}
catch(...)
{
  cerr << "Minion exited abnormally via an exception." << endl;
  exit(9);
}
}
