#include "minlib/minlib.hpp"

typedef int SysInt;
typedef int DomainInt;

bool randomise_tuples = false;

#include "squash.hpp"






std::set<Vint> make_life_tuples()
{
    std::set<Vint> life;

    // iterate through all the vectors for the outside of the life.
    for(const auto& outside : ContainerRange(std::vector<int>(8, 2)))
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
            push_back(cpy, 1, 0);
            life.insert(cpy);
        }
    }
    return life;
}

std::set<Vint> make_immigration_tuples()
{
    std::set<Vint> life;

    // iterate through all the vectors for the outside of the life.
    for(const auto& outside : ContainerRange(std::vector<int>(8, 3)))
    {
        int i = 0;
        int count1 = 0;
        int count2 = 0;
        for(auto j : outside)
        {
            if(j > 0) i++;
            if(j == 1) count1++;
            if(j == 2) count2++;
        }

        int new_val = (count1 > count2) ? 1 : 2;

        if(i == 2)
        {
            for(auto j : Range(3))
            {
                auto cpy = outside;
                push_back(cpy, j, j);
                life.insert(cpy);
            }
        }
        else if(i == 3)
        {
            auto cpy = outside;
            push_back(cpy, 0, new_val);
            life.insert(cpy);
            cpy = outside;
            push_back(cpy, 1, 1);
            life.insert(cpy);
            cpy = outside;
            push_back(cpy, 2, 2);
            life.insert(cpy);
               
        }
        else
        {
            for(auto j : Range(3))
            {
                auto cpy = outside;
                push_back(cpy, 0, j);
                life.insert(cpy);
            }
        }
    }
    return life;
}

std::set<Vint> make_quadlife_tuples()
{
    std::set<Vint> life;

    // iterate through all the vectors for the outside of the life.
    for(const auto& outside : ContainerRange(std::vector<int>(8, 5)))
    {
        int i = 0;
        int count1 = 0;
        int count2 = 0;
        int count3 = 0;
        int count4 = 0;
        for(auto j : outside)
        {
            if(j > 0) i++;
            if(j == 1) count1++;
            if(j == 2) count2++;
            if(j == 3) count3++;
            if(j == 4) count4++;
        }

        int newval = -1;

        if(count1 > 1)
            newval = 1;
        else if(count2 > 1)
            newval = 2;
        else if(count3 > 1)
            newval = 3;
        else if(count4 > 1)
            newval = 4;
        else if(count1 == 0)
            newval = 1;
        else if(count2 == 0)
            newval = 2;
        else if(count3 == 0)
            newval = 3;
        else if(count4 == 0)
            newval = 4;
        else assert(i != 3);

        if(i == 2)
        {
            for(auto j : Range(5))
            {
                auto cpy = outside;
                push_back(cpy, j, j);
                life.insert(cpy);
            }
        }
        else if(i == 3)
        {
            auto cpy = outside;
            push_back(cpy, 0, newval);
            life.insert(cpy);
            for(auto j : Range(1, 5))
            {
                cpy = outside;
                push_back(cpy, 1, j);
                life.insert(cpy);
            }
               
        }
        else
        {
            for(auto j : Range(5))
            {
                auto cpy = outside;
                push_back(cpy, 0, j);
                life.insert(cpy);
            }
        }
    }
    return life;
}

std::set<Vint> make_brain_tuples()
{
    std::set<Vint> life;

    // iterate through all the vectors for the outside of the life.
    for(const auto& outside : ContainerRange(std::vector<int>(8, 3)))
    {
        int sum = 0;
        for(auto i : outside)
            if(i == 1) sum++;

        if(sum == 2)
        {
            auto cpy = outside;
            push_back(cpy, 0, 1);
            life.insert(cpy);
        }
        else
        {
            auto cpy = outside;
            push_back(cpy, 0, 0);
            life.insert(cpy);
        }

        {
            auto cpy = outside;
            push_back(cpy, 1, 2);
            life.insert(cpy);
            cpy = outside;
            push_back(cpy, 2, 0);
            life.insert(cpy);
        }
    }
    return life;
}




void print(std::string name, std::string type, const std::set<Vint>& tuples)
{
    std::cout << "# " << name << " " << type << "\n";
    std::cout << "# " << tuples.size() << "\n";
    std::cout << "**SHORTTUPLELIST**\n";
    std::cout << "constraint " << " " << tuples.size() << "\n";

    for(auto tuple : tuples)
    {
        std::cout << "[";
        bool first = true;
        for(auto i : Range(tuple.size()))
        {
            if(tuple[i] != free_value)
            {
                if(first) first=false; else std::cout << ", ";
                std::cout << "(" << i << "," << tuple[i] << ")";
            }
        }
        std::cout << "]\n";
    }
    std::cout << "\n";
}

int main(int argc, char** argv)
{
    if(argc != 4)
    {
        std::cout << "prog <life|briansbrain|immigration|quadlife> <N: normal, E: eager, L: lazy> <random seed>" << std::endl;
        exit(1);
    }

    std::set<Vint> tuples;
    std::vector<std::set<int>> domain_max;
    if(argv[1] == std::string("life"))
    { 
        tuples = make_life_tuples();
        domain_max = std::vector<std::set<int>>(10,make_set(0,1));
    }
    else if(argv[1] == std::string("briansbrain"))
    {
        tuples = make_brain_tuples();
        domain_max = std::vector<std::set<int>>(10,make_set(0,1,2));
    }
    else if(argv[1] == std::string("immigration"))
    {
        tuples = make_immigration_tuples();
        domain_max = std::vector<std::set<int>>(10,make_set(0,1,2));
    }
    else if(argv[1] == std::string("quadlife"))
    {
        tuples = make_quadlife_tuples();
        domain_max = std::vector<std::set<int>>(10,make_set(0,1,2,3,4));
    }
    else
    { exit(1); }
    

    if(argv[2] == std::string("N"))
    {
        print(argv[1], argv[2], tuples);
        exit(0);
    }

    srand(fromstring<int>(std::string(argv[3])));
    if(fromstring<int>(std::string(argv[3])) > 0)
        randomise_tuples = true;
    
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
        for(auto t : pair_ret.first)
            constraint.insert(t);
        if(pair_ret.second.empty())
            break;//    exit(0);
        tuples = pair_ret.second;
    }

    print(argv[1], argv[2], constraint);
}