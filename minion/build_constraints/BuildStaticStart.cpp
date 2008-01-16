#include "BuildStart.h"
Constraint* build_constraint(ConstraintBlob& b) {
switch(b.constraint.type) {
case CT_ELEMENT : return build_constraint_CT_ELEMENT(b);
case CT_GACELEMENT : return build_constraint_CT_GACELEMENT(b);
case CT_ALLDIFF : return build_constraint_CT_ALLDIFF(b);
case CT_DISEQ : return build_constraint_CT_DISEQ(b);
case CT_EQ : return build_constraint_CT_EQ(b);
case CT_INEQ : return build_constraint_CT_INEQ(b);
case CT_LEXLEQ : return build_constraint_CT_LEXLEQ(b);
case CT_LEXLESS : return build_constraint_CT_LEXLESS(b);
case CT_MAX : return build_constraint_CT_MAX(b);
case CT_MIN : return build_constraint_CT_MIN(b);
case CT_OCCURRENCE : return build_constraint_CT_OCCURRENCE(b);
case CT_PRODUCT2 : return build_constraint_CT_PRODUCT2(b);
case CT_WEIGHTLEQSUM : return build_constraint_CT_WEIGHTLEQSUM(b);
case CT_WEIGHTGEQSUM : return build_constraint_CT_WEIGHTGEQSUM(b);
case CT_GEQSUM : return build_constraint_CT_GEQSUM(b);
case CT_LEQSUM : return build_constraint_CT_LEQSUM(b);
case CT_MINUSEQ : return build_constraint_CT_MINUSEQ(b);
}}

