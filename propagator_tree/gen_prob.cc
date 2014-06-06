#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <vector>
#include <set>
#include <iostream>
#include <ostream>
using namespace std;

int main(void)
{
	int tuple_count = 20;
	int tuple_size = 5;
	int dom_size = 2;
	set<vector<int> > constraint;
	
	while(constraint.size() != 20)
	{
		vector<int> tuple;
		for(int i = 0; i < tuple_size; ++i)
			tuple.push_back(rand() % dom_size);
		constraint.insert(tuple);
	}
	
	
	cout << "MINION 3\n";
	cout << "**TUPLELIST**\n";
	cout << tuple_count << " " << tuple_size << endl;
	
}