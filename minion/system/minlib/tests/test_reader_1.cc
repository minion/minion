#include "minlib/LettingReader/inputfile_parse.h"

int main(void)
{
    SimpleMap<std::string, MultiDimCon<int, int> > lets;
	readLettingStatements(lets, std::string("lettings1.txt"));

	assert(lets.get(std::string("L")).arity() == 0);
	assert(lets.get(std::string("L")).get(make_vec<int>()) == 1);
	assert(lets.get(std::string("Mac")).get(make_vec<int>()) == 2);
}