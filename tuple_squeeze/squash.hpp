#include "minlib/minlib.hpp"


typedef int SysInt;
typedef int DomainInt;

#include <vector>
#include <set>
#include <numeric>

static const int free_value = std::numeric_limits<int>::max();

typedef std::vector<int> Vint;

template<typename TupleCon>
std::pair<std::set<Vint>, std::set<Vint>>
squeeze_tuples(const TupleCon& tuples, const std::vector<std::set<int>>& domains, bool eager_prune)
{
    std::set<Vint> used_tuples;
    std::set<Vint> ret_tuples;
    for(const auto& tuple : tuples)
    {
        if(!eager_prune || used_tuples.count(tuple) == 0)
        {
            for(auto i : Range(domains.size()))
            {
                if(used_tuples.count(tuple) > 0 && eager_prune)
                    break;

                if(tuple[i] == free_value)
                    continue;

                auto tuple_copy = tuple;
                bool found = true;
                for(auto j : domains[i])
                {
                    tuple_copy[i] = j;
                    if(tuples.count(tuple_copy) == 0 || (eager_prune && used_tuples.count(tuple_copy) != 0))
                    {
                        found = false;
                        break;
                    }
                }
                if(found)
                {
                    for(auto j : domains[i])
                    {
                        tuple_copy[i] = j;
                        used_tuples.insert(tuple_copy);
                    }
                    tuple_copy[i] = free_value;
                    //std::cout << tuple_copy << std::endl;
                    ret_tuples.insert(tuple_copy);
                }
            }
        }
    }

    std::set<Vint> filtered_tuples;
    for(const auto& tuple : tuples)
    {
        if(used_tuples.count(tuple) == 0)
            filtered_tuples.insert(tuple);
    }
    return std::make_pair(filtered_tuples, ret_tuples);
}

template<typename Tuples>
std::vector<std::set<int>> gather_domains(const Tuples& tuples)
{
    if(tuples.size() == 0)
        return std::vector<std::set<int>>{};
    
    std::vector<std::set<int>> domains(tuples[0].size());

    for(auto& t: tuples)
    {
        for(auto i : Range(t.size()))
        {
            domains[i].insert(t[i]);
        }
    }

    return domains;
}

inline std::set<Vint>
full_squeeze_tuples(std::set<Vint> tuples, const std::vector<std::set<int>>& domain_max, bool eager)
{
    std::set<Vint> constraint;
    while(true)
    {
        auto pair_ret = squeeze_tuples(tuples, domain_max, eager);
        for(auto t : pair_ret.first)
            constraint.insert(t);
        if(pair_ret.second.empty())
            return constraint;//    exit(0);
        tuples = pair_ret.second;
    }
}

inline std::vector<std::vector<std::pair<SysInt,DomainInt>>>
makeShortTupleList(const std::set<Vint>& tuples)
{
    std::vector<std::vector<std::pair<SysInt,DomainInt>>> out;
    for(auto& t : tuples)
    {
        std::vector<std::pair<SysInt,DomainInt>> short_tup;
        for(auto i : Range(t.size()))
        {
            if(t[i] != free_value)
            {
                short_tup.push_back(std::make_pair(i,t[i]));
            }
        }
    }
    return out;
}