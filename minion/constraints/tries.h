/*
* Minion http://minion.sourceforge.net
* Copyright (C) 2006-09
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

#ifndef _TRIES_H_INCLUDE_15243
#define _TRIES_H_INCLUDE_15243

#include <numeric>

#include <vector>
#include <algorithm>
#include <cassert>

using namespace std;


struct TupleComparator
{
  int significantIndex;
  int arity;
  
  TupleComparator(int i, int a)
  {
    
    significantIndex = i;
    arity = a; 
  }
  
  // returns tuple1 <= tuple2 under our ordering.
  bool operator()(const vector<int>& tuple1, const vector<int>& tuple2)
  {
    if(tuple1[significantIndex] != tuple2[significantIndex])
      return tuple1[significantIndex] < tuple2[significantIndex];
    for(int tupleIndex = 0; tupleIndex < arity; tupleIndex++)
    {
      if(tuple1[tupleIndex] != tuple2[tupleIndex])
        return tuple1[tupleIndex] < tuple2[tupleIndex];
    }
    return false;
  }
};

struct TrieObj
{
  int val;
  TrieObj* offset_ptr;
};

struct TupleTrie
{
  static const int max_arity = 100;
  int arity;
  int sigIndex;
  TupleList* tuplelist;
  // A temporary tuple to store the most recently found
  // complete assignment.
  int current_tuple[max_arity];
  
  vector<vector<int> > tuples_vector;
  
  int map_depth(int depth)
  {
    if(depth==0) return sigIndex;
    if(depth <= sigIndex) return depth - 1;
    return depth;
  }
  
  int tuples(int num, int depth)
  { return tuples_vector[num][map_depth(depth)]; }
  
  
  TupleTrie(int _significantIndex, TupleList* tuplelist) :
  arity(tuplelist->tuple_size()), sigIndex(_significantIndex)
  {
    // TODO : Fix this hard limit.
    D_ASSERT(arity < 100);
    trie_data=NULL;
    tuples_vector.resize(tuplelist->size());

    // Need a copy so we can sort it and such things.
    for(int i = 0; i < tuplelist->size(); ++i)
      tuples_vector[i] = tuplelist->get_vector(i);

    std::stable_sort(tuples_vector.begin(), tuples_vector.end(), TupleComparator(sigIndex, arity));
    if(tuplelist->size()>0)
    {
      build_trie(0, tuplelist->size());
      build_final_trie();
    }
  }
  
  struct EarlyTrieObj
  {
    int val;
    int depth;
    int offset_ptr;
  };
  
  vector<EarlyTrieObj> initial_trie;
  
  TrieObj* trie_data;  // This is the actual trie used during search.
  
  void build_final_trie()
  {
    int size = initial_trie.size();
    trie_data = new TrieObj[size];
    for(int i = 0; i < size; ++i)
    {
      trie_data[i].val = initial_trie[i].val;
      if(initial_trie[i].offset_ptr == -1)
        trie_data[i].offset_ptr = NULL;
      else
        trie_data[i].offset_ptr = trie_data + initial_trie[i].offset_ptr;
    }
    
    // Little C++ trick to remove all the memory used by the initial_trie vector.
    vector<EarlyTrieObj> v;
    initial_trie.swap(v);
  }
  
  void print_trie()
  {
    for(int i = 0; i < initial_trie.size(); ++i)
    {
      printf("%d,%d,%d\n", initial_trie[i].depth, initial_trie[i].val, initial_trie[i].offset_ptr);
    }
  }
  
  void build_trie(const int start_pos, const int end_pos, int depth = 0)
  {
    const bool last_stage = (depth == arity - 1);
    if(depth == arity)
      return;
    
    assert(start_pos <= end_pos);
    int values = get_distinct_values(start_pos, end_pos, depth);
    
    int start_section = initial_trie.size();
    // Make space for this list of values.
    // '+1' is for end marker.
    initial_trie.resize(initial_trie.size() + values + 1);
    
    int current_val = tuples(start_pos, depth);
    int current_start = start_pos;
    int num_of_val = 0;
    
    for(int i = start_pos ; i < end_pos; ++i)
    {
      if(current_val != tuples(i, depth))
      {
        initial_trie[start_section + num_of_val].val = current_val;
        initial_trie[start_section + num_of_val].depth = depth;
        if(last_stage)
        {
          initial_trie[start_section + num_of_val].offset_ptr = -1;
        }
        else
        {
          initial_trie[start_section + num_of_val].offset_ptr = initial_trie.size();
          build_trie(current_start, i, depth + 1);
        }
        current_val = tuples(i, depth);
        current_start = i;
        num_of_val++;
      }
    }
    
    // Also have to cover last stretch of values.
    initial_trie[start_section + num_of_val].val = current_val;
    initial_trie[start_section + num_of_val].depth = depth;
    if(last_stage)
      initial_trie[start_section + num_of_val].offset_ptr = -1;
    else  
      initial_trie[start_section + num_of_val].offset_ptr = initial_trie.size();
    
    build_trie(current_start, end_pos, depth + 1);
    
    assert(num_of_val + 1 == values);
    initial_trie[start_section + values].val = DomainInt_Max;
    initial_trie[start_section + values].depth = depth;
    initial_trie[start_section + values].offset_ptr = -1;
  }
  
  // Find how many values there are for index 'depth' between tuples
  // start_pos and end_pos.
  int get_distinct_values(const int start_pos, const int end_pos, int depth)
  {
    int current_val = tuples(start_pos, depth);
    int found_values = 1;
    for(int i = start_pos; i < end_pos; ++i)
    {
      if(current_val != tuples(i, depth))
      {
        current_val = tuples(i, depth);
        found_values++;
      }
    }
    return found_values;
  }

  
  // Starting from the start of an array of TrieObjs, find the
  // values which is find_val
  TrieObj* get_next_ptr(TrieObj* obj, DomainInt find_val)
  {
    while(obj->val < find_val)
      ++obj;
    if(obj->val == find_val)
      return obj;
    else
      return NULL;
  }
  
  template<typename VarArray>
    bool search_trie(const VarArray& _vars, TrieObj** obj_list, int depth)
  {
    CON_INFO_ADDONE(SearchTrie);
    VarArray& vars = const_cast<VarArray&>(_vars);
    if(depth == arity)
      return true;
    
    obj_list[depth] = obj_list[depth - 1]->offset_ptr;  
    while(obj_list[depth]->val != DomainInt_Max)
    {
      if(vars[map_depth(depth)].inDomain(obj_list[depth]->val))
      {
        if(search_trie(_vars, obj_list, depth + 1))
          return true;
      }
      obj_list[depth]++;
    }
    return false;
  }
  
  // Variant for the lightweight table constraint.
  template<typename VarArray>
    bool search_trie_nostate(DomainInt domain_val, const VarArray& _vars)
  {
      MAKE_STACK_BOX(obj_list, TrieObj* , _vars.size());
      
      TrieObj* first_ptr = get_next_ptr(trie_data, domain_val);
      if(first_ptr == NULL)
        return false;
      
      obj_list[0] = first_ptr;
      
      return search_trie_nostate_internal(_vars, obj_list, 1);
  }
  
  // Same as search_trie 
  template<typename VarArray>
  bool search_trie_nostate_internal(const VarArray& _vars, box<TrieObj*> obj_list, int depth)
  {
    CON_INFO_ADDONE(SearchTrie);
    VarArray& vars = const_cast<VarArray&>(_vars);
    if(depth == arity)
      return true;
    
    obj_list[depth] = obj_list[depth - 1]->offset_ptr;  
    while(obj_list[depth]->val != DomainInt_Max)
    {
      if(vars[map_depth(depth)].inDomain(obj_list[depth]->val))
      {
        if(search_trie_nostate_internal(_vars, obj_list, depth + 1))
          return true;
      }
      obj_list[depth]++;
    }
    return false;
  }
  
  
  // search_trie_negative searches for a tuple which is not in the trie. 
  // For the negative table constraint.
  template<typename VarArray>
    bool search_trie_negative(const VarArray& _vars, TrieObj** obj_list, int depth, DomainInt* returnTuple)
  {
    CON_INFO_ADDONE(SearchTrie);
    VarArray& vars = const_cast<VarArray&>(_vars);
    if(depth == arity)
      return false;
    
    obj_list[depth] = obj_list[depth - 1]->offset_ptr;
    
    int dep=map_depth(depth);
    for(int i=vars[dep].getMin(); i<=vars[dep].getMax(); i++)
    {
        if(vars[dep].inDomain(i))
        {
            while(obj_list[depth]->val < i) obj_list[depth]++;
            returnTuple[dep]=i;
            if(obj_list[depth]->val > i)
            {   // includes case where val is maxint.
                // if the value is in the domain but not in the trie, we are nearly finished.
                // Just need to fill in the rest of returnTuple.
                // Is there any need to search from the previous position?? Yes. But not doing so yet.
                for(int depth2=depth+1; depth2<arity; depth2++) returnTuple[map_depth(depth2)]=vars[map_depth(depth2)].getMin();
                return true;
            }
            else
            {
                if(search_trie_negative(_vars, obj_list, depth+1, returnTuple))
                    return true;
            }
        }
    }
    return false;
  }
  
  void reconstructTuple(DomainInt* array, TrieObj** obj_list)
  {
    //D_ASSERT(check == obj_list[arity - 1] - obj_list[0]);
    for(int i = 0; i < arity; ++i)
      array[map_depth(i)] = obj_list[i]->val;
  }
  
  template<typename VarArray>
    bool loop_search_trie(const VarArray& _vars, TrieObj** obj_list, int depth)
  {
    CON_INFO_ADDONE(LoopSearchTrie);
      VarArray& vars = const_cast<VarArray&>(_vars);
      if(depth == arity)
        return true;
      
      if(obj_list[depth]->val == DomainInt_Max)
        return search_trie(_vars, obj_list, depth);
          
      if(vars[map_depth(depth)].inDomain(obj_list[depth]->val))
      {
        if(loop_search_trie(_vars, obj_list, depth + 1))
          return true;
      }
      
      TrieObj* initial_pos = obj_list[depth]; 
      
      obj_list[depth]++;
      while(obj_list[depth]->val != DomainInt_Max)
      {
        if(vars[map_depth(depth)].inDomain(obj_list[depth]->val))
        {
          if(search_trie(_vars, obj_list, depth + 1))
            return true;
        }
        obj_list[depth]++;
      }
      
      obj_list[depth] = obj_list[depth - 1]->offset_ptr;  
      
      while(obj_list[depth] != initial_pos)
      {
        if(vars[map_depth(depth)].inDomain(obj_list[depth]->val))
        {
          if(search_trie(_vars, obj_list, depth + 1))
            return true;
        }
        obj_list[depth]++;
      }
      return false;
  }
  
  
  // Find support for domain value i. This will be the value used by
  // the first variable.
  template<typename VarArray>
    int nextSupportingTuple(DomainInt domain_val, const VarArray& _vars, TrieObj** obj_list)
  {
    if(trie_data == NULL)
      return -1;
      
    VarArray& vars = const_cast<VarArray&>(_vars);
      
    if(obj_list[0] == NULL)
    {
      TrieObj* first_ptr = get_next_ptr(trie_data, domain_val);
      if(first_ptr == NULL)
        return -1;
    
      obj_list[0] = first_ptr;
      if(search_trie(vars, obj_list, 1))
        return obj_list[arity-1] - obj_list[0];  
      else
        return -1;
    }
    else
    {
      if(loop_search_trie(vars, obj_list, 1))
        return obj_list[arity-1] - obj_list[0];  
      else
        return -1;
/*    D_ASSERT(obj_list[0] == get_next_ptr(trie_data, domain_val));
      int OK_depth = 1;
      while(OK_depth < arity && vars[map_depth(OK_depth)].inDomain(obj_list[OK_depth]->val))
        OK_depth++;
      if(search_trie(vars,obj_list, OK_depth))
        return obj_list[arity-1] - obj_list[0];
      else
      {
        if(search_trie(vars, obj_list, 1))
          return obj_list[arity-1] - obj_list[0];  
        else
          return -1;
      }*/
    }
  }
  
  // Find support for domain value i. This will be the value used by
  // the first variable.
  template<typename VarArray>
    int nextSupportingTupleNegative(DomainInt domain_val, const VarArray& _vars, TrieObj** obj_list, DomainInt* recycTuple)
  {
    VarArray& vars = const_cast<VarArray&>(_vars);
    
    //if(obj_list[0] == NULL)
    {
      TrieObj* first_ptr = get_next_ptr(trie_data, domain_val);
      if(first_ptr == NULL)
      {
          // Hang on a minute. How do we ever get here? Should only be at root node.
          for(int depth2=1; depth2<arity; depth2++) recycTuple[map_depth(depth2)]=vars[map_depth(depth2)].getMin();
          recycTuple[map_depth(0)]=domain_val;
          return 0;
      }
    
      obj_list[0] = first_ptr;
    }
      
    bool flag=search_trie_negative(vars, obj_list, 1, recycTuple);
    
    if(!flag) return -1;
    return 0;
  }
};

//template<typename VarArray>
class TupleTrieArray {
public:
  TupleList* tuplelist;
  
  int arity; 
  TupleTrie* tupleTries;
  
  TupleTrie & getTrie(int varIndex) 
  { return tupleTries[varIndex]; };
  
  /* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    Constructor
    %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */
  
  TupleTrieArray(TupleList* _tuplelist) :
    tuplelist(_tuplelist)
  {
      tuplelist->finalise_tuples();
      arity = tuplelist->tuple_size();
      vector<int> dom_size(arity);
      vector<int> offset(arity);
      
      // create one trie for each element of scope.
      tupleTries = (TupleTrie*) malloc(sizeof(TupleTrie) * arity);
      if(!tupleTries)
      {
        cerr << "Out of memory in TupleTrie construction" << endl;
        FAIL_EXIT();
      }
      for (unsigned varIndex = 0; varIndex < arity; varIndex++)
        new (tupleTries + varIndex) TupleTrie(varIndex, tuplelist);
  }
};

#endif
