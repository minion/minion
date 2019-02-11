
#include "minion.h"
#include "search_dump.hpp"


class DumpTreeSQL : public SearchDumper
{  
  DumpTreeSQL(const DumpTreeSQL&);

  std::vector<long long> parent_stack;
public:
  DumpTreeSQL()
  { parent_stack.push_back(0); }

  void output_node(long long nodeCount, const std::vector<AnyVarRef>& vars, bool isSolution)
  {
    std::cout << "!!Node id" << nodeCount << std::endl;
    std::cout << "!!Parent id" << parent_stack.back() << std::endl;
    std::cout << "!!is a solution" << isSolution << std::endl;
    std::cout << "!!domains: " << get_dom_as_json(vars).str << std::endl; 
    parent_stack.push_back(nodeCount);
  }

  void backtrack() {
    std::cout << "!!Backtracking!" << std::endl;
    parent_stack.pop_back();
  }

  void branch(long long nodeCount, const std::string& varname, DomainInt val, bool isLeft)
  {
    std::cout << "!!Doing a branch!" << std::endl;
    if(isLeft) {
      std::cout << "!!branching on " << varname << " = " << val << std::endl;
    }
     else {
       std::cout << "!!branching on " << varname << " != " << val << std::endl;
     }
  }

};


std::shared_ptr<SearchDumper> makeDumpTreeSQL()
{ return std::shared_ptr<SearchDumper>(new DumpTreeSQL()); }