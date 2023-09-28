// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef STANDARD_QUEUE_H
#define STANDARD_QUEUE_H

#define QueueCon deque
#define queueTop front
#define queuePop pop_front

#include "../get_info/get_info.h"
#include "../solver.h"
#include "../triggering/constraint_abstract.h"
#include "../triggering/triggers.h"
#include "TriggerBacktrackQueue.h"
#include <deque>

class Queues {

  QueueCon<DynamicTriggerEvent> dynamicTriggerList;

  // Special triggers are those which can only be run while the
  // normal queue is empty. This list is at the moment only used
  // by reified constraints when they want to start propagation.
  // I don't like it, but it is necesasary.
  QueueCon<AbstractConstraint*> specialTriggers;

  TriggerBacktrackQueue tbq;

public:
  TriggerBacktrackQueue& getTbq() {
    return tbq;
  }

  Queues() {}

  void pushSpecialTrigger(AbstractConstraint* trigger) {
    CON_INFO_ADDONE(AddSpecialToQueue);
    specialTriggers.push_back(trigger);
  }

  void pushDynamicTriggers(DynamicTriggerEvent new_dynamic_trig_range) {
    CON_INFO_ADDONE(AddDynToQueue);
    dynamicTriggerList.push_back(new_dynamic_trig_range);
  }

  void clearQueues() {
    dynamicTriggerList.clear();

    if(!specialTriggers.empty()) {
      SysInt size = specialTriggers.size();
      for(SysInt i = 0; i < size; ++i)
        specialTriggers[i]->specialUnlock();
      specialTriggers.clear();
    }
  }

  bool isQueuesEmpty() {
    return dynamicTriggerList.empty() && specialTriggers.empty();
  }

  // next_queuePtr is defined in constraint_dynamic.
  // It is used if pointers are moved around.

  // Subclass this class and change the following three methods.

  template <bool is_root_node>
  bool propagateDynamicTriggerLists() {
    bool* failPtr = getState().getFailedPtr();
    while(!dynamicTriggerList.empty()) {
      DynamicTriggerEvent dte = dynamicTriggerList.queueTop();
      dynamicTriggerList.queuePop();

      DynamicTriggerList& dtl = *(dte.event());
      DomainDelta delta = dte.data;
      SysInt pos = 0;

      while(pos < dtl.size()) {
        if(*failPtr) {
          clearQueues();
          return true;
        }

        Trig_ConRef ref = dtl[pos];
        if(!ref.empty() && (!is_root_node || ref.con->fullPropagateDone)) {
          ref.propagate(delta);
        }

#ifdef WDEG
        if(*failPtr)
          ref.constraint()->incWdeg();
#endif

        pos++;
      }
      dtl.tryCompressList();
    }
    return false;
  }

  template <bool is_root_node>
  inline void propagateQueueImpl() {
    while(true) {
      while(!dynamicTriggerList.empty()) {
        if(propagateDynamicTriggerLists<is_root_node>())
          return;
      }

      if(specialTriggers.empty())
        return;

      AbstractConstraint* trig = specialTriggers.queueTop();
      specialTriggers.queuePop();

      CON_INFO_ADDONE(SpecialTrigger);
      trig->specialCheck();
#ifdef WDEG
      if(getState().isFailed())
        trig->incWdeg();
#endif

      if(getState().isFailed()) {
        clearQueues();
        return;
      }
    } // while(true)

  } // end Function

  inline void propagateQueueRoot() {
    return propagateQueueImpl<true>();
  }

  inline void propagateQueue() {
    return propagateQueueImpl<false>();
  }
};

#endif
