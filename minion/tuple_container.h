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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
* USA.
*/

#ifndef _TUPLE_CONTAINER_H
#define _TUPLE_CONTAINER_H

#include "system/system.h"

#define TUPLE_SQUASH
#ifdef TUPLE_SQUASH
#include "squash.hpp"
#endif

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
class EggShellData;
struct HaggisGACTuples;

inline size_t get_hash_val(DomainInt* ptr, SysInt length) {
  size_t hash_code = 1234;
  for(SysInt i = 0; i < length; ++i) {
    size_t val = checked_cast<SysInt>(ptr[i]);
    hash_code = (hash_code << 5) - hash_code + val;
  }
  hash_code += length;
  return hash_code;
}

inline SysInt get_hash_val(const vector<vector<DomainInt>>& vecs) {
  SysInt hash_code = 1234;
  for(SysInt i = 0; i < (SysInt)vecs.size(); ++i)
    for(SysInt j = 0; j < (SysInt)vecs[i].size(); ++j) {
      SysInt val = checked_cast<SysInt>(vecs[i][j]);
      hash_code = (hash_code << 5) - hash_code + val;
    }

  SysInt total_size = 0;
  for(SysInt i = 0; i < (SysInt)vecs.size(); ++i)
    total_size += vecs[i].size();
  hash_code += total_size;
  return hash_code;
}

class TupleList {
  string tuple_name;

  LiteralSpecificLists* litlists;
  Nightingale* nightingale;
  TupleTrieArray* triearray;
  Regin* regin;
  EggShellData* egg;

  DomainInt* tuple_data;
  SysInt tuple_length;
  SysInt number_of_tuples;
  bool tuples_locked;

  SysInt hash_code;

public:
  size_t get_hash() {
    if(hash_code != 0)
      return hash_code;

    hash_code = get_hash_val(tuple_data, tuple_length * number_of_tuples);
    return hash_code;
  }
  LiteralSpecificLists* getLitLists();
  Nightingale* getNightingale();
  TupleTrieArray* getTries();
  Regin* getRegin();
  EggShellData* getEggShellData(size_t varcount);

  /// Get raw pointer to the tuples.
  DomainInt* getPointer() {
    return tuple_data;
  }

  /// Get number of tuples.
  DomainInt size() const {
    return number_of_tuples;
  }

  // Get size of a particular tuple.
  DomainInt tuple_size() const {
    return tuple_length;
  }

  // This function is temporary while the new interface is being designed.
  vector<DomainInt> get_vector(SysInt pos) const {
    vector<DomainInt> vec(tuple_length);
    for(SysInt i = 0; i < tuple_length; ++i)
      vec[i] = tuple_data[pos * tuple_length + i];
    return vec;
  }

  TupleList(const vector<vector<DomainInt>>& tuple_list)
      : litlists(NULL),
        nightingale(NULL),
        triearray(NULL),
        regin(NULL),
        egg(NULL),
        tuples_locked(false),
        hash_code(0) {
    number_of_tuples = tuple_list.size();
    tuple_length = tuple_list[0].size();
    tuple_data = new DomainInt[number_of_tuples * tuple_length];
    for(SysInt i = 0; i < number_of_tuples; ++i)
      for(SysInt j = 0; j < tuple_length; ++j) {
        tuple_data[i * tuple_length + j] = tuple_list[i][j];
      }
    finalise_tuples();
  }

  TupleList(DomainInt _numtuples, DomainInt _tuplelength)
      : litlists(NULL),
        nightingale(NULL),
        triearray(NULL),
        regin(NULL),
        egg(NULL),
        tuple_length(checked_cast<SysInt>(_tuplelength)),
        number_of_tuples(checked_cast<SysInt>(_numtuples)),
        tuples_locked(false),
        hash_code(0) {
    tuple_data = new DomainInt[number_of_tuples * tuple_length];
  }

  const DomainInt* operator[](SysInt pos) const {
    return get_tupleptr(pos);
  }

  const DomainInt* get_tupleptr(SysInt pos) const {
    D_ASSERT(pos >= 0 && (pos < number_of_tuples || (number_of_tuples == 0 && pos == 0)));
    return tuple_data + pos * tuple_length;
  }

  void setName(string name) {
    tuple_name = name;
  }

  string getName() const {
    return tuple_name;
  }

  /// Original smallest value from each domain.
  vector<DomainInt> dom_smallest;
  /// Original size of each domain.
  vector<DomainInt> dom_size;

  /// Total number of literals in the variables at the start of search.
  DomainInt literal_num;

  /// Used by get_literal.
  vector<vector<DomainInt>> _map_vars_to_literal;

  /// Used to get a variable/value pair from a literal
  vector<pair<SysInt, DomainInt>> _map_literal_to_vars;

  /// Maps a variable/value pair to a literal.
  DomainInt get_literal(DomainInt var_num_in, DomainInt dom_num) {
    const SysInt var_num = checked_cast<SysInt>(var_num_in);
    D_ASSERT(var_num >= 0 && var_num < tuple_size());
    D_ASSERT(dom_num >= dom_smallest[var_num]);
    D_ASSERT(dom_num < dom_smallest[var_num] + dom_size[var_num]);
    return _map_vars_to_literal[var_num][checked_cast<SysInt>(dom_num - dom_smallest[var_num])];
  }

  pair<SysInt, DomainInt> get_varval_from_literal(DomainInt literal) {
    return _map_literal_to_vars[checked_cast<SysInt>(literal)];
  }

  /// Sets up the variable/value pair to literal mapping, used by get_literal.
  void finalise_tuples() {
    if(tuples_locked)
      return;
    tuples_locked = true;

    DomainInt arity = tuple_size();

    // Set up the table of tuples.
    for(SysInt i = 0; i < arity; ++i) {
      if(size() == 0) {
        dom_smallest.push_back(0);
        dom_size.push_back(0);
      } else {
        DomainInt min_val = get_tupleptr(0)[i];
        DomainInt max_val = get_tupleptr(0)[i];
        for(SysInt j = 1; j < size(); ++j) {
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

    for(SysInt i = 0; i < dom_size_size; ++i) {
      _map_vars_to_literal[i].resize(checked_cast<SysInt>(dom_size[i]) + 1);
      for(SysInt j = 0; j < dom_size[i]; ++j) {
        _map_vars_to_literal[i][j] = literal_count;
        _map_literal_to_vars.push_back(make_pair(i, j + dom_smallest[checked_cast<SysInt>(i)]));
        D_ASSERT(get_literal(i, j + dom_smallest[i]) == literal_count);
        D_ASSERT(get_varval_from_literal(literal_count).first == i);
        D_ASSERT(get_varval_from_literal(literal_count).second ==
                 j + dom_smallest[checked_cast<SysInt>(i)]);
        ++literal_count;
      }
    }
    literal_num = literal_count;
  }
};

class TupleListContainer {
  std::vector<TupleList*> Internal_TupleList;

public:
  TupleList* getNewTupleList(DomainInt numtuples, DomainInt tuplelength) {
    TupleList* tuplelist_ptr = new TupleList(numtuples, tuplelength);
    Internal_TupleList.push_back(tuplelist_ptr);
    return tuplelist_ptr;
  }

  TupleList* getNewTupleList(const vector<vector<DomainInt>>& tuples) {
    size_t tuple_hash = get_hash_val(tuples);
    for(SysInt i = 0; i < (SysInt)Internal_TupleList.size(); ++i) {
      bool equal = true;

      if(Internal_TupleList[i]->get_hash() != tuple_hash)
        equal = false;
      else {
        TupleList* ptr = Internal_TupleList[i];
        if((SysInt)tuples.size() != ptr->size() || (SysInt)tuples[0].size() != ptr->tuple_size()) {
          equal = false;
        } else {
          for(SysInt j = 0; j < (SysInt)tuples.size() && equal; ++j) {
            for(SysInt k = 0; k < (SysInt)tuples[j].size() && equal; ++k)
              if(tuples[j][k] != ptr->get_tupleptr(j)[k]) {
                equal = false;
              }
          }
        }

        if(equal)
          return Internal_TupleList[i];
      }
    }

    TupleList* tuplelist_ptr = new TupleList(tuples);
    Internal_TupleList.push_back(tuplelist_ptr);
    return tuplelist_ptr;
  }

  TupleList* getTupleList(DomainInt num) {
    return Internal_TupleList[checked_cast<SysInt>(num)];
  }

  SysInt size() {
    return Internal_TupleList.size();
  }
};

class ShortTupleList {
  vector<vector<pair<SysInt, DomainInt>>> short_tuples;
  vector<set<DomainInt>> initial_domains;
  string tuple_name;

  std::map<std::vector<std::pair<DomainInt, DomainInt>>, HaggisGACTuples*> hgt;

public:
  template <typename Vars>
  HaggisGACTuples* getHaggisData(const Vars& vars);

  // NOTE: initial domains may be empty, in which case they should be ignored.
  vector<set<DomainInt>> getInitialDomains() {
    return initial_domains;
  }

  ShortTupleList(const vector<vector<pair<SysInt, DomainInt>>>& _short_tuples)
      : short_tuples(_short_tuples) {}

  // method : 0 - nothing, 1 - long tuples, 2 - eager, 3 - lazy
  ShortTupleList(TupleList* long_tuples, MapLongTuplesToShort method) {
    D_ASSERT(method != MLTTS_NoMap);

    set<vector<DomainInt>> tuple_set;

    for(SysInt i = 0; i < long_tuples->size(); ++i)
      tuple_set.insert(long_tuples->get_vector(i));

    if(method == MLTTS_KeepLong) {
      short_tuples = makeShortTupleList(tuple_set);
      return;
    }

    D_ASSERT(method == MLTTS_Lazy || method == MLTTS_Eager);

    initial_domains = gather_domains(tuple_set);
    set<vector<DomainInt>> squashed =
        full_squeeze_tuples(tuple_set, initial_domains, (method == MLTTS_Eager));
    short_tuples = makeShortTupleList(squashed);

    cout << "# Squashed " + long_tuples->getName() + " : " << long_tuples->size() << " -> "
         << short_tuples.size() << "\n";
  }

  void setName(string name) {
    tuple_name = name;
  }

  string getName() const {
    return tuple_name;
  }

  SysInt size() const {
    return short_tuples.size();
  }

  vector<vector<pair<SysInt, DomainInt>>> const* tuplePtr() const {
    return &short_tuples;
  }

  // This validates a short c-tuple. This just means check no out of bound
  // variables
  void validateShortCTuples(SysInt var_count) {
    for(SysInt i = 0; i < (SysInt)short_tuples.size(); ++i) {
      for(SysInt j = 0; j < (SysInt)short_tuples[i].size(); ++j) {
        SysInt v = short_tuples[i][j].first;
        CHECK(v >= 0, "The short tuple '" + tuple_name + "' contains the negative variable index " +
                          tostring(v));
        CHECK(v < var_count, "The short tuple '" + tuple_name + "' contains variable index " +
                                 tostring(v) + ", but only contains " + tostring(var_count) +
                                 " variables (0 indexed)");
      }
    }
  }

  // This function validates a list of short tuples,
  // so checks for no repeated variables, no multiple literals
  void validateShortTuples(SysInt var_count) {
    validateShortCTuples(var_count);
    for(SysInt i = 0; i < (SysInt)short_tuples.size(); ++i) {
      for(SysInt j = 0; j < (SysInt)short_tuples[i].size(); ++j) {
        for(SysInt k = j + 1; k < (SysInt)short_tuples[i].size(); ++k) {
          SysInt v1 = short_tuples[i][j].first;
          SysInt v2 = short_tuples[i][k].first;
          if(v1 == v2) {
            if(short_tuples[i][j].second == short_tuples[i][k].second) {
              std::ostringstream oss;
              oss << "The short tuple '" + tuple_name +
                         "' contains a tuple with the repeated literal ";
              oss << "(" << short_tuples[i][j].first << "," << short_tuples[i][j].second << ")";
              CHECK(false, oss.str());
            } else {
              std::ostringstream oss;
              oss << "The short tuple '" + tuple_name +
                         "' contains a short tuple with the literals:\n";
              oss << "  (" << short_tuples[i][j].first << "," << short_tuples[i][j].second
                  << ") and ";
              oss << "(" << short_tuples[i][k].first << "," << short_tuples[i][k].second
                  << "), which both refer to the same variable.";
              CHECK(false, oss.str());
            }
          }
        }
      }
    }
  }
};

class ShortTupleListContainer {
  std::vector<ShortTupleList*> Internal_TupleList;

public:
  ShortTupleList* getNewShortTupleList(const vector<vector<pair<SysInt, DomainInt>>>& tuples) {
    ShortTupleList* tuplelist_ptr = new ShortTupleList(tuples);
    Internal_TupleList.push_back(tuplelist_ptr);
    return tuplelist_ptr;
  }

  ShortTupleList* getNewShortTupleList(TupleList* long_tuples, MapLongTuplesToShort method) {
    ShortTupleList* tuplelist_ptr = new ShortTupleList(long_tuples, method);
    Internal_TupleList.push_back(tuplelist_ptr);
    return tuplelist_ptr;
  }

  ShortTupleList* getShortTupleList(DomainInt num) {
    return Internal_TupleList[checked_cast<SysInt>(num)];
  }

  SysInt size() {
    return Internal_TupleList.size();
  }
};

/// The first GACtable implementation.
class LiteralSpecificLists {
public:
  TupleList* tuples;

  /// For each literal, a list of the tuples it is present in.
  vector<vector<vector<DomainInt>>> literal_specific_tuples;

  LiteralSpecificLists(TupleList* _tuples) : tuples(_tuples) {
    tuples->finalise_tuples();
    // For each literal, store the set of tuples which it allows.
    for(UnsignedSysInt i = 0; i < tuples->dom_size.size(); ++i) {
      for(DomainInt j = tuples->dom_smallest[i]; j <= tuples->dom_smallest[i] + tuples->dom_size[i];
          ++j) {
        vector<vector<DomainInt>> specific_tuples;
        for(SysInt k = 0; k < (*tuples).size(); ++k) {
          if((*tuples)[k][i] == j)
            specific_tuples.push_back((*tuples).get_vector(k));
        }
        literal_specific_tuples.push_back(specific_tuples);
        // D_ASSERT(literal_specific_tuples.size() - 1 == get_literal(i,j));
      }
    }
  }
};

inline LiteralSpecificLists* TupleList::getLitLists() {
  if(litlists == NULL)
    litlists = new LiteralSpecificLists(this);
  return litlists;
}

#endif
