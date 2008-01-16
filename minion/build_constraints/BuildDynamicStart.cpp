/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id$
*/

#include "BuildStart.h"
DynamicConstraint* build_dynamic_constraint(ConstraintBlob& b) {
switch(b.constraint.type) {
case CT_WATCHED_ELEMENT : return build_constraint_CT_WATCHED_ELEMENT(b);
case CT_WATCHED_GEQSUM : return build_constraint_CT_WATCHED_GEQSUM(b);
case CT_WATCHED_LEQSUM : return build_constraint_CT_WATCHED_LEQSUM(b);
case CT_WATCHED_TABLE : return build_constraint_CT_WATCHED_TABLE(b);
case CT_WATCHED_VECNEQ : return build_constraint_CT_WATCHED_VECNEQ(b);
case CT_WATCHED_LITSUM : return build_constraint_CT_WATCHED_LITSUM(b);
}}
