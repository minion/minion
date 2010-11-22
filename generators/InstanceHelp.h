#include <vector>
#include <iostream>
#include <sstream>
using namespace std;

struct BoundsList {
  int lower;
  int upper;
  int count;
  BoundsList(int i, int j, int k ) : lower(i), upper(j), count(k)
  {}
};

struct SparseList {
  vector<int> vals;
  int count;
  SparseList(vector<int> v, int c) : vals(v), count(c)
  {}
};


enum var_type
{
  Bool, NotBool, Bound, SparseBound, Discrete, SparseDiscrete, Constant
};


enum ConstraintName
{
 AllDiff,
 Diseq,
 Eq,
 Element,
 Ineq,
 LexLeq,
 LexLess,
 Max,
 Min,
 Occurrence,
 Product,
 WeightedSumLeq,
 WeightedSumGeq,
 SumGeq,
 SumLeq
};




struct Var
{
  var_type type;
  int val;
  Var(var_type t, int v) : type(t), val(v)
  {}
  Var()
  {}
};


struct instance {
  int bools;
  vector<BoundsList> bound;
  vector<SparseList> sparse_bound;
  vector<BoundsList> discrete;
  vector<SparseList> sparse_discrete;
  
    
  string str(Var v)
  {
    ostringstream s;
    switch(v.type)
	{
	  case Constant:
	   s << v.val;
		break;
		
	  case Bool:
		s << "x" << v.val;
		break;
		
	  case NotBool:
	   s << "nx" << v.val;
	   break;
	  
	  case Bound:
		v.val += bools;
		s << "x" << v.val;
		break;
		
	  case SparseBound:
		v.val += bools;
		for(unsigned i = 0; i < bound.size(); ++i)
		  v.val += bound[i].count;
		  s << "x" << v.val;
		break;
		
	  case Discrete:
		v.val += bools;
		for(unsigned i = 0; i < bound.size(); ++i)
		  v.val += bound[i].count;
		  for(unsigned i = 0; i < sparse_bound.size(); ++i)
			v.val += sparse_bound[i].count;
			s << "x" << v.val;
		break;
		
	  case SparseDiscrete:
		v.val += bools;
		for(unsigned i = 0; i < bound.size(); ++i)
		  v.val += bound[i].count;
		  for(unsigned i = 0; i < sparse_bound.size(); ++i)
			v.val += sparse_bound[i].count;
			for(unsigned i = 0; i < discrete.size(); ++i)
			  v.val += discrete[i].count;
			  
			  s << "x" << v.val;
		break;
		
	}
    return s.str();
  }
  
  
  string str(vector<Var> v)
  {
	ostringstream s;
	s << "[" << str(v[0]);
	for(unsigned i = 1; i < v.size(); ++i)
	  s << "," << str(v[i]);
	s << "]";
	return s.str();
  }
  
  string str(vector<int> v)
  {
    ostringstream s;
	s << "[" << v[0];
	for(unsigned i = 1; i < v.size(); ++i)
	s << "," << v[i];
	s << "]";
	return s.str();
  }

  string header()
  {
    ostringstream s;
    s << "# Input file for Minion built for Version 0.2" << endl;
    s << "#    http://sourceforge.net/projects/minion" << endl;
    return s.str();
  }
  
  #define STR_CASE(X,Y) case X: s << #Y; break;
  string str(ConstraintName c)
  {
    ostringstream s;
	switch(c)
	{
 STR_CASE(AllDiff,alldiff)
 STR_CASE(Diseq,diseq)
 STR_CASE(Eq,eq)
 STR_CASE(Element,element)
 STR_CASE(Ineq,ineq)
 STR_CASE(LexLeq,lexleq)
 STR_CASE(LexLess,lexless)
 STR_CASE(Max,max)
 STR_CASE(Min,min)
 STR_CASE(Occurrence,occurrence)
STR_CASE( Product,product)
 STR_CASE(WeightedSumLeq,weightedsumleq)
 STR_CASE(WeightedSumGeq,weightedsumgeq)
 STR_CASE(SumGeq,sumgeq)
 STR_CASE(SumLeq,sumleq)
	}
  return s.str();
  }
  
  instance() : bools(0)
  {}
  
  
  void print_bound_vars(vector<BoundsList>& b)
  {
	int bound_count = 0;
	for(int i=0; i < b.size(); ++i)
	  bound_count += b[i].count;
	
	cout << bound_count << endl;
	
	for(int i=0; i < b.size(); ++i)
	  cout << b[i].lower << " " << b[i].upper << " " << b[i].count << endl;
  }
  
  void print_sparse_vars(vector<SparseList>& s)
  {
	int sparse_count = 0;
	for(int i=0 ; i < s.size(); ++i)
	  sparse_count += s[i].count;
	
	cout << sparse_count << endl;;
	
	for(int i=0; i < s.size(); ++i)
	{
	  cout << "{";
	  cout << s[i].vals[0];
	  for(int j = 1;j < s[i].vals.size(); ++j)
		cout << "," << s[i].vals[j];
	  cout << "}" << " " << s[i].count << endl;
	}
  }
  
  void print_vars()
  {
	cout << bools << endl;
	cout << endl;
	
	print_bound_vars(bound);
	cout << endl;
	print_sparse_vars(sparse_bound);
	cout << endl;
	print_bound_vars(discrete);
	cout << endl;
	print_sparse_vars(sparse_discrete);
	cout << endl;
  }
  
  template<typename X, typename Y>
	void constraint(ConstraintName c, const X& x,const Y& y)
  { cout << str(c) << "(" << str(x) << " , " << str(y) << ")" << endl; }
  
  template<typename X, typename Y, typename Z>
  	void constraint(ConstraintName c, const X& x,const Y& y, const Z& z)
  { cout << str(c) << "(" << str(x) << " , " << str(y) << "," << str(z) << ")" << endl; }
  
  
  void constraintScalarLeq(vector<int> x, vector<Var> y,  Var z)
  { cout << "weightedsumleq(" << str(x) << "," << str(y) << "," <<  str(z) << ")" << endl; }
  
  void constraintScalarGeq( vector<int> x, vector<Var> y, Var z)
  { cout << "weightedsumgeq(" << str(x) << "," << str(y) << "," <<  str(z) << ")" << endl; }
  
  
  template<typename X, typename Y>
	void constraintReify(Var v , ConstraintName c, X x, Y y)
  { 
	cout << "reify("  ;
	cout << str(c) << "(" << str(x) << " , " << str(y) << ")";
	cout << "," << str(v)  << ")" << endl;
  }
  
  template<typename X, typename Y>
	void constraintReifyTrue(Var v, ConstraintName c, X x, Y y)
  {
	cout << "reifyimply(" ;
	cout << str(c) << "(" << str(x) << " , " << str(y) << ")";
	cout << "," <<  str(v) << ")" << endl;
  }
  
  void print_var_order(vector<Var> v)
  { cout << str(v) << endl; }
  
  void print_val_order(vector<char> v)
  {
    cout << "[" << v[0];
    for(unsigned i = 1; i < v.size(); ++i)
	{ cout << "," << v[i]; }
	cout << "]" << endl;
  }
  
  void optimise_min(Var v)
  { cout << "objective minimising " << str(v) << endl; }
  
  void optimise_max(Var v)
  { cout << "objective maximising " << str(v) << endl; }
  
  void optimise_none()
  { cout << "objective none" << endl; }
  
};
