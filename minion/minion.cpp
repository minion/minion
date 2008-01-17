/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id$
*/

/*
 *  minion.cpp
 *  cutecsp
 *
 *  Created by Chris Jefferson on 15/05/2006.
 *  Copyright 2006 __MyCompanyName__. All rights reserved.
 *
 */

// These are just because VC++ sucks.
#define _CRT_SECURE_NO_DEPRECATE 1
#define _CRT_NONSTDC_NO_DEPRECATE 1

#include "minion.h"
#include "CSPSpec.h"
using namespace ProbSpec;

#include "BuildConstraint.h"
#include "MinionInputReader.h"

#include "svn_header.h"



struct MinionArguments
{
  VarOrder order;
  int preprocess;
  unsigned random_seed;
  MinionArguments() : order(ORDER_ORIGINAL), preprocess(0), random_seed((unsigned)time(NULL))
  { }
};

void print_info()
{
 cout << "Usage: minion {options}* nameofprob.minion" << endl
	<< endl 
	<< "Options: [-findallsols]              Find all solutions" << endl 
	<< "         [-quiet] [-verbose]         Don't/do print parser progress" << endl
	<< "         [-printsols] [-noprintsols] Do/don't print solutions" << endl
	<< "         [-printsolsonly]            Print only solutions and end summary" << endl
	<< "         [-test]                     Run in test mode" << endl
	<< "         [-timelimit] N              Stop after N seconds" << endl
	<< "         [-sollimit] N               Stop after N solutions have been found" << endl
    << "                                   ( Automatically activates \"-findallsols\")" << endl
    << "         [-nodelimit] N              Stop after N nodes searched" << endl
  << endl
    << "   The following flags are experimental." << endl
    << "   These flags may changed or removed in future versions." << endl
    << "   These flags is not optimised and using them" << endl
    << "   can cause a severe performance drop." << endl
    << "         [-sac-root]				 Perform SAC at the first node of search" << endl
    << "         [-ssac-root]              Perform SSAC at first node of search (can take a LONG time)" << endl
    << "         [-varorder] order			 Change variable ordering strategy" << endl
	<< "		   order = sdf               Smallest Domain First (static breaks ties)" << endl
	<< "		   order = sdf-random        SDF (randomly break ties)" << endl
    << "           order = ldf               Largest Domain First (static breaks ties)" << endl
    << "           order = ldf-random        LDF (randomly break ties)" << endl
    << "           order = random            Random variable ordering" << endl
    << "           order = static            Standard static (but slower)" << endl
    << "            Note: These orderings do not cache any information between" << endl
    << "            nodes, so will perform poorly on problem with many variables." << endl

#ifndef NO_DEBUG
	<< "         [-dumptree]                 Dumps the search tree" << endl
	<< "  Note: The following tags should never change the results produced." << endl
	<< "        If they do, this is a bug." << endl
	<< "        -fullprop will always slow search down." << endl
	<< "        -nocheck will only speed up search." << endl
	<< "         [-fullprop]                 Used for debugging" << endl
	<< "         [-nocheck]                  Don't sanity check solutions" << endl

#endif
	<< endl
	<< "Notes: In problems with an optimisation function, -findallsols is ignored." << endl;
	
    cout << "This version of Minion was built with internal checking " <<
#ifdef NO_DEBUG
	  "off" <<
#else
	  "on" << 
#endif
	  endl << "    and verbose debug info "
#ifdef NO_PRINT
	  "off" << endl;
#else
	"on" << endl;
#endif
	exit(0);
}
  
BOOL randomise_valvaroder = false;

void parse_command_line(MinionInputReader& reader, MinionArguments& args, int argc, char** argv)
{
 for(int i = 1; i < argc - 1; ++i)
  {
    const string command(argv[i]);
	if(command == string("-findallsols"))
	{ Controller::find_all_solutions(); }
	else if(command == string("-quiet"))
	{ reader.parser_verbose = false; }
	else if(command == string("-printsols"))
	{ Controller::print_solution = true; }
	else if(command == string("-noprintsols"))
	{ Controller::print_solution = false; }
	else if(command == string("-printsolsonly"))
	{ Controller::print_only_solution = true; }
	else if(command == string("-verbose"))
	{ reader.parser_verbose = true; }
	else if(command == string("-sac-root"))
	{ args.preprocess = 1; }
	else if(command == string("-ssac-root"))
	{ args.preprocess = 2; }
	else if(command == string("-fullprop"))
	{
#ifndef NO_DEBUG
	  Controller::commandlineoption_fullpropogate = true; 
#else
	  cout << "This version of minion was not built to support the '-fullprop' command. Sorry" << endl;
	  FAIL_EXIT();
#endif
	}
	else if(command == string("-nocheck"))
	{
#ifndef NO_DEBUG
	  Controller::commandlineoption_nocheck = true; 
#else
	  cout << "# WARNING: This version of minion was not built to support the '-nocheck' command." << endl;
	  cout << "# WARNING: Solutions will not be checked in this version." << endl;
	  cout << "# WARNING: This is probably the behaviour you want but this option does nothing." << endl;
#endif
	}
	
	else if(command == string("-dumptree"))
	{
#ifndef NO_DEBUG
	  Controller::commandlineoption_dumptree = true; 
#else
	  cout << "This version of minion was not built to support the '-dumptree' command. Sorry" << endl;
	  FAIL_EXIT();
#endif
	}
	else if(command == string("-crash"))
	{ debug_crash = true; }
	else if(command == string("-nodelimit"))
	{
	  ++i;
	  Controller::commandlineoption_nodelimit = atoi(argv[i]);
	  if(Controller::commandlineoption_nodelimit == 0)
	  {
		cout << "Did not understand parameter to nodelimit:" << argv[i] << endl;
		FAIL_EXIT();
	  }
	}
	else if(command == string("-sollimit"))
	{
	  ++i;
	  Controller::commandlineoption_sollimit = atoi(argv[i]);
	  Controller::find_all_solutions(); 
	  if(Controller::commandlineoption_sollimit == 0)
	  {
	    cout << "Did not understand the parameter to sollimit:" << argv[i] << endl;
		FAIL_EXIT();
	  }
	}
	else if(command == string("-timelimit"))
	{
	  ++i;
	  time_limit = atoi(argv[i]);
	  if(time_limit == 0)
	  {
	    cout << "Did not understand the parameter to timelimit:" << argv[i] << endl;
		FAIL_EXIT();
	  }
	}
	else if(command == string("-varorder"))
	{ 
	  cout << "# -varorder is experimental and slower than minion's standard branching." << endl;
	  ++i;
	  
	  string order(argv[i]);
	  
	  if(order == "static")
		args.order = ORDER_STATIC;
	  else if(order == "sdf")
		args.order = ORDER_SDF;
	  else if(order == "sdf-random")
	  {
		args.order = ORDER_SDF;
		randomise_valvaroder = true;
	  }
	  else if(order == "ldf")
		args.order = ORDER_LDF;
	  else if(order == "ldf-random")
	  {
		args.order = ORDER_LDF;
		randomise_valvaroder = true;
	  }
	  else if(order == "random")
		randomise_valvaroder = true;
	  else
	  {
		cerr << "I do not understand the order:" << order << endl;
		FAIL_EXIT();
	  }
	}
	else if(command == string("-randomiseorder"))
	{
	  randomise_valvaroder = true;
	}
	else if(command == string("-randomseed"))
	{
	  ++i;
	  args.random_seed = atoi(argv[i]);
	}
	else
	{ 
	  cout << "I don't understand '" << command << "'. Sorry." << endl;
	  FAIL_EXIT();
	}
  }
}

pair<vector<AnyVarRef>, vector<BOOL> > var_val_order;

void BuildCSP(MinionInputReader& reader)
{
  // Set up variables
  BuildCon::build_variables(reader.instance.vars);
  
  // Set up variable and value ordering
  var_val_order = BuildCon::build_val_and_var_order(reader.instance);
  
  // Set up optimisation
  if(reader.instance.is_optimisation_problem)
  {
    if(reader.instance.optimise_minimising)
      Controller::optimise_minimise_var(BuildCon::get_AnyVarRef_from_Var(reader.instance.optimise_variable));
	else
	  Controller::optimise_maximise_var(BuildCon::get_AnyVarRef_from_Var(reader.instance.optimise_variable));
  }
  
  // Set up printing
  Controller::print_matrix.resize(reader.instance.print_matrix.size());
  for(unsigned i = 0; i < reader.instance.print_matrix.size(); ++i)
  {
    for(unsigned j = 0; j < reader.instance.print_matrix[i].size(); ++j)
	  Controller::print_matrix[i].push_back(BuildCon::get_AnyVarRef_from_Var(reader.instance.print_matrix[i][j]));
  }
  
  // Impose Constraints
  for(list<ConstraintBlob>::iterator it = reader.instance.constraints.begin();
	  it != reader.instance.constraints.end(); ++it)
  {
    if(it->is_dynamic())
    {
#ifdef DYNAMICTRIGGERS
      Controller::add_constraint(build_dynamic_constraint(*it));
      dynamic_triggers_used = true;
#else
      cout << "Sorry, cannot process this constraint as it needs dynamic triggers or watched literals." << endl ;
      cout << "use an alternative encoding or recompile with -DWATCHEDLITERALS or -DDYNAMICTRIGGERS in command line" << endl;
      FAIL_EXIT();
#endif
    }
    else
      Controller::add_constraint(build_constraint(*it));
  }


}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//Entrance:
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
int main(int argc, char** argv) {
  start_clock();
  
  if (argc == 1)
    print_info();

  MinionInputReader reader;
  MinionArguments args;
  parse_command_line(reader, args, argc, argv);
 
  
  cout << "# " << VERSION << endl ;
  cout << "# Svn version: " << SVN_VER << endl;

  if (!Controller::print_only_solution) 
  { 

    cout << "# Svn last changed date: " << SVN_DATE << endl;
          
    time_t rawtime;
    time(&rawtime);
    cout << "#  Run at: UTC " << asctime(gmtime(&rawtime)) << endl;
    cout << "#    http://minion.sourceforge.net" << endl;
    cout << "#  Minion is still very new and in active development." << endl;
    cout << "#  If you have problems with Minion or find any bugs, please tell us!" << endl;
    cout << "#  Either at the bug reporter at the website, or 'chris@bubblescope.net'" << endl;
    cout << "# Input filename: " << argv[argc-1] << endl;
    cout << "# Command line: " ;
    for (int i=0; i < argc; ++i) { cout << argv[i] << " " ; } 
    cout << endl;
  
  }

  reader.read(argv[argc - 1]) ;
  
  
  if (!Controller::print_only_solution) { print_timestep("Parsing Time: ");} 
  
  BuildCSP(reader);
  
  if (!Controller::print_only_solution) print_timestep("Setup Time: ");
  
  if(randomise_valvaroder)
  {
	cerr << "Using seed: " << args.random_seed << endl;
	srand( args.random_seed );
  
    std::random_shuffle(var_val_order.first.begin(), var_val_order.first.end());
    for(unsigned i = 0; i < var_val_order.second.size(); ++i)
      var_val_order.second[i] = (rand() % 100) > 50;
	  
	if(reader.parser_verbose)
	{
	  int size = var_val_order.first.size();
	  if(size != 0)
	  {
	    cout << "Var Order: <" << string(var_val_order.first[0]);
	    for(int i = 1; i < size; ++i)
	      cout << "," << string(var_val_order.first[i]);
	    cout << ">" << endl;
	  
	    cout << "Val Order: <" << var_val_order.second[0];
	    for(int i = 1; i < size; ++i)
	      cout << "," << var_val_order.second[i];
	    cout << ">" << endl;
	  }
	}
  }
  // Solve!
  
  Controller::initalise_search();
  if(!Controller::failed)
  {
	if(args.preprocess > 0)
	{
	  long long lits = lit_count(var_val_order.first);
	  clock_t start_SAC_time = clock();
	  PropogateSAC prop;
      prop(var_val_order.first);
      cout << "Preprocess Time: " << (clock() - start_SAC_time) / (1.0 * CLOCKS_PER_SEC) << endl;
	  cout << "Removed " << (lits - lit_count(var_val_order.first)) << " literals" << endl;
	  if(args.preprocess > 1)
	  {
		long long lits = lit_count(var_val_order.first);
		clock_t start_SAC_time = clock();
		PropogateSSAC prop;
		prop(var_val_order.first);
		cout << "Preprocess 2 Time: " << (clock() - start_SAC_time) / (1.0 * CLOCKS_PER_SEC) << endl;
		cout << "Removed " << (lits - lit_count(var_val_order.first)) << " literals" << endl;
	  }
	}  
	if(!Controller::failed)
      solve(args.order, var_val_order);
  }
 
  print_finaltimestep("Solve Time: ");
  cout << "Total Nodes: " << nodes << endl;
  cout << "Problem solvable?: " 
	<< (Controller::solutions == 0 ? "no" : "yes") << endl;
  cout << "Solutions Found: " << Controller::solutions << endl;
  
  
#ifdef MORE_SEARCH_INFO
  print_search_info();
#endif
  return 0;
}

