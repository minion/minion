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

/*****

  General pow semantics:
  0^neg : undef
  0^pos : 1
  (nonzero)^0 : 0

  So, only leaves 0^0. Lets make it 1 (agreed between C, Python and Java).

*****/



#ifndef CONSTRAINT_POW_CHECK_H
#define CONSTRAINT_POW_CHECK_H

#include "../constraints/constraint_checkassign.h"
#include <math.h>

// Not a constraint -- just contains a checker.

template<typename T1, typename T2, typename T3, bool undef_zero>
class PowConstraint_Check
{
  StateObj* stateObj;
public:
  typedef typename common_var_type3<T1,T2,T3>::type var_common;
  typedef std::array<var_common, 3> var_type;
private:
   var_type vars;
public:

  PowConstraint_Check(StateObj* _stateObj, const T1& v1, const T2& v2, const T3& v3)
  : stateObj(_stateObj)
  {
    vars[0] = v1; vars[1] = v2; vars[2] = v3;
  }

  string constraint_name() const
  { if(undef_zero) return "pow_undefzero"; else return "pow"; }

  CONSTRAINT_ARG_LIST3(vars[0], vars[1], vars[2])

  var_type& get_vars()
  { return vars; }

  double my_pow(DomainInt x, DomainInt y)
  { return pow(checked_cast<double>(x), checked_cast<double>(y));}

  virtual bool check_assignment(DomainInt* v, SysInt v_size)
  {
    D_ASSERT(v_size == 3);
    if(v[0] == 0)
    {
      if(v[1] == 0)
        return v[2] == 1;
      if(v[1] > 0)
        return v[2] == 0;
      else
        return false;
    }
    else
      return my_pow(v[0], v[1]) == v[2];
  }
};


#endif
