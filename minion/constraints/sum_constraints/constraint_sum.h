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

// This files branches between the 3 implementations of sum:

// constraint_fullsum.h  : Standard implementation.
// constraint_lightsum.h : Only for very short arrays by not storing any state.
// constraint_sum_bool.h : Only for arrays of booleans, summed to a constant.

#ifndef CONSTRAINT_SUM_QPWO
#define CONSTRAINT_SUM_QPWO

#include "constraint_fullsum.h"
#include "constraint_lightsum.h"
#include "constraint_sum_bool.h"

#endif
