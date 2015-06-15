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

#include "minion.h"

namespace Controller
{
/// Lists all structures that must be locked before search.
// @todo This could be done more neatly... 

void lock()
{  
  getVars().lock();
  
  SysInt size = getState().getConstraintList().size();
  for(SysInt i = 0 ; i < size;i++)
  getState().getConstraintList()[i]->setup();
  
  getMemory().monotonicSet().lock();
  
  getTriggerMem().finaliseTriggerLists();
  
  // No longer AC1, thank goodness.
  for(SysInt i = 0; i < size; ++i)
  {
    if(getState().isFailed()) 
      return;
    getState().getConstraintList()[i]->full_propagate();
    getState().getConstraintList()[i]->full_propagate_done=true;
    if(getState().isFailed()) 
      return;
    // If queues not empty, more work to do.
    if(!getQueue().isQueuesEmpty())
    {
        getQueue().propagateQueueRoot();
    }
  }
  
  getState().markLocked();

}
}
