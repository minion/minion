
#include "minion.h"
#include "search_dump.hpp"

class DumpTreeSQL : public SearchDumper {
  DumpTreeSQL(const DumpTreeSQL&);

  std::vector<long long> parent_stack;

public:
  DumpTreeSQL() {
    parent_stack.push_back(0);
  }

  void initialVariables(const std::vector<AnyVarRef>& vars) {
    getOutput() << "!!Initial domains" << getDom_as_json(vars).str << std::endl;
  }

  void output_node(long long nodeCount, const std::vector<AnyVarRef>& vars, bool isSolution) {
    getOutput() << "!!Node id" << nodeCount << std::endl;
    if(parent_stack.size() > 0)
      getOutput() << "!!Parent id" << parent_stack.back() << std::endl;
    getOutput() << "!!is a solution" << isSolution << std::endl;
    getOutput() << "!!domains: " << getDom_as_json(vars).str << std::endl;
  }

  void backtrack() {
    parent_stack.pop_back();
    getOutput() << "!!Backtracking to depth " << parent_stack.size() << std::endl;
  }

  void branch(long long nodeCount, const std::string& varname, DomainInt val, bool isLeft) {
    getOutput() << "!!Doing a branch!" << std::endl;
    if(isLeft) {
      // We do this twice as we will get back here twice, once for left
      // child, once for right child
      parent_stack.push_back(nodeCount);
      parent_stack.push_back(nodeCount);
      getOutput() << "!!branching on " << varname << " = " << val << std::endl;
    } else {
      getOutput() << "!!branching on " << varname << " != " << val << std::endl;
    }
  }

  ~DumpTreeSQL() {
    getOutput() << "!! Minion is exiting!" << std::endl;
  }
};

std::shared_ptr<SearchDumper> makeDumpTreeSQL() {
  return std::shared_ptr<SearchDumper>(new DumpTreeSQL());
}