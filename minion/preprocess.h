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

#ifndef PREPROCESS_H
#define PREPROCESS_H

#include "solver.h"


template<typename Var, typename Vars, typename Prop>
bool inline check_fail(StateObj* stateObj, Var& var, DomainInt val, Vars& vars, Prop prop)
{
  Controller::world_push(stateObj);
  var.propagateAssign(val);
  prop(stateObj, vars);
  
  bool check_failed = getState(stateObj).isFailed();
  getState(stateObj).setFailed(false);
  
  Controller::world_pop(stateObj);
  
  return check_failed;
}

inline bool check_sac_timeout(StateObj* stateObj)
{
    if(getState(stateObj).isAlarmActivated())
    {
        getState(stateObj).clearAlarm();
        if(getState(stateObj).isCtrlcPressed())
        {
            getState(stateObj).setFailed(true);
            return true;
        }
        else
        {
            getOptions(stateObj).printLine("Time out in preprocessing.");
            getTableOut().set("TimeOut", 1);
            getState(stateObj).setFailed(true);
            return true;
        }
    }
    return false;
}

template <typename Var, typename Prop>
void propagateSAC_internal(StateObj* stateObj, vector<Var>& vararray, Prop prop, bool onlyCheckBounds)
{
  getQueue(stateObj).propagateQueue();
  if(getState(stateObj).isFailed())
    return;
  bool reduced = true;
  while(reduced)
  {
    reduced = false;
    for(int i = 0; i < vararray.size(); ++i)
    {
      Var& var = vararray[i];
      if(onlyCheckBounds || var.isBound())
      {
        while(check_fail(stateObj, var, var.getMax(), vararray, prop))
        {
          if(check_sac_timeout(stateObj)) return;
          reduced = true;
          var.setMax(var.getMax() - 1);
          prop(stateObj, vararray);
          if(getState(stateObj).isFailed())
            return;
        }
        
        while(check_fail(stateObj, var, var.getMin(), vararray, prop))
        {
          if(check_sac_timeout(stateObj)) return;
          reduced = true;
          var.setMin(var.getMin() + 1);
          prop(stateObj, vararray);
          if(getState(stateObj).isFailed())
            return;
        }
      }
      else
      {
        for(DomainInt val = var.getMin(); val <= var.getMax(); ++val)
        {
          if(check_sac_timeout(stateObj)) return;
          if(var.inDomain(val) && check_fail(stateObj, var, val, vararray, prop))
          {
            reduced = true;
            var.removeFromDomain(val);
            prop(stateObj, vararray);
            if(getState(stateObj).isFailed())
              return;          
          }
        }
      }
    }
  }
  
}

struct PropagateGAC
{
  template<typename Vars>
  void operator()(StateObj* stateObj, Vars&)
  {getQueue(stateObj).propagateQueue();}
};

struct PropagateSAC
{
  template<typename Vars>
  void operator()(StateObj* stateObj, Vars& vars)
  {propagateSAC_internal(stateObj, vars, PropagateGAC(), false);}
};

struct PropagateSAC_Bounds
{
  template<typename Vars>
  void operator()(StateObj* stateObj, Vars& vars)
  {propagateSAC_internal(stateObj, vars, PropagateGAC(), true);}
};

struct PropagateSSAC
{
  template<typename Vars>
  void operator()(StateObj* stateObj, Vars& vars)
  {
    PropagateSAC sac;
    propagateSAC_internal(stateObj, vars, sac, false);
  }
};

struct PropagateSSAC_Bounds
{
  template<typename Vars>
  void operator()(StateObj* stateObj, Vars& vars)
  {
    PropagateSAC sac;
    propagateSAC_internal(stateObj, vars, sac, true);
  }
};

// Class heirarchy to allow virtual function calls to the above.
struct Propagate
{
    virtual void prop(StateObj* stateObj, vector<AnyVarRef>& vars) {};
};

struct PropGAC : Propagate
{
    PropagateGAC prop_obj;
    inline void prop(StateObj* stateObj, vector<AnyVarRef>& vars)
    {
        prop_obj(stateObj, vars); 
    }
};

struct PropSAC : Propagate
{
    PropagateSAC prop_obj;
    inline void prop(StateObj* stateObj, vector<AnyVarRef>& vars)
    {
        prop_obj(stateObj, vars); 
    }
};

struct PropSSAC : Propagate
{
    PropagateSSAC prop_obj;
    inline void prop(StateObj* stateObj, vector<AnyVarRef>& vars)
    {
        prop_obj(stateObj, vars); 
    }
};

struct PropSAC_Bounds : Propagate
{
    PropagateSAC_Bounds prop_obj;
    inline void prop(StateObj* stateObj, vector<AnyVarRef>& vars)
    {
        prop_obj(stateObj, vars); 
    }
};

struct PropSSAC_Bounds : Propagate
{
    PropagateSSAC_Bounds prop_obj;
    inline void prop(StateObj* stateObj, vector<AnyVarRef>& vars)
    {
        prop_obj(stateObj, vars); 
    }
};

void PropogateCSP(StateObj*, PropagationLevel, vector<AnyVarRef>&, bool print_info = false);

#endif
