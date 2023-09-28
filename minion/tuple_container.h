// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef _TUPLE_CONTAINER_H
#define _TUPLE_CONTAINER_H

#include "system/system.h"

#define TUPLE_SQUASH
#ifdef TUPLE_SQUASH
#include "squash.hpp"
#endif

using namespace std;

#include <utility>
#include <vector>

// This file contains lists fo tuples, as required by table clan. This
// is required as these tuples can get big, and it's important to keep them
// as compactly as possible.

class LiteralSpecificLists;
class Nightingale;
class TupleTrieArray;
class Regin;
class EggShellData;
struct HaggisGACTuples;

inline size_t get_hashVal(DomainInt* ptr, SysInt length) {
  size_t hash_code = 1234;
  for(SysInt i = 0; i < length; ++i) {
    size_t val = checked_cast<SysInt>(ptr[i]);
    hash_code = (hash_code << 5) - hash_code + val;
  }
  hash_code += length;
  return hash_code;
}

inline SysInt get_hashVal(const vector<vector<DomainInt>>& vecs) {
  SysInt hash_code = 1234;
  for(SysInt i = 0; i < (SysInt)vecs.size(); ++i)
    for(SysInt j = 0; j < (SysInt)vecs[i].size(); ++j) {
      SysInt val = checked_cast<SysInt>(vecs[i][j]);
      hash_code = (hash_code << 5) - hash_code + val;
    }

  SysInt totalSize = 0;
  for(SysInt i = 0; i < (SysInt)vecs.size(); ++i)
    totalSize += vecs[i].size();
  hash_code += totalSize;
  return hash_code;
}

class TupleList {
  string tuple_name;

  LiteralSpecificLists* litlists;
  Nightingale* nightingale;
  TupleTrieArray* triearray;
  Regin* regin;
  EggShellData* egg;

  DomainInt* tupleData;
  SysInt tupleLength;
  SysInt numberOfTuples;
  bool tuplesLocked;

  SysInt hash_code;

public:
  size_t get_hash() {
    if(hash_code != 0)
      return hash_code;

    hash_code = get_hashVal(tupleData, tupleLength * numberOfTuples);
    return hash_code;
  }
  LiteralSpecificLists* getLitLists();
  Nightingale* getNightingale();
  TupleTrieArray* getTries();
  Regin* getRegin();
  EggShellData* getEggShellData(size_t varcount);

  /// Get raw pointer to the tuples.
  DomainInt* getPointer() {
    return tupleData;
  }

  /// Get number of tuples.
  DomainInt size() const {
    return numberOfTuples;
  }

  // Get size of a particular tuple.
  DomainInt tupleSize() const {
    return tupleLength;
  }

  // This function is temporary while the new interface is being designed.
  vector<DomainInt> getVector(SysInt pos) const {
    vector<DomainInt> vec(tupleLength);
    for(SysInt i = 0; i < tupleLength; ++i)
      vec[i] = tupleData[pos * tupleLength + i];
    return vec;
  }

  TupleList(const vector<vector<DomainInt>>& tuple_list)
      : litlists(NULL),
        nightingale(NULL),
        triearray(NULL),
        regin(NULL),
        egg(NULL),
        tuplesLocked(false),
        hash_code(0) {
    numberOfTuples = tuple_list.size();
    tupleLength = tuple_list[0].size();
    tupleData = new DomainInt[numberOfTuples * tupleLength];
    for(SysInt i = 0; i < numberOfTuples; ++i)
      for(SysInt j = 0; j < tupleLength; ++j) {
        tupleData[i * tupleLength + j] = tuple_list[i][j];
      }
    finalise_tuples();
  }

  TupleList(DomainInt _numtuples, DomainInt _tuplelength)
      : litlists(NULL),
        nightingale(NULL),
        triearray(NULL),
        regin(NULL),
        egg(NULL),
        tupleLength(checked_cast<SysInt>(_tuplelength)),
        numberOfTuples(checked_cast<SysInt>(_numtuples)),
        tuplesLocked(false),
        hash_code(0) {
    tupleData = new DomainInt[numberOfTuples * tupleLength];
  }

  const DomainInt* operator[](SysInt pos) const {
    return getTupleptr(pos);
  }

  const DomainInt* getTupleptr(SysInt pos) const {
    D_ASSERT(pos >= 0 && (pos < numberOfTuples || (numberOfTuples == 0 && pos == 0)));
    return tupleData + pos * tupleLength;
  }

  void setName(string name) {
    tuple_name = name;
  }

  string getName() const {
    return tuple_name;
  }

  /// Original smallest value from each domain.
  vector<DomainInt> domSmallest;
  /// Original size of each domain.
  vector<DomainInt> domSize;

  /// Total number of literals in the variables at the start of search.
  DomainInt literalNum;

  /// Used by getLiteral.
  vector<vector<DomainInt>> _MapVarsToLiteral;

  /// Used to get a variable/value pair from a literal
  vector<pair<SysInt, DomainInt>> _MapLiteralToVars;

  /// Maps a variable/value pair to a literal.
  DomainInt getLiteral(DomainInt varNum_in, DomainInt domNum) {
    const SysInt varNum = checked_cast<SysInt>(varNum_in);
    D_ASSERT(varNum >= 0 && varNum < tupleSize());
    D_ASSERT(domNum >= domSmallest[varNum]);
    D_ASSERT(domNum < domSmallest[varNum] + domSize[varNum]);
    return _MapVarsToLiteral[varNum][checked_cast<SysInt>(domNum - domSmallest[varNum])];
  }

  pair<SysInt, DomainInt> getVarvalFromLiteral(DomainInt literal) {
    return _MapLiteralToVars[checked_cast<SysInt>(literal)];
  }

  /// Sets up the variable/value pair to literal mapping, used by getLiteral.
  void finalise_tuples() {
    if(tuplesLocked)
      return;
    tuplesLocked = true;

    DomainInt arity = tupleSize();

    // Set up the table of tuples.
    for(SysInt i = 0; i < arity; ++i) {
      if(size() == 0) {
        domSmallest.push_back(0);
        domSize.push_back(0);
      } else {
        DomainInt minVal = getTupleptr(0)[i];
        DomainInt maxVal = getTupleptr(0)[i];
        for(SysInt j = 1; j < size(); ++j) {
          minVal = mymin(minVal, getTupleptr(j)[i]);
          maxVal = mymax(maxVal, getTupleptr(j)[i]);
        }
        domSmallest.push_back(minVal);
        domSize.push_back(maxVal - minVal + 1);
      }
    }

    SysInt domSizeSize = checked_cast<SysInt>(domSize.size());
    _MapVarsToLiteral.resize(checked_cast<SysInt>(domSizeSize));
    // For each variable / value pair, get a literal
    DomainInt literalCount = 0;

    for(SysInt i = 0; i < domSizeSize; ++i) {
      _MapVarsToLiteral[i].resize(checked_cast<SysInt>(domSize[i]) + 1);
      for(SysInt j = 0; j < domSize[i]; ++j) {
        _MapVarsToLiteral[i][j] = literalCount;
        _MapLiteralToVars.push_back(make_pair(i, j + domSmallest[checked_cast<SysInt>(i)]));
        D_ASSERT(getLiteral(i, j + domSmallest[i]) == literalCount);
        D_ASSERT(getVarvalFromLiteral(literalCount).first == i);
        D_ASSERT(getVarvalFromLiteral(literalCount).second ==
                 j + domSmallest[checked_cast<SysInt>(i)]);
        ++literalCount;
      }
    }
    literalNum = literalCount;
  }
};

class TupleListContainer {
  std::vector<TupleList*> InternalTupleList;

public:
  TupleList* getNewTupleList(DomainInt numtuples, DomainInt tuplelength) {
    TupleList* tuplelistPtr = new TupleList(numtuples, tuplelength);
    InternalTupleList.push_back(tuplelistPtr);
    return tuplelistPtr;
  }

  TupleList* getNewTupleList(const vector<vector<DomainInt>>& tuples) {
    size_t tuple_hash = get_hashVal(tuples);
    for(SysInt i = 0; i < (SysInt)InternalTupleList.size(); ++i) {
      bool equal = true;

      if(InternalTupleList[i]->get_hash() != tuple_hash)
        equal = false;
      else {
        TupleList* ptr = InternalTupleList[i];
        if((SysInt)tuples.size() != ptr->size() || (SysInt)tuples[0].size() != ptr->tupleSize()) {
          equal = false;
        } else {
          for(SysInt j = 0; j < (SysInt)tuples.size() && equal; ++j) {
            for(SysInt k = 0; k < (SysInt)tuples[j].size() && equal; ++k)
              if(tuples[j][k] != ptr->getTupleptr(j)[k]) {
                equal = false;
              }
          }
        }

        if(equal)
          return InternalTupleList[i];
      }
    }

    TupleList* tuplelistPtr = new TupleList(tuples);
    InternalTupleList.push_back(tuplelistPtr);
    return tuplelistPtr;
  }

  TupleList* getTupleList(DomainInt num) {
    return InternalTupleList[checked_cast<SysInt>(num)];
  }

  SysInt size() {
    return InternalTupleList.size();
  }
};

class ShortTupleList {
  vector<vector<pair<SysInt, DomainInt>>> shortTuples;
  vector<set<DomainInt>> initialDomainList;
  string tuple_name;

  std::map<std::vector<std::pair<DomainInt, DomainInt>>, HaggisGACTuples*> hgt;

public:
  template <typename Vars>
  HaggisGACTuples* getHaggisData(const Vars& vars);

  // NOTE: initial domains may be empty, in which case they should be ignored.
  vector<set<DomainInt>> initialDomains() {
    return initialDomainList;
  }

  ShortTupleList(const vector<vector<pair<SysInt, DomainInt>>>& _shortTuples)
      : shortTuples(_shortTuples) {}

  // method : 0 - nothing, 1 - long tuples, 2 - eager, 3 - lazy
  ShortTupleList(TupleList* longTuples, MapLongTuplesToShort method) {
    D_ASSERT(method != MLTTS_NoMap);

    set<vector<DomainInt>> tupleSet;

    for(SysInt i = 0; i < longTuples->size(); ++i)
      tupleSet.insert(longTuples->getVector(i));

    if(method == MLTTS_KeepLong) {
      shortTuples = makeShortTupleList(tupleSet);
      return;
    }

    D_ASSERT(method == MLTTS_Lazy || method == MLTTS_Eager);

    initialDomainList = gatherDomains(tupleSet);
    set<vector<DomainInt>> squashed =
        full_squeeze_tuples(tupleSet, initialDomainList, (method == MLTTS_Eager));
    shortTuples = makeShortTupleList(squashed);

    cout << "# Squashed " + longTuples->getName() + " : " << longTuples->size() << " -> "
         << shortTuples.size() << "\n";
  }

  void setName(string name) {
    tuple_name = name;
  }

  string getName() const {
    return tuple_name;
  }

  SysInt size() const {
    return shortTuples.size();
  }

  vector<vector<pair<SysInt, DomainInt>>> const* tuplePtr() const {
    return &shortTuples;
  }

  // This validates a short c-tuple. This just means check no out of bound
  // variables
  void validateShortCTuples(SysInt varCount) {
    for(SysInt i = 0; i < (SysInt)shortTuples.size(); ++i) {
      for(SysInt j = 0; j < (SysInt)shortTuples[i].size(); ++j) {
        SysInt v = shortTuples[i][j].first;
        CHECK(v >= 0, "The short tuple '" + tuple_name + "' contains the negative variable index " +
                          tostring(v));
        CHECK(v < varCount, "The short tuple '" + tuple_name + "' contains variable index " +
                                tostring(v) + ", but only contains " + tostring(varCount) +
                                " variables (0 indexed)");
      }
    }
  }

  // This function validates a list of short tuples,
  // so checks for no repeated variables, no multiple literals
  void validateShortTuples(SysInt varCount) {
    validateShortCTuples(varCount);
    for(SysInt i = 0; i < (SysInt)shortTuples.size(); ++i) {
      for(SysInt j = 0; j < (SysInt)shortTuples[i].size(); ++j) {
        for(SysInt k = j + 1; k < (SysInt)shortTuples[i].size(); ++k) {
          SysInt v1 = shortTuples[i][j].first;
          SysInt v2 = shortTuples[i][k].first;
          if(v1 == v2) {
            if(shortTuples[i][j].second == shortTuples[i][k].second) {
              std::ostringstream oss;
              oss << "The short tuple '" + tuple_name +
                         "' contains a tuple with the repeated literal ";
              oss << "(" << shortTuples[i][j].first << "," << shortTuples[i][j].second << ")";
              CHECK(false, oss.str());
            } else {
              std::ostringstream oss;
              oss << "The short tuple '" + tuple_name +
                         "' contains a short tuple with the literals:\n";
              oss << "  (" << shortTuples[i][j].first << "," << shortTuples[i][j].second
                  << ") and ";
              oss << "(" << shortTuples[i][k].first << "," << shortTuples[i][k].second
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
  std::vector<ShortTupleList*> InternalTupleList;

public:
  ShortTupleList* getNewShortTupleList(const vector<vector<pair<SysInt, DomainInt>>>& tuples) {
    ShortTupleList* tuplelistPtr = new ShortTupleList(tuples);
    InternalTupleList.push_back(tuplelistPtr);
    return tuplelistPtr;
  }

  ShortTupleList* getNewShortTupleList(TupleList* longTuples, MapLongTuplesToShort method) {
    ShortTupleList* tuplelistPtr = new ShortTupleList(longTuples, method);
    InternalTupleList.push_back(tuplelistPtr);
    return tuplelistPtr;
  }

  ShortTupleList* getShortTupleList(DomainInt num) {
    return InternalTupleList[checked_cast<SysInt>(num)];
  }

  SysInt size() {
    return InternalTupleList.size();
  }
};

/// The first GACtable implementation.
class LiteralSpecificLists {
public:
  TupleList* tuples;

  /// For each literal, a list of the tuples it is present in.
  vector<vector<vector<DomainInt>>> literalSpecificTuples;

  LiteralSpecificLists(TupleList* _tuples) : tuples(_tuples) {
    tuples->finalise_tuples();
    // For each literal, store the set of tuples which it allows.
    for(UnsignedSysInt i = 0; i < tuples->domSize.size(); ++i) {
      for(DomainInt j = tuples->domSmallest[i]; j <= tuples->domSmallest[i] + tuples->domSize[i];
          ++j) {
        vector<vector<DomainInt>> specific_tuples;
        for(SysInt k = 0; k < (*tuples).size(); ++k) {
          if((*tuples)[k][i] == j)
            specific_tuples.push_back((*tuples).getVector(k));
        }
        literalSpecificTuples.push_back(specific_tuples);
        // D_ASSERT(literalSpecificTuples.size() - 1 == getLiteral(i,j));
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
