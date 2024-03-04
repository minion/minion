// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0



#ifndef CONSTRAINT_DYNAMIC_UNARY_ININTERVALSET_H
#define CONSTRAINT_DYNAMIC_UNARY_ININTERVALSET_H

// Checks if a variable is in a fixed set.
template <typename Var>
struct WatchInIntervalSetConstraint : public AbstractConstraint {
  virtual string constraintName() {
    return "w-inintervalset";
  }

  CONSTRAINT_ARG_LIST2(var, intervals); // Does redump.

  Var var;

  vector<std::pair<DomainInt, DomainInt>> intervals;

  template <typename T>
  WatchInIntervalSetConstraint(const Var& _var, const T& _vals) : var(_var) {
    CHECK(_vals.size() % 2 == 0, "Second argument of w-inintervalset "
                                 "constraint represents a list of pairs so it "
                                 "must have an even number of values.");
    CHECK(_vals.size() > 0, "w-inintervalset constraint requires at least one interval.");
    for(int i = 0; i < (SysInt)_vals.size(); i = i + 2) {
      CHECK(_vals[i] <= _vals[i + 1], "Malformed interval in w-inintervalset constraint.");
      // Allow intervals with just one value.
      intervals.push_back(std::make_pair(_vals[i], _vals[i + 1]));
    }

    std::stable_sort(intervals.begin(), intervals.end());

    for(SysInt i = 0; i < (SysInt)intervals.size() - 1; i++) {
      CHECK(intervals[i].second < intervals[i + 1].first - 1,
            "Intervals overlapping or touching in w-inintervalset constraint.");
    }
  }

  virtual SysInt dynamicTriggerCount() {
    return 1;
  }

  virtual void fullPropagate() {
    if(intervals.empty()) {
      getState().setFailed();
      return;
    }
    var.setMin(intervals.front().first);
    var.setMax(intervals.back().second);

    if(var.isBound()) {
      // May as well pass DomainRemoval
      moveTriggerInt(var, 0, DomainChanged);
      propagateDynInt(0, DomainDelta::empty());
    } else {
      // Prune everything between intervals.
      for(SysInt i = 0; i < (SysInt)intervals.size() - 1; ++i)
        for(DomainInt pos = intervals[i].second + 1; pos < intervals[i + 1].first; ++pos)
          var.removeFromDomain(pos);
    }
  }

  virtual void propagateDynInt(SysInt dt, DomainDelta) {
    PROP_INFO_ADDONE(WatchInIntervalSet);
    // If we are in here, we have a bounds variable.
    D_ASSERT(var.isBound());
    // This is basically lifted from "sparse SysInt bound vars"
    vector<std::pair<DomainInt, DomainInt>>::iterator it_low = std::lower_bound(
        intervals.begin(), intervals.end(), std::make_pair(var.min(), var.min()));
    if(it_low == intervals.end()) {
      // we must have reached the lower bound of the last interval, in which
      // case nothing more needs to be done.
      return;
    } else {
      // Check if the lower bound is in the interval below the one found.
      if(it_low != intervals.begin() && (*(it_low - 1)).second < var.min()) {
        // Lower bound of var is not in the interval below it_low. Prune it to
        // the bottom of it_low.
        var.setMin((*it_low).first);
      }
    }

    SysInt pos = intervals.size() - 1;
    while(pos >= 0 && intervals[pos].first > var.max())
      pos--;
    if(pos < 0) {
      // This will cause failure
      var.setMax(intervals[0].first - 1);
      return;
    }

    var.setMax(intervals[pos].second);
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    D_ASSERT(vSize == 1);
    vector<std::pair<DomainInt, DomainInt>>::iterator it_high =
        std::lower_bound(intervals.begin(), intervals.end(), std::make_pair(v[0], v[0]));
    // We really need to test two intervals
    // we could do some much clever reasoning to save two checks, but it's
    // easier
    // and possibly slightly cheaper to just do this.
    if(it_high != intervals.end() && (v[0] >= (*it_high).first && v[0] <= (*it_high).second))
      return true;

    // Step back one if required (watch out for falling off start!)
    if(it_high != intervals.begin())
      it_high--;
    else
      return false;

    if(v[0] >= (*it_high).first && v[0] <= (*it_high).second)
      return true;
    return false;
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> vars;
    vars.reserve(1);
    vars.push_back(var);
    return vars;
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    /// TODO: Make faster
    for(SysInt i = 0; i < (SysInt)intervals.size(); ++i) {
      for(DomainInt d = intervals[i].first; d <= intervals[i].second; d++) {
        if(var.inDomain(d)) {
          assignment.push_back(make_pair(0, d));
          return true;
        }
      }
    }
    return false;
  }

  virtual AbstractConstraint* reverseConstraint() {
    // It is its own reverse.
    vector<DomainInt> negintervals;

    DomainInt lowerbound = var.initialMin() < intervals.front().first - 1
                               ? var.initialMin()
                               : intervals.front().first - 1;
    DomainInt upperbound = var.initialMax() > intervals.back().second + 1
                               ? var.initialMax()
                               : intervals.back().second + 1;

    negintervals.push_back(lowerbound); // first interval starts at lowerbound
    negintervals.push_back(intervals.front().first - 1);

    // Make negintervals between intervals.
    for(int i = 0; i < (SysInt)intervals.size() - 1; i++) {
      negintervals.push_back(intervals[i].second + 1);
      negintervals.push_back(intervals[i + 1].first - 1);
    }

    negintervals.push_back(intervals.back().second + 1);
    negintervals.push_back(upperbound);

    return new WatchInIntervalSetConstraint<Var>(var, negintervals);
  }
};

template <typename VarArray1>
AbstractConstraint* BuildCT_WATCHED_ININTERVALSET(const VarArray1& _varArray_1,
                                                  const ConstraintBlob& b) {
  return new WatchInIntervalSetConstraint<typename VarArray1::value_type>(_varArray_1[0],
                                                                          b.constants[0]);
}

/* JSON
  { "type": "constraint",
    "name": "w-inintervalset",
    "internal_name": "CT_WATCHED_ININTERVALSET",
    "args": [ "read_var", "read_constant_list" ]
  }
*/
#endif
