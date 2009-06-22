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

/** @help constraints;reify References
See
   help constraints reification
*/

/** @help constraints;reifyimply References
See
   help constraints reification
*/

/** @help constraints;reification Description
Reification is provided in two forms: reify and reifyimply.

   reify(constraint, r) where r is a 0/1 var

ensures that r is set to 1 if and only if constraint is satisfied. That is, if r
is 0 the constraint must NOT be satisfied; and if r is 1 it must be satisfied as
normal. Conversely, if the constraint is satisfied then r must be 1, and if not
then r must be 0.

   reifyimply(constraint, r)

only checks that if r is set to 1 then constraint must be satisfied. If r is not
1, constraint may be either satisfied or unsatisfied. Furthermore r is never set
by propagation, only by search; that is, satisfaction of constraint does not
affect the value of r.
*/

/** @help constraints;reification Notes
ALMOST ALL constraints are are reifiable. Individual constraint entries mention
if the constraint is NOT reifiable.

ALL constraints are reifyimplyable.
*/


// Note: The whole constraint_locked thing is for the following case:
// Consider the following events are on the queue:
// "rareify boolean is assigned, Y is assigned"
// Now "rareify boolean is assigned" causes full_propagate to be called for
// the constraint. It will set up it's data structures based on the current
// assignment. Then later it will be given Y is assigned, but have already
// possibly used that. Confusion follows. Therefore when we want to propagate
// the function, we "lock" it until the queue empties, then start ping
// the constraint.

#ifndef REIFY_H
#define REIFY_H

#include "constraint_abstract.h"
#include "../memory_management/reversible_vals.h"
#include "../get_info/get_info.h"
#include "../queue/standard_queue.h"

#ifdef P
#undef P
#endif

//#define P(x) cout << x << endl
#define P(x)

#define NEWREIFY
//#define NODETRICK   // This is broken! leave switched off.

#ifdef NEWREIFY

// In here variables are numbered from child_constraints[0].get_vars(), then child_constraints[1].get_vars(), then reify_var

template<typename BoolVar>
struct reify : public ParentConstraint
{
  virtual string constraint_name()
  { return "Reify:" + child_constraints[0]->constraint_name(); }

  BoolVar reify_var;
  int reify_var_num;

  bool constraint_locked;
  Reversible<bool> full_propagate_called;

  #ifdef NODETRICK
  unsigned long long reifysetnode;
  #endif

  int dtcount;
  int c0vars;  // how many vars for child_constraints[0]

  typedef vector<vector<pair<int,int> > > triggerpairstype;
  D_DATA(triggerpairstype triggerpairs);

  reify(StateObj* _stateObj, AbstractConstraint* _poscon, BoolVar _rar_var) :
  ParentConstraint(_stateObj), reify_var(_rar_var), constraint_locked(false),
    full_propagate_called(stateObj, false)
  {
      #ifdef NODETRICK
      numeric_limits<unsigned long long> ull;
      reifysetnode=ull.max();
      #endif
    child_constraints.push_back(_poscon);
    AbstractConstraint* _negcon = _poscon->reverse_constraint();
    child_constraints.push_back(_negcon);
    // assume for the time being that the two child constraints have the same number of vars.
    reify_var_num=child_constraints[0]->get_vars_singleton()->size()+child_constraints[1]->get_vars_singleton()->size();
    //dtcount=dynamic_trigger_count();
    dtcount=child_constraints[0]->get_vars_singleton()->size()*2 + child_constraints[1]->get_vars_singleton()->size()*2;
    c0vars=child_constraints[0]->get_vars_singleton()->size();
    bool hasbound=false;
    vector<AnyVarRef>& t1=*(child_constraints[0]->get_vars_singleton());
    for(int i=0; i<t1.size(); i++)
    {
        if(t1[i].isBound() && t1[i].getInitialMin()!=t1[i].getInitialMax()) {
            hasbound=true;
        }
    }
    vector<AnyVarRef>& t2=*(child_constraints[1]->get_vars_singleton());
    for(int i=0; i<t2.size(); i++)
    {
        if(t2[i].isBound() && t2[i].getInitialMin()!=t2[i].getInitialMax()) {
            hasbound=true;
        }
    }
    if(hasbound)
    {
        cout<<"Warning: bound variables in reify degrade performance, because DomainRemoval triggers are translated into DomainChanged triggers." <<endl;
    }
    D_DATA(triggerpairs.resize(2));
  }

  // constructor which takes a negative constraint and constructs the positive one.
  // This is so that reify can be reversed. Called in reverse_constraint.
  reify(StateObj* _stateObj, AbstractConstraint* _negcon, BoolVar _rar_var, bool unused_argument) :
  ParentConstraint(_stateObj), reify_var(_rar_var), constraint_locked(false),
    full_propagate_called(stateObj, false)
  {
    AbstractConstraint* _poscon = _negcon->reverse_constraint();

    child_constraints.push_back(_poscon);
    child_constraints.push_back(_negcon);
    // assume for the time being that the two child constraints have the same number of vars.
    reify_var_num=child_constraints[0]->get_vars_singleton()->size()+child_constraints[1]->get_vars_singleton()->size();
    dtcount=dynamic_trigger_count();
    dtcount=child_constraints[0]->get_vars_singleton()->size()*2 + child_constraints[1]->get_vars_singleton()->size()*2;
    c0vars=child_constraints[0]->get_vars_singleton()->size();
    D_DATA(triggerpairs.resize(2));
  }

  virtual AbstractConstraint* reverse_constraint()
  {
      // reverse it by swapping the positive and negative constraints.
      return new reify<BoolVar>(stateObj, child_constraints[0], reify_var, true);
  }

  virtual int dynamic_trigger_count()
  {
    return child_constraints[0]->get_vars_singleton()->size()*2
        +child_constraints[1]->get_vars_singleton()->size()*2;  // *2 for each child constraint.
  }

  virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  {
    if(reify_var.inDomain(1))
    {
        bool flag=child_constraints[0]->get_satisfying_assignment(assignment);
        if(flag)
        {
            assignment.push_back(make_pair(reify_var_num, 1));
            return true;
        }
    }
    assignment.clear();
    if(reify_var.inDomain(0))
    {
        bool flag=child_constraints[1]->get_satisfying_assignment(assignment);
        if(flag)
        {
            assignment.push_back(make_pair(reify_var_num, 0));
            return true;
        }
    }
    return false;
  }

  virtual BOOL check_assignment(DomainInt* vals, int v_size)
  {
    DomainInt back_val = *(vals + (v_size - 1));
    if(back_val != 0)
    {
      return child_constraints[0]->check_assignment(vals, c0vars);
    }
    else
    {
      vals += c0vars;
      return child_constraints[1]->check_assignment(vals, (dtcount/2)-c0vars);
    }
  }

  virtual vector<AnyVarRef> get_vars()
  {
      // Push both sets of vars, then reify var.
    vector<AnyVarRef> vec0 = * child_constraints[0]->get_vars_singleton();
    vector<AnyVarRef> vec1 = * child_constraints[1]->get_vars_singleton();
    vector<AnyVarRef> c;
    c.reserve(vec0.size() + vec1.size() + 1);
    for(int i=0; i<vec0.size(); i++)
        c.push_back(vec0[i]);
    for(int i=0; i<vec1.size(); i++)
        c.push_back(vec1[i]);
    c.push_back(reify_var);
    return c;
  }

  virtual triggerCollection setup_internal()
  {
    triggerCollection triggers;
    triggers.push_back(make_trigger(reify_var, Trigger(this, -1000000000), Assigned));
    return triggers;
  }

  virtual void special_check()
  {
    D_ASSERT(constraint_locked);
    P("Special Check!");
    constraint_locked = false;
    if(reify_var.inDomain(0))
    {
        child_constraints[1]->full_propagate();
    }
    else
    {
        child_constraints[0]->full_propagate();
    }
    full_propagate_called = true;
  }

  virtual void special_unlock()
  {
    D_ASSERT(constraint_locked);
    P("Special unlock!");
    constraint_locked = false;
  }

  virtual void propagate(int i, DomainDelta domain)
  {
    PROP_INFO_ADDONE(Reify);
    P("Static propagate start");
    if(constraint_locked)
      return;

    if(i == -1000000000)
    {
      P("reifyvar assigned - Do full propagate");
      #ifdef NODETRICK
      if(reifysetnode==getState(stateObj).getNodeCount())
      {
          numeric_limits<unsigned long long> ull;  // I hope the compiler will get rid fo this..
          reifysetnode=ull.max();  // avoid this happening more than once.
          return;
      }
      #endif

      constraint_locked = true;
      getQueue(stateObj).pushSpecialTrigger(this);
      return;
    }

    if(full_propagate_called)
    {
      P("Already doing static full propagate");
      D_ASSERT(reify_var.isAssigned());
      if(reify_var.getAssignedValue() == 1)
      {
          pair<int,int> childTrigger = getChildStaticTrigger(i);
          if(childTrigger.first != 0)
          {
              return;
          }
          P("Passing trigger " << childTrigger.first <<","<< childTrigger.second << " on");
          child_constraints[0]->propagate(childTrigger.second, domain);
      }
      else
      {
          D_ASSERT(reify_var.getAssignedValue()==0)
          pair<int,int> childTrigger = getChildStaticTrigger(i);
          if(childTrigger.first != 1)
          {
              return;
          }
          P("Passing trigger " << childTrigger.first <<","<< childTrigger.second << " on");
          child_constraints[1]->propagate(childTrigger.second, domain);
      }
    }
  }

  virtual void propagate(DynamicTrigger* trig)
  {
    PROP_INFO_ADDONE(Reify);
    P("Dynamic prop start");
    if(constraint_locked)
      return;

    DynamicTrigger* dt = dynamic_trigger_start();
    //int numtriggers=dynamic_trigger_count();

    if(!full_propagate_called)
    {
        if(trig >= dt && trig < (dt + (c0vars*2)) )
        {// Lost assignments for positive constraint.
            P("Triggered on an assignment watch for the positive child constraint");
            #ifdef MINION_DEBUG
            // make sure that some WL has been lost.
            bool wllost=false;
            vector<AnyVarRef> t1=*(child_constraints[0]->get_vars_singleton());

            for(int i=0; i<triggerpairs[0].size(); i++)
            {
                if(!t1[triggerpairs[0][i].first].inDomain(triggerpairs[0][i].second))
                {
                    wllost=true;
                }
            }
            //D_ASSERT(wllost); This is not true because some WLs may be translated to domainchanged triggers.
            #endif
            bool flag;
            PROP_INFO_ADDONE(ReifyPropGetAssgPosCon);
            GET_ASSIGNMENT(assignment, child_constraints[0]);

            P("Find new assignment");
            if(!flag)
            { // No satisfying assignment to constraint
              P("Failed!");
              reify_var.propagateAssign(0);

              #ifdef NODETRICK
              reifysetnode=getState(stateObj).getNodeCount();
              #endif

              return;
            }
            P("Found new assignment");
            watch_assignment(assignment, *(child_constraints[0]->get_vars_singleton()), dt, dt+(c0vars*2));

            return;
        }
        else if(trig>= (dt+ (c0vars*2)) && trig < dt+dtcount)
        {// Lost assignments for negative constraint.
            P("Triggered on an assignment watch for the negative child constraint");
            #ifdef MINION_DEBUG
            // make sure that some WL has been lost.
            bool wllost=false;
            vector<AnyVarRef> t1=*(child_constraints[1]->get_vars_singleton());

            for(int i=0; i<triggerpairs[1].size(); i++)
            {
                if(!t1[triggerpairs[1][i].first].inDomain(triggerpairs[1][i].second))
                {
                    wllost=true;
                }
            }
            //D_ASSERT(wllost);
            #endif
            bool flag;
            GET_ASSIGNMENT(assignment, child_constraints[1]);
            PROP_INFO_ADDONE(ReifyPropGetAssgNegCon);

            P("Find new assignment");
            if(!flag)
            { // No satisfying assignment to constraint
              P("Failed!");
              reify_var.propagateAssign(1);

              #ifdef NODETRICK
              reifysetnode=getState(stateObj).getNodeCount();
              #endif

              return;
            }
            P("Found new assignment");
            watch_assignment(assignment, *(child_constraints[1]->get_vars_singleton()), dt+(c0vars*2), dt+dtcount);
            return;
        }
        else
        {
          P("Remove unused trigger");
          // This is an optimisation. Remove a trigger from stage 2.
          releaseTrigger(stateObj, trig);
        }
    }
    else // full_propagate_called
    {
        if(trig>= dt && trig < dt+dtcount)
        {   // is it a trigger from stage 1 .. if so, ignore.
            P("In stage 2, ignoring trigger from stage 1");
            return;
        }
      P("Pass triggers to children");
      D_ASSERT(reify_var.isAssigned());

      int child=getChildDynamicTrigger(trig);
      if(reify_var.getAssignedValue() == child)
      {
          P("Removing leftover trigger from other child constraint");
          releaseTrigger(stateObj, trig);
          return;
      }
      child_constraints[getChildDynamicTrigger(trig)]->propagate(trig);
    }

  }

  template<typename T, typename Vars, typename Trigger>
  void watch_assignment(const T& assignment, Vars& vars, Trigger* trig, Trigger* endtrig)
  {
    for(int i = 0; i < assignment.size(); ++i)
    {
      D_ASSERT(vars[assignment[i].first].inDomain(assignment[i].second));
      D_ASSERT(trig+i < endtrig);
      if(vars[assignment[i].first].isBound()) {
        vars[assignment[i].first].addDynamicTrigger(trig + i, DomainChanged);
      } else {
        vars[assignment[i].first].addDynamicTrigger(trig + i, DomainRemoval, assignment[i].second);
      }
    }
    // clear a contiguous block of used triggers up to (not including) endtrig
    D_DATA(int firstunattached = -1);
    for(int i=assignment.size(); (trig+i)<endtrig; i++)
    {
        if(!(trig+i)->isAttached())
        {
            D_DATA(firstunattached=i);
            break;
        }
        releaseTrigger(stateObj, trig + i);
    }

    #ifdef MINION_DEBUG
    if(firstunattached != -1)
    {
      for(int i=firstunattached; (trig+i)<endtrig; i++)
      {
          D_ASSERT(!(trig+i)->isAttached());
      }
    }
    // put the triggers into triggerpairs to check later.
    int cid=((trig==dynamic_trigger_start())?0:1);
    triggerpairs[cid].clear();
    for(int i=0; i<assignment.size(); i++)
    {
        triggerpairs[cid].push_back(assignment[i]);
    }
    #endif
  }

  virtual void full_propagate()
  {
    P("Full prop");
    P("reify " << child_constraints[0]->constraint_name());
    P("negation: " << child_constraints[1]->constraint_name());
    if(reify_var.isAssigned())
    {
        if(reify_var.getAssignedValue() > 0)
        {
          child_constraints[0]->full_propagate();
        }
        else
        {
          child_constraints[1]->full_propagate();
        }
        full_propagate_called = true;
        return;
    }

    DynamicTrigger* dt = dynamic_trigger_start();
    //int dt_count = dynamic_trigger_count();
    // Clean up triggers
    for(int i = 0; i < dtcount; ++i)
      releaseTrigger(stateObj, dt + i);

    bool flag;
    GET_ASSIGNMENT(assignment0, child_constraints[0]);
    PROP_INFO_ADDONE(ReifyFullPropGetAssgPosCon);
    if(!flag)
    { // No satisfying assignment to constraint
      reify_var.propagateAssign(0);

      #ifdef NODETRICK
      reifysetnode=getState(stateObj).getNodeCount();
      #endif

      return;
    }
    PROP_INFO_ADDONE(ReifyFullPropGetAssgNegCon);
    GET_ASSIGNMENT(assignment1, child_constraints[1]);
    if(!flag)
    { // No satisfying assignment to constraint
      reify_var.propagateAssign(1);
      #ifdef NODETRICK
      reifysetnode=getState(stateObj).getNodeCount();
      #endif

      return;
    }

    watch_assignment(assignment0, *(child_constraints[0]->get_vars_singleton()), dt, dt+(c0vars*2));
    watch_assignment(assignment1, *(child_constraints[1]->get_vars_singleton()), dt+(c0vars*2), dt+dtcount);
  }
};

#else
// ------------------------------------------------------------------------------------------------------------------

// NEWREIFY is not defined so use the older reify code.

template<typename BoolVar>
struct reify : public AbstractConstraint
{
  virtual string constraint_name()
  { return "Reify:" + poscon->constraint_name(); }

  AbstractConstraint* poscon;
  AbstractConstraint* negcon;
  BoolVar rar_var;

  // These two variables are only used in special cases.
  bool constraint_locked;
  Reversible<bool> full_propagate_called;

  reify(StateObj* _stateObj, AbstractConstraint* _poscon, BoolVar v) : AbstractConstraint(_stateObj), poscon(_poscon),
                                                               rar_var(v),  constraint_locked(false),
                                                               full_propagate_called(stateObj, false)
  { negcon = poscon->reverse_constraint();}

  virtual AbstractConstraint* reverse_constraint()
  {
    cerr << "You can't reverse a reified Constraint!";
    FAIL_EXIT();
  }

  virtual BOOL check_assignment(DomainInt* vals, int v_size)
  {
    DomainInt back_val = *(vals + (v_size - 1));
    if(back_val != 0)
    {
      return poscon->check_assignment(vals, poscon->get_vars_singleton()->size());
    }
    else
    {
      vals += poscon->get_vars_singleton()->size();
      return negcon->check_assignment(vals, negcon->get_vars_singleton()->size());
    }
  }

  virtual vector<AnyVarRef> get_vars()
  {
    // We have to push back both sets of variables, as they may
    // have been transformed in different ways.
    vector<AnyVarRef> vec1 = poscon->get_vars();
    vector<AnyVarRef> vec2 = negcon->get_vars();
    vec1.reserve(vec1.size() + vec2.size() + 1);
    for(unsigned i = 0; i < vec2.size(); ++i)
      vec1.push_back(vec2[i]);
    vec1.push_back(rar_var);
    return vec1;
  }

  virtual triggerCollection setup_internal()
  {
    triggerCollection postrig = poscon->setup_internal();
    triggerCollection negtrig = negcon->setup_internal();
    triggerCollection triggers;
    for(unsigned int i=0;i<postrig.size();i++)
    {
      postrig[i]->trigger.info = postrig[i]->trigger.info * 2;
      postrig[i]->trigger.constraint = this;
      triggers.push_back(postrig[i]);
    }

    for(unsigned int i=0;i<negtrig.size();i++)
    {
      negtrig[i]->trigger.info = negtrig[i]->trigger.info * 2 + 1;
      negtrig[i]->trigger.constraint = this;
      triggers.push_back(negtrig[i]);
    }

    triggers.push_back(make_trigger(rar_var, Trigger(this, -99999), LowerBound));
    triggers.push_back(make_trigger(rar_var, Trigger(this, -99998), UpperBound));
    return triggers;
  }


  virtual void special_check()
  {
    D_ASSERT(constraint_locked);
    constraint_locked = false;
    D_ASSERT(rar_var.isAssigned());
    if(rar_var.getAssignedValue() > 0)
      poscon->full_propagate();
    else
      negcon->full_propagate();
    full_propagate_called = true;
  }

  virtual void special_unlock()
  {
    D_ASSERT(constraint_locked);
      constraint_locked = false;
  }

  virtual void propagate(int i, DomainDelta domain)
  {
    PROP_INFO_ADDONE(Reify);

    if(constraint_locked)
      return;

    if(i == -99998 || i == -99999)
    {
        constraint_locked = true;
      getQueue(stateObj).pushSpecialTrigger(this);
      return;
    }

    if(full_propagate_called)
    {
      D_ASSERT(rar_var.isAssigned());
      if(rar_var.getAssignedValue() == 1)
        { if(i%2 == 0) poscon->propagate(i/2, domain); }
      else
        { if(i%2 == 1) negcon->propagate((i-1)/2, domain); }
      return;
    }

    if(i%2 == 0)
    {
#ifdef MINION_DEBUG
      {
    bool flag;
    GET_ASSIGNMENT(assignment0, poscon);
    bool unsat = poscon->check_unsat(i/2, domain);
    D_ASSERT((!flag && unsat) || (flag && !unsat));
      }
#endif
      PROP_INFO_ADDONE(ReifyCheckUnsatPosCon);
      if(poscon->check_unsat(i/2, domain))
      {
        rar_var.propagateAssign(false);
      }
    }
    else
    {
#ifdef MINION_DEBUG
      {
    bool flag;
    GET_ASSIGNMENT(assignment0, negcon);
    bool unsat = negcon->check_unsat((i-1)/2,domain);
    D_ASSERT((!flag && unsat) || (flag && !unsat));
      }
#endif
      PROP_INFO_ADDONE(ReifyCheckUnsatNegCon);
      if(negcon->check_unsat((i-1)/2,domain))
      {
        rar_var.propagateAssign(true);
      }
    }
  }

  virtual void full_propagate()
  {
#ifdef MINION_DEBUG
    {
      bool flag;
      GET_ASSIGNMENT(assignment0, poscon);
      bool unsat = poscon->full_check_unsat();
      D_ASSERT((!flag && unsat) || (flag && !unsat));
    }
#endif
    PROP_INFO_ADDONE(ReifyFullCheckUnsatPosCon);
    if(poscon->full_check_unsat())
    {
      rar_var.propagateAssign(false);
    }

#ifdef MINION_DEBUG
    {
      bool flag;
      GET_ASSIGNMENT(assignment0, negcon);
      bool unsat = negcon->full_check_unsat();
      D_ASSERT((!flag && unsat) || (flag && !unsat));
    }
#endif
    PROP_INFO_ADDONE(ReifyFullCheckUnsatNegCon);
    if(negcon->full_check_unsat())
    {
      rar_var.propagateAssign(true);
    }

    if(rar_var.isAssigned())
    {
      if(rar_var.getAssignedValue() > 0)
        poscon->full_propagate();
      else
        negcon->full_propagate();
      full_propagate_called = true;
    }
  }
};

#endif
// end of ifdef NEWREIFY

template<typename BoolVar>
reify<BoolVar>*
reifyCon(StateObj* stateObj, AbstractConstraint* c, BoolVar var)
{ return new reify<BoolVar>(stateObj, &*c, var); }
#endif
