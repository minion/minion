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

//int set_size = 7;
//int pow_size = intpow(2, set_size);

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
        function.push_back(original_rep[i]);
        function.back().insert(function.back().end(), rep.function[j].begin(),
        										      rep.function[j].end());
      }
    }
  }
};


int var_pos(int i, int j)
{
  assert(i < v && i >= 0);
  assert(j < b && j >= 0);
  return i * b + j;
}


void print_tuple(const vector<bool>& vec)
{
 cout << "<" << vec[0];
 for(int i = 1; i < vec.size(); ++i)
   cout << "," << vec[i];
 cout << "> ";
}

void print_tuple_list(const vector<vector<bool> >& vecs)
{
  cout << "{";
  print_tuple(vecs[0]);
  for(int i = 1; i < vecs.size(); ++i)
  {
    cout << ",";
    print_tuple(vecs[i]);
  }
  cout << "}" << endl;
}

int main(int argc, char** argv)
{
  
  v = atoi(argv[1]);
  b = atoi(argv[2]);
  r = atoi(argv[3]);
  k = atoi(argv[4]);
  l = atoi(argv[5]);
  
  srand(atoi(argv[6]));
  
  Rep rep(v);
  rep.randomise();
  
  cout << "MINION 1" << endl;
  cout << "# Random generated BIBD examples" << endl;
  cout << "# <v,b,r,k,l> = <" << v << ',' << b << ',' << r << ','
       << k << ',' << l << '>' << endl;
       
  cout << v*b*2 << endl;
  cout << "0 0 0 0" << endl;
  // We only want the basic matrix in the variable ordering
  cout << "[x0";
  for(int i = 1; i < v * b; ++i)
    cout << ",x" << i;
  cout << "]" << endl;
  
  cout << "[a";
  for(int i = 1; i < v * b; ++i)
    cout << ",a";
  cout << "]" << endl;
  cout << "0 0 0" << endl;
  cout << "objective none" << endl;
  cout << "print none" << endl;
  cout << "eq(x0,x1)" << endl;
  
  // Sum constraints on rows
  for(int i = 0; i < v; ++i)
  {
    cout << "table([x" << var_pos(i,0);
    
    for(int j = 1; j < b; ++j)
      cout << ",x" << var_pos(i,j);
    cout << "],";
    
    print_tuple_list(rep.get_satisfying_tuples(sum_function(r)));
    cout << ")" << endl;
  }
  
  // " i in S" constraints.  
  for(int i = 0; i < v; ++i)
  {
    for(int j = 0; j < b; ++j)
    {
      cout << "table([x" << var_pos(i,0);
      for(int loop = 1; loop < b; ++loop)
        cout << ",x" << var_pos(i,loop);
      cout << ",x" << var_pos(i,j) + v*b;
      cout << "],";
      
      
      vector<vector<bool> > true_tuples = rep.get_satisfying_tuples(in_set(j));
      for(int loop = 0; loop < true_tuples.size(); ++loop)
        true_tuples[loop].push_back(1);
        
      vector<vector<bool> > false_tuples = rep.get_satisfying_tuples(not_in_set(j));
      for(int loop = 0; loop < false_tuples.size(); ++loop)
        false_tuples[loop].push_back(0);
        
      true_tuples.insert(true_tuples.end(), false_tuples.begin(), false_tuples.end());
      
      print_tuple_list(true_tuples);
      cout << ")" << endl;
    }
  }
  
  // Now put sum on columns.
  
  for(int i = 0; i < v; ++i)
  {
    cout << "sumleq([x" << var_pos(i,0) + v*b;
    for(int j = 0; j < b; ++j)
      cout << ",x" << var_pos(i,j) + v*b;
    cout << "], " << k << ")" << endl;

    cout << "sumgeq([x" << var_pos(i,0) + v*b;
    for(int j = 0; j < b; ++j)
      cout << ",x" << var_pos(i,j) + v*b;
    cout << "], " << k << ")" << endl;
  }
  
  // Finally, scalar product.
  {
    Rep scalar_rep(rep);
    scalar_rep.join(rep);
    vector<vector<bool> > scalar_tuples = 
      scalar_rep.get_satisfying_tuples(scalar_product(l));
    for(int i = 0; i < v; ++i)
      for(int j = i + 1; j < v; ++j)
      {
        cout << "table([x" << var_pos(i,0);
        for(int k = 1; k < b; ++k)
          cout << ",x" << var_pos(i,k);
        for(int k = 0; k < b; ++k)
          cout << ",x" << var_pos(j,k);
        cout << "]," << endl;
        print_tuple_list(scalar_tuples);
        cout << ")" << endl;
      }
  }
  
  
  
}




