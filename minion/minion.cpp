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

#define IN_MAIN

#include "minion.h"
#include "CSPSpec.h"

using namespace ProbSpec;

#include "BuildConstraint.h"
#include "MinionInputReader.h"

#include "inputfile_parse.h"

#include "svn_header.h"

#include "system/defined_macros.h"

#include "commandline_parse.h"

#ifdef USE_BOOST
#include <fstream>
#include <iostream>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filter/bzip2.hpp>

using namespace boost;
using namespace iostreams;
#endif

/** @help switches Description 
Minion supports a number of switches to augment default behaviour.  To
see more information on any switch, use the help system. The list
below contains all available switches. For example to see help on
-quiet type something similar to

   minion help switches -quiet

replacing 'minion' by the name of the executable you're using.
*/

/** @help switches;-findallsols Description
Find all solutions and count them. This option is ignored if the
problem contains any minimising or maximising objective.
*/

/** @help switches;-quiet Description
Do not print parser progress.
*/

/** @help switches;-quiet References
help switches -verbose
*/

/** @help switches;-verbose Description
Print parser progress.
*/

/** @help switches;-verbose References
help switches -quiet
*/

/** @help switches;-printsols Description
Print solutions.
*/

/** @help switches;-noprintsols Description
Do not print solutions.
*/

/** @help switches;-printsolsonly Description
Print only solutions and a summary at the end.
*/

/** @help switches;-preprocess

This switch allows the user to choose what level of preprocess is
applied to their model before search commences.

The choices are:

- GAC 
- generalised arc consistency (default)
- all propagators are run to a fixed point
- if some propagators enforce less than GAC then the model will
not necessarily be fully GAC at the outset

- SACBounds 
- singleton arc consistency on the bounds of each variable
- AC can be achieved when any variable lower or upper bound is a 
singleton in its own domain

- SAC 
- singleton arc consistency
- AC can be achieved in the model if any value is a singleton in
its own domain

- SSACBounds
- singleton singleton bounds arc consistency
- SAC can be achieved in the model when domains are replaced by either
the singleton containing their upper bound, or the singleton containing
their lower bound

- SSAC 
- singleton singleton arc consistency
- SAC can be achieved when any value is a singleton in its own domain

These are listed in order of roughly how long they take to
achieve. Preprocessing is a one off cost at the start of search. The
success of higher levels of preprocessing is problem specific; SAC
preprocesses may take a long time to complete, but may reduce search
time enough to justify the cost.
*/

/** @help switches;-preprocess Example
To enforce SAC before search:

   minion -preprocess SAC myinputfile.minion
*/

/** @help switches;-preprocess References
help switches -X-prop-node
*/

/** @help switches;-X-prop-node Description
Allows the user to choose the level of consistency to be enforced
during search.

See entry 'help switches -preprocess' for details of the available
levels of consistency.
*/

/** @help switches;-X-prop-node Example
To enforce SSAC during search:

   minion -X-prop-node SSAC input.minion
*/

/** @help switches;-X-prop-node References
help switches -preprocess
*/

/** @help switches;-dumptree Description
Print out the branching decisions and variable states at each node.
*/

/** @help switches;-fullprop Description
Disable incremental propagation.
*/

/** @help switches;-fullprop Notes
This should always slow down search while producing exactly the same
search tree.

Only available in a DEBUG executable.
*/

/** @help switches;-nocheck Description 
Do not check solutions for correctness before printing them out.
*/

/** @help switches;-nocheck Notes
This option is the default on non-DEBUG executables.
*/

/** @help switches;-check Description 
Check solutions for correctness before printing them out.
*/

/** @help switches;-check Notes
This option is the default for DEBUG executables.
*/

/** @help switches;-nodelimit Description
To stop search after N nodes, do

   minion -nodelimit N myinput.minion
*/

/** @help switches;-nodelimit References
help switches -timelimit
help switches -sollimit
*/

/** @help switches;-timelimit Description
To stop search after N seconds, do

   minion -timelimit N myinput.minion
*/

/** @help switches;-timelimit References
help switches -nodelimit
help switches -sollimit
*/

/** @help switches;-sollimit Description
To stop search after N solutions have been found, do

   minion -sollimit N myinput.minion
*/

/** @help switches;-sollimit References
help switches -nodelimit
help switches -timelimit
*/

/** @help switches;-varorder Description 

Enable a particular variable ordering for the search process. This
flag is experimental and minion's default ordering might be faster.

The available orders are:

- sdf - smallest domain first, break ties lexicographically

- sdf-random - sdf, but break ties randomly

- ldf - largest domain first, break ties lexicographically

- ldf-random - ldf, but break ties randomly

- random - random variable ordering

- static - 
*/

/* @help switches;-varorder Example 
To use smallest domain first ordering (probably the most sensible of
the available orderings) do:

   minion -varorder sdf myinput.minion
*/

/** @help switches;-randomseed Description 
Set the pseudorandom seed to N. This allows 'random' behaviour to be
repeated in different runs of minion.
*/

/** @help switches;-tableout Description 
Append a line of data about the current run of minion to a named file.
This data includes minion version information, arguments to the
executable, build and solve time statistics, etc. See the file itself
for a precise schema of the supplied information.
*/

/** @help switches;-tableout Example
To add statistics about solving myproblem.minion to mystats.txt do

   minion -tableout mystats.txt myproblem.minion
*/

/** @help switches;-solsout Description 
Append all solutionsto a named file.
Each solution is placed on a line, with no extra formatting.
*/

/** @help switches;-solsout Example
To add the solutions of myproblem.minion to mysols.txt do

   minion -solsout mysols.txt myproblem.minion
*/


/** @help switches;-randomiseorder Description 
Randomises the ordering of the decision variables. If the input file
specifies as ordering it will randomly permute this. If no ordering is
specified a random permutation of all the variables is used.
*/





//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//Entrance:
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

#include "help/help.cpp"

void print_default_help(char** argv)
{
  cout << "Type '" << argv[0] << " help' for usage." << endl;
  cout << endl << "Usage: " << argv[0] << " {switch}* [input file]" << endl;
  help("switches");
  cout << endl;
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
}

int main(int argc, char** argv) {
// Wrap main in a try/catch just to stop exceptions leaving main,
// as windows gets really annoyed when that happens.
  
try {
  StateObj* stateObj = new StateObj();

  getState(stateObj).getTimer().startClock();
  
  cout << "# " << VERSION << endl ;
  cout << "# Svn version: " << SVN_VER << endl; 

  if (argc == 1) {
    print_default_help(argv);
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
  } else {
  }
    
  CSPInstance instance;
  MinionArguments args;

  parse_command_line(stateObj, args, argc, argv);
  
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
    cout << "# Input filename: " << getOptions(stateObj).instance_name << endl;
    cout << "# Command line: " ;
    for (int i=0; i < argc; ++i) { cout << argv[i] << " " ; } 
    cout << endl;
  }
  
  if( getOptions(stateObj).instance_name != "--" )
  {
    // Why do I have to put this filename into a variable before I feed it into the ConcreteFileReader?
    // I'm not completely sure, but it works this way.
    string fname = getOptions(stateObj).instance_name;
    const char* filename = getOptions(stateObj).instance_name.c_str();
    string extension;
    if(fname.find_last_of(".") < fname.size())
      extension = fname.substr(fname.find_last_of("."), fname.size());
     
#ifdef USE_BOOST
    ifstream file(filename, ios_base::in | ios_base::binary);
    
     if (!file) {
        INPUT_ERROR("Can't open given input file '" + getOptions(stateObj).instance_name + "'.");
      }
    filtering_istream in;
    
    if(extension == ".gz" || extension == ".gzip" || extension == ".z" || extension == ".gzp")
    {
      cout << "# Using gzip uncompression" << endl;
      in.push(gzip_decompressor());
    }    
    
    if(extension == ".bz2" || extension == ".bz" || extension == ".bzip2" || extension == ".bzip")
    {
      cout << "# Using bzip2 uncompression" << endl;
      in.push(bzip2_decompressor());
    }
        
    in.push(file);
    
    ConcreteFileReader<filtering_istream> infile(in, filename);
#else
     if(extension == ".gz" || extension == ".gzip" || extension == ".z" || extension == ".gzp" ||
        extension == ".bz2" || extension == ".bz" || extension == ".bzip2" || extension == ".bzip")
     { INPUT_ERROR("This copy of Minion was built without gzip and bzip2 support!"); }

    ifstream ifm(filename);
    ConcreteFileReader<ifstream> infile(ifm, filename);
#endif 
    if (infile.failed_open() || infile.eof()) {
      INPUT_ERROR("Can't open given input file '" + getOptions(stateObj).instance_name + "'.");
    }   
    instance = readInput(&infile, getOptions(stateObj).parser_verbose);
  }
  else
  {
    ConcreteFileReader<std::basic_istream<char, std::char_traits<char> > > infile(cin, "Standard Input");
    instance = readInput(&infile, getOptions(stateObj).parser_verbose);
  }

  getState(stateObj).setTupleListContainer(instance.tupleListContainer);
  
  // Copy args into tableout
  tableout.set("RandomSeed", to_string(args.random_seed));
  {   const char * b = "";
    switch (args.preprocess) {
      case PropLevel_None:
        b = "None"; break;
      case PropLevel_SAC:
        b = "SAC"; break;
      case PropLevel_SSAC:
        b = "SSAC"; break;
      case PropLevel_SACBounds:
        b = "SACBounds"; break;
      case PropLevel_SSACBounds:
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
  
  delete stateObj;
  
  return 0;
    
}
catch(...)
{ cerr << "Minion exited abnormally via an exception." << endl; }
}

