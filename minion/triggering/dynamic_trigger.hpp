

inline void Trig_ConRef::propagate()
  {
    D_ASSERT(con);
    con->propagateDynInt(conListPos);
  }

inline void DynamicTriggerList::add(Trig_ConRef t) {
    releaseMergedTrigger(t);
    elems.push_back(t);
    TRIGP("LA:" << elems.size() << ":" << t);
    t.con->_reportTriggerMovementToConstraint(t.conListPos, Con_TrigRef{this, (SysInt)(elems.size()-1)});
  }

inline bool DynamicTriggerList::sanity_check_list() {
  for(size_t i = 0; i < elems.size(); ++i)
  {
    Trig_ConRef trig = elems[i];
    if(!trig.empty()) {
      Con_TrigRef con = trig.con->_getTrigRef(trig.conListPos);
      D_CHECK(con.dtl == this);
      D_CHECK(con.triggerListPos == i);
    }
  }
    return true;
  }


inline void releaseMergedTrigger(Con_TrigRef t, TrigOp op) {
  TRIGP("CTR_Release:" << t);
  if(t.empty()) {
    TRIGP("Empty");
    return;
  }


    Trig_ConRef tcr = t.dtl->_getConRef(t.triggerListPos);

    if(op == TO_Backtrack)
    {
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

    if(op == TO_Backtrack)
    {
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

inline void _restoreTriggerOnBacktrack(Trig_ConRef tcr)
{ getQueue().getTbq().restoreTriggerOnBacktrack(tcr); }
