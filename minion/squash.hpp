
#include <limits>
#include <numeric>
#include <set>
#include <vector>

static const DomainInt freeValue = std::numeric_limits<SysInt>::max();

typedef std::vector<DomainInt> Vint;

template <typename TupleCon>
std::pair<std::set<Vint>, std::set<Vint>>
squeeze_tuples(const TupleCon& tuples, const std::vector<std::set<DomainInt>>& domains,
               bool eager_prune) {
  std::set<Vint> used_tuples;
  std::set<Vint> ret_tuples;
  for(typename TupleCon::iterator it = tuples.begin(); it != tuples.end(); ++it) {
    const std::vector<DomainInt>& tuple = *it;
    if(!eager_prune || used_tuples.count(tuple) == 0) {
      for(SysInt i = 0; i < (SysInt)domains.size(); ++i) {
        if(used_tuples.count(tuple) > 0 && eager_prune)
          break;

        if(tuple[i] == freeValue)
          continue;

        Vint tupleCopy = tuple;
        bool found = true;
        for(std::set<DomainInt>::iterator it = domains[i].begin(); it != domains[i].end(); ++it) {
          DomainInt j = *it;
          tupleCopy[i] = j;
          if(tuples.count(tupleCopy) == 0 || (eager_prune && used_tuples.count(tupleCopy) != 0)) {
            found = false;
            break;
          }
        }
        if(found) {
          for(std::set<DomainInt>::iterator it = domains[i].begin(); it != domains[i].end(); ++it) {
            DomainInt j = *it;
            tupleCopy[i] = j;
            used_tuples.insert(tupleCopy);
          }
          tupleCopy[i] = freeValue;
          // std::cout << tupleCopy << std::endl;
          ret_tuples.insert(tupleCopy);
        }
      }
    }
  }

  std::set<Vint> filtered_tuples;
  for(std::set<Vint>::iterator it = tuples.begin(); it != tuples.end(); ++it) {
    if(used_tuples.count(*it) == 0)
      filtered_tuples.insert(*it);
  }
  return std::make_pair(filtered_tuples, ret_tuples);
}

template <typename Tuples>
std::vector<std::set<DomainInt>> gatherDomains(const Tuples& tuples) {
  if(tuples.size() == 0)
    return std::vector<std::set<DomainInt>>();

  std::vector<std::set<DomainInt>> domains((tuples.begin())->size());

  for(std::set<Vint>::iterator it = tuples.begin(); it != tuples.end(); ++it) {
    for(SysInt i = 0; i < (SysInt)it->size(); ++i) {
      domains[i].insert((*it)[i]);
    }
  }

  return domains;
}

inline std::set<Vint> full_squeeze_tuples(std::set<Vint> tuples,
                                          const std::vector<std::set<DomainInt>>& domainMax,
                                          bool eager) {
  std::set<Vint> constraint;
  while(true) {
    std::pair<std::set<Vint>, std::set<Vint>> pair_ret = squeeze_tuples(tuples, domainMax, eager);
    for(std::set<Vint>::iterator it = pair_ret.first.begin(); it != pair_ret.first.end(); ++it)
      constraint.insert(*it);
    if(pair_ret.second.empty())
      return constraint; //    exit(0);
    tuples = pair_ret.second;
  }
}

inline std::vector<std::vector<std::pair<SysInt, DomainInt>>>
makeShortTupleList(const std::set<Vint>& tuples) {
  std::vector<std::vector<std::pair<SysInt, DomainInt>>> out;
  for(std::set<Vint>::const_iterator it = tuples.begin(); it != tuples.end(); ++it) {
    std::vector<std::pair<SysInt, DomainInt>> shortTup;
    for(SysInt i = 0; i < (SysInt)it->size(); ++i) {
      if((*it)[i] != freeValue) {
        shortTup.push_back(std::make_pair(i, (*it)[i]));
      }
    }
    out.push_back(shortTup);
  }
  return out;
}
