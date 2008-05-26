#include "BuildStart.h"
AbstractConstraint* build_dynamic_constraint(StateObj* stateObj, ConstraintBlob& b) {
switch(b.constraint.type) {
case CT_WATCHED_ELEMENT : return build_constraint_CT_WATCHED_ELEMENT(stateObj, b);
case CT_WATCHED_NEQ : return build_constraint_CT_WATCHED_NEQ(stateObj, b);
case CT_WATCHED_LESS : return build_constraint_CT_WATCHED_LESS(stateObj, b);
case CT_WATCHED_GEQSUM : return build_constraint_CT_WATCHED_GEQSUM(stateObj, b);
case CT_WATCHED_LEQSUM : return build_constraint_CT_WATCHED_LEQSUM(stateObj, b);
case CT_WATCHED_TABLE : return build_constraint_CT_WATCHED_TABLE(stateObj, b);
case CT_WATCHED_NEGATIVE_TABLE : return build_constraint_CT_WATCHED_NEGATIVE_TABLE(stateObj, b);
case CT_WATCHED_VECNEQ : return build_constraint_CT_WATCHED_VECNEQ(stateObj, b);
case CT_WATCHED_LITSUM : return build_constraint_CT_WATCHED_LITSUM(stateObj, b);
case CT_WATCHED_OR : return build_constraint_CT_WATCHED_OR(stateObj, b);
case CT_WATCHED_VEC_OR_LESS : return build_constraint_CT_WATCHED_VEC_OR_LESS(stateObj, b);
case CT_WATCHED_VEC_OR_AND : return build_constraint_CT_WATCHED_VEC_OR_AND(stateObj, b);
case CT_WATCHED_HAMMING : return build_constraint_CT_WATCHED_HAMMING(stateObj, b);
case CT_WATCHED_NEW_OR : return build_constraint_CT_WATCHED_NEW_OR(stateObj, b);
}}
