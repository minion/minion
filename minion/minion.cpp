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

void print_info()
{
 cout << "Usage: minion {options}* nameofprob.minion" << endl
	<< endl 
	<< "Options: [-findallsols]              Find all solutions" << endl 
	<< "         [-quiet] [-verbose]         Don't/do print parser progress" << endl
	<< "         [-printsols] [-noprintsols] Do/don't print solutions" << endl
	<< "         [-test]                     Run in test mode" << endl
	<< "         [-timelimit] N              Stop after N seconds" << endl
	<< "         [-sollimit] N               Stop after N solutions have been found" << endl
    << "                                   ( Automatically activates \"-findallsols\")" << endl
#ifdef MORE_SEARCH_INFO
	<< "         [-nodelimit] N              Stop after N nodes searched" << endl
	<< "         [-dumptree]                 Dumps the search tree" << endl
	<< "         [-randomiseorder]           Randomises the variable ordering" << endl
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

void parse_command_line(MinionInputReader& reader, int argc, char** argv)
{
 for(int i = 1; i < argc - 1; ++i)
  {
    string command(argv[i]);
	if(command == string("-findallsols"))
	{ Controller::find_all_solutions(); }
	else if(command == string("-quiet"))
	{ reader.parser_verbose = false; }
	else if(command == string("-printsols"))
	{ Controller::print_solution = true; }
	else if(command == string("-noprintsols"))
	{ Controller::print_solution = false; }
	else if(command == string("-verbose"))
	{ reader.parser_verbose = true; }
	else if(command == string("-fullprop"))
	{
#ifdef MORE_SEARCH_INFO
	  Controller::commandlineoption_fullpropogate = true; 
#else
	  cout << "This version of minion was not built to support the '-fullprop' command. Sorry" << endl;
	  FAIL_EXIT();
#endif
	}
	else if(command == string("-nocheck"))
	{
#ifdef MORE_SEARCH_INFO
	  Controller::commandlineoption_nocheck = true; 
#else
	  cout << "This version of minion was not built to support the '-nocheck' command. Sorry" << endl;
	  FAIL_EXIT();
#endif
	}
	
	else if(command == string("-dumptree"))
	{
#ifdef MORE_SEARCH_INFO
	  Controller::commandlineoption_dumptree = true; 
#else
	  cout << "This version of minion was not built to support the '-dumptree' command. Sorry" << endl;
	  FAIL_EXIT();
#endif
	}
	else if(command == string("-nodelimit"))
	{
#ifdef MORE_SEARCH_INFO
	  ++i;
	  Controller::commandlineoption_nodelimit = atoi(argv[i]);
	  if(Controller::commandlineoption_nodelimit == 0)
	  {
		cout << "Did not understand parameter to nodelimit:" << argv[i] << endl;
		FAIL_EXIT();
	  }
#else
	  cout << "This version of minion was not built to support the '-nodelimit' command. Sorry" << endl;
	  FAIL_EXIT();
#endif
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
	else if(command == string("-test"))
	{ 
	  Controller::test_mode = true; 
	}
	else if(command == string("-randomiseorder"))
	{
	  randomise_valvaroder = true;
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
  for(unsigned i = 0; i < reader.instance.constraints.size(); ++i)
  {
    if(reader.instance.constraints[i].is_dynamic())
    {
#ifdef DYNAMICTRIGGERS
      Controller::add_constraint(build_dynamic_constraint(reader.instance.constraints[i]));
      dynamic_triggers_used = true;
#else
      cout << "Sorry, cannot process this constraint as it needs dynamic triggers or watched literals." << endl ;
      cout << "use an alternative encoding or recompile with -DWATCHEDLITERALS or -DDYNAMICTRIGGERS in command line" << endl;
      FAIL_EXIT();
#endif
    }
    else
      Controller::add_constraint(build_constraint(reader.instance.constraints[i]));
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
  
  parse_command_line(reader, argc, argv);
 
  
  cout << "# " << VERSION << endl ;
	// << REVISION << endl  // Sadly only gives revision number of minion.h
	
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

  reader.read(argv[argc - 1]) ;
  
  
  print_timestep("Parsing Time: ");
  // Used by test mode.
  BOOL test_checkonesol = false, test_nosols = false;
  // Just a number no-one would ever type :)
  int test_solcount = -1987;
 
  if(Controller::test_mode)
  {  // Now we read the solution!
    string s;
	ifstream infile(argv[argc - 1]);
	
	// Grab the first line, which we don't want.
	char* buf = new char[10000];
	infile.getline(buf,10000);
	delete[] buf;
	
	infile >> s;
	if(s != "#TEST")
	{
	  cout << "Test files must begin with '#TEST' after the version number" << endl;
	  cout << "Instead got '" << s << "'" << endl;
	  FAIL_EXIT();
	}
	
	infile >> s;
	if(s == "CHECKONESOL")
	{
	  test_checkonesol = true;
	    for(unsigned i = 0; i < reader.instance.print_matrix[0].size(); ++i)
	  { 
		int val;
		infile >> val;
		Controller::test_solution.push_back(val);
	  }
	  cout << Controller::test_solution.size() << endl;
    }
	else if(s == "NOSOLS")
	  test_nosols = true;
	else if(s == "SOLCOUNT")
	{
	  infile >> test_solcount;
	  Controller::find_all_solutions();
	}
	else
	{ 
	  cout << "I don't understand" << s << endl;
	  FAIL_EXIT();
	}
	
  }
  
  BuildCSP(reader);
  
  print_timestep("Setup Time: ");

  if(test_checkonesol)
    Controller::set_solution_check_function(&test_check_solution);
  
  if(randomise_valvaroder)
  {
    unsigned seed = (unsigned)time(NULL);
	cerr << "Using seed: " << seed << endl;
	srand( seed );
  
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
  
  Controller::StaticVariableOrder order(var_val_order.first, var_val_order.second);
  Controller::solve(order, var_val_order.first);
    
  if(Controller::test_mode)
  {
    if(test_solcount != -1987)
	{
	  if(Controller::solutions != test_solcount)
	  {
	   cerr << "Error! Expected " << test_solcount << " solutions, but got ";
	   cerr << Controller::solutions << " solutions!" << endl;
	   FAIL_EXIT();
	  }
	}
	
    if(test_checkonesol)
	  if(Controller::solutions == 0)
	  {
	    cerr << "Error! Should be a solution!" << endl;
		FAIL_EXIT();
	  }
	
	if(test_nosols)
	  if(Controller::solutions != 0)
	  {
		cerr << "Error! Should be no solutions!" << endl;
		FAIL_EXIT();
	  }
  }
   
  print_finaltimestep("Solve Time: ");
  cout << "Total Nodes: " << nodes << endl;
  cout << "Problem solvable?: " 
	<< (Controller::solutions == 0 ? "no" : "yes") << endl;
  cout << "Solutions Found: " << Controller::solutions << endl;
  return 0;
}

