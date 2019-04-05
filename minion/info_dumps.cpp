#include "minion.h"

#include "BuildVariables.h"

#include "commandline_parse.h"
#include "inputfile_parse/inputfile_parse.h"

#include "MILtools/print_CSP.h"

#include "MILtools/sym_output.h"

void infoDumps(CSPInstance& instance)
{
    
    if(getOptions().graph) {
      GraphBuilder graph(instance);
      // graph.g.output_graph();
      graph.g.output_nauty_graph(instance);
      exit(0);
    }

    if(getOptions().instance_stats) {
      InstanceStats s(instance);
      s.output_stats();

      // Do the minimal amount of setting up to create the constraint objects
      getState().setTupleListContainer(instance.tupleListContainer);
      getState().setShortTupleListContainer(instance.shortTupleListContainer);

      BuildCon::buildVariables(instance.vars);

      // Create Constraints
      vector<AbstractConstraint*> cons;
      while(!instance.constraints.empty()) {
        cons.push_back(build_constraint(instance.constraints.front()));
        instance.constraints.pop_front();
      }

      s.output_stats_tightness(cons);
      exit(0);
    }

    if(getOptions().redump) {
      MinionInstancePrinter printer(instance);
      printer.buildInstance();
      cout << printer.getInstance();
      exit(0);
    }

}