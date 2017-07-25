// This should be kept in sync with CSPSpec.h

#include "../inputfile_parse/CSPSpec.h"
#include "../system/system.h"
#include "../variables/AnyVarRef.h"
#include <algorithm>
#ifndef MINION_NEIGHBOURHOOD_DEF_H
#define MINION_NEIGHBOURHOOD_DEF_H

#ifdef DEBUG_MODE
#define debug_log(x) std::cout << x << std::endl;
#define debug_code(x) x
#else
#define debug_log(x)  // nothing
#define debug_code(x) // nothing
#endif
struct NeighbourhoodGroup {
  std::string name;
  std::vector<AnyVarRef> vars;
  std::vector<int> neighbourhoodIndexes;
  NeighbourhoodGroup() {}
  NeighbourhoodGroup(const std::string& name, const ParsedNeighbourhoodGroup& p)
      : name(name), vars(get_AnyVarRef_from_Var(p.vars)) {}
};

struct Neighbourhood {
  enum NeighbourhoodType { STANDARD, OPEN, CLOSED };
  std::string name;
  AnyVarRef activation;
  AnyVarRef deviation;
  std::vector<AnyVarRef> vars;
  std::shared_ptr<NeighbourhoodGroup> group;
  NeighbourhoodType type = STANDARD;
  Neighbourhood(const ParsedNeighbourhood& p, std::shared_ptr<NeighbourhoodGroup>& g)
      : name(p.name),
        activation(get_AnyVarRef_from_Var(p.activation)),
        deviation(get_AnyVarRef_from_Var(p.deviation)),
        vars(get_AnyVarRef_from_Var(p.vars)),
        group(g) {}

  Neighbourhood(NeighbourhoodType type, std::shared_ptr<NeighbourhoodGroup>& g)
      : group(g), type(type) {
    if(type == OPEN) {
      name = "open(" + group->name + ")";
    } else if(type == CLOSED) {
      name = "closed(" + group->name + ")";
    }
  }

  size_t getNumberVariables() {
    return 1 + vars.size() + group->vars.size();
  }
};

template <typename Container, typename Func>
inline void printNames(const Container& items, const Func& key) {
  std::cout << "[";
  bool first = true;
  for(auto item : items) {
    if(first) {
      first = false;
    } else {
      std::cout << ",";
    }
    std::cout << key(item);
  }
  std::cout << "]";
}
struct NeighbourhoodContainer {
  std::vector<std::vector<AnyVarRef>> shadow_mapping;
  std::unordered_map<AnyVarRef, AnyVarRef> shadowLookup;
  AnyVarRef shadow_disable;
  std::unordered_map<std::string, std::shared_ptr<NeighbourhoodGroup>> neighbourhoodGroups;
  typedef decltype(*neighbourhoodGroups.begin()) NGMapPair;
  std::vector<Neighbourhood> neighbourhoods;
  std::vector<std::vector<int>> neighbourhoodCombinations;

  NeighbourhoodContainer(const ParsedNeighbourhoodContainer& p)
      : shadow_mapping(get_AnyVarRef_from_Var(p.shadow_mapping)),
        shadow_disable(get_AnyVarRef_from_Var(p.shadow_disable)) {
    // buil shadow lookup
    for(int i = 0; i < shadow_mapping[0].size(); ++i) {
      shadowLookup.emplace(shadow_mapping[0][i], shadow_mapping[1][i]);
    }
    // build groups, insuring all groups have the open and closed neighbourhoods
    std::vector<std::shared_ptr<NeighbourhoodGroup>> groups;
    for(auto& g : p.neighbourhoodGroups) {
      auto nhGroupPtr =
          neighbourhoodGroups
              .emplace(g.first, std::make_shared<NeighbourhoodGroup>(g.first, g.second))
              .first->second;
      neighbourhoods.emplace_back(Neighbourhood::CLOSED, nhGroupPtr);
      nhGroupPtr->neighbourhoodIndexes.push_back(neighbourhoods.size() - 1);
      neighbourhoods.emplace_back(Neighbourhood::OPEN, nhGroupPtr);
      nhGroupPtr->neighbourhoodIndexes.push_back(neighbourhoods.size() - 1);
      groups.push_back(nhGroupPtr);
    }
    // build neighbourhoods, adding their indices to their parent group
    for(const ParsedNeighbourhood& parsedNH : p.neighbourhoods) {
      auto nhGroupPtr = neighbourhoodGroups[parsedNH.groupName];
      neighbourhoods.emplace_back(parsedNH, nhGroupPtr);
      nhGroupPtr->neighbourhoodIndexes.push_back(neighbourhoods.size() - 1);
    }
    std::vector<int> currentCombination;
    buildCombinations(groups, currentCombination);
    std::cout << "\nParsed groups:\n";
    printNames(neighbourhoodGroups, [](NGMapPair& g) { return g.first; });
    std::cout << "\nParsed neighbourhoods:\n";
    printNames(neighbourhoods, [](const Neighbourhood& n) { return n.name; });
    std::cout << "\nNumber neighbourhood combinations: " << neighbourhoodCombinations.size()
              << "\n";
  }

  inline void buildCombinations(const std::vector<std::shared_ptr<NeighbourhoodGroup>>& groups,
                                std::vector<int>& currentCombination) {
    int currentGroup = currentCombination.size();
    if(currentGroup == groups.size()) {
      // push this combination multiple times, each with a different primary neighbourhood
      neighbourhoodCombinations.push_back(currentCombination);
      for(int i = 1; i < groups.size(); ++i) {
        if(neighbourhoods[currentCombination[i]].type != Neighbourhood::STANDARD) {
          continue;
        }
        neighbourhoodCombinations.push_back(currentCombination);
        std::swap(neighbourhoodCombinations.back()[0], neighbourhoodCombinations.back()[i]);
      }
      return;
    }
    for(int neighbourhoodIndex : groups[currentGroup]->neighbourhoodIndexes) {
      if(currentGroup == 0 && neighbourhoods[neighbourhoodIndex].type != Neighbourhood::STANDARD) {
        continue;
      }
      currentCombination.push_back(neighbourhoodIndex);
      buildCombinations(groups, currentCombination);
      currentCombination.pop_back();
    }
    std::random_shuffle(neighbourhoodCombinations.begin(), neighbourhoodCombinations.end());
  }

  inline bool isCombinationEnabled(int combIndex) const {
    const std::vector<int>& combination = neighbourhoodCombinations[combIndex];
    for(const int nh : combination) {
      if(neighbourhoods[nh].type == Neighbourhood::STANDARD &&
         !neighbourhoods[nh].activation.inDomain(1)) {
        return false;
      }
    }
    return true;
  }

  int getMaxNeighbourhoodSize() {
    auto nhIter =
        std::find_if(neighbourhoods.begin(), neighbourhoods.end(),
                     [](const Neighbourhood& n) { return n.type == Neighbourhood::STANDARD; });
    if(nhIter == neighbourhoods.end()) {
      D_FATAL_ERROR("Could not find any standard neighbourhoods.");
    }
    for(auto nhIter2 = nhIter + 1; nhIter2 != neighbourhoods.end(); ++nhIter2) {
      if(nhIter2->type == Neighbourhood::STANDARD &&
         nhIter2->deviation.getMax() > nhIter->deviation.getMax()) {
        nhIter = nhIter2;
      }
    }
    return checked_cast<int>(nhIter->deviation.getMax());
  }
};

#endif
