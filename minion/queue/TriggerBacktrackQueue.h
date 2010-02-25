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

#ifdef P
#undef P
#endif

#define P(x)
//#define P(x) cout << x << endl

struct TriggerBacktrackQueue
{
    StateObj* stateObj;

    TriggerBacktrackQueue(StateObj* _stateObj) :
        stateObj(_stateObj)
    {
    }

    typedef vector<pair<DynamicTrigger*, DynamicTrigger*> > TriggerList;

    vector<TriggerList> queue;

    TriggerBacktrackQueue()
    {
        queue.resize(1);
    }

    void addTrigger(DynamicTrigger* trig)
    {
        PROP_INFO_ADDONE(Counter3);
        queue.back().push_back(make_pair(trig, trig->getQueue()));
    }

    void world_push()
    {
        P("TBQ:World_push");
        queue.push_back(TriggerList());
    }

    void world_pop()
    {
        P("TBQ: World_pop");
        TriggerList& tl = queue.back();

        for (int i = tl.size() - 1; i >= 0; --i)
        {
            if (tl[i].second == NULL)
            {
                P("Release " << tl[i].first);
                releaseTrigger(stateObj, tl[i].first BT_CALL_STORE);
            }
            else
            {
                P("Add " << tl[i].first << " to " << tl[i].second);
#ifdef NO_DYN_CHECK
                tl[i].first->add_after(tl[i].second);
#else
                DynamicTrigger* nulldt = NULL;
                tl[i].first->add_after(tl[i].second, nulldt);
#endif
                tl[i].first->setQueue(tl[i].second);
            }
        }
        queue.pop_back();
        // The first layer of the queue consists of things added in full_propagate()
        // So should never get popped.
        D_ASSERT(!queue.empty());
    }
};

#endif
