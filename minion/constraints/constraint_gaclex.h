// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0







#ifndef CONSTRAINT_GACLEX_H
#define CONSTRAINT_GACLEX_H

template <typename VarArray1, typename VarArray2, BOOL Less = false>
struct GacLexLeqConstraint : public AbstractConstraint {
  virtual string constraintName() {
    if(Less)
      return "lexless[rv]";
    else
      return "lexleq[rv]";
  }

  typedef GacLexLeqConstraint<VarArray2, VarArray1, !Less> NegConstraintType;
  typedef typename VarArray1::value_type ArrayVarRef1;
  typedef typename VarArray2::value_type ArrayVarRef2;

  ReversibleInt alpha;
  ReversibleInt beta;
  ReversibleInt F;

  VarArray1 x;
  VarArray2 y;

  virtual string fullOutputName() {
    VarArray1 cx(x);
    VarArray2 cy(y);
    for(SysInt i = 0; i < (SysInt)cx.size(); ++i) {
      if(cx[i].isAssigned() && cy[i].isAssigned() &&
         (cx[i].assignedValue() == cy[i].assignedValue())) {
        cx.erase(cx.begin() + i);
        cy.erase(cy.begin() + i);
        i--;
      }
    }
    return ConOutput::printCon(constraintName(), cx, cy);
  }

  vector<pair<DomainInt, DomainInt>> earliest_occurrence_x;
  vector<pair<DomainInt, DomainInt>> earliest_occurrence_y;

  GacLexLeqConstraint(const VarArray1& _x, const VarArray2& _y)
      : alpha(), beta(), F(), x(_x), y(_y) {
    CHECK(x.size() == y.size(), "gaclex only works on vectors of equal length");
    for(SysInt i = 0; i < (SysInt)x.size(); ++i) {
      if(x[i].getBaseVar() == y[i].getBaseVar()) {
        static bool printed = false;
        if(!printed) {
          std::cerr << "# A GacLex constraint has a variable repeated at index " << i << ",\n";
          std::cerr << "# So it will not achieve GAC\n";
          printed = true;
        }
      }
    }

    for(SysInt i = 0; i < (SysInt)x.size(); ++i) {
      Var base = x[i].getBaseVar();
      pair<DomainInt, DomainInt> pos = make_pair(0, i);
      for(SysInt j = 0; j < i; ++j) {
        if(x[j].getBaseVar() == base) {
          pos = make_pair(0, j);
          j = i;
        }
        if(y[j].getBaseVar() == base) {
          pos = make_pair(1, j);
          j = i;
        }
      }
      earliest_occurrence_x.push_back(pos);
    }

    for(SysInt i = 0; i < (SysInt)y.size(); ++i) {
      Var base = y[i].getBaseVar();
      pair<DomainInt, DomainInt> pos = make_pair(1, i);
      for(SysInt j = 0; j < i; ++j) {
        if(x[j].getBaseVar() == base) {
          pos = make_pair(0, j);
          j = i;
        }
        if(y[j].getBaseVar() == base) {
          pos = make_pair(1, j);
          j = i;
        }
      }
      earliest_occurrence_y.push_back(pos);
    }

    SysInt xSize = x.size();
    alpha = 0;
    if(Less)
      beta = xSize;
    else
      beta = 100000;
    F = 0;
  }

  virtual SysInt dynamicTriggerCount() {
    return std::max(x.size(), y.size()) * 4;
  }

  void setupTriggers() {
    SysInt xSize = x.size();
    for(SysInt i = 0; i < xSize; ++i) {
      moveTriggerInt(x[i], i * 4, LowerBound);
      moveTriggerInt(x[i], i * 4 + 1, UpperBound);
    }

    SysInt ySize = y.size();
    for(SysInt i = 0; i < ySize; ++i) {
      moveTriggerInt(y[i], i * 4 + 2, LowerBound);
      moveTriggerInt(y[i], i * 4 + 3, UpperBound);
    }
  }

  virtual AbstractConstraint* reverseConstraint() {
    return new GacLexLeqConstraint<VarArray2, VarArray1, !Less>(y, x);
  }

  void updateAlpha(SysInt i) {
    SysInt n = x.size();
    if(Less) {
      if(i == n || i == beta) {
        getState().setFailed(true);
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
    getState().setFailed(true);
  }

  virtual void propagateDynInt(SysInt i_in, DomainDelta) {
    const SysInt i = checked_cast<SysInt>(i_in) / 4;
    PROP_INFO_ADDONE(Lex);
    if(F) {
      return;
    }
    SysInt a = alpha, b = beta;

    // Not sure why we need this, but we seem to.
    if(b <= a) {
      getState().setFailed(true);
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

    gacpass();
  }

  void gacpass() {
    SysInt a = alpha;
    SysInt n = x.size();
    // SysInt b = beta;

    if(x[a].max() == y[a].max()) {
      // We need to find support for x[a] = max.
      for(SysInt i = a + 1; i < n; ++i) {
        DomainInt xVal;
        DomainInt yVal;
        if(earliest_occurrence_x[i].second != a) {
          if(earliest_occurrence_x[i].first == 0)
            xVal = x[i].min();
          else
            xVal = x[i].max();
        } else
          xVal = x[i].max();

        if(earliest_occurrence_y[i].second != a) {
          if(earliest_occurrence_y[i].first == 0)
            yVal = y[i].min();
          else
            yVal = y[i].max();
        } else
          yVal = y[i].max();

        if(xVal < yVal)
          goto y_case;

        if(xVal > yVal) {
          x[a].setMax(y[a].max() - 1);
          goto y_case;
        }
      }
    }

  y_case:

    // cout << "!!" << endl;
    if(x[a].min() == y[a].min()) {
      // We need to find support for y[a] = min.
      for(SysInt i = a + 1; i < n; ++i) {
        DomainInt xVal;
        DomainInt yVal;
        if(earliest_occurrence_x[i].second != a) {
          if(earliest_occurrence_x[i].first == 0)
            xVal = x[i].min();
          else
            xVal = x[i].max();
        } else
          xVal = x[i].min();

        if(earliest_occurrence_y[i].second != a) {
          if(earliest_occurrence_y[i].first == 0)
            yVal = y[i].min();
          else
            yVal = y[i].max();
        } else
          yVal = y[i].min();

        // cout << xVal << "." << yVal << endl;
        // cout << earliest_occurrence_x[i] << "." << earliest_occurrence_y[i]
        // << endl;

        if(xVal < yVal)
          return;

        if(xVal > yVal) {
          // cout << "Prop trigger!" << endl;
          // cout << a << ":" << i << ":" << (SysInt)(beta) << endl;
          y[a].setMin(x[a].min() + 1);
          return;
        }
      }
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
        getState().setFailed(true);
      propagateDynInt((SysInt)alpha * 4,
                      DomainDelta::empty()); // initial propagation, if necessary.
    } else {
      if(Less)
        getState().setFailed(true);
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
AbstractConstraint* BuildCT_GACLEXLEQ(const VarArray1& x, const VarArray2& y, ConstraintBlob&) {
  return new GacLexLeqConstraint<VarArray1, VarArray2>(x, y);
}

/* JSON
{ "type": "constraint",
  "name": "lexleq[rv]",
  "internal_name": "CT_GACLEXLEQ",
  "args": [ "read_list", "read_list" ]
}
*/

#endif
