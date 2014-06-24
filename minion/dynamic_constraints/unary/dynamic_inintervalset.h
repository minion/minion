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

/** @help constraints;w-inintervalset Description
The constraint w-inintervalset(x, [a1,a2, b1,b2, ... ]) ensures that the value
of x belongs to one of the intervals  {a1,...,a2}, {b1,...,b2} etc. The list of
intervals must be given in numerical order.
*/

#ifndef CONSTRAINT_DYNAMIC_UNARY_ININTERVALSET_H
#define CONSTRAINT_DYNAMIC_UNARY_ININTERVALSET_H

// Checks if a variable is in a fixed set.
template<typename Var>
  struct WatchInIntervalSetConstraint : public AbstractConstraint
{
  virtual string constraint_name()
    { return "w-inintervalset"; }

  CONSTRAINT_ARG_LIST2(var, intervals);   // Does redump.

  Var var;

  vector<std::pair<DomainInt, DomainInt> > intervals;

  template<typename T>
  WatchInIntervalSetConstraint(StateObj* _stateObj, const Var& _var, const T& _vals) :
  AbstractConstraint(_stateObj), var(_var)
    {
        CHECK(_vals.size()%2==0, "Second argument of w-inintervalset constraint represents a list of pairs so it must have an even number of values.");
        CHECK(_vals.size()>0,"w-inintervalset constraint requires at least one interval.");
        for(int i=0; i<(SysInt)_vals.size(); i=i+2) {
            CHECK(_vals[i]<=_vals[i+1], "Malformed interval in w-inintervalset constraint.");
            // Allow intervals with just one value.
            intervals.push_back(std::make_pair(_vals[i], _vals[i+1]));
        }

        std::stable_sort(intervals.begin(), intervals.end());

        for(SysInt i=0; i<(SysInt)intervals.size()-1; i++) {
            CHECK(intervals[i].second<intervals[i+1].first-1, "Intervals overlapping or touching in w-inintervalset constraint.");
        }

    }

  virtual SysInt dynamic_trigger_count()
    { return 1; }

  virtual void full_propagate()
  {
    DynamicTrigger* dt = dynamic_trigger_start();
    if(intervals.empty())
    {
        getState(stateObj).setFailed(true);
        return;
    }
    var.setMin(intervals.front().first);
    var.setMax(intervals.back().second);

    if(var.isBound())
    {
      // May as well pass DomainRemoval
      var.addDynamicTrigger(dt, DomainChanged);
      propagate(NULL);
    }
    else
    {
        // Prune everything between intervals.
      for(SysInt i = 0; i < (SysInt)intervals.size() - 1; ++i)
        for(DomainInt pos = intervals[i].second + 1; pos < intervals[i+1].first; ++pos)
        var.removeFromDomain(pos);
    }
  }


  virtual void propagate(DynamicTrigger* dt)
  {
    PROP_INFO_ADDONE(WatchInIntervalSet);
    // If we are in here, we have a bounds variable.
    D_ASSERT(var.isBound());
    // This is basically lifted from "sparse SysInt bound vars"
    vector<std::pair<DomainInt,DomainInt> >::iterator it_low = std::lower_bound(intervals.begin(), intervals.end(), std::make_pair(var.getMin(), var.getMin()));
    if(it_low == intervals.end())
    {
        // we must have reached the lower bound of the last interval, in which case nothing more needs to be done.
        return;
    }
    else
    {
        // Check if the lower bound is in the interval below the one found.
        if( it_low!=intervals.begin() && (*(it_low-1)).second<var.getMin()) {
            // Lower bound of var is not in the interval below it_low. Prune it to the bottom of it_low.
            cout << "Pruning lower bound:"<< (*it_low).first << endl;
            var.setMin((*it_low).first);
        }

    }

    vector<std::pair<DomainInt,DomainInt> >::iterator it_high = std::lower_bound(intervals.begin(), intervals.end(), std::make_pair(var.getMax(), var.getMax()));
    if(it_high == intervals.end())
    {
      //var.setMax(*(it_high - 1).second);  // didn't we already do this in full prop?
      return;
    }

    var.setMax((*it_high).second);
  }

  virtual BOOL check_assignment(DomainInt* v, SysInt v_size)
  {
    D_ASSERT(v_size == 1);
    vector<std::pair<DomainInt,DomainInt> >::iterator it_high = std::lower_bound(intervals.begin(), intervals.end(), std::make_pair(v[0], v[0]));
    // We really need to test two intervals
    // we could do some much clever reasoning to save two checks, but it's easier
    // and possibly slightly cheaper to just do this.
    if(it_high != intervals.end() && (v[0]>=(*it_high).first && v[0]<=(*it_high).second))
      return true;

    // Step back one if required (watch out for falling off start!)
    if(it_high != intervals.begin())
      it_high--;
    else
      return false;

    if(v[0]>=(*it_high).first && v[0]<=(*it_high).second)
      return true;
    return false;
  }

  virtual vector<AnyVarRef> get_vars()
  {
    vector<AnyVarRef> vars;
    vars.reserve(1);
    vars.push_back(var);
    return vars;
  }

  virtual bool get_satisfying_assignment(box<pair<SysInt,DomainInt> >& assignment)
  {
    /// TODO: Make faster
    for(SysInt i = 0; i < (SysInt)intervals.size(); ++i)
    {
        for(DomainInt d = intervals[i].first; d <= intervals[i].second; d++) {
            if(var.inDomain(d)) {
                assignment.push_back(make_pair(0, d));
                return true;
            }
        }
    }
    return false;
  }

  virtual AbstractConstraint* reverse_constraint()
  {
      // It is its own reverse.
      vector<DomainInt> negintervals;

      DomainInt lowerbound=var.getInitialMin()<intervals.front().first-1?var.getInitialMin():intervals.front().first-1;
      DomainInt upperbound=var.getInitialMax()>intervals.back().second+1?var.getInitialMax():intervals.back().second+1;

      negintervals.push_back(lowerbound);   // first interval starts at lowerbound
      negintervals.push_back(intervals.front().first-1);

      // Make negintervals between intervals.
      for(int i=0; i<(SysInt)intervals.size()-1; i++) {
          negintervals.push_back(intervals[i].second+1);
          negintervals.push_back(intervals[i+1].first-1);
      }

      negintervals.push_back(intervals.back().second+1);
      negintervals.push_back(upperbound);

      return new WatchInIntervalSetConstraint<Var>(stateObj, var, negintervals);
  }

};

#endif
