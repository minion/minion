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

/*****

  General pow semantics:
  0^neg : undef
  0^pos : 1
  (nonzero)^0 : 0

  So, only leaves 0^0. Lets make it 1 (agreed between C, Python and Java).

*****/

#ifndef CONSTRAINT_POW_CHECK_H
#define CONSTRAINT_POW_CHECK_H

#include "../constraint_checkassign.h"
#include <math.h>

// Not a constraint -- just contains a checker.

template <typename T1, typename T2, typename T3, bool undef_zero>
class PowConstraint_Check {

public:
  typedef typename common_var_type3<T1, T2, T3>::type var_common;
  typedef std::array<var_common, 3> var_type;

private:
  var_type vars;

public:
  PowConstraint_Check(const T1& v1, const T2& v2, const T3& v3)

  {
    vars[0] = v1;
    vars[1] = v2;
    vars[2] = v3;
  }

  string constraint_name() const {
    if(undef_zero)
      return "pow_undefzero";
    else
      return "pow";
  }

  CONSTRAINT_ARG_LIST3(vars[0], vars[1], vars[2])

  var_type& get_vars() {
    return vars;
  }

  double my_pow(DomainInt x, DomainInt y) {
    return pow(checked_cast<double>(x), checked_cast<double>(y));
  }

  virtual bool check_assignment(DomainInt* v, SysInt v_size) {
    D_ASSERT(v_size == 3);
    if(v[0] == 0) {
      if(v[1] == 0)
        return v[2] == 1;
      if(v[1] > 0)
        return v[2] == 0;
      else
        return false;
    } else
      return my_pow(v[0], v[1]) == v[2];
  }
};

#include "../forward_checking.h"
#include "constraint_pow.h"

template <typename V1, typename V2>
inline AbstractConstraint* BuildCT_POW(const V1& vars, const V2& var2, ConstraintBlob&) {
  D_ASSERT(vars.size() == 2);
  D_ASSERT(var2.size() == 1);
  // vars1 is special to avoid 0^0
  if(vars[0].getInitialMin() < 0 || vars[1].getInitialMin() <= 0 || var2[0].getInitialMin() < 0) {
    typedef PowConstraint_Check<typename V1::value_type, typename V1::value_type,
                                typename V2::value_type, false>
        PowConC;
    AbstractConstraint* pow =
        new CheckAssignConstraint<PowConC, false>(PowConC(vars[0], vars[1], var2[0]));
    // Now wrap it in new FC thing. Horrible hackery.
    return forwardCheckingCon(pow);
  }

  return new PowConstraint<typename V1::value_type, typename V1::value_type,
                           typename V2::value_type>(vars[0], vars[1], var2[0]);
}

/* JSON
{ "type": "constraint",
  "name": "pow",
  "internal_name": "CT_POW",
  "args": [ "read_2_vars", "read_var" ]
}
*/
#endif
