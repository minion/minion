// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0













#ifndef CONSTRAINT_LEX_H
#define CONSTRAINT_LEX_H

template <typename VarArray1, typename VarArray2, BOOL Less = false>
struct LexLeqConstraint : public AbstractConstraint {
  virtual string constraintName() {
    if(Less)
      return "lexless";
    else
      return "lexleq";
  }

  typedef LexLeqConstraint<VarArray2, VarArray1, !Less> NegConstraintType;
  typedef typename VarArray1::value_type ArrayVarRef1;
  typedef typename VarArray2::value_type ArrayVarRef2;

  ReversibleInt alpha;
  ReversibleInt beta;
  ReversibleInt F;

  VarArray1 x;
  VarArray2 y;

  CONSTRAINT_ARG_LIST2(x, y);

  LexLeqConstraint(const VarArray1& _x, const VarArray2& _y) : alpha(), beta(), F(), x(_x), y(_y) {
    CHECK(x.size() == y.size(), "LexLeq and LexLess only work with equal length vectors");
    alpha = 0;
    CHECK(x.size() < 100000, "overflow in lexleq");
    if(Less)
      beta = x.size();
    else
      beta = 100000;
    F = 0;
  }

  virtual SysInt dynamicTriggerCount() {
    return x.size() * 4;
  }

  void setupTriggers() {
    for(SysInt i = 0; i < (SysInt)x.size(); i++) {
      moveTriggerInt(x[i], i * 4, LowerBound);
      moveTriggerInt(x[i], i * 4 + 1, UpperBound);
      moveTriggerInt(y[i], i * 4 + 2, LowerBound);
      moveTriggerInt(y[i], i * 4 + 3, UpperBound);
    }
  }

  virtual AbstractConstraint* reverseConstraint() {
    return new LexLeqConstraint<VarArray2, VarArray1, !Less>(y, x);
  }

  void updateAlpha(SysInt i) {
    SysInt n = x.size();
    if(Less) {
      if(i == n || i == beta) {
        getState().setFailed();
        return;
      }
      if(!x[i].isAssigned() || !y[i].isAssigned() ||
         x[i].assignedValue() != y[i].assignedValue()) {
        alpha = i;
        propagateDynInt(i * 4, DomainDelta::empty());
      } else
        updateAlpha(i + 1);
    } else {
      while(i < n) {
        if(!x[i].isAssigned() || !y[i].isAssigned() ||
           x[i].assignedValue() != y[i].assignedValue()) {
          alpha = i;
          propagateDynInt(i * 4, DomainDelta::empty());
          return;
        }
        i++;
      }
      F = true;
    }
  }

  ///////////////////////////////////////////////////////////////////////////////
  // updateBeta()
  void updateBeta(SysInt i) {
    SysInt a = alpha;
    while(i >= a) {
      if(x[i].min() < y[i].max()) {
        beta = i + 1;
        if(!(x[i].max() < y[i].min()))
          propagateDynInt(i * 4, DomainDelta::empty());
        return;
      }
      i--;
    }
    getState().setFailed();
  }

  virtual void propagateDynInt(SysInt i_in, DomainDelta) {
    const SysInt i = i_in / 4;
    PROP_INFO_ADDONE(Lex);
    if(F) {
      return;
    }
    SysInt a = alpha, b = beta;

    // Not sure why we need this, but we seem to.
    if(b <= a) {
      getState().setFailed();
      return;
    }

    if(Less) {
      if(i < a || i >= b)
        return;
    } else {
      if(i >= b)
        return;
    }

    if(i == a && i + 1 == b) {
      x[i].setMax(y[i].max() - 1);
      y[i].setMin(x[i].min() + 1);
      if(checkLex(i)) {
        F = true;
        return;
      }
    } else if(i == a && i + 1 < b) {
      x[i].setMax(y[i].max());
      y[i].setMin(x[i].min());
      if(checkLex(i)) {
        F = true;
        return;
      }
      if(x[i].isAssigned() && y[i].isAssigned() &&
         x[i].assignedValue() == y[i].assignedValue())
        updateAlpha(i + 1);
    } else if(a < i && i < b) {
      if((i == b - 1 && x[i].min() == y[i].max()) || x[i].min() > y[i].max())
        updateBeta(i - 1);
    }
  }

  BOOL checkLex(SysInt i) {
    if(Less) {
      return x[i].max() < y[i].min();
    } else {
      SysInt n = x.size();
      if(i == n - 1)
        return (x[i].max() <= y[i].min());
      else
        return (x[i].max() < y[i].min());
    }
  }

  virtual void fullPropagate() {
    setupTriggers();
    SysInt i, n = x.size();
    for(i = 0; i < n; i++) {
      if(!x[i].isAssigned())
        break;
      if(!y[i].isAssigned())
        break;
      if(x[i].assignedValue() != y[i].assignedValue())
        break;
    }
    if(i < n) {
      alpha = i;
      if(checkLex(i)) {
        F = true;
        return;
      }
      SysInt betaBound = -1;
      for(; i < n; i++) {
        if(x[i].min() > y[i].max())
          break;
        if(x[i].min() == y[i].max()) {
          if(betaBound == -1)
            betaBound = i;
        } else
          betaBound = -1;
      }
      if(!Less) {
        if(i == n)
          beta = 1000000;
        else if(betaBound == -1)
          beta = i;
        else
          beta = betaBound;
      } else {
        if(i == n)
          beta = n;
        if(betaBound == -1)
          beta = i;
        else
          beta = betaBound;
      }
      if(alpha >= beta)
        getState().setFailed();
      propagateDynInt((SysInt)alpha * 4,
                      DomainDelta::empty()); // initial propagation, if necessary.
    } else {
      if(Less)
        getState().setFailed();
      else
        F = true;
    }
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    D_ASSERT(vSize == (SysInt)x.size() + (SysInt)y.size());
    size_t xSize = x.size();

    for(size_t i = 0; i < xSize; i++) {
      if(v[i] < v[i + xSize])
        return true;
      if(v[i] > v[i + xSize])
        return false;
    }
    if(Less)
      return false;
    else
      return true;
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    size_t xSize = x.size();
    for(size_t i = 0; i < xSize; ++i) {
      DomainInt x_i_min = x[i].min();
      DomainInt y_iMax = y[i].max();

      if(x_i_min > y_iMax) {
        return false;
      }

      assignment.push_back(make_pair(i, x_i_min));
      assignment.push_back(make_pair(i + xSize, y_iMax));
      if(x_i_min < y_iMax)
        return true;
    }

    if(Less)
      return false;
    return true;
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> arrayCopy;
    for(UnsignedSysInt i = 0; i < x.size(); i++)
      arrayCopy.push_back(AnyVarRef(x[i]));

    for(UnsignedSysInt i = 0; i < y.size(); i++)
      arrayCopy.push_back(AnyVarRef(y[i]));
    return arrayCopy;
  }
};

template <typename VarArray1, typename VarArray2>
AbstractConstraint* BuildCT_LEXLEQ(const VarArray1& x, const VarArray2& y, ConstraintBlob&) {
  return new LexLeqConstraint<VarArray1, VarArray2>(x, y);
}

template <typename VarArray1, typename VarArray2>
AbstractConstraint* BuildCT_LEXLESS(const VarArray1& x, const VarArray2& y, ConstraintBlob&) {
  return new LexLeqConstraint<VarArray1, VarArray2, true>(x, y);
}

/* JSON
{ "type": "constraint",
  "name": "lexleq",
  "internal_name": "CT_LEXLEQ",
  "args": [ "read_list", "read_list" ]
}
*/

/* JSON
{ "type": "constraint",
  "name": "lexless",
  "internal_name": "CT_LEXLESS",
  "args": [ "read_list", "read_list" ]
}
*/
#endif
