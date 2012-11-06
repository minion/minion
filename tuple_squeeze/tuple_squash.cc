#include "minlib/minlib.hpp"

static const int free_value = 8;

using Vint=std::vector<int>;

template<typename TupleCon>
std::pair<std::set<Vint>, std::set<Vint>>
squeeze_tuples(const TupleCon& tuples, Vint domain_max, bool eager_prune)
{
    std::set<Vint> used_tuples;
    std::set<Vint> ret_tuples;
    for(const auto& tuple : tuples)
    {
        if(!eager_prune || used_tuples.count(tuple) == 0)
        {
            for(auto i : Range(domain_max.size()))
            {
                if(used_tuples.count(tuple) > 0 && eager_prune)
                    break;

                if(tuple[i] == free_value)
                    continue;

                auto tuple_copy = tuple;
                bool found = true;
                for(auto j : Range(domain_max[i]))
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
                    for(auto j : Range(domain_max[i]))
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
    for(auto tuple : tuples)
    {
        if(used_tuples.count(tuple) == 0)
            filtered_tuples.insert(tuple);
    }
    return std::make_pair(filtered_tuples, ret_tuples);
}

std::set<Vint> make_life_tuples()
{
    std::set<Vint> life;

    // iterate through all the vectors for the outside of the life.
    for(auto outside : ContainerRange(std::vector<int>(8, 2)))
    {
        int i = std::accumulate(outside.begin(), outside.end(), 0);
        if(i == 2)
        {
            auto cpy = outside;
            push_back(cpy, 1, 1);
            life.insert(cpy);
            cpy = outside;
            push_back(cpy, 0, 0);
            life.insert(cpy);
        }
        else if(i == 3)
        {
            auto cpy = outside;
            push_back(cpy, 0, 1);
            life.insert(cpy);
            cpy = outside;
            push_back(cpy, 1, 1);
            life.insert(cpy);
        }
        else
        {
            auto cpy = outside;
            push_back(cpy, 0, 0);
            life.insert(cpy);
            cpy = outside;
            push_back(cpy, 0, 1);
            life.insert(cpy);
        }
    }
    
    return life;
}

void print(std::string name, std::string type, std::set<Vint> tuples)
{
    std::cout << "# " << name << " " << type << "\n";
    std::cout << "# " << tuples.size() << "\n";
    std::cout << "**TUPLELIST**\n";
    Vint output;
    for(auto tuple : tuples)
    {
        for(auto i : Range(tuple.size()))
        {
            if(tuple[i] != free_value)
                push_back(output, i, tuple[i]);
        }
        push_back(output, -1, -1);
    }
    std::cout << "constraint " << 0 << " " << output.size() << "\n";
    for(auto i : output)
        std::cout << i << " ";
    std::cout << "\n";
}

int main(int argc, char** argv)
{
    if(argc < 3)
    {
        std::cout << "prog <life> <N: normal, E: eager, L: lazy>" << std::endl;
        exit(1);
    }

    std::set<Vint> tuples;
    Vint domain_max;
    if(argv[1] == std::string("life"))
    { 
        tuples = make_life_tuples();
        domain_max = Vint(10,2);
    }
    else
    { exit(1); }
    

    if(argv[2] == std::string("N"))
    {
        print(argv[1], argv[2], tuples);
        exit(0);
    }
    
    bool eager = false;
    if(argv[2] == std::string("E"))
        eager = true;
    else if(argv[2] == std::string("L"))
        eager = false;
    else
    {
        std::cerr << "Option 2 = N/E/L\n";
        exit(1);
    }

    std::set<Vint> constraint;
    while(true)
    {
        auto pair_ret = squeeze_tuples(tuples, domain_max, eager);
        std::cout << pair_ret << std::endl;
        for(auto t : pair_ret.first)
            constraint.insert(t);
        if(pair_ret.second.empty())
            break;//    exit(0);
        tuples = pair_ret.second;
    }


    print(argv[1], argv[2], constraint);
}