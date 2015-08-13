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

/** @help switches Description
Minion supports a number of switches to augment default behaviour.  To
see more information on any switch, use the help system. The list
below contains all available switches. For example to see help on
-quiet type something similar to

   minion help switches -quiet

replacing 'minion' by the name of the executable you're using.
*/

/** @help switches;-redump Description
Print the minion input instance file to standard out. No search is
carried out when this switch is used.
*/

/** @help switches;-findallsols Description
Find all solutions and count them. This option is ignored if the
problem contains any minimising or maximising objective.
*/

/** @help switches;-varorder Description

Enable a particular variable ordering for the search process. This
flag is experimental and minion's default ordering might be faster.

The available orders are:

- sdf - smallest domain first, break ties lexicographically

- sdf-random - sdf, but break ties randomly

- srf - smallest ratio first, chooses unassigned variable with smallest
  percentage of its initial values remaining, break ties lexicographically

- srf-random - srf, but break ties randomly

- ldf - largest domain first, break ties lexicographically

- ldf-random - ldf, but break ties randomly

- random - random variable ordering

- static - lexicographical ordering
*/

/* @help switches;-varorder Example
To use smallest domain first ordering (probably the most sensible of
the available orderings) do:

   minion -varorder sdf myinput.minion
*/

#include "commandline_parse.h"

extern bool in_cspcomp_for_failexit;

#define INCREMENT_i(flag)                                                                          \
  {                                                                                                \
    ++i;                                                                                           \
    if (i == argc || argv[i][0] == '-') {                                                          \
      cerr << #flag << " requires a value\n";                                                      \
      exit(1);                                                                                     \
    }                                                                                              \
  }

void parse_command_line(SearchMethod &args, SysInt argc, char **argv) {
  for (SysInt i = 1; i < argc; ++i) {
    const string command(argv[i]);
    if (command == string("-findallsols")) {
      getOptions().findAllSolutions();
    } else if (command == string("-crash")) {
      debug_crash = true;
    }
    /** @help switches;-quiet Description
    Do not print parser progress (default)
    */

    /** @help switches;-quiet References
    help switches -verbose
    */
    else if (command == string("-quiet")) {
      getOptions().parser_verbose = false;
    }

    else if (command == string("-redump")) {
      getOptions().redump = true;
    }
    /** @help switches;-outputCompressedDomains Description
    Try to rduce the initial domains of variables, and output them.
    This is in general not useful for users, but is provided for other systems.
    */
    else if (command == string("-outputCompressedDomains")) {
      getOptions().outputCompressedDomains = true;
    }
    /** @help switches;-outputCompressed Description
    Output a Minion instance with some basic reasoning performed to
    reduce the size of the file. This file should produce identical
    output the original instance but may solve faster.
    */

    /** @help switches;-outputCompressed Example
    To compress a file 'infile.minion' to a file 'smaller.minion'

       minion infile.minion -outputCompressed smaller.minion
    */
    else if (command == string("-outputCompressed")) {
      INCREMENT_i(-outputCompressed);
      getOptions().outputCompressed = argv[i];
    } else if (command == string("-instancestats")) {
      getOptions().instance_stats = true;
    } else if (command == string("-Xgraph")) {
      getOptions().graph = true;
      getOptions().silent = true;
    } else if (command == string("-outputType")) {
      INCREMENT_i(-outputType);
      getOptions().outputType = atoi(argv[i]);
    }
    /** @help switches;-printsols Description
    Print solutions (default).
    */
    else if (command == string("-printsols")) {
      getOptions().print_solution = true;
    }
    /** @help switches;-noprintsols Description
    Do not print solutions.
    */
    else if (command == string("-noprintsols")) {
      getOptions().print_solution = false;
    } else if (command == string("-notimers")) {
      getOptions().noTimers = true;
    }
    /** @help switches;-printsolsonly Description
    Print only solutions and a summary at the end.
    */
    else if (command == string("-printsolsonly")) {
      getOptions().silent = true;
    }
    /** @help switches;-printonlyoptimal Description
    In optimisation problems, only print the optimal value, and
    not intermediate values.
    */
    else if (command == string("-printonlyoptimal")) {
      getOptions().printonlyoptimal = true;
    } else if (command == string("-cspcomp")) {
      getOptions().silent = true;
      getOptions().cspcomp = true;
      in_cspcomp_for_failexit = true;
    }
    /** @help switches;-verbose Description
    Print parser progress.
    */

    /** @help switches;-verbose References
    help switches -quiet
    */
    else if (command == string("-verbose")) {
      getOptions().parser_verbose = true;
    }
    /** @help switches;-prop-node Description
    Allows the user to choose the level of consistency to be enforced
    during search.

    See entry 'help switches -preprocess' for details of the available
    levels of consistency.
    */

    /** @help switches;-prop-node Example
    To enforce SSAC during search:

       minion -prop-node SSAC input.minion
    */

    /** @help switches;-prop-node References
    help switches -preprocess
    */
    else if (command == string("-prop-node") || command == string("-X-prop-node")) {
      INCREMENT_i(-X - prop - node);
      string prop_mode(argv[i]);
      args.prop_method = GetPropMethodFromString(prop_mode);
      if (args.prop_method == PropLevel_None) {
        output_fatal_error("Cannot use 'None' for -prop-node, must propagate at each node!");
      }
    }
    /** @help switches;-map-long-short Description
    Automatically generate a short tuple list from each long tuple list.

    The methods of compression are:

    none : No short tuple list generated (default)
    eager : Use a fast algorithm to produce a reasonable short tuple list (best
    as first choice)
    lazy : Work harder (possibly exponentially) to produce a shorter short tuple
    list
    keeplong : Make a 'short tuple list' with no short tuples (only for
    benchmarking)
    */
    else if (command == string("-map-long-short")) {
      INCREMENT_i(-X - prop - node);
      string prop_mode(argv[i]);
      if (prop_mode == "none")
        getOptions().map_long_short = MLTTS_NoMap;
      else if (prop_mode == "keeplong")
        getOptions().map_long_short = MLTTS_KeepLong;
      else if (prop_mode == "eager")
        getOptions().map_long_short = MLTTS_Eager;
      else if (prop_mode == "lazy")
        getOptions().map_long_short = MLTTS_Lazy;
      else {
        output_fatal_error(" -X-map-long-short <none|keeplong|eager|lazy>");
      }
    }
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
    help switches -prop-node
    */
    else if (command == string("-preprocess")) {
      INCREMENT_i(-preprocess);
      string prop_mode(argv[i]);
      args.preprocess = GetPropMethodFromString(prop_mode);
    }
    /** @help switches;-nocheck Description
    Do not check solutions for correctness before printing them out.
    */

    /** @help switches;-nocheck Notes
    This option is the default on non-DEBUG executables.
    */
    else if (command == string("-nocheck")) {
      getOptions().nocheck = true;
    }
    /** @help switches;-check Description
    Check solutions for correctness before printing them out.
    */

    /** @help switches;-check Notes
    This option is the default for DEBUG executables.
    */
    else if (command == string("-check")) {
      getOptions().nocheck = false;
    }
    /** @help switches;-dumptree Description
    Print out the branching decisions and variable states at each node.
    */
    else if (command == string("-dumptree")) {
      getOptions().dumptree = true;
    }
    /** @help switches;-nodelimit Description
    To stop search after N nodes, do

       minion -nodelimit N myinput.minion
    */

    /** @help switches;-nodelimit References
    help switches -cpulimit
    help switches -timelimit
    help switches -sollimit
    */
    else if (command == string("-nodelimit")) {
      INCREMENT_i(-nodelimit);
      try {
        getOptions().nodelimit = fromstring<long long int>(argv[i]);
        if (getOptions().nodelimit < 0)
          throw "Invalid lower bound";
      } catch (...) {
        cout << "Did not understand parameter to nodelimit:" << argv[i] << endl;
        exit(1);
      }
    }
    /** @help switches;-sollimit Description
    To stop search after N solutions have been found, do

       minion -sollimit N myinput.minion
    */

    /** @help switches;-sollimit References
    help switches -cpulimit
    help switches -nodelimit
    help switches -timelimit
    */
    else if (command == string("-sollimit")) {
      INCREMENT_i(-sollimit);
      try {
        getOptions().sollimit = fromstring<SysInt>(argv[i]);
        if (getOptions().sollimit <= 0)
          throw "Invalid lower bound";
      } catch (...) {
        cout << "Did not understand the parameter to sollimit:" << argv[i] << endl;
        exit(1);
      }
    }
    /** @help switches;-timelimit Description
    To stop search after N seconds (real time), do

       minion -timelimit N myinput.minion
    */

    /** @help switches;-timelimit References
    help switches -cpulimit
    help switches -nodelimit
    help switches -sollimit
    */
    else if (command == string("-timelimit")) {
      INCREMENT_i(-timelimit);
      if (getOptions().timeout_active) {
        cout << "Only one '-cpulimit' or '-timelimit' per instance" << endl;
        exit(1);
      }
      getOptions().timeout_active = true;
      try {
        getOptions().time_limit = fromstring<SysInt>(argv[i]);
        getOptions().time_limit_is_CPU_time = false;
      } catch (...) {
        cout << "Did not understand the parameter to timelimit:" << argv[i] << endl;
        exit(1);
      }
    }
    /** @help switches;-skipautoaux Description
    By default Minion adds all variables to the varorder, to ensure that all
    variables
    are branched assigned before a solution is outputted. This option disables
    that
    behaviour. This means minion Minion may output solutions incorrectly, or
    incorrect
    numbers of solutions. This flag is provided because some users require this
    low-level control over the search, but is in general useless and dangerous.
    In particular,
    it will not speed up search (except when the speed up is due to producing
    garbage of course!)
    */
    else if (command == string("-skipautoaux")) {
      cout << "# WARNING: -skipautoaux can lead to incorrect solutions being "
              "produced\n";
      cout << "# WARNING: This is by design, but use this option with extreme "
              "care\n";
      getOptions().ensure_branch_on_all_vars = false;
    }
    /** @help switches;-cpulimit Description
    To stop search after N seconds (CPU time), do

       minion -cpulimit N myinput.minion
    */

    /** @help switches;-cpulimit References
    help switches -timelimit
    help switches -nodelimit
    help switches -sollimit
    */
    else if (command == string("-cpulimit")) {
      INCREMENT_i(-cpulimit);
      if (getOptions().timeout_active) {
        cout << "Only one '-cpulimit', or '-timelimit' per instance" << endl;
        exit(1);
      }
      getOptions().timeout_active = true;
      try {
        getOptions().time_limit = fromstring<SysInt>(argv[i]);
        getOptions().time_limit_is_CPU_time = true;
      } catch (...) {
        cout << "Did not understand the parameter to cpulimit:" << argv[i] << endl;
        exit(1);
      }
    } // TODO : Should remove -varorder for beta orderings.
    else if (command == string("-varorder")) {
      INCREMENT_i(-varorder);

      string order(argv[i]);

      if (order == "static")
        args.order = ORDER_STATIC;
      else if (order == "srf")
        args.order = ORDER_SRF;
      else if (order == "staticlimited") {
        args.order = ORDER_STATIC_LIMITED;
        INCREMENT_i(staticlimited);
        // Parse an integer for the limit.
        unsigned int tmp;
        std::istringstream iss(argv[i]);
        if (!(iss >> tmp)) {
          output_fatal_error("-varorder staticlimited requires a positive integer value");
        }
        args.limit = tmp;
      } else if (order == "srf-random") {
        args.order = ORDER_SRF;
        getOptions().randomise_valvarorder = true;
      } else if (order == "sdf")
        args.order = ORDER_SDF;
      else if (order == "sdf-random") {
        args.order = ORDER_SDF;
        getOptions().randomise_valvarorder = true;
      } else if (order == "ldf")
        args.order = ORDER_LDF;
      else if (order == "ldf-random") {
        args.order = ORDER_LDF;
        getOptions().randomise_valvarorder = true;
      } else if (order == "random")
        getOptions().randomise_valvarorder = true;
      else if (order == "conflict")
        args.order = ORDER_CONFLICT;
      else if (order == "wdeg") {
        args.order = ORDER_WDEG;
      } else if (order == "domoverwdeg") {
        args.order = ORDER_DOMOVERWDEG;
      } else {
        ostringstream oss;
        oss << "I do not understand the order:" << order;
        output_fatal_error(oss.str());
      }
    }

    /** @help switches;-randomiseorder Description
    Randomises the ordering of the decision variables, and the value ordering.
    If the input file specifies as ordering it will randomly permute this. If no
    ordering is
    specified a random permutation of all the variables is used.
    */
    else if (command == string("-randomiseorder")) {
      getOptions().randomise_valvarorder = true;
    }
    /** @help switches;-randomseed Description
    Set the pseudorandom seed to N. This allows 'random' behaviour to be
    repeated in different runs of minion.
    */
    else if (command == string("-randomseed")) {
      INCREMENT_i(-randomseed);
      args.random_seed = atoi(argv[i]);
    }
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
    else if (command == string("-tableout") || command == string("-tableout0")) {
      getOptions().tableout = true;
      INCREMENT_i(-tableout);
      getTableOut().set_filename(argv[i]);
    }
    /** @help switches;-solsout Description
    Append all solutionsto a named file.
    Each solution is placed on a line, with no extra formatting.
    */

    /** @help switches;-solsout Example
    To add the solutions of myproblem.minion to mysols.txt do

       minion -solsout mysols.txt myproblem.minion
    */
    else if (command == string("-solsout") || command == string("-solsout0")) {
      getOptions().solsoutWrite = true;
      INCREMENT_i(-solsout);
      solsoutFile.open(argv[i], ios::app);
      if (!solsoutFile) {
        ostringstream oss;
        oss << "Cannot open '" << argv[i] << "' for writing.";
        output_fatal_error(oss.str());
      }
    }
    /** @help switches;-makeresume Description
    Write a resume file on timeout or being killed.
    */
    else if (command == string("-makeresume")) {
      getOptions().noresumefile = false;
    }
    /** @help switches;-noresume Description
    Do not write a resume file on timeout or being killed. (default)
    */
    else if (command == string("-noresume")) {
      getOptions().noresumefile = true;
    }
    /** @help switches;-gap Description
    Give name of gap executable (defaults to gap.sh)
    */
    else if (command == string("-gap")) {
      INCREMENT_i(-gap);
      getOptions().gapname = argv[i];
    }
    /** @help switches;-split Description
    When Minion is terminated before the end of search, write out two new input
    files that split the remaining search space in half. Each of the files will
    have
    all the variables and constraints of the original file plus constraints that
    rule out the search already done. In addition, the domain of the variable
    under
    consideration when Minion was stopped is split in half with each of the new
    input files considering a different half.

    This feature is experimental and intended to facilitate parallelisation --
    to
    parallelise the solving of a single constraint problem, stop and split
    repeatedly. Please note that large-scale testing of this feature was limited
    to
    Linux systems and it might not work on others (especially Windows).

    The name of the new input files is composed of the name of the original
    instance, the string 'resume', a timestamp, the process ID of Minion, the
    name
    of the variable whose domain is being split and 0 or 1. Each of the new
    input
    files has a comment identifying the name of the input file which it was
    split
    from. Similarly, Minion's output identifies the new input files it writes
    when
    splitting.

    The new input files can be run without any special flags.

    This flag is intended to be used with the -timelimit, -sollimit, -nodelimit
    or -cpulimit flags. Please note that changing other flags between
    runs (such as -varorder) may have unintended consequences.

    Implies -makeresume.
    */
    else if (command == string("-split")) {
      getOptions().split = true;
      getOptions().noresumefile = false;
      getOptions().splitstderr = false;
    }
    /** @help switches;-split-stderr Description
    The flag -split-stderr has the same function as the flag -split, however the
    two new Minion input files are sent to standard error rather than written to
    files.

    See documentation for -split.
    */
    else if (command == string("-split-stderr")) {
      getOptions().split = true;
      getOptions().noresumefile = false;
      getOptions().splitstderr = true;
    } else if (command[0] == '-' && command != string("--")) {
      cout << "I don't understand '" << command << "'. Sorry. " << endl;
      exit(1);
    } else {
      if (getOptions().instance_name == "")
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
  for (SysInt i = 1; i < argc; ++i) {
    if (i < argc - 1)
      s = s + argv[i] + ",";
    else
      s = s + argv[i];
  }
  getTableOut().set("CommandLineArguments", s);
}
