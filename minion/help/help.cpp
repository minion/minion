// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#include "minion.h"

#include "help.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

namespace {

const char* const DOCS = "https://minion-solver.readthedocs.io/en/latest/";

struct FlagHelp {
  const char* group;
  const char* name;
  const char* arg;
  const char* summary;
  bool hidden; // true for aliases, which --help lists only under 'all'
};

// Every flag parseCommandLine() accepts, and nothing else.
// sphinxdocs/check-docs-sync.py fails if this list and the parser disagree.
const FlagHelp flagTable[] = {
    {"Search limits", "-nodelimit", "<N>", "stop after N search nodes", false},
    {"Search limits", "-sollimit", "<N>", "stop after N solutions (default 1)", false},
    {"Search limits", "-timelimit", "<N>", "stop after N seconds of wall-clock time", false},
    {"Search limits", "-cpulimit", "<N>", "stop after N seconds of CPU time", false},
    {"Search limits", "-findallsols", "", "find every solution; ignored when optimising", false},

    {"Search", "-varorder", "<order>", "variable ordering; see 'minion --help varorder'", false},
    {"Search", "-valorder", "<order>", "value ordering: ascend, descend or random", false},
    {"Search", "-randomseed", "<N>", "seed for every randomised choice", false},
    {"Search", "-randomiseorder", "", "randomise the variable and value ordering", false},
    {"Search", "-restarts", "", "restart search on a geometric schedule", false},
    {"Search", "-restarts-multiplier", "<F>", "growth factor of that schedule (default 1.5)", false},
    {"Search", "-no-restarts-bias", "", "stop biasing value choice at each restart", false},
    {"Search", "-prop-node", "<level>", "propagation at each node (default GAC)", false},
    {"Search", "-preprocess", "<level>", "propagation applied once before search", false},
    {"Search", "-nocheck", "", "do not re-check solutions (default when optimised)", false},
    {"Search", "-check", "", "re-check each solution against every constraint", false},
    {"Search", "-skipautoaux", "", "do not branch on auxiliary vars (UNSAFE)", false},

    {"Parallel search", "-parallel", "", "search in parallel by forking (beta)", false},
    {"Parallel search", "-cores", "<N>", "processes to fork for -parallel", false},
    {"Parallel search", "-steallow", "", "steal from low in the tree, not high", false},

    {"Output", "-printsols", "", "print each solution (default)", false},
    {"Output", "-noprintsols", "", "do not print solutions, only count them", false},
    {"Output", "-printsolsonly", "", "print solutions and nothing else", false},
    {"Output", "-printonlyoptimal", "", "when optimising, print only the best solution", false},
    {"Output", "-verbose", "", "report progress while parsing the instance", false},
    {"Output", "-quiet", "", "do not report parsing progress (default)", false},
    {"Output", "-solsout", "<file>", "append solutions to a file, one per line", false},
    {"Output", "-jsonsolsout", "<file>", "append solutions to a file as JSON", false},
    {"Output", "-tableout", "<file>", "append a line of run statistics to a file", false},
    {"Output", "-jsontableout", "<file>", "append run statistics to a file as JSON", false},
    {"Output", "-dumptree", "", "print the search tree as it is explored", false},
    {"Output", "-dumptreejson", "<file>", "write the search tree to a file as JSON", false},
    {"Output", "-dumptreesql", "", "print the search tree as SQL inserts", false},

    {"Inspecting an instance (these exit without searching)", "-instancestats", "",
     "print statistics about the instance", false},
    {"Inspecting an instance (these exit without searching)", "-redump", "",
     "print the instance back out in Minion format", false},
    {"Inspecting an instance (these exit without searching)", "-Xgraph", "",
     "print the constraint graph in nauty format", false},
    {"Inspecting an instance (these exit without searching)", "-outputCompressed", "<name>",
     "print the instance with variables renamed", false},
    {"Inspecting an instance (these exit without searching)", "-outputCompressedDomains", "",
     "as -outputCompressed, also shrinking domains", false},

    {"Stopping and resuming", "-makeresume", "", "on hitting a limit, write a resume instance",
     false},
    {"Stopping and resuming", "-noresume", "", "do not write a resume instance (default)", false},
    {"Stopping and resuming", "-split", "", "on hitting a limit, write two instances that\n"
                                            "split the remaining search between them", false},
    {"Stopping and resuming", "-split-stderr", "", "as -split, to stderr rather than to files",
     false},
    {"Stopping and resuming", "-command-list", "<in> <out>", "drive search from a command file",
     false},

    {"Other", "-gap", "<file>", "the GAP executable to call (default gap.sh)", false},
    {"Other", "-map-long-short", "<mode>", "map long/short tuples: none, keeplong,\n"
                                           "eager or lazy", false},

    {"Experimental (these interfaces may change without notice)", "-X-parallelThreads", "<N>",
     "portfolio search across N OS threads", false},
    {"Experimental (these interfaces may change without notice)", "-X-parallelWorkSteal", "<N>",
     "work-stealing search across N OS threads", false},
    {"Experimental (these interfaces may change without notice)", "-X-parallelWorkStealPortfolio",
     "", "give each work-steal worker its own heuristic", false},
    {"Experimental (these interfaces may change without notice)", "-X-parallelPreprocess", "[N]",
     "run SAC preprocessing in N processes", false},
    {"Experimental (these interfaces may change without notice)", "-X-AMO", "",
     "gather at-most-one constraints", false},
    {"Experimental (these interfaces may change without notice)", "-X-AMO-extra", "",
     "as -X-AMO, over more constraint types", false},
    {"Experimental (these interfaces may change without notice)", "-X-tabulation", "",
     "replace small constraints with table constraints", false},

    {"Aliases", "-X-instancestats", "", "alias for -instancestats", true},
    {"Aliases", "-X-prop-node", "<level>", "alias for -prop-node", true},
    {"Aliases", "-solsout0", "<file>", "alias for -solsout", true},
    {"Aliases", "-tableout0", "<file>", "alias for -tableout", true},
};

const int flagCount = sizeof(flagTable) / sizeof(FlagHelp);

// The variable orderings -varorder accepts, and what they mean.
const char* const varOrders[][2] = {
    {"sdf", "smallest domain first, ties broken lexicographically"},
    {"sdf-random", "smallest domain first, ties broken randomly"},
    {"srf", "smallest proportion of the initial domain left first"},
    {"srf-random", "as srf, ties broken randomly"},
    {"ldf", "largest domain first, ties broken lexicographically"},
    {"ldf-random", "largest domain first, ties broken randomly"},
    {"static", "the order the variables were declared in"},
    {"random", "a random order"},
    {"conflict", "conflict-directed ordering"},
    {"wdeg", "weighted degree"},
    {"domoverwdeg", "domain size divided by weighted degree"},
};

/// The anchor Sphinx generates for a heading: lower case, with each run of
/// characters that are not letters or digits replaced by a single '-'.
std::string docAnchor(const std::string& name) {
  std::string out;
  for(char c : name) {
    if((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
      out += c;
    else if(c >= 'A' && c <= 'Z')
      out += (char)(c - 'A' + 'a');
    else if(!out.empty() && out[out.size() - 1] != '-')
      out += '-';
  }
  while(!out.empty() && out[out.size() - 1] == '-')
    out.erase(out.size() - 1);
  return out;
}

/// Levenshtein distance, used only to suggest a flag when one is misspelled.
int editDistance(const std::string& a, const std::string& b) {
  std::vector<int> prev(b.size() + 1), cur(b.size() + 1);
  for(size_t j = 0; j <= b.size(); ++j)
    prev[j] = (int)j;
  for(size_t i = 1; i <= a.size(); ++i) {
    cur[0] = (int)i;
    for(size_t j = 1; j <= b.size(); ++j) {
      int sub = prev[j - 1] + (a[i - 1] == b[j - 1] ? 0 : 1);
      cur[j] = std::min(sub, std::min(prev[j] + 1, cur[j - 1] + 1));
    }
    prev = cur;
  }
  return prev[b.size()];
}

std::string flagLabel(const FlagHelp& f) {
  std::string label = f.name;
  if(f.arg[0] != '\0')
    label += std::string(" ") + f.arg;
  return label;
}

/// Prints one flag, continuing an embedded newline in the summary under the
/// first line rather than back in the left-hand column.
void printFlag(const FlagHelp& f, size_t column) {
  std::string label = flagLabel(f);
  getOutput() << "  " << label;
  if(label.size() < column)
    getOutput() << std::string(column - label.size(), ' ');
  else
    getOutput() << "\n  " << std::string(column, ' ');

  std::string summary = f.summary;
  size_t start = 0;
  while(true) {
    size_t nl = summary.find('\n', start);
    if(nl == std::string::npos)
      break;
    getOutput() << summary.substr(start, nl - start) << "\n  "
                << std::string(column, ' ');
    start = nl + 1;
  }
  getOutput() << summary.substr(start) << "\n";
}

void printFlagTable(bool includeHidden) {
  size_t column = 0;
  for(int i = 0; i < flagCount; ++i)
    if(includeHidden || !flagTable[i].hidden)
      column = std::max(column, flagLabel(flagTable[i]).size() + 2);

  std::string group;
  for(int i = 0; i < flagCount; ++i) {
    if(!includeHidden && flagTable[i].hidden)
      continue;
    if(group != flagTable[i].group) {
      group = flagTable[i].group;
      getOutput() << "\n" << group << "\n";
    }
    printFlag(flagTable[i], column);
  }
}

void printConstraints() {
  std::vector<std::string> names;
  for(SysInt i = 0; i < numOfConstraints; ++i) {
    // Constraints the solver builds for itself; they cannot be written in an
    // input file, so there is nothing to document.
    if(constraint_list[i].name.compare(0, 2, "__") == 0 ||
       constraint_list[i].name.find('(') != std::string::npos)
      continue;
    names.push_back(constraint_list[i].name);
  }
  std::sort(names.begin(), names.end());

  getOutput() << "This copy of Minion supports " << names.size() << " constraints:\n\n";

  // Laid out down the columns, so the names stay alphabetical when read the
  // way a list is read.  One constraint name is much longer than the rest, so
  // each column is measured separately rather than all sharing one width.
  const size_t total = names.size();
  size_t columns = 1, rows = total;
  for(size_t tryColumns = 2; tryColumns <= 6; ++tryColumns) {
    const size_t tryRows = (total + tryColumns - 1) / tryColumns;
    size_t line = 2;
    for(size_t c = 0; c < tryColumns; ++c) {
      size_t widest = 0;
      for(size_t r = 0; r < tryRows; ++r) {
        const size_t index = c * tryRows + r;
        if(index < total)
          widest = std::max(widest, names[index].size());
      }
      line += widest + 2;
    }
    if(line > 80)
      break;
    columns = tryColumns;
    rows = tryRows;
  }

  for(size_t r = 0; r < rows; ++r) {
    getOutput() << "  ";
    for(size_t c = 0; c < columns; ++c) {
      const size_t index = c * rows + r;
      if(index >= total)
        break;
      size_t widest = 0;
      for(size_t r2 = 0; r2 < rows; ++r2)
        if(c * rows + r2 < total)
          widest = std::max(widest, names[c * rows + r2].size());
      getOutput() << names[index];
      if(c + 1 < columns && index + rows < total)
        getOutput() << std::string(widest - names[index].size() + 2, ' ');
    }
    getOutput() << "\n";
  }

  getOutput() << "\nFor one constraint, name it:\n"
              << "  minion --help gacalldiff\n"
              << "\nWhat they all mean, and which to prefer:\n"
              << "  " << DOCS << "usage/constraints.html\n";
}

void printVarOrders() {
  getOutput() << "Orderings accepted by -varorder:\n\n";
  const int count = sizeof(varOrders) / sizeof(varOrders[0]);
  for(int i = 0; i < count; ++i)
    getOutput() << "  " << varOrders[i][0]
                << std::string(14 - std::min<size_t>(13, strlen(varOrders[i][0])), ' ')
                << varOrders[i][1] << "\n";
  getOutput() << "\nwdeg and domoverwdeg need a build configured with --wdeg; this one is "
#ifdef WDEG
              << "such a build.\n"
#else
              << "not.\n"
#endif
              << "\nThe value ordering is set separately: minion --help valorder\n";
}

} // namespace

void printVersion() {
  getOutput() << MinionVersion << "\n"
              << "Git version: " << tostring(GIT_VER) << "\n";
}

void printUsageBanner(const char* argv0) {
  getOutput() << "Usage: " << argv0 << " [switch]... <input file>\n"
              << "Run '" << argv0 << " --help' for the list of switches.\n";
}

std::string suggestFlag(const std::string& flag) {
  std::string best;
  int bestDistance = 0;
  for(int i = 0; i < flagCount; ++i) {
    int d = editDistance(flag, flagTable[i].name);
    if(best.empty() || d < bestDistance) {
      best = flagTable[i].name;
      bestDistance = d;
    }
  }
  // Two edits on a short flag is already more guess than suggestion.
  if(bestDistance > 2 || bestDistance * 3 > (int)flag.size())
    return "";
  return best;
}

bool help(const std::string& topic) {
  if(topic == "constraints") {
    printConstraints();
    return true;
  }
  if(topic == "varorder") {
    printVarOrders();
    return true;
  }
  if(topic == "valorder") {
    getOutput() << "Orderings accepted by -valorder:\n\n"
                << "  ascend        try the smallest value in the domain first\n"
                << "  descend       try the largest value in the domain first\n"
                << "  random        try the values in a random order\n"
                << "\n-valorder overrides any VALORDER in the input file, and applies to\n"
                << "every search variable.  With neither, values are tried in ascending\n"
                << "order.\n"
                << "\nThe variable ordering is set separately: minion --help varorder\n";
    return true;
  }
  if(topic == "variables") {
    getOutput() << "Minion has four variable types -- BOOL, DISCRETE, BOUND and SPARSEBOUND --\n"
                << "and the choice between them affects both speed and the size of the search.\n"
                << "  " << DOCS << "usage/variables.html\n";
    return true;
  }
  if(topic == "input") {
    getOutput() << "The .minion file format, section by section.\n"
                << "  " << DOCS << "usage/input.html\n";
    return true;
  }
  if(topic == "examples") {
    getOutput() << "Worked models: the farmers problem, SEND+MORE=MONEY, the zebra puzzle,\n"
                << "n-queens, and a graceful graph labelling.\n"
                << "  " << DOCS << "usingminion.html\n";
    return true;
  }

  // 'minion --help alldiff' -- anything this build knows as a constraint.
  for(SysInt i = 0; i < numOfConstraints; ++i) {
    if(constraint_list[i].name != topic)
      continue;
    getOutput() << topic << " is a constraint, taking " << constraint_list[i].numberOfParams
                << " argument" << (constraint_list[i].numberOfParams == 1 ? "" : "s") << ".\n"
                << "  " << DOCS << "usage/constraints.html#" << docAnchor(topic) << "\n";
    return true;
  }

  if(!topic.empty() && topic != "all" && topic != "switches") {
    std::cerr << "There is no help topic '" << topic << "'.\n"
              << "Topics: constraints, varorder, valorder, variables, input, examples, all,\n"
              << "or the name of any constraint (see 'minion --help constraints').\n";
    return false;
  }

  printVersion();
  getOutput() << "\nUsage: minion [switch]... <input file>\n"
              << "       minion --help [topic]\n";

  printFlagTable(topic == "all");

  getOutput() << "\nHelp topics\n"
              << "  minion --help constraints    the constraints this copy supports\n"
              << "  minion --help <constraint>   one constraint, by name\n"
              << "  minion --help varorder       the variable orderings -varorder takes\n"
              << "  minion --help valorder       the value orderings -valorder takes\n"
              << "  minion --help variables      the four variable types\n"
              << "  minion --help input          the .minion file format\n"
              << "  minion --help examples       worked models\n";
  if(topic != "all")
    getOutput() << "  minion --help all            as above, plus deprecated aliases\n";

  getOutput() << "\nFull documentation: " << DOCS << "\n";
  return true;
}
