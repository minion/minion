
#include "minion.h"
#include "search_dump.hpp"


class DumpTreeSQL : public SearchDumper
{  
  DumpTreeSQL(const DumpTreeSQL&);

  std::vector<long long> parent_stack;
public:
  DumpTreeSQL()
  { parent_stack.push_back(0); }


  void initial_variables(const std::vector<AnyVarRef>& vars)
  { 
    std::cout << "!!Initial domains" << get_dom_as_json(vars).str << std::endl;
  }

  void output_node(long long nodeCount, const std::vector<AnyVarRef>& vars, bool isSolution)
  {
    std::cout << "!!Node id" << nodeCount << std::endl;
    if(parent_stack.size() > 0)
      std::cout << "!!Parent id" << parent_stack.back() << std::endl;
    std::cout << "!!is a solution" << isSolution << std::endl;
    std::cout << "!!domains: " << get_dom_as_json(vars).str << std::endl; 
  }

  void backtrack() {
    parent_stack.pop_back();
    std::cout << "!!Backtracking to depth " << parent_stack.size() << std::endl;
  }

  void branch(long long nodeCount, const std::string& varname, DomainInt val, bool isLeft)
  {
    std::cout << "!!Doing a branch!" << std::endl;
    if(isLeft) {
      // We do this twice as we will get back here twice, once for left
      // child, once for right child
      parent_stack.push_back(nodeCount);
      parent_stack.push_back(nodeCount);
      std::cout << "!!branching on " << varname << " = " << val << std::endl;
    }
     else {
       std::cout << "!!branching on " << varname << " != " << val << std::endl;
     }
  }

  ~DumpTreeSQL()
  {
    std::cout << "!! Minion is exiting!" << std::endl;
  }

};


std::shared_ptr<SearchDumper> makeDumpTreeSQL()
{ return std::shared_ptr<SearchDumper>(new DumpTreeSQL()); }