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

#include "tries.h"

#ifdef P
#undef P
#endif

//#define P(x) cout << x << endl
#define P(x)

#include "table_common.h"

class TableData : public BaseTableData {
public:
  TableData(TupleList* _tupleData) : BaseTableData(_tupleData) {}

  // TODO : Optimise possibly?
  bool checkTuple(DomainInt* tuple, SysInt tupleSize) {
    D_ASSERT(tupleSize == getVarCount());
    for(SysInt i = 0; i < checked_cast<SysInt>(getNumOfTuples()); ++i) {
      if(std::equal(tuple, tuple + tupleSize, tupleData->getTupleptr(i)))
        return true;
    }
    return false;
  }
};

class TrieState {
  TrieData* data;
  vector<TrieObj**> trieCurrentSupport;
  vector<DomainInt> scratch_tuple;

public:
  TrieState(TrieData* _data) : data(_data) {
    const SysInt litcount = checked_cast<SysInt>(data->getLiteralCount());
    trieCurrentSupport.resize(litcount);
    for(SysInt i = 0; i < litcount; ++i) {
      trieCurrentSupport[i] = new TrieObj*[checked_cast<SysInt>(data->getVarCount())];
      for(SysInt j = 0; j < data->getVarCount(); ++j)
        trieCurrentSupport[i][j] = NULL;
    }
    scratch_tuple.resize(litcount);
  }

  template <typename VarArray>
  vector<DomainInt>* findSupportingTuple(const VarArray& vars, Literal lit) {
    // SysInt tupleSize = data->getVarCount();
    // SysInt length = data->getNumOfTuples();
    // SysInt* tupleData = data->getPointer();

    SysInt varIndex = lit.var;
    DomainInt val = lit.val;

    DomainInt litnum = data->getLiteralPos(lit);

    DomainInt newSupport = data->tupleTrieArrayptr->getTrie(varIndex).nextSupportingTuple(
        val, vars, trieCurrentSupport[checked_cast<SysInt>(litnum)]);

    if(newSupport < 0)
      return NULL;
    else {
      data->tupleTrieArrayptr->getTrie(varIndex).reconstructTuple(
          &scratch_tuple.front(), trieCurrentSupport[checked_cast<SysInt>(litnum)]);
      return &scratch_tuple;
    }
  }
};

class TableState {
  TableData* data;

  vector<DomainInt> scratch_tuple;
  /// The constructor of TableState should set up all structures to 'sensible'
  /// default values. It should not look for actual valid supports.
public:
  TableState(TableData* _data) : data(_data) {
    scratch_tuple.resize(checked_cast<SysInt>(data->getVarCount()));
  }

  /// This function should return a pointer to a valid tuple, if one exists,
  /// and return NULL if none exists. The vector should be stored inside the
  /// state, and need not be thread-safe.
  template <typename VarArray>
  vector<DomainInt>* findSupportingTuple(const VarArray& vars, Literal lit) {
    SysInt tupleSize = checked_cast<SysInt>(data->getVarCount());
    SysInt length = checked_cast<SysInt>(data->getNumOfTuples());
    DomainInt* tupleData = data->getPointer();

    for(SysInt i = 0; i < length; ++i) {
      DomainInt* tuple_start = tupleData + i * tupleSize;
      bool success = true;
      if(tuple_start[checked_cast<SysInt>(lit.var)] != lit.val)
        success = false;
      for(SysInt j = 0; j < tupleSize && success; ++j) {
        if(!vars[j].inDomain(tuple_start[j]))
          success = false;
      }
      if(success) {
        std::copy(tuple_start, tuple_start + tupleSize, scratch_tuple.begin());
        return &scratch_tuple;
      }
    }
    return NULL;
  }
};

template <typename VarArray, typename TableDataType = TrieData, typename TableStateType = TrieState>
struct NewTableConstraint : public AbstractConstraint {

  virtual string extendedName() {
    return "table(new)";
  }

  virtual string constraintName() {
    return "table";
  }

  CONSTRAINT_ARG_LIST2(vars, tuples);

  typedef typename VarArray::value_type VarRef;
  VarArray vars;

  TableDataType* data;

  TableStateType state;

  TupleList* tuples;

  NewTableConstraint(const VarArray& _vars, TupleList* _tuples)
      : vars(_vars), data(new TableDataType(_tuples)), state(data), tuples(_tuples) {
    CheckNotBound(vars, "table constraint");
    if(_tuples->tupleSize() != (SysInt)_vars.size()) {
      cout << "Table constraint: Number of variables " << _vars.size()
           << " does not match length of tuples " << _tuples->tupleSize() << "." << endl;
      FAIL_EXIT();
    }
  }

  LiteralSpecificLists* lists;

  void* CurrentSupport;

  virtual SysInt dynamicTriggerCount() {
    return checked_cast<SysInt>(data->getLiteralCount() * ((SysInt)vars.size() - 1));
  }

  virtual void propagateDynInt(SysInt trigger_pos, DomainDelta) {
    PROP_INFO_ADDONE(DynGACTable);

    SysInt propagated_literal = trigger_pos / ((SysInt)vars.size() - 1);

    Literal lit = data->getLiteralFromPos(propagated_literal);

    P(propagated_literal << "." << vars.size() << "." << lit.var << "." << lit.val);
    if(!vars[lit.var].inDomain(lit.val)) {
      // releaseTrigger(propagated_trig , TO_Backtrack);
      P("Quick return");
      return;
    }

    vector<DomainInt>* supporting_tuple = state.findSupportingTuple(vars, lit);
    if(supporting_tuple) {
      P("Found new support!");
      setup_watches(lit, propagated_literal, *supporting_tuple);
    } else {
      P("Failed to find new support");
      vars[lit.var].removeFromDomain(lit.val);
      // clear_watches(lit, propagated_literal);
    }
  }

  void setup_watches(Literal lit, DomainInt lit_pos, const vector<DomainInt>& support) {
    D_ASSERT(data->getLiteralPos(lit) == lit_pos);
    SysInt varsSize = vars.size();
    DomainInt trigPos = checked_cast<SysInt>(lit_pos * (varsSize - 1));
    for(SysInt v = 0; v < varsSize; ++v) {
      if(v != lit.var) {
        P(vars.size() << ".Watching " << v << "." << support[v] << " for " << lit.var << "."
                      << lit.val);
        D_ASSERT(vars[v].inDomain(support[v]));
        PROP_INFO_ADDONE(CounterA);
        moveTriggerInt(vars[v], trigPos, DomainRemoval, support[v], TO_Store);
        ++trigPos;
      }
    }
  }

  void clear_watches(Literal lit, SysInt lit_pos) {
    D_ASSERT(data->getLiteralPos(lit) == lit_pos);
    SysInt varsSize = vars.size();
    SysInt pos = lit_pos * (varsSize - 1);
    for(SysInt v = 0; v < varsSize; ++v) {
      releaseTriggerInt(pos, TO_Backtrack);
      ++pos;
    }
  }

  virtual void fullPropagate() {
    D_ASSERT(!vars.empty());

    for(UnsignedSysInt i = 0; i < vars.size(); ++i) {
      pair<DomainInt, DomainInt> bounds = data->getDomainBounds(i);
      vars[i].setMin(bounds.first);
      vars[i].setMax(bounds.second);

      if(getState().isFailed())
        return;

      for(DomainInt x = vars[i].min(); x <= vars[i].max(); ++x) {
        vector<DomainInt>* support = state.findSupportingTuple(vars, Literal(i, x));
        if(support) {
          setup_watches(Literal(i, x), data->getLiteralPos(Literal(i, x)), *support);
        } else {
          vars[i].removeFromDomain(x);
        }
      }
    }
  }

  //  inline DomainInt min(SysInt x, SysInt y) {return (x<y)?x:y; }
  //  inline DomainInt max(SysInt x, SysInt y) {return (x>y)?x:y; }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    pair<DomainInt, DomainInt> bounds = data->getDomainBounds(0);

    for(DomainInt x = max(vars[0].min(), bounds.first);
        x <= min(vars[0].max(), bounds.second); ++x) {
      if(vars[0].inDomain(x)) {
        vector<DomainInt>* support = state.findSupportingTuple(vars, Literal(0, x));
        if(support) {
          for(SysInt i = 0; i < (SysInt)vars.size(); i++) {
            D_ASSERT(vars[i].inDomain((*support)[i]));
            assignment.push_back(make_pair(i, (*support)[i]));
          }
          return true;
        }
      }
    }

    return false;
  }

  virtual AbstractConstraint* reverseConstraint() {
    return GACNegativeTableCon(vars, tuples);
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    return data->checkTuple(v, vSize);
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> anyvars;
    for(UnsignedSysInt i = 0; i < vars.size(); ++i)
      anyvars.push_back(vars[i]);
    return anyvars;
  }
};

template <typename VarArray>
AbstractConstraint* GACTableCon(const VarArray& vars, TupleList* tuples) {
  return new NewTableConstraint<VarArray>(vars, tuples);
}
