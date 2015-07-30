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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

#ifndef TRIG_BACKTRACK_QUEUE_H
#define TRIG_BACKTRACK_QUEUE_H

// This container stores a list of triggers and queues. On backtrack
// It puts the triggers back onto the queue they were on previously.

#include "../triggering/constraint_abstract.h"

#ifdef P
#undef P
#endif

#define P(x)
//#define P(x) cout << x << endl

struct TriggerBacktrackQueue {

  typedef vector<pair<DynamicTriggerList*, Trig_ConRef>> TriggerList;

  vector<TriggerList> queue;

  TriggerBacktrackQueue() {
    queue.resize(1);
  }

  void restoreTriggerOnBacktrack(Trig_ConRef t)
  {
    Con_TrigRef conref = t.con->_getTrigRef(t.conListPos);
    P("TBQ: Restore on backtrack:" << conref.dtl << ":" << t);
    queue.back().push_back(make_pair(conref.dtl, t));

  }
/* XXX
  void addTrigger(DynamicTrigger *trig) {
    PROP_INFO_ADDONE(Counter3);
    queue.back().push_back(make_pair(trig, trig->getQueue()));
  }
*/
  void world_push() {
    P("TBQ: World_push");
    queue.push_back(TriggerList());
  }

  void world_pop() {
    P("TBQ: World_pop");
    TriggerList &tl = queue.back();
    for (SysInt i = (SysInt)tl.size() - 1; i >= 0; --i) {
      if (tl[i].first == NULL) {
        P("TBQ: Release " << tl[i].second);
        releaseMergedTrigger(tl[i].second);
      } else {
        P("TBQ: Add " << tl[i].second << " to " << tl[i].first);
        tl[i].first->add(tl[i].second);
      }
    }
    queue.pop_back();
    // The first layer of the queue consists of things added in full_propagate()
    // So should never get popped.
    D_ASSERT(!queue.empty());
  }
};

#endif
