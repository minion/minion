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

/** @help constraints;lexleq[rv] Description
  The constraint

  lexle[rv](vec0, vec1)

  takes two vectors vec0 and vec1 of the same length and ensures that
  vec0 is lexicographically less than or equal to vec1 in any solution.
*/

/** @help constraints;lexleq[rv] Notes
  This constraint achieves GAC even when some variables are repeated in
  vec0 and vec1. However, the extra propagation this achieves is rarely
  worth the extra work.
*/

/** @help constraints;lexleq[rv] References
  See also

  help constraints lexleq[quick]

  for a much faster logically identical constraint, with lower
  propagation.
*/

#ifndef CONSTRAINT_GACLEX_H
#define CONSTRAINT_GACLEX_H

template <typename VarArray1, typename VarArray2, BOOL Less = false>
struct GacLexLeqConstraint : public AbstractConstraint {
  virtual string constraint_name() {
    if (Less)
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

  virtual string full_output_name() {
    VarArray1 cx(x);
    VarArray2 cy(y);
    for (SysInt i = 0; i < (SysInt)cx.size(); ++i) {
      if (cx[i].isAssigned() && cy[i].isAssigned() &&
          (cx[i].getAssignedValue() == cy[i].getAssignedValue())) {
        cx.erase(cx.begin() + i);
        cy.erase(cy.begin() + i);
        i--;
      }
    }
    return ConOutput::print_con(constraint_name(), cx, cy);
  }

  vector<pair<DomainInt, DomainInt>> earliest_occurrence_x;
  vector<pair<DomainInt, DomainInt>> earliest_occurrence_y;

  GacLexLeqConstraint(const VarArray1 &_x, const VarArray2 &_y)
      : alpha(), beta(), F(), x(_x), y(_y) {
    CHECK(x.size() == y.size(), "gaclex only works on vectors of equal length");
    for (SysInt i = 0; i < (SysInt)x.size(); ++i) {
      if (x[i].getBaseVar() == y[i].getBaseVar())
        D_FATAL_ERROR("GacLex constraints cannot have a variable repeated at an index");
    }

    for (SysInt i = 0; i < (SysInt)x.size(); ++i) {
      Var base = x[i].getBaseVar();
      pair<DomainInt, DomainInt> pos = make_pair(0, i);
      for (SysInt j = 0; j < i; ++j) {
        if (x[j].getBaseVar() == base) {
          pos = make_pair(0, j);
          j = i;
        }
        if (y[j].getBaseVar() == base) {
          pos = make_pair(1, j);
          j = i;
        }
      }
      earliest_occurrence_x.push_back(pos);
    }

    for (SysInt i = 0; i < (SysInt)y.size(); ++i) {
      Var base = y[i].getBaseVar();
      pair<DomainInt, DomainInt> pos = make_pair(1, i);
      for (SysInt j = 0; j < i; ++j) {
        if (x[j].getBaseVar() == base) {
          pos = make_pair(0, j);
          j = i;
        }
        if (y[j].getBaseVar() == base) {
          pos = make_pair(1, j);
          j = i;
        }
      }
      earliest_occurrence_y.push_back(pos);
    }

    SysInt x_size = x.size();
    alpha = 0;
    if (Less)
      beta = x_size;
    else
      beta = 100000;
    F = 0;
  }

  virtual SysInt dynamic_trigger_count() {
    return std::max(x.size(), y.size()) * 4;
  }

  void setup_triggers() {
    SysInt x_size = x.size();
    for (SysInt i = 0; i < x_size; ++i) {
      moveTriggerInt(x[i], i*4, LowerBound);
      moveTriggerInt(x[i], i*4 + 1, UpperBound);
    }

    SysInt y_size = y.size();
    for (SysInt i = 0; i < y_size; ++i) {
      moveTriggerInt(y[i], i*4 + 2, LowerBound);
      moveTriggerInt(y[i], i*4 + 3, UpperBound);
    }
  }

  virtual AbstractConstraint *reverse_constraint() {
    return new GacLexLeqConstraint<VarArray2, VarArray1, !Less>(y, x);
  }

  void updateAlpha(SysInt i) {
    SysInt n = x.size();
    if (Less) {
      if (i == n || i == beta) {
        getState().setFailed(true);
        return;
      }
      if (!x[i].isAssigned() || !y[i].isAssigned() ||
          x[i].getAssignedValue() != y[i].getAssignedValue()) {
        alpha = i;
        propagateDynInt(i*4);
      } else
        updateAlpha(i + 1);
    } else {
      while (i < n) {
        if (!x[i].isAssigned() || !y[i].isAssigned() ||
            x[i].getAssignedValue() != y[i].getAssignedValue()) {
          alpha = i;
          propagateDynInt(i*4);
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
    while (i >= a) {
      if (x[i].getMin() < y[i].getMax()) {
        beta = i + 1;
        if (!(x[i].getMax() < y[i].getMin()))
          propagateDynInt(i*4);
        return;
      }
      i--;
    }
    getState().setFailed(true);
  }

  virtual void propagateDynInt(SysInt i_in) {
    const SysInt i = checked_cast<SysInt>(i_in)/4;
    PROP_INFO_ADDONE(Lex);
    if (F) {
      return;
    }
    SysInt a = alpha, b = beta;

    // Not sure why we need this, but we seem to.
    if (b <= a) {
      getState().setFailed(true);
      return;
    }

    if (Less) {
      if (i < a || i >= b)
        return;
    } else {
      if (i >= b)
        return;
    }

    if (i == a && i + 1 == b) {
      x[i].setMax(y[i].getMax() - 1);
      y[i].setMin(x[i].getMin() + 1);
      if (checkLex(i)) {
        F = true;
        return;
      }
    } else if (i == a && i + 1 < b) {
      x[i].setMax(y[i].getMax());
      y[i].setMin(x[i].getMin());
      if (checkLex(i)) {
        F = true;
        return;
      }
      if (x[i].isAssigned() && y[i].isAssigned() &&
          x[i].getAssignedValue() == y[i].getAssignedValue())
        updateAlpha(i + 1);
    } else if (a < i && i < b) {
      if ((i == b - 1 && x[i].getMin() == y[i].getMax()) || x[i].getMin() > y[i].getMax())
        updateBeta(i - 1);
    }

    gacpass();
  }

  void gacpass() {
    SysInt a = alpha;
    SysInt n = x.size();
    // SysInt b = beta;

    if (x[a].getMax() == y[a].getMax()) {
      // We need to find support for x[a] = max.
      for (SysInt i = a + 1; i < n; ++i) {
        DomainInt x_val;
        DomainInt y_val;
        if (earliest_occurrence_x[i].second != a) {
          if (earliest_occurrence_x[i].first == 0)
            x_val = x[i].getMin();
          else
            x_val = x[i].getMax();
        } else
          x_val = x[i].getMax();

        if (earliest_occurrence_y[i].second != a) {
          if (earliest_occurrence_y[i].first == 0)
            y_val = y[i].getMin();
          else
            y_val = y[i].getMax();
        } else
          y_val = y[i].getMax();

        if (x_val < y_val)
          goto y_case;

        if (x_val > y_val) {
          x[a].setMax(y[a].getMax() - 1);
          goto y_case;
        }
      }
    }

  y_case:

    // cout << "!!" << endl;
    if (x[a].getMin() == y[a].getMin()) {
      // We need to find support for y[a] = min.
      for (SysInt i = a + 1; i < n; ++i) {
        DomainInt x_val;
        DomainInt y_val;
        if (earliest_occurrence_x[i].second != a) {
          if (earliest_occurrence_x[i].first == 0)
            x_val = x[i].getMin();
          else
            x_val = x[i].getMax();
        } else
          x_val = x[i].getMin();

        if (earliest_occurrence_y[i].second != a) {
          if (earliest_occurrence_y[i].first == 0)
            y_val = y[i].getMin();
          else
            y_val = y[i].getMax();
        } else
          y_val = y[i].getMin();

        // cout << x_val << "." << y_val << endl;
        // cout << earliest_occurrence_x[i] << "." << earliest_occurrence_y[i]
        // << endl;

        if (x_val < y_val)
          return;

        if (x_val > y_val) {
          // cout << "Prop trigger!" << endl;
          // cout << a << ":" << i << ":" << (SysInt)(beta) << endl;
          y[a].setMin(x[a].getMin() + 1);
          return;
        }
      }
    }
  }

  BOOL checkLex(SysInt i) {
    if (Less) {
      return x[i].getMax() < y[i].getMin();
    } else {
      SysInt n = x.size();
      if (i == n - 1)
        return (x[i].getMax() <= y[i].getMin());
      else
        return (x[i].getMax() < y[i].getMin());
    }
  }

  virtual void full_propagate() {
    setup_triggers();
    SysInt i, n = x.size();
    for (i = 0; i < n; i++) {
      if (!x[i].isAssigned())
        break;
      if (!y[i].isAssigned())
        break;
      if (x[i].getAssignedValue() != y[i].getAssignedValue())
        break;
    }
    if (i < n) {
      alpha = i;
      if (checkLex(i)) {
        F = true;
        return;
      }
      SysInt betaBound = -1;
      for (; i < n; i++) {
        if (x[i].getMin() > y[i].getMax())
          break;
        if (x[i].getMin() == y[i].getMax()) {
          if (betaBound == -1)
            betaBound = i;
        } else
          betaBound = -1;
      }
      if (!Less) {
        if (i == n)
          beta = 1000000;
        else if (betaBound == -1)
          beta = i;
        else
          beta = betaBound;
      } else {
        if (i == n)
          beta = n;
        if (betaBound == -1)
          beta = i;
        else
          beta = betaBound;
      }
      if (alpha >= beta)
        getState().setFailed(true);
      propagateDynInt((SysInt)alpha*4); // initial propagation, if necessary.
    } else {
      if (Less)
        getState().setFailed(true);
      else
        F = true;
    }
  }

  virtual BOOL check_assignment(DomainInt *v, SysInt v_size) {
    D_ASSERT(v_size == (SysInt)x.size() + (SysInt)y.size());
    size_t x_size = x.size();

    for (size_t i = 0; i < x_size; i++) {
      if (v[i] < v[i + x_size])
        return true;
      if (v[i] > v[i + x_size])
        return false;
    }
    if (Less)
      return false;
    else
      return true;
  }

  virtual bool get_satisfying_assignment(box<pair<SysInt, DomainInt>> &assignment) {
    size_t x_size = x.size();
    for (size_t i = 0; i < x_size; ++i) {
      DomainInt x_i_min = x[i].getMin();
      DomainInt y_i_max = y[i].getMax();

      if (x_i_min > y_i_max) {
        return false;
      }

      assignment.push_back(make_pair(i, x_i_min));
      assignment.push_back(make_pair(i + x_size, y_i_max));
      if (x_i_min < y_i_max)
        return true;
    }

    if (Less)
      return false;
    return true;
  }

  virtual vector<AnyVarRef> get_vars() {
    vector<AnyVarRef> array_copy;
    for (UnsignedSysInt i = 0; i < x.size(); i++)
      array_copy.push_back(AnyVarRef(x[i]));

    for (UnsignedSysInt i = 0; i < y.size(); i++)
      array_copy.push_back(AnyVarRef(y[i]));
    return array_copy;
  }
};

template <typename VarArray1, typename VarArray2>
AbstractConstraint *BuildCT_GACLEXLEQ(const VarArray1 &x, const VarArray2 &y, ConstraintBlob &) {
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
