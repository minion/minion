#include "BuildStart.h"
Constraint* build_constraint(StateObj* stateObj, ConstraintBlob& b) {
switch(b.constraint.type) {
case CT_ELEMENT : return build_constraint_CT_ELEMENT(stateObj, b);
case CT_GACELEMENT : return build_constraint_CT_GACELEMENT(stateObj, b);
case CT_ALLDIFF : return build_constraint_CT_ALLDIFF(stateObj, b);
case CT_ALLDIFF_GACSLOW : return build_constraint_CT_ALLDIFF_GACSLOW(stateObj, b);
case CT_DISEQ : return build_constraint_CT_DISEQ(stateObj, b);
case CT_EQ : return build_constraint_CT_EQ(stateObj, b);
case CT_INEQ : return build_constraint_CT_INEQ(stateObj, b);
case CT_LEXLEQ : return build_constraint_CT_LEXLEQ(stateObj, b);
case CT_LEXLESS : return build_constraint_CT_LEXLESS(stateObj, b);
case CT_MAX : return build_constraint_CT_MAX(stateObj, b);
case CT_MIN : return build_constraint_CT_MIN(stateObj, b);
case CT_OCCURRENCE : return build_constraint_CT_OCCURRENCE(stateObj, b);
case CT_LEQ_OCCURRENCE : return build_constraint_CT_LEQ_OCCURRENCE(stateObj, b);
case CT_GEQ_OCCURRENCE : return build_constraint_CT_GEQ_OCCURRENCE(stateObj, b);
case CT_PRODUCT2 : return build_constraint_CT_PRODUCT2(stateObj, b);
case CT_WEIGHTLEQSUM : return build_constraint_CT_WEIGHTLEQSUM(stateObj, b);
case CT_WEIGHTGEQSUM : return build_constraint_CT_WEIGHTGEQSUM(stateObj, b);
case CT_GEQSUM : return build_constraint_CT_GEQSUM(stateObj, b);
case CT_LEQSUM : return build_constraint_CT_LEQSUM(stateObj, b);
case CT_MINUSEQ : return build_constraint_CT_MINUSEQ(stateObj, b);
case CT_POW : return build_constraint_CT_POW(stateObj, b);
case CT_DIV : return build_constraint_CT_DIV(stateObj, b);
case CT_MODULO : return build_constraint_CT_MODULO(stateObj, b);
case CT_GADGET : return build_constraint_CT_GADGET(stateObj, b);
}}
