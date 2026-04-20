// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef VARIABLE_ORDERINGS_H
#define VARIABLE_ORDERINGS_H

#include <cfloat>
#include "../variables/AnyVarRef.h"
// #include "../system/system.h"
// #include "../memory_management/reversible_vals.h"

// Result of a branching decision. Empty when search has exhausted the order.
struct BranchChoice {
  AnyVarRef var;
  DomainInt val;
  bool isAux;
};

template <typename T>
DomainInt chooseVal(T& var, ValOrder vo) {
  switch(vo.type) {
  case VALORDER_ASCEND: return var.min();
  case VALORDER_DESCEND: return var.max();

  case VALORDER_RANDOM: {
    uniform_int_distribution<int> dist100(0, 100);
    uniform_int_distribution<int> dist2(0, 1);
    if(vo.bias != 0) {
      if(vo.bias > 0) {
        if(dist100(GET_GLOBAL(global_random_gen)) <= vo.bias) {
          return var.max();
        }
      } else {
        if(dist100(GET_GLOBAL(global_random_gen)) <= -vo.bias) {
          return var.min();
        }
      }
    }
    if(var.isBound()) {
      switch(dist2(GET_GLOBAL(global_random_gen))) {
      case 0: return var.min();
      case 1: return var.max();
      default: abort();
      }
    }
    DomainInt minVal = var.min();
    DomainInt maxVal = var.max();
    uniform_int_distribution<int> distdom(0, checked_cast<SysInt>(maxVal - minVal));
    DomainInt val = distdom(GET_GLOBAL(global_random_gen)) + minVal;
    D_ASSERT(val >= minVal);
    D_ASSERT(val <= maxVal);
    if(var.inDomain(val))
      return val;
    switch(dist2(GET_GLOBAL(global_random_gen))) {
    case 0: {
      val--;
      while(!var.inDomain(val))
        val--;
      return val;
    }
    case 1: {
      val++;
      while(!var.inDomain(val))
        val++;
      return val;
    }
    default: abort();
    }
  }

  default: abort();
  }
}

struct VariableOrder {
  vector<AnyVarRef> varOrder;

  VariableOrder() {}
  VariableOrder(const vector<AnyVarRef>& _varOrder) : varOrder(_varOrder) {}

  // Returns a chosen (var, val) plus whether it belongs to an aux block.
  // Empty BranchChoice::var.data means no branch possible.
  virtual BranchChoice pickVarVal() = 0;

  vector<AnyVarRef>& getVars() {
    return varOrder;
  }

  virtual bool hasAuxVars() const {
    return false;
  }

  // Position of the first aux variable in the concatenated getVars() view.
  // Still used by -X-tabulation (TabSearchManager) to size its tabulation
  // tables. Branch-level aux detection goes via triple::isAux now.
  virtual DomainInt auxVarStart() const {
    abort();
  }

  virtual ~VariableOrder() {}
};

// Container for multiple variable orderings
struct MultiBranch : public VariableOrder {
  vector<shared_ptr<VariableOrder>> vovector;
  Reversible<SysInt> pos;

  bool hasAux;

  virtual bool hasAuxVars() const {
    return hasAux;
  }

  virtual DomainInt auxVarStart() const {
    D_ASSERT(hasAuxVars());
    return variableOffset.back();
  }

  // need to patch up the returned variable index
  vector<DomainInt> variableOffset;

  MultiBranch(const vector<shared_ptr<VariableOrder>> _vovector, bool _hasAux)
      : vovector(_vovector), hasAux(_hasAux) {
    pos = 0;
    variableOffset.resize(vovector.size());
    variableOffset[0] = 0;
    for(SysInt i = 1; i < (SysInt)vovector.size(); i++) {
      const vector<AnyVarRef>& vars = vovector[i - 1]->getVars();
      variableOffset[i] = variableOffset[i - 1] + vars.size();
      varOrder.insert(varOrder.end(), vars.begin(), vars.end());
    }

    const vector<AnyVarRef>& lastvars = vovector.back()->getVars();
    varOrder.insert(varOrder.end(), lastvars.begin(), lastvars.end());
  }

  BranchChoice pickVarVal() {
    SysInt pos2 = pos;

    BranchChoice t = vovector[pos2]->pickVarVal();
    while(!t.var.data) {
      pos2++;
      if(pos2 == (SysInt)vovector.size()) {
        return BranchChoice{};
      }

      t = vovector[pos2]->pickVarVal();
    }
    pos = pos2;
    // Aux status is a property of which block we branched from: the last
    // block is aux iff hasAux.
    if(hasAux && pos2 == (SysInt)vovector.size() - 1) {
      t.isAux = true;
    }
    return t;
  }
};

struct StaticBranch : public VariableOrder {
  vector<ValOrder> valOrder;
  Reversible<SysInt> pos;

  StaticBranch(const vector<AnyVarRef>& _varOrder, const vector<ValOrder>& _valOrder)
      : VariableOrder(_varOrder), valOrder(_valOrder), pos() {
    pos = 0;
    D_ASSERT(varOrder.size() == valOrder.size());
  }

  BranchChoice pickVarVal() {
    SysInt vSize = varOrder.size();

    while(pos < vSize && varOrder[pos].isAssigned())
      pos = pos + 1;

    if(pos == vSize)
      return BranchChoice{};

    DomainInt val = chooseVal(varOrder[pos], valOrder[pos]);

    return BranchChoice{varOrder[pos], val, false};
  }
};

struct SDFBranch : public VariableOrder {
  vector<ValOrder> valOrder;

  SDFBranch(const vector<AnyVarRef>& _varOrder, const vector<ValOrder>& _valOrder)
      : VariableOrder(_varOrder), valOrder(_valOrder) {}

  // THIS DOES NOT DO SDF -- just an approximation with the bounds.

  BranchChoice pickVarVal() {
    SysInt length = varOrder.size();
    SysInt smallestDom = -1;
    DomainInt domSize = DomainInt_Max;

    for(SysInt i = 0; i < length; ++i) {
      DomainInt maxval = varOrder[i].max();
      DomainInt minval = varOrder[i].min();

      if((maxval != minval) && ((maxval - minval) < domSize)) {
        domSize = maxval - minval;
        smallestDom = i;
        if(maxval - minval == 1) { // Binary domain, must be smallest
          break;
        }
      }
    }

    if(smallestDom == -1) { // all assigned
      return BranchChoice{};
    }

    DomainInt val = chooseVal(varOrder[smallestDom], valOrder[smallestDom]);

    return BranchChoice{varOrder[smallestDom], val, false};
  }
};

struct SlowStaticBranch : public VariableOrder {
  vector<ValOrder> valOrder;

  SlowStaticBranch(const vector<AnyVarRef>& _varOrder, const vector<ValOrder>& _valOrder)
      : VariableOrder(_varOrder), valOrder(_valOrder) {}

  BranchChoice pickVarVal() {
    UnsignedSysInt vSize = varOrder.size();
    UnsignedSysInt pos = 0;
    while(pos < vSize && varOrder[pos].isAssigned())
      ++pos;

    if(pos == vSize)
      return BranchChoice{};

    DomainInt val = chooseVal(varOrder[pos], valOrder[pos]);

    return BranchChoice{varOrder[pos], val, false};
  }
};

#ifdef WDEG
// see Boosting Systematic Search by Weighting Constraints by Boussemart et al
struct WdegBranch : public VariableOrder {
  vector<ValOrder> valOrder;

  WdegBranch(const vector<AnyVarRef>& _varOrder, const vector<ValOrder>& _valOrder)
      : VariableOrder(_varOrder), valOrder(_valOrder) {}

  BranchChoice pickVarVal() {
    SysInt best = varOrder.size(); // the variable with the best score so far (init to none)
    SysInt best_score = -1;        //... and its score (all true scores are positive)
    size_t varOrderSize = varOrder.size();
    for(size_t i = 0; i < varOrderSize; i++) { // we will find the score for each var
      // cout << "i=" << i << endl;
      // cout << "best=" << best << endl;
      // cout << "best_score=" << best_score << endl;
      AnyVarRef v = varOrder[i];
      if(v.isAssigned()) {
        // cout << "assigned -- stop" << endl;
        continue;
      }
      SysInt base_wdeg = checked_cast<SysInt>(v.getBaseWdeg());
      // cout << "basewdeg=" << base_wdeg << endl;
      if(base_wdeg <= best_score) {
        // cout << "too low before deductions" << endl;
        continue; // stop if base score is too low before deductions
      }
      vector<AbstractConstraint*>* constrs = v.getConstraints();
      size_t constrsSize = constrs->size();
      for(size_t j = 0; j < constrsSize; j++) { // find constrs to be deducted from var wdeg
        AbstractConstraint* c = (*constrs)[j];
        // cout << "con wdeg=" << c->getWdeg() << endl;
        vector<AnyVarRef>* c_vars = c->getVarsSingleton();
        size_t c_varsSize = c_vars->size();
        SysInt uninst = 0;
        for(size_t k = 0; k < c_varsSize; k++)
          if(!(*c_vars)[k].isAssigned())
            if(++uninst > 1) { // when multiple unassigned we needn't deduct
              // cout << "don't deduct" << endl;
              break;
            }
        if(uninst <= 1) {
          D_ASSERT(uninst == 1);
          // cout << "deduct" << endl;
          base_wdeg -= c->getWdeg();
          if(base_wdeg <= best_score) {
            // cout << "too low during deductions" << endl;
            break;
          }
        }
      }
      // cout << "basewdeg=" << base_wdeg << endl;
      if(best_score < base_wdeg) {
        // cout << "replacing top score" << endl;
        best_score = base_wdeg;
        best = i;
      }
    }

    // new bit. pn
    if(best == varOrder.size())
      return BranchChoice{};

    DomainInt val = chooseVal(varOrder[best], valOrder[best]);
    return BranchChoice{varOrder[best], val, false};
  }
};

struct DomOverWdegBranch : VariableOrder {
  vector<ValOrder> valOrder;

  DomOverWdegBranch(const vector<AnyVarRef>& _varOrder, const vector<ValOrder>& _valOrder)
      : VariableOrder(_varOrder), valOrder(_valOrder) {}

  BranchChoice pickVarVal() {
    // cout << "using domoverwdeg" << endl;
    SysInt best = varOrder.size(); // the variable with the best score so far (init to none)
    float best_score = FLT_MAX;    //... and its score (all true scores are positive)
    size_t varOrderSize = varOrder.size();
    bool anyUnassigned = false;
    for(size_t i = 0; i < varOrderSize; i++) { // we will find the score for each var
      // cout << "i=" << i << endl;
      // cout << "best=" << best << endl;
      // cout << "best_score=" << best_score << endl;
      AnyVarRef v = varOrder[i];
      if(v.isAssigned()) {
        // cout << "assigned -- stop" << endl;
        continue;
      } else if(!anyUnassigned) {
        // always use the first unassigned as a fallback in case later
        // calculations don't find
        // any variables with finite score
        best = i;
        anyUnassigned = true;
      }
      const SysInt domSize_approx = checked_cast<SysInt>(v.max() - v.min() + 1);
      DomainInt base_wdeg = v.getBaseWdeg();
      // cout << "basewdeg=" << base_wdeg << endl;
      if((float)domSize_approx / checked_cast<SysInt>(base_wdeg) >= best_score) {
        // cout << "too high before deductions" << endl;
        continue; // stop if base score is too low before deductions
      }
      vector<AbstractConstraint*>* constrs = v.getConstraints();
      size_t constrsSize = constrs->size();
      for(size_t j = 0; j < constrsSize; j++) { // find constrs to be deducted from var wdeg
        AbstractConstraint* c = (*constrs)[j];
        // cout << "con wdeg=" << c->getWdeg() << endl;
        vector<AnyVarRef>* c_vars = c->getVarsSingleton();
        size_t c_varsSize = c_vars->size();
        SysInt uninst = 0;
        for(size_t k = 0; k < c_varsSize; k++)
          if(!(*c_vars)[k].isAssigned())
            if(++uninst > 1) { // when multiple unassigned we needn't deduct
              // cout << "don't deduct" << endl;
              break;
            }
        if(uninst <= 1) {
          D_ASSERT(uninst == 1);
          // cout << "deduct" << endl;
          base_wdeg -= c->getWdeg();
          if((float)domSize_approx / checked_cast<SysInt>(base_wdeg) >= best_score) {
            // cout << "too high during deductions,base_wdeg=" << base_wdeg <<
            // endl;
            break;
          }
        }
      }
      // cout << "basewdeg=" << base_wdeg << endl;
      if(best_score > (float)domSize_approx / checked_cast<SysInt>(base_wdeg)) {
        // cout << "replacing top score" << endl;
        best_score = (float)domSize_approx / checked_cast<SysInt>(base_wdeg);
        best = i;
      }
    }
    // cout << "dec=" << best << "@" << best_score << endl;
    D_ASSERT(!anyUnassigned || best != varOrderSize);

    // new bit. pn
    if(best == varOrder.size())
      return BranchChoice{};

    DomainInt val = chooseVal(varOrder[best], valOrder[best]);
    return BranchChoice{varOrder[best], val, false};
  }
};
#endif

struct SRFBranch : VariableOrder {
  vector<ValOrder> valOrder;

  SRFBranch(const vector<AnyVarRef>& _varOrder, const vector<ValOrder>& _valOrder)
      : VariableOrder(_varOrder), valOrder(_valOrder) {}

  BranchChoice pickVarVal() {
    SysInt length = varOrder.size();
    SysInt smallestDom = length;

    float ratio = 2;

    for(SysInt i = 0; i < length; ++i) {
      DomainInt maxval = varOrder[i].max();
      DomainInt minval = varOrder[i].min();

      DomainInt original_minval = varOrder[i].initialMin();
      DomainInt originalMaxval = varOrder[i].initialMax();

      float new_ratio = (checked_cast<float>(maxval - minval) * 1.0) /
                        checked_cast<float>(originalMaxval - original_minval);
      if((maxval != minval) && (new_ratio < ratio)) {
        ratio = new_ratio;
        smallestDom = i;
      }
    }

    if(smallestDom == length)
      return BranchChoice{};

    DomainInt val = chooseVal(varOrder[smallestDom], valOrder[smallestDom]);
    return BranchChoice{varOrder[smallestDom], val, false};
  }
};

struct LDFBranch : VariableOrder {
  vector<ValOrder> valOrder;

  LDFBranch(const vector<AnyVarRef>& _varOrder, const vector<ValOrder>& _valOrder)
      : VariableOrder(_varOrder), valOrder(_valOrder) {}

  BranchChoice pickVarVal() {
    SysInt length = varOrder.size();

    SysInt pos = 0;
    while(pos < length && varOrder[pos].isAssigned())
      ++pos;
    if(pos == length) {
      return BranchChoice{};
    }

    SysInt largestDom = pos;
    DomainInt domSize = varOrder[pos].max() - varOrder[pos].min();

    ++pos;

    for(; pos < length; ++pos) {
      DomainInt maxval = varOrder[pos].max();
      DomainInt minval = varOrder[pos].min();

      if(maxval - minval > domSize) {
        domSize = maxval - minval;
        largestDom = pos;
      }
    }

    DomainInt val = chooseVal(varOrder[largestDom], valOrder[largestDom]);
    return BranchChoice{varOrder[largestDom], val, false};
  }
};

struct ConflictBranch : VariableOrder {
  // Implements the conflict variable ordering from
  // "Last Conflict based Reasoning", Lecoutre et al, ECAI 06.
  vector<ValOrder> valOrder;

  Reversible<SysInt> pos;

  VariableOrder* innervarorder;

  ConflictBranch(const vector<AnyVarRef>& _varOrder, const vector<ValOrder>& _valOrder,
                 VariableOrder* _innervarorder)
      : VariableOrder(_varOrder),
        valOrder(_valOrder),
        pos(),
        innervarorder(_innervarorder),
        last_returnedValOrder(VALORDER_NONE),
        in_conflict(false) {
    pos = 0;
    pos2 = 0;
  }

  // pos maintains a 'depth' which is actually the number of calls to
  // pickVarVals. pos and pos2 are used to see if we have backtracked.

  SysInt pos2;

  // Last variable returned by pickVarVal, and the ValOrder entry it was
  // paired with. last_returnedVar.data is empty before the first call.
  AnyVarRef last_returnedVar;
  ValOrder last_returnedValOrder;
  bool in_conflict;

  BranchChoice pickVarVal() {
    if(in_conflict && last_returnedVar.isAssigned()) {
      // Conflict var successfully assigned — come out of conflict mode.
      in_conflict = false;
    }

    if(pos2 > pos) {
      pos2 = pos;

      if(last_returnedVar.data && !last_returnedVar.isAssigned()) {
        // Backtracked since the last call. Go into conflict mode.
        in_conflict = true;
      }
    }

    pos = pos + 1;
    pos2++;

    if(in_conflict) {
      DomainInt val = chooseVal(last_returnedVar, last_returnedValOrder);
      return BranchChoice{last_returnedVar, val, false};
    } else {
      BranchChoice temp = innervarorder->pickVarVal();
      if(temp.var.data) {
        // Remember the var and its ValOrder entry. varOrder/valOrder are
        // parallel arrays, so scan by pointer identity to recover the
        // ValOrder for the var the inner order picked.
        bool found = false;
        for(SysInt i = 0; i < (SysInt)varOrder.size(); ++i) {
          if(varOrder[i].data.get() == temp.var.data.get()) {
            last_returnedVar = temp.var;
            last_returnedValOrder = valOrder[i];
            found = true;
            break;
          }
        }
        D_ASSERT(found);
        (void)found;
        D_ASSERT(temp.var.inDomain(temp.val));
      } else {
        last_returnedVar = AnyVarRef{};
      }
      return temp;
    }
  }
};

#endif
