/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id$
*/

/*
 *  test_functions.h
 *  cutecsp
 *
 *  Created by Chris Jefferson on 17/05/2006.
 *  Copyright 2006 __MyCompanyName__. All rights reserved.
 *
 */

void test_check_solution();

template<typename Var>
string get_dom_as_string(Var& v)
{
  ostringstream s;
  if(v.isAssigned())
  {
    s << v.getAssignedValue();
  }
  else
  {
    if(v.isBound())
    { s << "[" << v.getMin() << "," << v.getMax() << "]"; }
    else
    {
	  s << "{" << v.getMin();
      for(int i = v.getMin() + 1; i <= v.getMax(); ++i)
	    if(v.inDomain(i))
		  s << "," << i;
	  s << "}";
    }
  }
  return s.str();
}

template<typename T>
string get_dom_as_string(vector<T>& vec)
{
  string output("<");
  if(!vec.empty())
  {
    output += get_dom_as_string(vec[0]);
	for(unsigned i = 1; i < vec.size(); ++i)
	{
	  output += ",";
	  output += get_dom_as_string(vec[i]);
	}
  }
  output += ">";
  return output;
}

template<typename Vars>
long long lit_count(Vars& v)
{
  long long lits = 0;
  for(int i = 0; i < v.size(); ++i)
  {
    if(v[i].isBound())
    {
      lits += v[i].getMax() - v[i].getMin() + 1;
    }
    else
    {
      for(int j = v[i].getMin(); j <= v[i].getMax(); ++j)
        if(v[i].inDomain(j))
          lits++;
    }
  }
  return lits;
}
