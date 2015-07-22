
#include <vector>
#include <set>
#include <numeric>
#include <limits>

static const DomainInt free_value = std::numeric_limits<SysInt>::max();

typedef std::vector<DomainInt> Vint;

template <typename TupleCon>
std::pair<std::set<Vint>, std::set<Vint>>
squeeze_tuples(const TupleCon &tuples, const std::vector<std::set<DomainInt>> &domains,
               bool eager_prune) {
  std::set<Vint> used_tuples;
  std::set<Vint> ret_tuples;
  for (typename TupleCon::iterator it = tuples.begin(); it != tuples.end(); ++it) {
    const std::vector<DomainInt> &tuple = *it;
    if (!eager_prune || used_tuples.count(tuple) == 0) {
      for (SysInt i = 0; i < (SysInt)domains.size(); ++i) {
        if (used_tuples.count(tuple) > 0 && eager_prune)
          break;

        if (tuple[i] == free_value)
          continue;

        Vint tuple_copy = tuple;
        bool found = true;
        for (std::set<DomainInt>::iterator it = domains[i].begin(); it != domains[i].end(); ++it) {
          DomainInt j = *it;
          tuple_copy[i] = j;
          if (tuples.count(tuple_copy) == 0 ||
              (eager_prune && used_tuples.count(tuple_copy) != 0)) {
            found = false;
            break;
          }
        }
        if (found) {
          for (std::set<DomainInt>::iterator it = domains[i].begin(); it != domains[i].end();
               ++it) {
            DomainInt j = *it;
            tuple_copy[i] = j;
            used_tuples.insert(tuple_copy);
          }
          tuple_copy[i] = free_value;
          // std::cout << tuple_copy << std::endl;
          ret_tuples.insert(tuple_copy);
        }
      }
    }
  }

  std::set<Vint> filtered_tuples;
  for (std::set<Vint>::iterator it = tuples.begin(); it != tuples.end(); ++it) {
    if (used_tuples.count(*it) == 0)
      filtered_tuples.insert(*it);
  }
  return std::make_pair(filtered_tuples, ret_tuples);
}

template <typename Tuples>
std::vector<std::set<DomainInt>> gather_domains(const Tuples &tuples) {
  if (tuples.size() == 0)
    return std::vector<std::set<DomainInt>>();

  std::vector<std::set<DomainInt>> domains((tuples.begin())->size());

  for (std::set<Vint>::iterator it = tuples.begin(); it != tuples.end(); ++it) {
    for (SysInt i = 0; i < (SysInt)it->size(); ++i) {
      domains[i].insert((*it)[i]);
    }
  }

  return domains;
}

inline std::set<Vint> full_squeeze_tuples(std::set<Vint> tuples,
                                          const std::vector<std::set<DomainInt>> &domain_max,
                                          bool eager) {
  std::set<Vint> constraint;
  while (true) {
    std::pair<std::set<Vint>, std::set<Vint>> pair_ret = squeeze_tuples(tuples, domain_max, eager);
    for (std::set<Vint>::iterator it = pair_ret.first.begin(); it != pair_ret.first.end(); ++it)
      constraint.insert(*it);
    if (pair_ret.second.empty())
      return constraint; //    exit(0);
    tuples = pair_ret.second;
  }
}

inline std::vector<std::vector<std::pair<SysInt, DomainInt>>>
makeShortTupleList(const std::set<Vint> &tuples) {
  std::vector<std::vector<std::pair<SysInt, DomainInt>>> out;
  for (std::set<Vint>::const_iterator it = tuples.begin(); it != tuples.end(); ++it) {
    std::vector<std::pair<SysInt, DomainInt>> short_tup;
    for (SysInt i = 0; i < (SysInt)it->size(); ++i) {
      if ((*it)[i] != free_value) {
        short_tup.push_back(std::make_pair(i, (*it)[i]));
      }
    }
    out.push_back(short_tup);
  }
  return out;
}
