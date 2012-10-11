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

#ifndef _TUPLE_CONTAINER_H
#define _TUPLE_CONTAINER_H

#include "system/system.h"

using namespace std;

#include <vector>
#include <utility>

// This file contains lists fo tuples, as required by table clan. This
// is required as these tuples can get big, and it's important to keep them
// as compactly as possible.

class LiteralSpecificLists;
class Nightingale;
class TupleTrieArray;
class Regin;

class TupleList
{

  LiteralSpecificLists* litlists;
  Nightingale* nightingale;
  TupleTrieArray* triearray;
  Regin* regin;
  
  DomainInt* tuple_data;
  SysInt tuple_length;
  SysInt number_of_tuples;
  bool tuples_locked;
 
  
  public:
  
  LiteralSpecificLists* getLitLists();
  Nightingale* getNightingale();
  TupleTrieArray* getTries();
  Regin* getRegin();
  
  /// Get raw pointer to the tuples.
  DomainInt* getPointer()
  { return tuple_data; }
  
  /// Get number of tuples.
  DomainInt size() const
  { return number_of_tuples; }
  
  // Get size of a particular tuple.
  DomainInt tuple_size() const
  { return tuple_length; }
  
  // This function is temporary while the new interface is being designed.
  vector<DomainInt> get_vector(SysInt pos) const
  {
    vector<DomainInt> vec(tuple_length);
    for(SysInt i = 0; i < tuple_length; ++i)
      vec[i] = tuple_data[pos * tuple_length + i];
    return vec;
  }
  
  TupleList(const vector<vector<DomainInt> >& tuple_list) : litlists(NULL), 
    nightingale(NULL), triearray(NULL), regin(NULL),  tuples_locked(false)
  {
    number_of_tuples = tuple_list.size();
    tuple_length = tuple_list[0].size();
    tuple_data = new DomainInt[number_of_tuples * tuple_length];
    for(SysInt i = 0; i < number_of_tuples; ++i)
      for(SysInt j = 0; j < tuple_length; ++j)
      { tuple_data[i * tuple_length + j] = tuple_list[i][j]; }
    finalise_tuples();
  }
  
  TupleList(DomainInt _numtuples, DomainInt _tuplelength) : litlists(NULL),
     nightingale(NULL), triearray(NULL), regin(NULL), tuple_length(checked_cast<SysInt>(_tuplelength)),
    number_of_tuples(checked_cast<SysInt>(_numtuples)), tuples_locked(false)
  { tuple_data = new DomainInt[number_of_tuples * tuple_length]; }
  
  const DomainInt* operator[](SysInt pos) const
  { return get_tupleptr(pos); }
  
  const DomainInt* get_tupleptr(SysInt pos) const
  { 
    D_ASSERT(pos >= 0 && (pos < number_of_tuples || (number_of_tuples==0 && pos==0)));
    return tuple_data + pos*tuple_length;
  }
  

  
 /// Original smallest value from each domain.
  vector<DomainInt> dom_smallest;
  /// Original size of each domain.
  vector<DomainInt> dom_size;
  
  /// Total number of literals in the variables at the start of search.
  DomainInt literal_num;
  
  /// Used by get_literal.
  vector<vector<DomainInt> > _map_vars_to_literal;
  
  /// Used to get a variable/value pair from a literal
  vector<pair<SysInt,DomainInt> > _map_literal_to_vars;
  
  /// Maps a variable/value pair to a literal.
  DomainInt get_literal(DomainInt var_num_in, DomainInt dom_num)
  {
     const SysInt var_num = checked_cast<SysInt>(var_num_in);
     D_ASSERT(var_num >= 0 && var_num < tuple_size());
     D_ASSERT(dom_num >= dom_smallest[var_num]);
     D_ASSERT(dom_num < dom_smallest[var_num] + dom_size[var_num]);
    return _map_vars_to_literal[var_num][checked_cast<SysInt>(dom_num - dom_smallest[var_num])]; 
  }
  
  pair<SysInt,DomainInt> get_varval_from_literal(DomainInt literal)
  { return _map_literal_to_vars[checked_cast<SysInt>(literal)]; }

  /// Sets up the variable/value pair to literal mapping, used by get_literal.
  void finalise_tuples()
  {
    if(tuples_locked)
      return;
    tuples_locked = true;

    DomainInt arity = tuple_size();   

    // Set up the table of tuples.
    for(SysInt i = 0; i < arity; ++i)
    {
      if(size() == 0)
      {
        dom_smallest.push_back(0);
        dom_size.push_back(0);
      }
      else
      {
        DomainInt min_val = get_tupleptr(0)[i];
        DomainInt max_val = get_tupleptr(0)[i];
        for(SysInt j = 1; j < size(); ++j)
        {
          min_val = mymin(min_val, get_tupleptr(j)[i]);
          max_val = mymax(max_val, get_tupleptr(j)[i]);
        }
        dom_smallest.push_back(min_val);
        dom_size.push_back(max_val - min_val + 1);
      }
    }

    SysInt dom_size_size = checked_cast<SysInt>(dom_size.size());
    _map_vars_to_literal.resize(checked_cast<SysInt>(dom_size_size));
    // For each variable / value pair, get a literal
    DomainInt literal_count = 0;

    for(SysInt i = 0; i < dom_size_size; ++i)
    {
      _map_vars_to_literal[i].resize(checked_cast<SysInt>(dom_size[i]) + 1);
      for(SysInt j = 0; j < dom_size[i]; ++j)
      {
        _map_vars_to_literal[i][j] = literal_count;
        _map_literal_to_vars.push_back(make_pair(i, j + dom_smallest[checked_cast<SysInt>(i)]));
        D_ASSERT(get_literal(i, j + dom_smallest[i]) == literal_count);
        D_ASSERT(get_varval_from_literal(literal_count).first == i);
        D_ASSERT(get_varval_from_literal(literal_count).second == j + dom_smallest[checked_cast<SysInt>(i)]);
        ++literal_count;
      }
    }
    literal_num = literal_count;
  }

};

class TupleListContainer
{
  std::vector<TupleList*> Internal_TupleList;

public:
  TupleList* getNewTupleList(DomainInt numtuples, DomainInt tuplelength)
  {
    TupleList* tuplelist_ptr = new TupleList(numtuples, tuplelength);
    Internal_TupleList.push_back(tuplelist_ptr);
    return tuplelist_ptr;
  }

  TupleList* getNewTupleList(const vector<vector<DomainInt> >& tuples)
  { 
    TupleList* tuplelist_ptr = new TupleList(tuples);
    Internal_TupleList.push_back(tuplelist_ptr);
    return tuplelist_ptr;
  }

  TupleList* getTupleList(DomainInt num)
  { return Internal_TupleList[checked_cast<SysInt>(num)]; }

  SysInt size()
  { return Internal_TupleList.size(); }
};


/// The first GACtable implementation.
class LiteralSpecificLists
{
public:
  TupleList* tuples;
  
  /// For each literal, a list of the tuples it is present in.  
  vector<vector<vector<DomainInt> > > literal_specific_tuples;
  
  LiteralSpecificLists(TupleList* _tuples) : tuples(_tuples)
  { 
      tuples->finalise_tuples();
      // For each literal, store the set of tuples which it allows.
      for(UnsignedSysInt i = 0; i < tuples->dom_size.size(); ++i)
      {
        for(DomainInt j = tuples->dom_smallest[i]; 
                      j <= tuples->dom_smallest[i] + tuples->dom_size[i];
                      ++j)
        {
          vector<vector<DomainInt> > specific_tuples;
          for(SysInt k = 0; k < (*tuples).size(); ++k)
          {
            if((*tuples)[k][i] == j)
              specific_tuples.push_back((*tuples).get_vector(k));
          }
          literal_specific_tuples.push_back(specific_tuples);
          //D_ASSERT(literal_specific_tuples.size() - 1 == get_literal(i,j));
        }
      }
  }
};

inline LiteralSpecificLists* TupleList::getLitLists()
{
  if(litlists == NULL)
    litlists = new LiteralSpecificLists(this);
  return litlists;
}

#endif
