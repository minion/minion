/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id: constraint_gcc.h 668 2007-09-26 15:14:50Z pete_n $
*/

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


/** @help constraints;gcc Description
The Generalized Cardinality Constraint (GCC) constrains the number of each value
that a set of variables can take. 

gcc([primary variables], [capacity variables])

For each value in the initial domains of the primary variables, there must be 
a capacity variable. 

For example, if the union of the initial domains of the 
primary variables is {-5,-3,-1,0,2,3,5} then there would be 11 capacity variables, 
specifying the number of occurrences of each value in the interval [-5 ... 5]. 

This constraint is new, and its syntax and implementation are not finalised.

*/

/** @help constraints;gcc Example 


Suppose the input file had the following vectors of variables defined:

DISCRETE myVec[9] {1..9}
BOUND cap[9] {0..2}

The following constraint would restrict the occurrence of values 1..9 in myVec to be
at most 2 each initially, and finally equal to the values of the cap vector. 

gcc(myVec, cap)

*/

/** @help constraints;gcc Reifiability
This constraint is reifyimply'able but not reifiable.
*/

/** @help constraints;gcc Notes
This constraint enforces a hybrid consistency. It reads the bounds of the 
capacity variables, then enforces GAC over the primary variables only. 
Then the bounds of the capacity variables are updated by counting values
in the domains of the primary variables.

*/




#include "gcc_common.h"

template<typename VarArray1, typename VarArray2>
AbstractConstraint*
GlobalCardCon(StateObj* stateObj, const VarArray1& var_array, const VarArray2& cap_array)
{ return new GCC<VarArray1, VarArray2>(stateObj, var_array, cap_array); }

BUILD_CONSTRAINT2(CT_GCC, GlobalCardCon)



