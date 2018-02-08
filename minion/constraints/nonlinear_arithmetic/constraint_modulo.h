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

/** @help constraints;modulo Description
The constraint

   modulo(x,y,z)

ensures that x%y=z i.e. z is the remainder of dividing x by y.
For negative values, we ensure that:

y(x/y) + x%y = x

To be fully concrete, here are some examples:

3 % 5 = 3
-3 % 5 = 2
3 % -5 = -2
-3 % -5 = -3
*/

/** @help constraints;modulo References
help constraints div
*/

/** @help constraints;mod_undefzero Description
The constraint

   mod_undefzero(x,y,z)

is the same as mod except the constraint is always
true when y = 0, instead of false.

This constraint exists for certain special requirements.
In general, if you are unsure what constraint to use,
then what you want is a plain mod constraint!
*/

/** @help constraints;mod_undefzero References
help constraints mod
*/

#ifndef CONSTRAINT_MODULO_H
#define CONSTRAINT_MODULO_H

// Not a constraint -- just contains a checker.

template <typename T1, typename T2, typename T3, bool undef_zero>
class SlowModConstraint {

public:
  typedef typename common_var_type3<T1, T2, T3>::type var_common;
  typedef std::array<var_common, 3> var_type;

private:
  var_type vars;

public:
  SlowModConstraint(const T1& v1, const T2& v2, const T3& v3) {
    vars[0] = v1;
    vars[1] = v2;
    vars[2] = v3;
    DomainInt check1[3] = {-3, 5, 2};
    DomainInt check2[3] = {3, -5, -2};
    DomainInt check3[3] = {-3, -5, -3};
    CHECK(check_assignment(check1, 3), "Your copy of Minion has a broken mod "
                                       "operator. Please report to the "
                                       "developers!");
    CHECK(check_assignment(check2, 3), "Your copy of Minion has a broken mod "
                                       "operator. Please report to the "
                                       "developers!");
    CHECK(check_assignment(check3, 3), "Your copy of Minion has a broken mod "
                                       "operator. Please report to the "
                                       "developers!");
  }
  string constraint_name() const {
    if(undef_zero)
      return "modulo_undefzero";
    else
      return "modulo";
  }

  CONSTRAINT_ARG_LIST3(vars[0], vars[1], vars[2])

  var_type& get_vars() {
    return vars;
  }

  virtual bool check_assignment(DomainInt* v, SysInt v_size) {
    D_ASSERT(v_size == 3);
    if(v[1] == 0) {
      if(undef_zero)
        return (v[2] == 0);
      else
        return false;
    }
    // There might well be a slightly better way to do this, but I can't be
    // bothered to figure it out.
    DomainInt r = v[0] % abs(v[1]);
    if(r < 0)
      r += abs(v[1]);
    if(v[1] < 0 && r > 0)
      r -= abs(v[1]);
    return r == v[2];
  }
};

#include "../constraint_checkassign.h"
#include "../forward_checking.h"

template <typename V1, typename V2>
inline AbstractConstraint* BuildCT_MODULO(const V1& vars, const V2& var2, ConstraintBlob&) {
  D_ASSERT(vars.size() == 2);
  D_ASSERT(var2.size() == 1);
  typedef SlowModConstraint<typename V1::value_type, typename V1::value_type,
                            typename V2::value_type, false> ModCon;
  AbstractConstraint* modct =
      new CheckAssignConstraint<ModCon, false>(ModCon(vars[0], vars[1], var2[0]));
  return forwardCheckingCon(modct);
}

/* JSON
{ "type": "constraint",
  "name": "modulo",
  "internal_name": "CT_MODULO",
  "args": [ "read_2_vars", "read_var" ]
}
*/

template <typename V1, typename V2>
inline AbstractConstraint* BuildCT_MODULO_UNDEFZERO(const V1& vars, const V2& var2,
                                                    ConstraintBlob&) {
  D_ASSERT(vars.size() == 2);
  D_ASSERT(var2.size() == 1);
  // Do FC. Same as CT_MODULO except for last template parameter of
  // SlowModConstraint
  typedef SlowModConstraint<typename V1::value_type, typename V1::value_type,
                            typename V2::value_type, true> ModCon;
  AbstractConstraint* modct =
      new CheckAssignConstraint<ModCon, false>(ModCon(vars[0], vars[1], var2[0]));
  return forwardCheckingCon(modct);
}

/* JSON
{ "type": "constraint",
  "name": "modulo_undefzero",
  "internal_name": "CT_MODULO_UNDEFZERO",
  "args": [ "read_2_vars", "read_var" ]
}
*/

#endif
