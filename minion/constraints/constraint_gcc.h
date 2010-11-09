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


/** @help constraints;gcc Description 
The Generalized Cardinality Constraint (GCC) constrains the number of each value
that a set of variables can take.

gcc([primary variables], [values of interest], [capacity variables])

For each value of interest, there must be a capacity variable, which specifies
the number of occurrences of the value in the primary variables.

This constraint only restricts the number of occurrences of the values in
the value list. There is no restriction on the occurrences of other values.
Therefore the semantics of gcc are identical to a set of occurrence 
constraints:

occurrence([primary variables], val1, cap1)
occurrence([primary variables], val2, cap2)
...

*/

/** @help constraints;gcc Example 
Suppose the input file had the following vectors of variables defined:

DISCRETE myVec[9] {1..9}
BOUND cap[9] {0..2}

The following constraint would restrict the occurrence of values 1..9 in myVec
to be at most 2 each initially, and finally equal to the values of the cap
vector.

gcc(myVec, [1,2,3,4,5,6,7,8,9], cap)
*/

/** @help constraints;gcc Notes 
This constraint enforces a hybrid consistency. It reads the bounds of the
capacity variables, then enforces GAC over the primary variables only.  Then the
bounds of the capacity variables are updated using flow algorithms similar to
those proposed by Quimper et al, Improved Algorithms for the Global Cardinality
Constraint (CP 2004).

This constraint provides stronger propagation to the capacity variables than the
gccweak constraint.
*/

#ifndef CONSTRAINT_GCC_H_PQWOEI
#define CONSTRAINT_GCC_H_PQWOEI

#include "gcc_common.h"

#endif
