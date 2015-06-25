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

/** @help constraints;div Description
The constraint

   div(x,y,z)

ensures that floor(x/y)=z.

For example:

10/3 = 3
(-10)/3 = -4
10/(-3) = -4
(-10)/(-3) = 3

div and mod satisfy together the condition that:

y*(x/y) + x % y = x

The constraint is always false when y = 0
*/


/** @help constraints;div References
help constraints modulo
*/


/** @help constraints;div_undefzero Description
The constraint

   div_undefzero(x,y,z)

is the same as div (it ensures that floor(x/y)=z)
except the constraint is always true when y = 0,
instead of false.

This constraint exists for certain special requirements.
In general, if you are unsure what constraint to use,
then what you want is a plain div constraint!
*/


/** @help constraints;div_undefzero References
help constraints div
*/


#ifndef CONSTRAINT_DIV_H
#define CONSTRAINT_DIV_H

#include "../constraints/constraint_checkassign.h"
#include <math.h>

// Not a constraint -- just contains a checker.

template<typename T1, typename T2, typename T3, bool undef_zero>
class DivConstraint
{
  
public:
  typedef typename common_var_type3<T1,T2,T3>::type var_common;
  typedef std::array<var_common, 3> var_type;
private:
   var_type vars;
public:

  DivConstraint(const T1& v1, const T2& v2, const T3& v3)
  
  {
    vars[0] = v1; vars[1] = v2; vars[2] = v3;
    // I do this test here because technically the behaviour is implementation
    // defined, and this tiny cheap test will hopefully stop people suffering
    // wrong answers if they recompile minion on a strange CPU.
    DomainInt check1[3] = {-10,3,-4};
    DomainInt check2[3] = {-10,-3,3};
    DomainInt check3[3] = {10,-3,-4};
    CHECK(check_assignment(check1, 3), "Your copy of Minion has a broken div operator. Please report to the developers!");
    CHECK(check_assignment(check2, 3), "Your copy of Minion has a broken div operator. Please report to the developers!");
    CHECK(check_assignment(check3, 3), "Your copy of Minion has a broken div operator. Please report to the developers!");
  }

  string constraint_name() const
  { if(undef_zero) return "div_undefzero"; else return "div"; }

  CONSTRAINT_ARG_LIST3(vars[0], vars[1], vars[2])

  var_type& get_vars()
  { return vars; }

  virtual bool check_assignment(DomainInt* v, SysInt v_size)
  {
    D_ASSERT(v_size == 3);
    if(v[1] == 0)
    {
      if(undef_zero)
        return v[2] == 0;
      else
        return false;
    }

    bool negsign = (v[0] < 0 || v[1] < 0) && (v[0] > 0 || v[1] > 0);
    DomainInt r = v[0]/v[1];
    if(negsign && r * v[1] != v[0])
      r--;
    return r == v[2];
//    return v[2] == (v[0] / v[1] - (v[0] < 0 && v[0] % v[1] != 0 ? 1 : 0));
  }
};


#include "../constraints/forward_checking.h"
template<typename V1, typename V2>
inline AbstractConstraint*
BuildCT_DIV(const V1& vars, const V2& var2, ConstraintBlob&)
{
  D_ASSERT(vars.size() == 2);
  D_ASSERT(var2.size() == 1);
  
//  Old version that uses check constraint
  typedef DivConstraint<typename V1::value_type, typename V1::value_type, typename V2::value_type, false> DivCon;
  AbstractConstraint* div=new CheckAssignConstraint<DivCon, false>(DivCon(vars[0], vars[1], var2[0]));
  // Now wrap it in new FC thing. Horrible hackery.
  return forwardCheckingCon(div);
}

/* JSON
{ "type": "constraint",
  "name": "div",
  "internal_name": "CT_DIV",
  "args": [ "read_2_vars", "read_var" ]
}
*/

template<typename V1, typename V2>
inline AbstractConstraint*
BuildCT_DIV_UNDEFZERO(const V1& vars, const V2& var2, ConstraintBlob&)
{
  D_ASSERT(vars.size() == 2);
  D_ASSERT(var2.size() == 1);
  typedef DivConstraint<typename V1::value_type, typename V1::value_type, typename V2::value_type, true> DivCon;
  AbstractConstraint* div=new CheckAssignConstraint<DivCon, false>(DivCon(vars[0], vars[1], var2[0]));
  return forwardCheckingCon(div);
}

/* JSON
{ "type": "constraint",
  "name": "div_undefzero",
  "internal_name": "CT_DIV_UNDEFZERO",
  "args": [ "read_2_vars", "read_var" ]
}
*/
#endif
