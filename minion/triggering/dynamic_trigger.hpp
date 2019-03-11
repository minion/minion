

inline void Trig_ConRef::propagate(DomainDelta d) {
  D_ASSERT(con);
  con->propagateDynInt(conListPos, d);
}

inline void DynamicTriggerList::add(Trig_ConRef t) {
  releaseMergedTrigger(t);
  if(slack.empty()) {
    elems.push_back(t);
    TRIGP("LA:" << elems.size() << ":" << t);
    Con_TrigRef ctr{this, (SysInt)(elems.size() - 1)};
    t.con->_reportTriggerMovementToConstraint(t.conListPos, ctr);
  } else {
    SysInt pos = slack.back();
    slack.pop_back();
    D_ASSERT(elems.size() > pos && elems[pos].empty());
    elems[pos] = t;
    TRIGP("LApos:" << pos << ":" << t);
    Con_TrigRef ctr{this, pos};
    t.con->_reportTriggerMovementToConstraint(t.conListPos, ctr);
  }

  TRIGP("LA:" << elems.size() << ":" << t);
}

inline bool DynamicTriggerList::sanity_check_list() {
  for(size_t i = 0; i < elems.size(); ++i) {
    Trig_ConRef trig = elems[i];
    if(!trig.empty()) {
      Con_TrigRef con = trig.con->_getTrigRef(trig.conListPos);
      D_CHECK(con.dtl == this);
      D_CHECK(con.triggerListPos == i);
    }
  }
  return true;
}

inline void DynamicTriggerList::verify_slack() const {
// Note: In non-debug mode, this does nothing
#ifdef MINION_DEBUG
  size_t slack_debugCount = 0;
  for(int i = 0; i < elems.size(); ++i) {
    if(elems[i].empty())
      slack_debugCount++;
  }
  D_ASSERT(slack.size() == slack_debugCount);
#endif
}

// Warning: This method is horrible.
// We go through the list of triggers, compressing and removing
// any blank spaces. We want to move as few triggers as possible.
// Therefore, we go forward through the list looking for spaces,
// moving the last trigger into that space. We make sure to skip
// any spaces we find at the end of the list.
inline void DynamicTriggerList::
    tryCompressList() { /*
                         // In debug mode, let's check our slack counter is correct
                         verify_slack();

                         SysInt slacksize = slack.size();
                         // Quick early return
                         if(slacksize <= elems.size() / 4) return;

                         // Now we are here until we clear the slack!
                         slack.clear();

                         SysInt elemsize = elems.size();
                         // Begin by removing any empty members at the end
                         while(elemsize > 0 && elems.back().empty())
                         {
                           elems.pop_back();
                           elemsize--;
                           slacksize--;
                         }

                         if(slacksize == 0)
                         {
                           verify_slack();
                           return;
                         }

                         size_t pos = 0;
                         while(true) {
                           D_ASSERT(elems.size() > 0);
                           D_ASSERT(!elems.back().empty());
                           // Scan for first space
                           while(pos < elemsize && !elems[pos].empty()) {
                               pos++;
                           }
                           if(pos == elemsize)
                             return;
                           // Move last trigger back into space
                           Trig_ConRef old_ref = elems.back();
                           elems[pos] = old_ref;
                           elems.pop_back();
                           Con_TrigRef new_con_ref{this, (SysInt)pos};
                           old_ref.con->_reportTriggerMovementToConstraint(old_ref.conListPos,
                         new_con_ref);
                           elemsize--;
                           slacksize--;
                           // The first pass around this loop will get rid of the now moved element
                           // then we search for a non-empty cell.
                           // We do not have to check slack (as either we will find a non-empty
                         position,
                           // or empty the list)
                            while(elemsize > 0 && elems.back().empty()) {
                             elems.pop_back();
                             elemsize--;
                             slacksize--;
                           }
                           D_ASSERT(slacksize >= 0);
                           if(slacksize == 0)
                             return;
                           D_ASSERT(pos < elemsize && elemsize == elems.size());
                         }*/
}

inline void releaseMergedTrigger(Con_TrigRef t, TrigOp op) {
  TRIGP("CTR_Release:" << t);
  if(t.empty()) {
    TRIGP("Empty");
    return;
  }

  Trig_ConRef tcr = t.dtl->_getConRef(t.triggerListPos);

  if(op == TO_Backtrack) {
    getQueue().getTbq().restoreTriggerOnBacktrack(tcr);
  }

#ifdef MINION_DEBUG
  Con_TrigRef test = tcr.con->_getTrigRef(tcr.conListPos);
  D_ASSERT(t == test);
#endif

  tcr.con->_reportTriggerRemovalToConstraint(tcr.conListPos);
  t.dtl->_reportTriggerRemovalToList(t.triggerListPos);

  D_ASSERT(t.dtl->sanity_check_list());
}

inline void releaseMergedTrigger(Trig_ConRef t, TrigOp op) {
  TRIGP("TCR_Release:" << t);

  Con_TrigRef ctr = t.con->_getTrigRef(t.conListPos);

  if(op == TO_Backtrack) {
    getQueue().getTbq().restoreTriggerOnBacktrack(t);
  }

  if(ctr.empty()) {
    TRIGP("Empty");
    return;
  }

#ifdef MINION_DEBUG
  Trig_ConRef test = ctr.dtl->_getConRef(ctr.triggerListPos);
  D_ASSERT(t == test);
#endif
  t.con->_reportTriggerRemovalToConstraint(t.conListPos);
  ctr.dtl->_reportTriggerRemovalToList(ctr.triggerListPos);

  D_ASSERT(ctr.dtl->sanity_check_list());
}

inline void _restoreTriggerOnBacktrack(Trig_ConRef tcr) {
  getQueue().getTbq().restoreTriggerOnBacktrack(tcr);
}
