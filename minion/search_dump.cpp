
#include "minion.h"

#include "search_dump.hpp"

class DumpTreeJson : public SearchDumper {
  DumpTreeJson();
  DumpTreeJson(const DumpTreeJson&);

  JSONStreamer streamer;

public:
  DumpTreeJson(std::ostream* o) : streamer(o) {}

  void initial_variables(const std::vector<AnyVarRef>& vars) {}

  void output_node(long long nodeCount, const std::vector<AnyVarRef>& vars, bool isSolution) {
    streamer.mapElement("Node", getState().getNodeCount());
    streamer.mapElement("Domains", get_dom_as_json(vars));
    streamer.newline();
    if(isSolution) {
      streamer.mapElement("solution", 1);
    }
  }

  void backtrack() {
    streamer.closeMap();
  }

  void branch(long long nodeCount, const std::string& varname, DomainInt val, bool isLeft) {
    if(isLeft) {
      streamer.mapElement("branchVar", varname);
      streamer.mapElement("branchVal", val);
      streamer.openMapWithKey("left");
    } else {
      streamer.openMapWithKey("right");
    }
  }
};

std::shared_ptr<SearchDumper> makeDumpTreeJson(std::ostream* o) {
  return std::shared_ptr<SearchDumper>(new DumpTreeJson(o));
}