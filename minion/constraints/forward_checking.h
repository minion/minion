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

#ifndef FORWARD_CHECKING_H_HIUO
#define FORWARD_CHECKING_H_HIUO

#include "constraint_abstract.h"
#include "../memory_management/reversible_vals.h"
#include "../get_info/get_info.h"
#include "../queue/standard_queue.h"

#ifdef P
#undef P
#endif

//#define P(x) cout << x << endl
#define P(x)

struct Forward_Checking : public AbstractConstraint
{

  virtual string extended_name()
  { return constraint_name() + ":" + child->extended_name(); }

  virtual string constraint_name()
  { return "forwardchecking"; }

  CONSTRAINT_ARG_LIST1(child);

  AbstractConstraint* child;

  Forward_Checking(StateObj* _stateObj, AbstractConstraint* _con) :
  AbstractConstraint(_stateObj), child(_con), trig1(-1), trig2(-1)
  { }

  virtual AbstractConstraint* reverse_constraint()
  {
    return new Forward_Checking(stateObj, child->reverse_constraint());
  }


  virtual ~Forward_Checking()
    { delete child; }

  virtual SysInt dynamic_trigger_count()
   { return 2; }

  virtual bool get_satisfying_assignment(box<pair<SysInt,DomainInt> >& assignment)
  { return child->get_satisfying_assignment(assignment); }

  virtual BOOL check_assignment(DomainInt* v, SysInt v_size)
  { return child->check_assignment(v, v_size); }

  virtual vector<AnyVarRef> get_vars()
  { return child->get_vars(); }
  
  SysInt trig1, trig2;
  
  virtual void propagate(DynamicTrigger* dt)
  {
      DynamicTrigger* dtstart = dynamic_trigger_start();
      SysInt size = child->get_vars_singleton()->size();
      vector<AnyVarRef>* vars = child->get_vars_singleton();
      
      if(dt==NULL) {
          trig1=trig2=-1;
          
          trig1=find_new_trigger(-1, -1, dtstart, size, vars);
          
          // if all variables assigned
          if(trig1==-1) {
              if(full_assignment_failed(size, vars)) {
                  getState(stateObj).setFailed(true);
              }
              return;
          }
          
          trig2=find_new_trigger(trig1, -1, dtstart+1, size, vars);
          
          if(trig2==-1) {   // One variable unassigned
              perform_forward_checking(trig1, size, vars);
          }
          return;
      }
      else if(dt==dtstart) {
          // Find trigger 1 again.
          
          SysInt temp=find_new_trigger(trig2, trig1, dtstart, size, vars);
          
          if(temp!=-1) {
              trig1=temp;
          }
          else {
              // At most one var is unassigned.
              perform_forward_checking(trig2, size, vars);
          }
          return;
      }
      else if(dt==dtstart+1) {
          // Find trigger 2 again.
          
          SysInt temp=find_new_trigger(trig1, trig2, dtstart+1, size, vars);
          
          if(temp!=-1) {
              trig2=temp;
          }
          else {
              // At most one var is unassigned.
              perform_forward_checking(trig1, size, vars);
          }
          return;
      }
      else {
          D_ASSERT(false);
      }
  }
  
  bool full_assignment_failed(SysInt size, vector<AnyVarRef>* vars) {
    MAKE_STACK_BOX(b, DomainInt, size);
    for(SysInt i = 0; i < size; ++i)
        b.push_back((*vars)[i].getAssignedValue());
    
    if(!check_assignment(&b[0], size)) {
        return true;  // true means failed.
    }
    else {
        return false;
    }
  }
  
  SysInt find_new_trigger(SysInt toavoid, SysInt start, DynamicTrigger* dtthis, SysInt size, vector<AnyVarRef>* vars) {
      SysInt i=start+1;
      for(; i<size ; i++) {
          if(i!=toavoid  &&  !(*vars)[i].isAssigned())
          {
              (*vars)[i].addDynamicTrigger(dtthis, DomainChanged);
              return i;
          }
      }
      
      // Wrap around.
      for(i=0; i<=start; i++) {
          if(i!=toavoid  &&  !(*vars)[i].isAssigned())
          {
              (*vars)[i].addDynamicTrigger(dtthis, DomainChanged);
              return i;
          }
      }
      
      return -1;
  }
  
  void perform_forward_checking(SysInt var, SysInt size, vector<AnyVarRef>* vars) {
      MAKE_STACK_BOX(b, DomainInt, size);
      AnyVarRef v=(*vars)[var];
      
      for(SysInt i = 0; i < size; ++i) {
          if(i!=var) {
              D_ASSERT((*vars)[i].isAssigned());
              b.push_back((*vars)[i].getAssignedValue());
          }
          else {
              b.push_back(-1000000);
          }
      }
      
      DomainInt maxval=v.getMax();
      
      for(DomainInt value=v.getMin(); value<=maxval; value++) {
          if(v.inDomain(value)) {
              b[var]=value;
              if(!check_assignment(&b[0], size)) {
                  v.removeFromDomain(value);
              }
          }
      }
  }
  
  
  
  virtual void full_propagate()
  { propagate(NULL); }
};

inline AbstractConstraint*
forwardCheckingCon(StateObj* stateObj, AbstractConstraint* c)
{ return new Forward_Checking(stateObj, c); }

#endif
