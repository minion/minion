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

#ifndef _NONBACKTRACK_MEMORY_H
#define _NONBACKTRACK_MEMORY_H

#include "backtrackable_memory.h"



// \addtogroup Memory
// @{

/// Encapsulates both the backtrackable and nonbacktrackable memory of a CSP instance.
class Memory
{
  BackTrackMemory backtrack_memory;
  NewMemoryBlock nonbacktrack_memory;
  
public:
  MonotonicSet * monotonic_set;   // this needs to become a ptr to getVars(stateObj).getBigRangevarContainer().bms_array

  BackTrackMemory& backTrack() { return backtrack_memory; }
  NewMemoryBlock& nonBackTrack() { return nonbacktrack_memory; }
  MonotonicSet& monotonicSet() { return *monotonic_set; }
};

// @}

#endif
