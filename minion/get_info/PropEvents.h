#ifndef VAR_EVENT
#ifndef PROP_EVENT
#error This file should only be included once one of VAR_EVENT or PROP_EVENT is defined.
#endif
#endif

#ifdef VAR_EVENT
#define PROP_EVENT(x) 
#else
#define VAR_EVENT(x,y) 
#endif


VAR_EVENT(construct, NULL_EVENT)
VAR_EVENT(copy, NULL_EVENT)
VAR_EVENT(isAssigned, READ_EVENT)
VAR_EVENT(getAssignedValue, READ_EVENT)
VAR_EVENT(isAssignedValue, READ_EVENT)
VAR_EVENT(inDomain, READ_EVENT)
VAR_EVENT(inDomain_noBoundCheck, READ_EVENT)
VAR_EVENT(getMax, READ_EVENT)
VAR_EVENT(getMin, READ_EVENT)
VAR_EVENT(getInitialMax, READ_EVENT)
VAR_EVENT(getInitialMin, READ_EVENT)
VAR_EVENT(setMax, READ_EVENT)
VAR_EVENT(setMin, READ_EVENT)
VAR_EVENT(uncheckedAssign, WRITE_EVENT)
VAR_EVENT(propagateAssign, WRITE_EVENT)
VAR_EVENT(RemoveFromDomain, WRITE_EVENT)
VAR_EVENT(addTrigger, WRITE_EVENT)
VAR_EVENT(addConstraint, NULL_EVENT)
VAR_EVENT(getConstraints, NULL_EVENT)
VAR_EVENT(getDomainChange, READ_EVENT)
VAR_EVENT(addDynamicTrigger, WRITE_EVENT)
VAR_EVENT(getIdent, NULL_EVENT)
VAR_EVENT(getBaseVal, NULL_EVENT)
VAR_EVENT(getBaseWdeg, READ_EVENT)
VAR_EVENT(incWdeg, WRITE_EVENT)

PROP_EVENT(CheckAssign)
PROP_EVENT(BoundTable)
PROP_EVENT(Reify)
PROP_EVENT(ReifyTrue)
PROP_EVENT(Table)
PROP_EVENT(ArrayNeq)
PROP_EVENT(BinaryNeq)
PROP_EVENT(NonGACElement)
PROP_EVENT(GACElement)
PROP_EVENT(Lex)
PROP_EVENT(FullSum)
PROP_EVENT(BoolSum)
PROP_EVENT(AlldiffGacSlow)
PROP_EVENT(LightSum)
PROP_EVENT(WeightBoolSum)
PROP_EVENT(ReifyEqual)
PROP_EVENT(Equal)
PROP_EVENT(BinaryLeq)
PROP_EVENT(Min)
PROP_EVENT(OccEqual)
PROP_EVENT(Pow)
PROP_EVENT(And)
PROP_EVENT(Product)
PROP_EVENT(DynSum)
PROP_EVENT(DynSumSat)
PROP_EVENT(Dyn3SAT)
PROP_EVENT(Dyn2SAT)
PROP_EVENT(DynLitWatch)
PROP_EVENT(DynElement)
PROP_EVENT(DynVecNeq)
PROP_EVENT(DynGACTable)
PROP_EVENT(Mod)
PROP_EVENT(Gadget)
PROP_EVENT(Abs)

#undef VAR_EVENT
#undef PROP_EVENT
