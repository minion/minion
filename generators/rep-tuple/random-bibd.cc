#include<algorithm>
#include<numeric>
#include<vector>
#include<cassert>
#include<iostream>
#include<cmath>

using namespace std;


struct sum_function
{
  int sum;
  sum_function(int _sum) : sum(_sum) {}
  
  bool operator()(vector<bool> b)
  { return accumulate(b.begin(), b.end(), 0) == sum; }
  
};

struct in_set
{
  int val;
  in_set(int _val) : val(_val) {}
  
  bool operator()(vector<bool> b)
  { return b[val]; }
};

struct not_in_set
{
  int val;
  not_in_set(int _val) : val(_val) {}
  
  bool operator()(vector<bool> b)
  { return !b[val]; }
};


struct scalar_product
{
  int scalar;
  scalar_product(int _scalar) : scalar(_scalar) {}
  
  bool operator()(vector<bool> b)
  {
	int half_size = (b.end() - b.begin()) / 2;
	return scalar == (inner_product(b.begin(), b.begin() + half_size,
									b.begin() + half_size, 0));
  }
};

/* Variable layout:
*
*  v * b : basic matrix.
*  v * b : auxillery variables for row sum.
*  v * b : auxillert variables for column sum.
*/

int v,b,r,k,l;

unsigned intpow(unsigned num, unsigned pow)
{
  unsigned result = 1;
  for(unsigned i = 0; i < pow; ++i)
	result *= num;
  return result;
}

vector<bool> int_to_vec(unsigned input, int tuple_size)
{
  assert(input < intpow(2,tuple_size));
  vector<bool> result(tuple_size);
  
  for(int i = tuple_size - 1; i >= 0; i--)
  {
	if(input >= intpow(2,i))
	{
	  input -= intpow(2,i);
	  result[i] = 1;
	}
	else
	{
	  result[i] = 0;
	}
  }
  assert(input == 0);
  reverse(result.begin(), result.end());
  return result;
}

struct Rep
{
  vector<vector<bool> > function;
  int rep_size;
  
  Rep(int _rep_size) : rep_size(_rep_size)
  {
	int pow_size = intpow(2, rep_size);
	function.reserve(pow_size);
	for(int i = 0; i < pow_size; ++i)
	{
	  function.push_back(int_to_vec(i, rep_size));
	}
  }
  
  void randomise()
  { random_shuffle(function.begin(), function.end()); }
  
  template<typename F>
	vector<vector<bool> > get_satisfying_tuples(F f)
  {
	  vector<vector<bool> > output;
	  for(int i = 0; i < function.size(); ++i)
	  {
		if(f(function[i]))
		  output.push_back(int_to_vec(i, rep_size));    
	  }
	  return output;
  }
  
  void join(const Rep& rep)
  {
	vector<vector<bool> > original_rep;
	original_rep.swap(function);
	
	rep_size += rep.rep_size;
	
	function.reserve(original_rep.size() * rep.function.size());
	
	for(int i = 0; i < original_rep.size(); ++i)
	{
	  for(int j = 0; j < rep.function.size(); ++j)
	  {
		//function.push_back(original_rep[i]);
		//function.back().insert(function.back().end(), rep.function[j].begin(),
		//					   rep.function[j].end());
		function.push_back(rep.function[j]);
		function.back().insert(function.back().end(), original_rep[i].begin(),
							   original_rep[i].end());
		
		
	  }
	}
  }
};


int var_pos(int i, int j, int offset)
{
  assert(i < v && i >= 0);
  assert(j < b && j >= 0);
  return i * b + j + offset;
}


void print_tuple(const vector<bool>& vec)
{
  for(int i = 0; i < vec.size(); ++i)
	cout << vec[i] << " ";
  cout << endl;
}

void print_tuple_list(vector<vector<bool> > vecs)
{
  sort(vecs.begin(),vecs.end());
  cout << vecs.size() << " " << vecs[0].size() << endl;
  for(int i = 0; i < vecs.size(); ++i)
  {
	print_tuple(vecs[i]);
  }
}

void build_scalar_con(Rep rep)
{
  Rep scalar_rep(rep);
  scalar_rep.join(rep);
  vector<vector<bool> > scalar_tuples = 
	scalar_rep.get_satisfying_tuples(scalar_product(l));
  print_tuple_list(scalar_tuples);
}

void channelling_constraint(const Rep& r1, const Rep& r2)
{
  vector<vector<bool> > channel;
  channel.reserve(r1.function.size());
  
  for(int i = 0; i < r1.function.size(); ++i)
  {
	vector<bool> tuple;

    vector<bool> val1 = int_to_vec(i, r1.function[0].size());
    
    int pos = -1;
    
    for(int loop = 0; loop < r2.function.size(); loop++)
    {
      if(r2.function[loop] == r1.function[i])
        pos = loop;
    }
    
    assert(pos != -1);
    
    vector<bool> val2 = int_to_vec(pos, r2.function[0].size());
    
    tuple.insert(tuple.end(), val1.begin(), val1.end());
    tuple.insert(tuple.end(), val2.begin(), val2.end());
    
//	for(int j = 0; j < r1.function[i].size(); ++j)
//	  tuple.push_back(r1.function[i][j]);
	
	
//	for(int j = 0; j < r2.function[i].size(); ++j)
//	  tuple.push_back(r2.function[i][j]);
	
	
	//reverse(tuple.begin(), tuple.end());
	
	
	channel.push_back(tuple);
  }
  
  print_tuple_list(channel);
}

void build_constraints(Rep& rep)
{
  build_scalar_con(rep);
  
  print_tuple_list(rep.get_satisfying_tuples(sum_function(r)));
  for(int j = 0; j < b; ++j)
  {
	vector<vector<bool> > true_tuples = rep.get_satisfying_tuples(in_set(j));
	for(int loop = 0; loop < true_tuples.size(); ++loop)
	  true_tuples[loop].push_back(1);
	
	vector<vector<bool> > false_tuples = rep.get_satisfying_tuples(not_in_set(j));
	for(int loop = 0; loop < false_tuples.size(); ++loop)
	  false_tuples[loop].push_back(0);
	
	true_tuples.insert(true_tuples.end(), false_tuples.begin(), false_tuples.end());
	
	print_tuple_list(true_tuples);
  } 
}

void print_constraints(int var_offset, int con_offset)
{
  // Sum constraints on rows
  for(int i = 0; i < v; ++i)
  {
	cout << " table([x" << var_pos(i, 0, var_offset);
	
	for(int j = 1; j < b; ++j)
	  cout << ",x" << var_pos(i, j, var_offset);
	cout << "],t" << 1 + con_offset << ")" << endl;
  }
  
  // " i in S" constraints.  
  for(int i = 0; i < v; ++i)
  {
	for(int j = 0; j < b; ++j)
	{
	  cout << "table([x" << var_pos(i,0, var_offset);
	  for(int loop = 1; loop < b; ++loop)
		cout << ",x" << var_pos(i,loop, var_offset);
	  cout << ",x" << var_pos(i,j, var_offset) + v*b;
	  cout << "],t" << 2 + j + con_offset << ")" << endl;
	}
  }
  
  // Now put sum on columns.
  
  for(int i = 0; i < v; ++i)
  {
	cout << "sumleq([x" << var_pos(i, 0, var_offset) + v*b;
	for(int j = 1; j < b; ++j)
	  cout << ",x" << var_pos(i, j, var_offset) + v*b;
	cout << "], " << k << ")" << endl;
	
	cout << "sumgeq([x" << var_pos(i, 0, var_offset) + v*b;
	for(int j = 1; j < b; ++j)
	  cout << ",x" << var_pos(i, j, var_offset) + v*b;
	cout << "], " << k << ")" << endl;
  }
  
  // Finally, scalar product.
  {
	
	for(int i = 0; i < v; ++i)
	  for(int j = i + 1; j < v; ++j)
	  {
		cout << "table([x" << var_pos(i, 0, var_offset);
		for(int k = 1; k < b; ++k)
		  cout << ",x" << var_pos(i, k, var_offset);
		for(int k = 0; k < b; ++k)
		  cout << ",x" << var_pos(j, k, var_offset);
		cout << "],t" << con_offset << ")" << endl;
	  }
  }	
}

int main(int argc, char** argv)
{
  if(argc < 7 || argc < atoi(argv[6]) + 7)
  {
    cout << "v b r k l <instances> <branch_all> <list of seeds (0 = no random)>" << endl;
    cout << "Need more parameters!" << endl;
    exit(0);
  }
  
  v = atoi(argv[1]);
  b = atoi(argv[2]);
  r = atoi(argv[3]);
  k = atoi(argv[4]);
  l = atoi(argv[5]);

  int branch_all = atoi(argv[6]);
  
  int instances = atoi(argv[7]);
    
  int branch_instances = branch_all ? instances : 1;
  
  vector<Rep> rep;
  rep.reserve(instances);
  
  for(int i = 0; i < instances; ++i)
	rep.push_back(Rep(v));
  
  for(int i = 0; i < instances; ++i)
  {
    if(atoi(argv[8 + i]) != 0)
    { 
      srand(atoi(argv[8 + i]));
      rep[i].randomise();
    }
  }
  
  cout << "MINION 1" << endl;
  cout << "# Random generated BIBD examples" << endl;
  cout << "# <v,b,r,k,l> = <" << v << ',' << b << ',' << r << ','
	<< k << ',' << l << '>' << endl;
  cout << "# instances = " << instances << endl;
  cout << "# branch all = " << branch_all << endl;
  cout << "# random seed = ";
  for(int i = 0; i < instances; ++i)
    cout << atoi(argv[8 + i]) << ",";
  cout << endl;
  
  for(int i = 0; i < rep.size(); ++i)
  {  
	cout << "#";
    for(int j = 0; j < rep[i].function.size(); ++j)
	{
	  for(int k = 0; k < rep[i].function[j].size(); ++k)
		cout << rep[i].function[j][k];
	  cout << " ";
	}
	cout << endl;
  }
  
  int var_count = v * b * 2;
  
  cout << var_count * instances << endl;
  cout << "0 0 0 0" << endl;
  // We only want the basic matrix in the variable ordering
  
  cout << "[";
  for(int ins = 0; ins < branch_instances; ++ins)
  {
	if(ins > 0)
	  cout << "," << endl;
	cout << "x" << var_count * ins;
	for(int i = 1; i < v * b; ++i)
	  cout << ",x" << i + var_count * ins;
  }
  
  cout << "]" << endl;
  
  cout << "[a";
  for(int i = 1; i < v * b * branch_instances; ++i)
	cout << ",a";
  cout << "]" << endl;
  
  cout << "0 0 0" << endl;
  cout << "tuplelists " << 
	(2 + b) * instances + (instances * (instances - 1))/2 
	<< endl;
  
  for(int i = 0; i < instances; ++i)
	build_constraints(rep[i]);
  
  for(int i = 0; i < instances; ++i)
	for(int j = i + 1; j < instances; ++j)
	  channelling_constraint(rep[i], rep[j]);
	  
  cout << "objective none" << endl;
  cout << "print none" << endl;
  
  int con_num = 0;
  for(int i = 0; i < instances; ++i)
	for(int j = i + 1; j < instances; ++j)
	{
	  for(int k = 0; k < v; ++k)
	  {
		cout << "table([";
		
		cout << "x" << v*b*2*i + k*b;
		for(int l = 1; l < b; ++l)
		  cout << ",x" << v*b*2*i + k*b + l;
		
		for(int l = 0; l < b; ++l)
		  cout << ",x" << v*b*2*j + k*b + l;
		
		cout << "], t" << (2 + b)*instances + con_num
		  << ")" << endl;
	  }
	  
	  con_num++;
	}
  
  for(int i = 0; i < instances; ++i)
	print_constraints(v * b * 2 * i, (2 + b) * i); 
}
