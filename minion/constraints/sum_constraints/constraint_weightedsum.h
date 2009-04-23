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

/** @help constraints;weightedsumleq Description
The constraint

   weightedsumleq(constantVec, varVec, total)

ensures that constantVec.varVec <= total, where constantVec.varVec is
the scalar dot product of constantVec and varVec.
*/

/** @help constraints;weightedsumleq References
help constraints weightedsumgeq
help constraints sumleq
help constraints sumgeq
*/

/** @help constraints;weightedsumgeq Description
The constraint

   weightedsumgeq(constantVec, varVec, total)

ensures that constantVec.varVec >= total, where constantVec.varVec is
the scalar dot product of constantVec and varVec.
*/

/** @help constraints;weightedsumgeq References
help constraints weightedsumleq
help constraints sumleq
help constraints sumgeq
*/

#ifndef CONSTRAINT_WEIGHTSUM_H
#define CONSTRAINT_WEIGHTSUM_H

#include "constraint_sum.h"
#endif
