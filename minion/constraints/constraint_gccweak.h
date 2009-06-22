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


/** @help constraints;gccweak Description 
The Generalized Cardinality Constraint (GCC) (weak variant) constrains the 
number of each value that a set of variables can take.

gccweak([primary variables], [values of interest], [capacity variables])

For each value of interest, there must be a capacity variable, which specifies
the number of occurrences of the value in the primary variables.

This constraint is new, and its syntax and implementation are not finalised.
*/

/** @help constraints;gccweak Example 
Suppose the input file had the following vectors of variables defined:

DISCRETE myVec[9] {1..9}
BOUND cap[9] {0..2}

The following constraint would restrict the occurrence of values 1..9 in myVec
to be at most 2 each initially, and finally equal to the values of the cap
vector.

gccweak(myVec, [1,2,3,4,5,6,7,8,9], cap)
*/

/** @help constraints;gccweak Notes 
This constraint enforces a hybrid consistency. It reads the bounds of the
capacity variables, then enforces GAC over the primary variables only.  Then the
bounds of the capacity variables are updated by counting values in the domains
of the primary variables.

The consistency over the capacity variables is weaker than the gcc constraint, 
hence the name gccweak.
*/

#ifndef CONSTRAINT_GCCWEAK_H_PQWOEI
#define CONSTRAINT_GCCWEAK_H_PQWOEI

#include "gcc_common.h"

#endif
