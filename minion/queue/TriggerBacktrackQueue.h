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

#ifndef TRIG_BACKTRACK_QUEUE_H
#define TRIG_BACKTRACK_QUEUE_H

// This container stores a list of triggers and queues. On backtrack
// It puts the triggers back onto the queue they were on previously.

#include "../constraints/constraint_abstract.h"

struct TriggerBacktrackQueue
{
	typedef vector<pair<DynamicTrigger*, DynamicTrigger*> > TriggerList;

    vector<TriggerList> queue;

    TriggerBacktrackQueue()
    { queue.resize(1); }

    void addTrigger(DynamicTrigger* trig)
    { queue.back().push_back(make_pair(trig, trig->getQueue())); }

    void world_push()
    { queue.push_back(TriggerList()); }

    void world_pop()
    {
    	TriggerList& tl = queue.back();
    	DynamicTrigger* nulldt = NULL;
    	for(int i = 0; i < tl.size(); ++i)
    	{
    		if(tl[i].second == NULL)
    			tl[i].first->remove(nulldt);
    		else
    			tl[i].first->add_after(tl[i].second, nulldt);
    	}
    }
};




#endif
