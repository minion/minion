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

/** @help constraints;pow Description
The constraint

   pow(x,y,z)

ensures that x^y=z.
*/

/** @help constraints;pow Notes
This constraint is only available for positive domains x, y and z.
*/

#ifndef CONSTRAINT_POW_H
#define CONSTRAINT_POW_H

#include <math.h>
#include "../constraint_checkassign.h"

// This constraint is half-way to being changed from using
// LRINT to roundup and rounddown. Still don't quite have my head around
// this +0.5 business. Or at least I'm not convinced that it's OK.
// at least now it passes test_nightingale_pow.minion.

#ifndef LRINT
#define LRINT(x) static_cast<SysInt>(x + 0.5)
#endif

/// var1 ^ var2 = var3
template <typename VarRef1, typename VarRef2, typename VarRef3>
struct PowConstraint : public AbstractConstraint {
  virtual string constraint_name() {
    return "pow";
  }

  VarRef1 var1;
  VarRef2 var2;
  VarRef3 var3;

  CONSTRAINT_ARG_LIST3(var1, var2, var3);

  PowConstraint(VarRef1 _var1, VarRef2 _var2, VarRef3 _var3)
      : var1(_var1), var2(_var2), var3(_var3) {
    CHECK((var1.getInitialMin() >= 0 && var2.getInitialMin() >= 0 && var3.getInitialMin() >= 0),
          "The 'pow' constraint only supports non-negative numbers at present.");
    CHECK(var2.getInitialMin() != 0, "The 'pow' constraint (x^y = z) does not "
                                     "allow y to contain 0, to avoid the case "
                                     "0^0.");

    // Take this check out for now; it seems to be too restrictive. The
    // propagator
    // uses doubles anyway so integer overflow is not the problem.
    // BigInt pow=1;
    // for(SysInt i=0; i<var2.getInitialMax(); i++) {
    //    pow=pow*var1.getInitialMax();
    //    CHECKSIZE(pow, "Magnitude of domain bounds is too large in 'pow'
    //    constraint.");
    //}
  }

  virtual SysInt dynamic_trigger_count() {
    return 6;
  }

  void setup_triggers() {
    moveTriggerInt(var1, 3, LowerBound);
    moveTriggerInt(var2, 4, LowerBound);
    moveTriggerInt(var3, 5, LowerBound);
    moveTriggerInt(var1, 0, UpperBound);
    moveTriggerInt(var2, 1, UpperBound);
    moveTriggerInt(var3, 2, UpperBound);
  }

  inline SysInt roundup(double x) {
    // remember all numbers are non-negative in here, so
    // how are we going to hit the lower limit for ints?
    if(x < std::numeric_limits<SysInt>::min()) {
      return std::numeric_limits<SysInt>::min();
    } else {
      return static_cast<SysInt>(x); // Actually this should round up!
    }
  }

  inline SysInt rounddown(double x) {
    if(x > std::numeric_limits<SysInt>::max()) {
      return std::numeric_limits<SysInt>::max();
    } else {
      return checked_cast<SysInt>(x);
    }
  }

  double my_pow(DomainInt x, DomainInt y) {
    return pow(checked_cast<double>(x), checked_cast<double>(y));
  }

  double my_y(DomainInt x, DomainInt z) {
    return log(checked_cast<double>(z)) / log(checked_cast<double>(x));
  }

  double my_x(DomainInt y, DomainInt z) {
    return exp(log(checked_cast<double>(z)) / checked_cast<double>(y));
  }

  virtual void propagateDynInt(SysInt flag, DomainDelta) {
    PROP_INFO_ADDONE(Pow);
    switch(checked_cast<SysInt>(flag)) {
    case 3: {
      // var3 >= min(var1) ^ min(var2)
      var3.setMin(LRINT(my_pow(var1.getMin(), var2.getMin())));
      DomainInt var1_min = var1.getMin();
      if(var1_min > 1)
        // var2 <= log base max(var3) of min(var1)
        var2.setMax(LRINT(my_y(var1_min, var3.getMax())));
      break;
    }
    case 4:
      // var3>= min(var1) ^ min(var2)
      var3.setMin(LRINT(my_pow(var1.getMin(), var2.getMin())));
      var1.setMax(LRINT(my_x(var2.getMin(), var3.getMax())));
      break;

    case 5: {
      var1.setMin(LRINT(my_x(var2.getMax(), var3.getMin())));
      DomainInt var1_max = var1.getMax();
      if(var1_max > 1)
        var2.setMin(LRINT(my_y(var1_max, var3.getMin())));
      break;
    }
    case 0: {
      var3.setMax(
          rounddown(my_pow(var1.getMax(),
                           var2.getMax()))); // wraparound was occurring here, so use rounddown
      DomainInt var1_max = var1.getMax();
      if(var1_max > 1)
        var2.setMin(LRINT(my_y(var1_max, var3.getMin())));
      break;
    }
    case 1:
      var3.setMax(rounddown(my_pow(var1.getMax(), var2.getMax()))); // wraparound here.
      var1.setMin(LRINT(my_x(var2.getMax(), var3.getMin())));
      break;

    case 2: {
      var1.setMax(LRINT(my_x(var2.getMin(), var3.getMax())));
      DomainInt var1_min = var1.getMin();
      if(var1_min > 1)
        var2.setMax(LRINT(my_y(var1_min, var3.getMax())));
      break;
    }
    }
  }

  virtual void full_propagate() {
    setup_triggers();
    for(int i = 0; i < 6; ++i)
      propagateDynInt(i, DomainDelta::empty());
  }

  virtual BOOL check_assignment(DomainInt* v, SysInt v_size) {
    D_ASSERT(v_size == 3);
    return my_pow(v[0], v[1]) == v[2];
  }

  virtual vector<AnyVarRef> get_vars() {
    vector<AnyVarRef> v;
    v.push_back(var1);
    v.push_back(var2);
    v.push_back(var3);
    return v;
  }

  virtual bool get_satisfying_assignment(box<pair<SysInt, DomainInt>>& assignment) {
    for(DomainInt v1 = var1.getMin(); v1 <= var1.getMax(); ++v1) {
      if(var1.inDomain(v1)) {
        for(DomainInt v2 = var2.getMin(); v2 <= var2.getMax(); ++v2) {
          if(var2.inDomain(v2) && var3.inDomain(my_pow(v1, v2))) // implicit conversion here causes
          // a warning -- perh use roundup or
          // rounddown
          {
            assignment.push_back(make_pair(0, v1));
            assignment.push_back(make_pair(1, v2));
            assignment.push_back(make_pair(2, my_pow(v1, v2)));
            return true;
          }
        }
      }
    }
    return false;
  }

  // Function to make it reifiable in the lousiest way.
  virtual AbstractConstraint* reverse_constraint() {
    return forward_check_negation(this);
  }
};
#endif
