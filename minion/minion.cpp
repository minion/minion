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

#include "system/defined_macros.h"

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
    
    cout << "The following preprocessor flags were active:" << endl;
    print_macros();
	exit(0);
}


template<typename Reader>
void parse_command_line(StateObj* stateObj, Reader& reader, MinionArguments& args, int argc, char** argv)
{
 for(int i = 1; i < argc - 1; ++i)
  {
    const string command(argv[i]);
	if(command == string("-findallsols"))
	{ getOptions(stateObj).findAllSolutions(); }
	else if(command == string("-quiet"))
	{ reader.parser_verbose = false; }
	else if(command == string("-printsols"))
	{ getOptions(stateObj).print_solution = true; }
	else if(command == string("-noprintsols"))
	{ getOptions(stateObj).print_solution = false; }
	else if(command == string("-printsolsonly"))
	{ getOptions(stateObj).print_only_solution = true; }
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
    else if(command == string("-X-prop-node"))
    {
      cout << "# WARNING: -X-prop-node is experimental. Do not use for benchmarking!" << endl;
      ++i;
      string prop_mode(argv[i]);
      args.prop_method = MinionArguments::getPropMethod(prop_mode);
      if(args.prop_method == MinionArguments::None)
        cerr << "Must propagate at each node!" << endl;
    }
    else if(command == string("-X-preprocess"))
    {
      cout << "# WARNING: -X-preprocess is experimental. Do not use for benchmarking!" << endl;
      ++i;
      string prop_mode(argv[i]);
      args.preprocess = MinionArguments::getPropMethod(prop_mode);
    }
	else if(command == string("-fullprop"))
	{
#ifndef NO_DEBUG
	  getOptions(stateObj).fullpropagate = true; 
#else
	  cout << "This version of minion was not built to support the '-fullprop' command. Sorry" << endl;
	  FAIL_EXIT();
#endif
	}
	else if(command == string("-nocheck"))
	{
#ifndef NO_DEBUG
	  getOptions(stateObj).nocheck = true; 
#else
	  cout << "# WARNING: This version of minion was not built to support the '-nocheck' command." << endl;
	  cout << "# WARNING: Solutions will not be checked in this version." << endl;
	  cout << "# WARNING: This is probably the behaviour you want but this option does nothing." << endl;
#endif
	}
	
	else if(command == string("-dumptree"))
	{ getOptions(stateObj).dumptree = true; }
	else if(command == string("-crash"))
	{ debug_crash = true; }
	else if(command == string("-nodelimit"))
	{
	  ++i;
	  getOptions(stateObj).nodelimit = atoi(argv[i]);
	  if(getOptions(stateObj).nodelimit == 0)
	  {
		cout << "Did not understand parameter to nodelimit:" << argv[i] << endl;
		FAIL_EXIT();
	  }
	}
	else if(command == string("-sollimit"))
	{
	  ++i;
	  getOptions(stateObj).sollimit = atoi(argv[i]);
	  getOptions(stateObj).findAllSolutions(); 
	  if(getOptions(stateObj).sollimit == 0)
	  {
	    cout << "Did not understand the parameter to sollimit:" << argv[i] << endl;
		FAIL_EXIT();
	  }
	}
	else if(command == string("-timelimit"))
	{
	  ++i;
	  getOptions(stateObj).time_limit = atoi(argv[i]);
	  if(getOptions(stateObj).time_limit == 0)
	  {
	    cout << "Did not understand the parameter to timelimit:" << argv[i] << endl;
		FAIL_EXIT();
	  }
	}// TODO : Should remove -varorder for beta orderings.
	else if(command == string("-varorder") || command == string("-X-varorder") )
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
	  else
	  {
		cerr << "I do not understand the order:" << order << endl;
		FAIL_EXIT();
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
    else if(command == string("-tableout"))
    {
        getOptions(stateObj).tableout=true;
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



/// Reads the CSP given by infile into reader.
/** Most of the code in this function related to trying to provide nice
  * output in the case when a parsing error occurs
  */
template<typename Reader>
void ReadCSP(Reader& reader, InputFileReader* infile, char* filename)
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
#include "help/help.cpp"

int main(int argc, char** argv) {
  
  StateObj* stateObj = new StateObj();

  
  getState(stateObj).getTimer().startClock();
  
  cout << "# " << VERSION << endl ;
  cout << "# Svn version: " << SVN_VER << endl; 
  if (argc == 1) {
    cout << "Type '" << argv[0] << " help' for usage." << endl;
    return EXIT_SUCCESS;
  }

  if(!strcmp(argv[1], "help")) {
    std::string sect("");
    if(argc != 2) {
      for(size_t i = 2; i < argc - 1; i++) 
	sect.append(argv[i]).append(" ");
      sect.append(argv[argc - 1]);
    }
    help(sect);
    return EXIT_SUCCESS;
  } 
    
  
  if (!getOptions(stateObj).print_only_solution) 
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
  
  CSPInstance instance;
  MinionArguments args;
  
  if(inputFileVersionNumber == 3)
  {
    MinionThreeInputReader reader;
    parse_command_line(stateObj, reader, args, argc, argv);
    ReadCSP(reader, infile, argv[argc - 1]);
    instance = reader.instance;
  } 
  else
  {
    MinionInputReader reader;
    parse_command_line(stateObj, reader, args, argc, argv);
    ReadCSP(reader, infile, argv[argc - 1]);
    instance = reader.instance;
  }  
  
  getState(stateObj).setTupleListContainer(instance.tupleListContainer);
  
  // Copy args into tableout
  tableout.set("RandomSeed", to_string(args.random_seed));
  {   const char * b = "";
    switch (args.preprocess) {
      case MinionArguments::None:
        b = "None"; break;
      case MinionArguments::SAC:
        b = "SAC"; break;
      case MinionArguments::SSAC:
        b = "SSAC"; break;
      case MinionArguments::SACBounds:
        b = "SACBounds"; break;
      case MinionArguments::SSACBounds:
        b = "SSACBounds"; break;
    }
    tableout.set("Preprocess", string(b));
  }
  // should be one for varorder as well.
  tableout.set("MinionVersion", SVN_VER);
  tableout.set("TimeOut", 0); // will be set to 1 if a timeout occurs.
  getState(stateObj).getTimer().maybePrintTimestepStore("Parsing Time: ", "ParsingTime", tableout, !getOptions(stateObj).print_only_solution);
  
  BuildCSP(stateObj, instance);
  SolveCSP(stateObj, instance, args);
  
  delete infile;
  delete stateObj;
  
  return 0;
}

