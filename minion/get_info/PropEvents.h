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
VAR_EVENT(decisionAssign, WRITE_EVENT)
VAR_EVENT(RemoveFromDomain, WRITE_EVENT)
VAR_EVENT(addTrigger, WRITE_EVENT)
VAR_EVENT(addConstraint, NULL_EVENT)
VAR_EVENT(getConstraints, NULL_EVENT)
VAR_EVENT(getDomainChange, READ_EVENT)
VAR_EVENT(addDynamicTrigger, WRITE_EVENT)
VAR_EVENT(getBaseVal, NULL_EVENT)
VAR_EVENT(getBaseVar, NULL_EVENT)
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
PROP_EVENT(Difference)
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
PROP_EVENT(WatchNEQ)
PROP_EVENT(WatchInSet)
PROP_EVENT(WatchNotInSet)
PROP_EVENT(WatchInRange)
PROP_EVENT(WatchNotInRange)
PROP_EVENT(ReifyFullPropGetAssgNegCon)
PROP_EVENT(ReifyFullPropGetAssgPosCon)
PROP_EVENT(ReifyPropGetAssgNegCon)
PROP_EVENT(ReifyPropGetAssgPosCon)
PROP_EVENT(ReifyFullCheckUnsatNegCon)
PROP_EVENT(ReifyFullCheckUnsatPosCon)
PROP_EVENT(ReifyCheckUnsatNegCon)
PROP_EVENT(ReifyCheckUnsatPosCon)
PROP_EVENT(IneqFullCheckUnsat)
PROP_EVENT(IneqCheckUnsat)
PROP_EVENT(IneqGetSatAssg)
PROP_EVENT(FullsumFullCheckUnsat)
PROP_EVENT(FullsumCheckUnsat)
PROP_EVENT(FullsumGetSatAssg)
PROP_EVENT(LightsumFullCheckUnsat)
PROP_EVENT(LightsumCheckUnsat)
PROP_EVENT(LightsumGetSatAssg)
PROP_EVENT(ReifyImplyFullCheckUnsat)
PROP_EVENT(ReifyImplyCheckUnsat)
PROP_EVENT(ReifyImplyGetSatAssg)
PROP_EVENT(Clique)

PROP_EVENT(Counter1)
PROP_EVENT(Counter2)
PROP_EVENT(Counter3)
PROP_EVENT(Counter4)
PROP_EVENT(Counter5)
PROP_EVENT(Counter6)
PROP_EVENT(Counter7)
PROP_EVENT(Counter8)
PROP_EVENT(Counter9)

PROP_EVENT(CounterA)
PROP_EVENT(CounterB)
PROP_EVENT(CounterC)
PROP_EVENT(CounterD)
PROP_EVENT(CounterE)
PROP_EVENT(CounterF)
PROP_EVENT(CounterG)
PROP_EVENT(CounterH)
PROP_EVENT(CounterI)

#undef VAR_EVENT
#undef PROP_EVENT
