/* Minion
* Copyright (C) 2006
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



// QUICK_COMPILE won't work on this file.
#undef QUICK_COMPILE

#define NO_MAIN
#include "minion.h"
#include "CSPSpec.h"
using namespace ProbSpec;

#ifdef DYNAMICTRIGGERS

#define DYNAMIC_BUILD_CONSTRAINT
#include "BuildConstraintConstructs.h"

namespace BuildCon
{


template<>
struct BuildDynamicConstraint<1, 0>
{
  template<typename T1>
  static 
  DynamicConstraint* build(const pair<EmptyType, vector<T1>* >& vars, ConstraintBlob& b, int pos)
  { 
	switch(b.constraint.type)
	{
	  case CT_WATCHED_TABLE:
		return GACTableCon(*(vars.second), b.tuples);
		
	  default:
		D_FATAL_ERROR( "unsupported constraint");						
	}
  }
};

DynamicConstraint*
build_dynamic_constraint_table(ConstraintBlob& b)
{ return BuildDynamicConstraint<1, 1>::build(EmptyType(), b, 0); }
}


#endif

