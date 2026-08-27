// A model Minion will not run must come back as an error, not take the host
// process with it.  outputFatalError used to abort(), which killed any
// program embedding the library; it now throws, and runMinion reports
// MINION_INVALID_INSTANCE.
//
// Build:
//   c++ -std=c++14 -I ../minion -I src/ -DLIBMINION -DWDEG -O2 \
//       ../test_instances/test_bad_model.cpp -L. -lminion -lpthread -o test_bad_model

#include "libwrapper.h"
#include "inputfile_parse/CSPSpec.h"
#include "minion.h"

#include <iostream>

using namespace ProbSpec;

int main() {
  MinionContext* ctx = minion_newContext();
  minion_activateContext(ctx);

  CSPInstance instance;
  newVar(instance, "x", VAR_DISCRETE, {1, 5});
  Var x = instance.vars.getSymbol("x");
  instance.print_matrix.push_back({x});
  instance.searchOrder.push_back(SearchOrder({x}, ORDER_STATIC, false));

  // w-inrange takes exactly two values; three is a model Minion refuses.
  ConstraintBlob bad(lib_getConstraint(CT_WATCHED_INRANGE));
  bad.vars.push_back({x});
  bad.constants.push_back({1, 3, 5});
  instance.constraints.push_back(bad);

  SearchOptions options;
  options.findAllSolutions();
  SearchMethod method;

  MinionResult rc = runMinion(ctx, options, method, instance,
                              +[](MinionContext*, void*) -> bool { return true; }, nullptr);

  // Reaching this line at all is most of the test: before, the process died.
  if(rc != MINION_INVALID_INSTANCE) {
    std::cerr << "expected MINION_INVALID_INSTANCE (" << (int)MINION_INVALID_INSTANCE
              << "), got " << (int)rc << std::endl;
    return 1;
  }

  minion_freeContext(ctx);
  std::cout << "1 / 1 tests passed." << std::endl;
  return 0;
}
