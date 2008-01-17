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
  enum PreProcess
  { None, SAC, SSAC, SACBounds, SSACBounds  };

  VarOrder order;
  int preprocess;   // Why is this an int and not an enum PreProcess??
  unsigned random_seed;
  MinionArguments() : order(ORDER_ORIGINAL), preprocess(None), random_seed((unsigned)time(NULL))
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
    << "         [-sac-bound-root], [-ssac-bound-root] Only check bounds" << endl
    << "         [-varorder] order			 Change variable ordering strategy" << endl
	<< "		   order = sdf               Smallest Domain First (static breaks ties)" << endl
	<< "		   order = sdf-random        SDF (randomly break ties)" << endl
    << "           order = ldf               Largest Domain First (static breaks ties)" << endl
    << "           order = ldf-random        LDF (randomly break ties)" << endl
    << "           order = random            Random variable ordering" << endl
    << "           order = static            Standard static (but slower)" << endl
    << "            Note: These orderings do not cache any information between" << endl
    << "            nodes, so will perform poorly on problem with many variables." << endl
    << "         [-randomseed] N             Set the random seed used to N." << endl
    << "         [-dumptree]                 Dumps the search tree" << endl
    << "         [-tableout] filename        Writes a line of statistics to the file" << endl
  
#ifndef NO_DEBUG
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
VARDEF(TableOut tableout);

template<typename Reader>
void parse_command_line(Reader& reader, MinionArguments& args, int argc, char** argv)
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
	{ args.preprocess = MinionArguments::SAC; }
	else if(command == string("-ssac-root"))
	{ args.preprocess = MinionArguments::SSAC; }
    else if(command == string("-sac-bound-root"))
	{ args.preprocess = MinionArguments::SACBounds; }
	else if(command == string("-ssac-bound-root"))
	{ args.preprocess = MinionArguments::SSACBounds; }
    
	else if(command == string("-fullprop"))
	{
#ifndef NO_DEBUG
	  Controller::commandlineoption_fullpropagate = true; 
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
	  Controller::commandlineoption_dumptree = true; 
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
    else if(command == string("-tableout"))
    {
        Controller::commandlineoption_tableout=true;
        ++i;
        tableout.set_filename(argv[i]);
    }
	else
	{ 
	  cout << "I don't understand '" << command << "'. Sorry." << endl;
	  FAIL_EXIT();
	}
  }
  // bundle all options together and store
  string s=string("");
  for(int i = 1; i < argc - 1; ++i)
  {
      if(i<argc-2)
          s=s+argv[i]+",";
      else
          s=s+argv[i];
  }
  tableout.set("CommandLineArguments", s);
}

pair<vector<AnyVarRef>, vector<int> > var_val_order;

template<typename Reader>
void BuildCSP(Reader& reader)
{
  // Fix up Bound / Sparse Bound
  
  reader.instance.fixDiscrete(SmallDiscreteCheck());
  
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


template<typename Reader>
void SolveCSP(Reader& reader, MinionArguments args)
{
    // Copy args into tableout
    tableout.set("RandomSeed", to_string(args.random_seed));
    {   char * b;
        if(args.preprocess == MinionArguments::None) b="None";  // All this junk is because of C++'s lousy enums.
        if(args.preprocess == MinionArguments::SAC) b="SAC";
        if(args.preprocess == MinionArguments::SSAC) b="SSAC";
        if(args.preprocess == MinionArguments::SACBounds) b="SACBounds";
        if(args.preprocess == MinionArguments::SSACBounds) b="SSACBounds";
        tableout.set("Preprocess", string(b));}
    // should be one for varorder as well.
    tableout.set("MinionVersion", SVN_VER);
    
    maybe_print_timestep_store("Parsing Time: ", "ParsingTime", tableout, !Controller::print_only_solution);
    
    BuildCSP(reader);
    
    if(randomise_valvaroder)
    {
        cout << "Using seed: " << args.random_seed << endl;
        srand( args.random_seed );
        
        std::random_shuffle(var_val_order.first.begin(), var_val_order.first.end());
        for(unsigned i = 0; i < var_val_order.second.size(); ++i)
          var_val_order.second[i] = (rand() % 100) > 50;
        
        if(reader.parser_verbose)
        {
          int size = var_val_order.first.size();
          if(size != 0)
          {
            cout << "Var Order: <" << var_val_order.first[0];
            for(int i = 1; i < size; ++i)
              cout << "," << var_val_order.first[i];
            cout << ">" << endl;
            
            cout << "Val Order: <" << var_val_order.second[0];
            for(int i = 1; i < size; ++i)
              cout << "," << var_val_order.second[i];
            cout << ">" << endl;
          }
        }
    }
  // Solve!
  maybe_print_timestep_store("Setup Time: ", "SetupTime", tableout, !Controller::print_only_solution);
  
  long long initial_lit_count = 0;
  
  if(args.preprocess != MinionArguments::None)
    initial_lit_count = lit_count(var_val_order.first);
  
  Controller::initalise_search();
  
  if(!Controller::failed)
  {
	if(args.preprocess != MinionArguments::None)
	{
      bool bounds_check = (args.preprocess == MinionArguments::SACBounds) ||
      (args.preprocess == MinionArguments::SSACBounds);
	  long long lits = lit_count(var_val_order.first);
      cout << "Initial GAC loop literal removal:" << initial_lit_count - lits << endl;
	  clock_t start_SAC_time = clock();
	  PropagateSAC prop;
      prop(var_val_order.first, bounds_check);
      cout << "Preprocess Time: " << (clock() - start_SAC_time) / (1.0 * CLOCKS_PER_SEC) << endl;
	  cout << "Removed " << (lits - lit_count(var_val_order.first)) << " literals" << endl;
	  if(args.preprocess == MinionArguments::SSAC || args.preprocess == MinionArguments::SSACBounds)
	  {
		long long lits = lit_count(var_val_order.first);
		clock_t start_SAC_time = clock();
		PropagateSSAC prop;
		prop(var_val_order.first, bounds_check);
		cout << "Preprocess 2 Time: " << (clock() - start_SAC_time) / (1.0 * CLOCKS_PER_SEC) << endl;
		cout << "Removed " << (lits - lit_count(var_val_order.first)) << " literals" << endl;
	  }
	}
    maybe_print_timestep_store("First node time: ", "FirstNodeTime", tableout, !Controller::print_only_solution);
	if(!Controller::failed)
        solve(args.order, var_val_order);   // add a maybe_print_timestep_store to search..
  }
  else
  {
      maybe_print_timestep_store("First node time: ", "FirstNodeTime", tableout, !Controller::print_only_solution);
  }
  
  maybe_print_finaltimestep_store("Solve Time: ", "SolveTime", tableout, !Controller::print_only_solution);
  cout << "Total Nodes: " << nodes << endl;
  cout << "Problem solvable?: " 
	<< (Controller::solutions == 0 ? "no" : "yes") << endl;
  cout << "Solutions Found: " << Controller::solutions << endl;
  
  tableout.set("Nodes", to_string(nodes));
  tableout.set("Satisfiable", (Controller::solutions==0 ? 0 : 1));
  tableout.set("SolutionsFound", Controller::solutions);
  
  if(Controller::commandlineoption_tableout)
  {
      tableout.print_line();  // Outputs a line to the table file.
  }
  
#ifdef MORE_SEARCH_INFO
  print_search_info();
#endif
  
}

template<typename Reader>
void ReadCSP(Reader& reader,  InputFileReader* infile, char* filename)
{
#ifndef NOCATCH  
  try
{
#endif
  
  reader.read(infile) ;
  tableout.set(string("Filename"), string(filename));
#ifndef NOCATCH
}
catch(parse_exception& s)
{
  cerr << "Error in input." << endl;
  cerr << s.what() << endl;
  
  ConcreteFileReader<ifstream>* stream_cast = 
	   dynamic_cast<ConcreteFileReader<ifstream>*>(infile);
  if(stream_cast)
  {
    // This nasty line will tell us the current position in the file 
    // even if a parse fail has occurred.
    int error_pos = stream_cast->infile.rdbuf()->pubseekoff(0, ios_base::cur, ios_base::in);
    int line_num = 0;
    int end_prev_line = 0;
    char* buf = new char[1000000];
    stream_cast->infile.close();
    // Open a new stream, because we don't know what kind of a mess
    // the old one might be in.
    ifstream error_file(filename);
    while(error_pos > error_file.tellg())
    { 
      end_prev_line = error_file.tellg();
      error_file.getline(buf,1000000);
      line_num++;
    }
    cerr << "Error on line:" << line_num << ". Gave up parsing somewhere around here:" << endl;
    cerr << string(buf) << endl;
    for(int i = 0; i < (error_pos - end_prev_line); ++i)
      cerr << "-";
    cerr << "^" << endl;
  }
  else
  {
    // Had an input stream.
    cerr << "At the moment we can't show where a problem occured in the input stream." << endl;
    char* buf = new char[100000];
    buf[0]='\0';
    cin.getline(buf, 100000);
    cerr << "The rest of the line that went wrong is:" << endl;
    cerr << buf << endl;
    cerr << "Try saving your output to a temporary file." << endl;
  }
  
  cerr << "Sorry it didn't work out." << endl;
  exit(1);
}
#endif
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//Entrance:
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
int main(int argc, char** argv) {
  start_clock();
  
  cout << "# " << VERSION << endl ;
  cout << "# Svn version: " << SVN_VER << endl; 
  if (argc == 1)
    print_info();
  
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
  
 
  InputFileReader* infile;
  
  if( argv[argc - 1] != string("--") )
  {
    infile = new ConcreteFileReader<ifstream>(argv[argc - 1]);
	if (infile->failed_open()) {
	  D_FATAL_ERROR("Can't open given input file '" + string(argv[argc - 1]) + "'.");
	}    
  }
  else
    infile = new ConcreteFileReader<std::basic_istream<char, std::char_traits<char> >&>(cin);
  
  string test_name = infile->get_string();
  if(test_name != "MINION")
    D_FATAL_ERROR("All Minion input files must begin 'MINION'");
  
  int inputFileVersionNumber = infile->read_num();
  
  if(inputFileVersionNumber > 3)
    D_FATAL_ERROR("This version of Minion only supports formats up to 3");
  
  if(inputFileVersionNumber == 3)
  {
    MinionThreeInputReader reader;
    MinionArguments args;
    parse_command_line(reader, args, argc, argv);
    ReadCSP(reader, infile, argv[argc - 1]);

    SolveCSP(reader, args);
  }
  else
  {
    MinionInputReader reader;
    MinionArguments args;
    parse_command_line(reader, args, argc, argv);
     ReadCSP(reader, infile, argv[argc - 1]);
    SolveCSP(reader, args);
  }
  
  return 0;
}

