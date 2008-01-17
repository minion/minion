#include "BuildStart.h"
DynamicConstraint* build_dynamic_constraint(StateObj* stateObj, ConstraintBlob& b) {
switch(b.constraint.type) {
case CT_WATCHED_ELEMENT : return build_constraint_CT_WATCHED_ELEMENT(stateObj, b);
case CT_WATCHED_GEQSUM : return build_constraint_CT_WATCHED_GEQSUM(stateObj, b);
case CT_WATCHED_LEQSUM : return build_constraint_CT_WATCHED_LEQSUM(stateObj, b);
case CT_WATCHED_TABLE : return build_constraint_CT_WATCHED_TABLE(stateObj, b);
case CT_WATCHED_VECNEQ : return build_constraint_CT_WATCHED_VECNEQ(stateObj, b);
case CT_WATCHED_LITSUM : return build_constraint_CT_WATCHED_LITSUM(stateObj, b);
}}
