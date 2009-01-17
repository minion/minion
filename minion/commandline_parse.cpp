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

#include "commandline_parse.h"

extern bool in_cspcomp_for_failexit;

void parse_command_line(StateObj* stateObj, MinionArguments& args, int argc, char** argv)
{
 for(int i = 1; i < argc; ++i)
  {
    const string command(argv[i]);
	if(command == string("-findallsols"))
	{ getOptions(stateObj).findAllSolutions(); }
	else if(command == string("-findgenerators"))
  { getOptions(stateObj).find_generators = true; }
	else if(command == string("-crash"))
  { debug_crash = true; }
	else if(command == string("-quiet"))
	{ getOptions(stateObj).parser_verbose = false; }

	else if(command == string("-redump"))
  { getOptions(stateObj).redump = true; }
  else if(command == string("-Xgraph"))
  { getOptions(stateObj).graph = true; }
  else if(command == string("-outputType"))
  {
    ++i;
    getOptions(stateObj).outputType = atoi(argv[i]);
  }
	else if(command == string("-printsols"))
	{ getOptions(stateObj).print_solution = true; }
	else if(command == string("-noprintsols"))
	{ getOptions(stateObj).print_solution = false; }
	else if(command == string("-notimers"))
  { getOptions(stateObj).noTimers = true; }
	else if(command == string("-printsolsonly"))
	{ getOptions(stateObj).silent = true; }
	else if(command == string("-cspcomp"))
  { 
    getOptions(stateObj).silent = true;
    getOptions(stateObj).cspcomp = true;
    in_cspcomp_for_failexit = true;
  }
	else if(command == string("-verbose"))
	{ getOptions(stateObj).parser_verbose = true; }
    else if(command == string("-X-prop-node"))
    {
      cout << "# WARNING: -X-prop-node is experimental. Do not use for benchmarking!" << endl;
      ++i;
      string prop_mode(argv[i]);
      args.prop_method = GetPropMethodFromString(prop_mode);
      if(args.prop_method == PropLevel_None)
        cerr << "Must propagate at each node!" << endl;
    }
    else if(command == string("-preprocess"))
    {
      ++i;
      string prop_mode(argv[i]);
      args.preprocess = GetPropMethodFromString(prop_mode);
    }
	else if(command == string("-fullprop"))
	{
#ifndef NO_DEBUG
	  getOptions(stateObj).fullpropagate = true; 
#else
    FAIL_EXIT("This version of minion was not built to support the '-fullprop' command. Sorry");
#endif
	}
	else if(command == string("-nocheck"))
	{
	  getOptions(stateObj).nocheck = true; 
	}
  else if(command == string("-check"))
  {
    getOptions(stateObj).nocheck = false;
  }	
	else if(command == string("-dumptree"))
	{ getOptions(stateObj).dumptree = true; }
	else if(command == string("-nodelimit"))
	{
	  ++i;
	  getOptions(stateObj).nodelimit = atoi(argv[i]);
	  if(getOptions(stateObj).nodelimit == 0)
	  {
		cout << "Did not understand parameter to nodelimit:" << argv[i] << endl;
		exit(1);
	  }
	}
	else if(command == string("-sollimit"))
	{
	  ++i;
	  getOptions(stateObj).sollimit = atoi(argv[i]);
	  if(getOptions(stateObj).sollimit == 0)
	  {
	    cout << "Did not understand the parameter to sollimit:" << argv[i] << endl;
		  exit(1);
	  }
	}
	else if(command == string("-timelimit"))
	{
	  ++i;
	  getOptions(stateObj).time_limit = atoi(argv[i]);
	  if(getOptions(stateObj).time_limit == 0)
	  {
	    cout << "Did not understand the parameter to timelimit:" << argv[i] << endl;
      exit(1);
	  }
	}// TODO : Should remove -varorder for beta orderings.
	else if(command == string("-varorder") || command == string("-X-varorder") )
	{ 
	  cout << "# -varorder is experimental and slower than minion's standard branching." << endl;
	  ++i;
	  
	  string order(argv[i]);
	  
	  if(order == "static")
		args.order = ORDER_STATIC;
		else if(order == "srf")
    args.order = ORDER_SRF;
    else if(order == "srf-random")
    {
      args.order = ORDER_SRF;
      getOptions(stateObj).randomise_valvarorder = true;
    }  
	  else if(order == "sdf")
		args.order = ORDER_SDF;
	  else if(order == "sdf-random")
	  {
		args.order = ORDER_SDF;
		getOptions(stateObj).randomise_valvarorder = true;
	  }
	  else if(order == "ldf")
		args.order = ORDER_LDF;
	  else if(order == "ldf-random")
	  {
		args.order = ORDER_LDF;
		getOptions(stateObj).randomise_valvarorder = true;
	  }
	  else if(order == "random")
		getOptions(stateObj).randomise_valvarorder = true;
	  else if(order == "conflict")
	    args.order = ORDER_CONFLICT;
	  else if(order == "wdeg") {
	    args.order = ORDER_WDEG;
	    getOptions(stateObj).wdeg_on = true;
	  } else if(order == "domoverwdeg") {
	    args.order = ORDER_DOMOVERWDEG;
	    getOptions(stateObj).wdeg_on = true;
	  } 
	  else
	  {
		cerr << "I do not understand the order:" << order << endl;
    exit(1);
	  }
	}
	else if(command == string("-randomiseorder"))
	{
	  getOptions(stateObj).randomise_valvarorder = true;
	}
	else if(command == string("-randomseed"))
	{
	  ++i;
	  args.random_seed = atoi(argv[i]);
	}
    else if(command == string("-tableout") || command == string("-tableout0"))
    {
        getOptions(stateObj).tableout=true;
        ++i;
        oldtableout.set_filename(argv[i]);
    }
    else if(command == string("-solsout") || command == string("-solsout0"))
    {
      getOptions(stateObj).solsoutWrite=true;
      ++i;
      solsoutFile.open(argv[i], ios::app);
      if(!solsoutFile)
      {
        cerr << "Cannot open '" << argv[i] << "' for writing." << endl;
        exit(1);
      }
    }
    else if(command[0] == '-' && command != string("--"))
    {
      cout << "I don't understand '" << command << "'. Sorry. " << endl;
      exit(1);
    }
	else
	{ 
	  if(getOptions(stateObj).instance_name == "")
      getOptions(stateObj).instance_name = command;
    else
    {
	    cout << "I was confused by '" << command << "'. Sorry." << endl;
      cout << "You can only give one instance file." << endl;
      exit(1);
    }
	}
  }
  // bundle all options together and store
  string s=string("");
  for(int i = 1; i < argc; ++i)
  {
      if(i<argc-1)
          s=s+argv[i]+",";
      else
          s=s+argv[i];
  }
  oldtableout.set("CommandLineArguments", s);
}
