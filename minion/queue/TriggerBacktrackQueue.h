// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

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

  void restoreTriggerOnBacktrack(Trig_ConRef t) {
    Con_TrigRef conref = t.con->_getTrigRef(t.conListPos);
    P("TBQ: Restore on backtrack:" << conref.dtl << ":" << t);
    queue.back().push_back(make_pair(conref.dtl, t));
  }

  void worldPush() {
    P("TBQ: World_push");
    queue.push_back(TriggerList());
  }

  void worldPop() {
    P("TBQ: World_pop");
    TriggerList& tl = queue.back();
    for(SysInt i = (SysInt)tl.size() - 1; i >= 0; --i) {
      if(tl[i].first == NULL) {
        P("TBQ: Release " << tl[i].second);
        releaseMergedTrigger(tl[i].second);
      } else {
        P("TBQ: Add " << tl[i].second << " to " << tl[i].first);
        tl[i].first->add(tl[i].second);
      }
    }
    queue.pop_back();
    // The first layer of the queue consists of things added in fullPropagate()
    // So should never get popped.
    D_ASSERT(!queue.empty());
  }
};

#endif
