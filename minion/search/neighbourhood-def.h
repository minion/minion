// This should be kept in sync with CSPSpec.h

#include "../inputfile_parse/CSPSpec.h"
#include "../system/system.h"
#include "../variables/AnyVarRef.h"

#ifndef MINION_NEIGHBOURHOOD_DEF_H
#define MINION_NEIGHBOURHOOD_DEF_H

#ifdef DEBUG_MODE
#define debug_log(x) std::cout << x << std::endl;
#define debug_code(x) x
#else
#define debug_log(x)  // nothing
#define debug_code(x) // nothing
#endif

struct Neighbourhood {
  std::string name;
  AnyVarRef activation;
  AnyVarRef deviation;
  std::vector<AnyVarRef> vars;
  int numberOfRuns;
  int currentValue;

  Neighbourhood(const ParsedNeighbourhood& p)
      : name(p.name),
        activation(get_AnyVarRef_from_Var(p.activation)),
        deviation(get_AnyVarRef_from_Var(p.deviation)),
        vars(get_AnyVarRef_from_Var(p.vars)),
        numberOfRuns(0),
        currentValue(0) {}
};

struct NeighbourhoodContainer {
  AnyVarRef soft_violation_count;
  std::vector<std::vector<AnyVarRef>> shadow_mapping;
  AnyVarRef shadow_disable;
  std::vector<Neighbourhood> neighbourhoods;
  NeighbourhoodContainer(const ParsedNeighbourhoodContainer& p)
      : soft_violation_count(get_AnyVarRef_from_Var(p.soft_violation_count)),
        shadow_mapping(get_AnyVarRef_from_Var(p.shadow_mapping)),
        shadow_disable(get_AnyVarRef_from_Var(p.shadow_disable)) {
    for(auto& n : p.neighbourhoods) {
      neighbourhoods.push_back(Neighbourhood(n));
    }
  }
};

#endif