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

  virtual string extended_name()
  { return constraint_name() + ":" + child_constraints[0]->extended_name(); }

  virtual string constraint_name()
  { return "reify"; }

  CONSTRAINT_ARG_LIST2(child_constraints[0], reify_var);

  BoolVar reify_var;
  SysInt reify_var_num;

  bool constraint_locked;
  Reversible<bool> full_propagate_called;

  #ifdef NODETRICK
  unsigned long long reifysetnode;
  #endif

  SysInt dtcount;
  SysInt c0vars;  // how many vars for child_constraints[0]

  typedef vector<vector<pair<DomainInt, DomainInt> > > triggerpairstype;
  D_DATA(triggerpairstype triggerpairs);

  reify(AbstractConstraint* _poscon, BoolVar _rar_var) :
  ParentConstraint(), reify_var(_rar_var), constraint_locked(false),
    full_propagate_called(false)
  {
      CHECK(reify_var.getInitialMin() >= 0 && reify_var.getInitialMax() <= 1, "reify only works on Boolean variables");
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

    D_DATA(triggerpairs.resize(2));
  }

  virtual AbstractConstraint* reverse_constraint()
  {
      // reverse it by swapping the positive and negative constraints.
      // we call 'reverse_constraint' here to force a new copy of the constraint
      return new reify<BoolVar>(child_constraints[0]->reverse_constraint(), reify_var);
  }

  virtual SysInt dynamic_trigger_count()
  {
    return child_constraints[0]->get_vars_singleton()->size()*2
        +child_constraints[1]->get_vars_singleton()->size()*2;  // *2 for each child constraint.
  }

  virtual bool get_satisfying_assignment(box<pair<SysInt,DomainInt> >& assignment)
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
            for(SysInt i = 0; i < (SysInt)assignment.size(); ++i)
              assignment[i].first += c0vars;
            assignment.push_back(make_pair(reify_var_num, 0));
            return true;
        }
    }
    return false;
  }

  virtual BOOL check_assignment(DomainInt* vals, SysInt v_size)
  {
    DomainInt back_val = *(vals + v_size - 1);
    if(back_val == 1)
    {
      return child_constraints[0]->check_assignment(vals, c0vars);
    }
    else if(back_val == 0)
    {
      vals += c0vars;
      return child_constraints[1]->check_assignment(vals, (dtcount/2)-c0vars);
    }
    else
      return false;
  }

  virtual vector<AnyVarRef> get_vars()
  {
      // Push both sets of vars, then reify var.
    vector<AnyVarRef> vec0 = * child_constraints[0]->get_vars_singleton();
    vector<AnyVarRef> vec1 = * child_constraints[1]->get_vars_singleton();
    vector<AnyVarRef> c;
    c.reserve(vec0.size() + vec1.size() + 1);
    for(SysInt i=0; i<(SysInt)vec0.size(); i++)
        c.push_back(vec0[i]);
    for(SysInt i=0; i<(SysInt)vec1.size(); i++)
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
    D_ASSERT(reify_var.isAssigned() &&
             (reify_var.getAssignedValue() == 0 ||
              reify_var.getAssignedValue() == 1) );
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

  virtual void propagate(DomainInt i, DomainDelta domain)
  {
    PROP_INFO_ADDONE(Reify);
    P("Static propagate start");
    if(constraint_locked)
      return;

    if(i == -1000000000)
    {
        if(!full_propagate_called) {
            P("reifyvar assigned - Do full propagate");
            #ifdef NODETRICK
            if(reifysetnode==getState().getNodeCount())
            {
              numeric_limits<unsigned long long> ull;  // I hope the compiler will get rid fo this..
              reifysetnode=ull.max();  // avoid this happening more than once.
              return;
            }
            #endif

            constraint_locked = true;
            getQueue().pushSpecialTrigger(this);
        }
        return;
    }

    if(full_propagate_called)
    {
      P("Already doing static full propagate");
      D_ASSERT(reify_var.isAssigned());
      if(reify_var.getAssignedValue() == 1)
      {
          pair<DomainInt, DomainInt> childTrigger = getChildStaticTrigger(i);
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
          pair<DomainInt, DomainInt> childTrigger = getChildStaticTrigger(i);
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
    //SysInt numtriggers=dynamic_trigger_count();

    if(!full_propagate_called)
    {
        if(trig >= dt && trig < (dt + (c0vars*2)) )
        {// Lost assignments for positive constraint.
            P("Triggered on an assignment watch for the positive child constraint");
            #ifdef MINION_DEBUG
            // make sure that some WL has been lost.
            bool wllost=false;
            vector<AnyVarRef> t1=*(child_constraints[0]->get_vars_singleton());

            for(SysInt i=0; i<(SysInt)triggerpairs[0].size(); i++)
            {
                if(!t1[checked_cast<SysInt>(triggerpairs[0][i].first)].inDomain(triggerpairs[0][i].second))
                {
                    wllost=true;
                }
            }
            (void)wllost;
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
              reifysetnode=getState().getNodeCount();
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

            for(SysInt i=0; i<(SysInt)triggerpairs[1].size(); i++)
            {
                if(!t1[checked_cast<SysInt>(triggerpairs[1][i].first)].inDomain(triggerpairs[1][i].second))
                {
                    wllost=true;
                }
            }
            (void)wllost;
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
              reifysetnode=getState().getNodeCount();
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
          releaseTrigger(trig);
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

      SysInt child=getChildDynamicTrigger(trig);
      if(reify_var.getAssignedValue() == child)
      {
          P("Removing leftover trigger from other child constraint");
          releaseTrigger(trig);
          return;
      }
      child_constraints[getChildDynamicTrigger(trig)]->propagate(trig);
    }

  }

  template<typename T, typename Vars, typename Trigger>
  void watch_assignment(const T& assignment, Vars& vars, Trigger* trig, Trigger* endtrig)
  {
    for(SysInt i = 0; i < (SysInt)assignment.size(); ++i)
    {
      const SysInt aif = checked_cast<SysInt>(assignment[i].first);
      D_ASSERT(vars[aif].inDomain(assignment[i].second));
      D_ASSERT(trig+i < endtrig);
      if(vars[aif].isBound()) {
        moveTrigger(vars[aif], trig + i, DomainChanged);
      } else {
        moveTrigger(vars[aif], trig + i, DomainRemoval, assignment[i].second);
      }
    }
    // clear a contiguous block of used triggers up to (not including) endtrig
    D_DATA(SysInt firstunattached = -1);
    for(SysInt i=assignment.size(); (trig+i)<endtrig; i++)
    {
      /// XXX : This is inefficent, but required for constant variables
      /*  if(!(trig+i)->isAttached())
        {
            D_DATA(firstunattached=i);
            break;
        } */
        releaseTrigger(trig + i);
    }

    #ifdef MINION_DEBUG
    if(firstunattached != -1)
    {
      for(SysInt i=firstunattached; (trig+i)<endtrig; i++)
      {
          D_ASSERT(!(trig+i)->isAttached());
      }
    }
    // put the triggers into triggerpairs to check later.
    SysInt cid=((trig==dynamic_trigger_start())?0:1);
    triggerpairs[cid].clear();
    for(SysInt i=0; i<(SysInt)assignment.size(); i++)
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

    D_ASSERT(reify_var.getMin() >= 0);
    D_ASSERT(reify_var.getMax() <= 1);
    if(getState().isFailed()) return;

    if(reify_var.isAssigned())
    {
        if(reify_var.getAssignedValue() == 1)
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
    //SysInt dt_count = dynamic_trigger_count();
    // Clean up triggers
    for(SysInt i = 0; i < dtcount; ++i)
      releaseTrigger(dt + i);

    bool flag;
    GET_ASSIGNMENT(assignment0, child_constraints[0]);
    PROP_INFO_ADDONE(ReifyFullPropGetAssgPosCon);
    if(!flag)
    { // No satisfying assignment to constraint
      reify_var.propagateAssign(0);

      #ifdef NODETRICK
      reifysetnode=getState().getNodeCount();
      #endif

      return;
    }
    PROP_INFO_ADDONE(ReifyFullPropGetAssgNegCon);
    GET_ASSIGNMENT(assignment1, child_constraints[1]);
    if(!flag)
    { // No satisfying assignment to constraint
      reify_var.propagateAssign(1);
      #ifdef NODETRICK
      reifysetnode=getState().getNodeCount();
      #endif

      return;
    }

    watch_assignment(assignment0, *(child_constraints[0]->get_vars_singleton()), dt, dt+(c0vars*2));
    watch_assignment(assignment1, *(child_constraints[1]->get_vars_singleton()), dt+(c0vars*2), dt+dtcount);
  }
};

#else

#error OLD_REIFY is gone

#endif
// end of ifdef NEWREIFY

template<typename BoolVar>
reify<BoolVar>*
reifyCon(AbstractConstraint* c, BoolVar var)
{ return new reify<BoolVar>(&*c, var); }

template<typename VarArray>
inline AbstractConstraint*
BuildCT_REIFY(const VarArray& vars, ConstraintBlob& bl)
{
  switch(bl.internal_constraints[0].constraint->type)
  {
    case CT_EQ:
    {
      ConstraintBlob blob(bl.internal_constraints[0]);
      blob.vars.push_back(make_vec(bl.vars[0][0]));
      blob.constraint = get_constraint(CT_EQ_REIFY);
      return build_constraint(blob);
    }
    case CT_DISEQ:
    {
      ConstraintBlob blob(bl.internal_constraints[0]);
      blob.vars.push_back(make_vec(bl.vars[0][0]));
      blob.constraint = get_constraint(CT_DISEQ_REIFY);
      return build_constraint(blob);
    }
    case CT_MINUSEQ:
    {
      ConstraintBlob blob(bl.internal_constraints[0]);
      blob.vars.push_back(make_vec(bl.vars[0][0]));
      blob.constraint = get_constraint(CT_MINUSEQ_REIFY);
      return build_constraint(blob);
    }
    default:
      return reifyCon(build_constraint(bl.internal_constraints[0]), vars[0]);
  }
}

/* JSON
{ "type": "constraint",
  "name": "reify",
  "internal_name": "CT_REIFY",
  "args": [ "read_constraint", "read_var" ]
}
*/

#endif
