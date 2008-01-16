/* Minion
* Copyright (C) 2006
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <vector>
#include <bitset>

#include "constraint_test.h"

using namespace std;

inline long long ipow(long long x, long long y)
{
  long long out=1;
  while(y--)
    out*=x;
  return out;
}

template<typename VarRef>
inline void test_prop(vector<pair<int,bool> >& prop_pairs,
					  unsigned long i, unsigned long i2,
					  vector<VarRef>& v1, vector<VarRef>& v2, bool checkGAC)
{
  Controller::world_push();
  for(unsigned int loop=0;loop<prop_pairs.size();loop++)
  {
    int j = prop_pairs[loop].first;
    if(prop_pairs[loop].second)
    {
      if((1<<j)&i)
		v1[j].propogateAssign(0);  
    }
    else
    {
      if((1<<j)&i2)
		v1[j].propogateAssign(1);  
    }
    Controller::propogate_queue();
  }
  
  Controller::propogate_queue();
  
  bool first_fail = Controller::failed;
  Controller::failed = false;
  
  for(unsigned int loop=0;loop < prop_pairs.size();loop++)
  {
    int j = prop_pairs[loop].first;
    if(prop_pairs[loop].second)
    {
      if((1<<j)&i)
	  {
	    D_INFO(1, DI_TEST, string("Setting var ") + to_string(j) + " to 0.");
		v2[j].propogateAssign(0);  
	  }
    }
    else
    {
      if((1<<j)&i2)
	  {
	    D_INFO(1, DI_TEST, string("Setting var ") + to_string(j) + " to 1.");
		v2[j].propogateAssign(1);  
	  }
    }
    Controller::propogate_queue();
  }

  
  Controller::propogate_queue();
  
  bool second_fail = Controller::failed;
  
  bool total_prop = true;
  for(unsigned int loop = 0; loop < v1.size(); ++loop)
  {
    if(! ( ((1<<loop)&i) || ((1<<loop)&i2) ) )
      total_prop = false;
  }
  
  if(checkGAC || total_prop)
  {
    D_ASSERT(first_fail == second_fail);
    if(first_fail == false)
    {
      //cerr << string(boolean_container) << endl;
      for(unsigned int j=0;j<v1.size();j++)
      {
		for(int k = min(v1[j].getMin(),v2[j].getMin());
			k <= max(v1[j].getMax(), v2[j].getMax()); ++k)
		{
		  D_ASSERT(v1[j].inDomain(k) == v2[j].inDomain(k));
		}
      }
    }
  }
  else
  {
    if(second_fail)
      D_ASSERT(first_fail);
    if(first_fail == false)
    {
      for(unsigned int j=0;j<v1.size();j++)
      {
		for(int k = min(v1[j].getMin(),v2[j].getMin());
			k <= max(v1[j].getMax(), v2[j].getMax()); ++k)
		{
		  if(v1[j].inDomain(k))
			D_ASSERT(v2[j].inDomain(k));
		}
      }
      
    }
  }
  Controller::failed = false;
  Controller::world_pop();
  
}

template<typename VarRef>
void test_equal(vector<VarRef>& v1, vector<VarRef>& v2, bool checkGAC = true)
{
  D_ASSERT(v1.size() == v2.size());
  Controller::propogate_queue();
  for(unsigned int j=0;j<v1.size();j++)
  {
    for( int k = min(v1[j].getMin(), v2[j].getMin());
		 k <= max(v1[j].getMax(), v2[j].getMax()); ++k)
    { D_ASSERT(v1[j].inDomain(k) == v2[j].inDomain(k)); }
  }
  
  vector<pair<int,bool> > prop_pairs;

  for(unsigned int i=0;i<v1.size();++i)
  {
    prop_pairs.push_back(make_pair(i,false));
    prop_pairs.push_back(make_pair(i,true));
  }
  
  unsigned int i_max = ipow(2,v1.size());
  unsigned int i2_max = ipow(2,v2.size());

  for(unsigned long i = 0; i < i_max; i++)
  {
    for(unsigned long i2 = 0; i2 < i2_max; i2++)
    {
      if(!(i&i2))
      {
		test_prop(prop_pairs,i,i2,v1,v2, checkGAC);
		vector<pair<int,bool> > prop_copy(prop_pairs);
		for(int loop = 0;loop < 20; ++loop)
		{
		  random_shuffle(prop_copy.begin(),prop_copy.end());
		  test_prop(prop_pairs,i,i2,v1,v2, checkGAC);
		}
      }
    }
  }  
  Controller::finish();
}

