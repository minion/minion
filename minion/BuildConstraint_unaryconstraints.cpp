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

#define NO_MAIN
#include "minion.h"
#include "CSPSpec.h"
using namespace ProbSpec;

#include "BuildConstraintConstructs.h"

namespace BuildCon
{
  
  DEFCON1(NeqCon);
  DEFCON3(OccEqualCon);
  
  template<>
	struct BuildConstraint<1, 0>
  {
	template<typename ConData>
	static 
	Constraint* build(const ConData& vars, ConstraintBlob& b, int pos)
  { 
	  BoolVarRef reifyVar;
	  if(b.reified)
		reifyVar = boolean_container.get_var_num(b.reify_var.pos);
	  
	  D_ASSERT(pos == 1);
	  D_ASSERT(b.vars.size() == 1 || (b.vars.size() == 3 && b.constraint.type == CT_OCCURRENCE));
	  switch(b.constraint.type)
	  {
		case CT_ALLDIFF:
		  return MakeNeqCon(*(vars.second), b, reifyVar);
		  
		  // Just grab table directly!
		case CT_OCCURRENCE:
		{
		  int val_to_count = b.vars[1][0].pos;
		  int occs = b.vars[2][0].pos;
		  return MakeOccEqualCon(*(vars.second), runtime_val(val_to_count), runtime_val(occs), b, reifyVar);
		}
		  
		default:
		  D_FATAL_ERROR( "unsupported constraint");
	  }
  }
  };
  
  
  Constraint*
	build_constraint_unary(ConstraintBlob& b)
  { return BuildConstraint<1, 1>::build(EmptyType(), b, 0); }
}

