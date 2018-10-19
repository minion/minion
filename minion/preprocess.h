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

#ifndef PREPROCESS_H
#define PREPROCESS_H

#include "solver.h"

template <typename Var, typename Vars, typename Prop>
bool inline check_fail(Var& var, DomainInt val, Vars& vars, Prop prop) {
  Controller::world_push();
  var.assign(val);
  prop(vars);

  bool check_failed = getState().isFailed();

  Controller::world_pop();

  return check_failed;
}

template <typename Var, typename Vars, typename Prop>
bool inline check_fail_range(Var& var, DomainInt lowval, DomainInt highval, Vars& vars, Prop prop) {
  Controller::world_push();
  var.setMin(lowval);
  var.setMax(highval);
  prop(vars);

  bool check_failed = getState().isFailed();
  getState().setFailed(false);

  Controller::world_pop();

  return check_failed;
}

inline bool check_sac_timeout() {
  if(Parallel::isAlarmActivated()) {
    if(Parallel::isCtrlCPressed()) {
      getState().setFailed(true);
      return true;
    } else {
      getOptions().printLine("Time out in preprocessing.");
      getTableOut().set("TimeOut", 1);
      return true;
    }
  }
  return false;
}

template<typename Var, typename Prop>
bool prune_domain_top(Var& var, vector<Var>& vararray, Prop prop, bool limit)
{
  bool pruned = false;
  bool everfailed = false;
  DomainInt gallop = 1;
  while(true) {
    if(check_sac_timeout())
      throw EndOfSearch();
    if(var.getMin() == var.getMax()) {
      return pruned;
    }
    DomainInt maxval = var.getMax();
    DomainInt step = maxval - gallop;
    bool check = check_fail_range(var, step+1, maxval, vararray, prop);
    if(check) {
      pruned = true;
      var.setMax(step);
      prop(vararray);
      if(getState().isFailed())
        return pruned;
      if(everfailed && limit) {
        gallop /= 2;
      }
      else {
        gallop *= 2;
      }
      DomainInt maxstep = var.getMax() - var.getMin();
      if(maxstep == 0)
        return pruned;
      gallop = min(gallop, maxstep);
    }
    else {
      everfailed = true;
      gallop /= 2;
    }
    if(gallop == 0) {
      return pruned;
    }
  }
}

template<typename Var, typename Prop>
bool prune_domain_bottom(Var& var, vector<Var>& vararray, Prop prop, bool limit)
{
  bool pruned = false;
  bool everfailed = false;
  DomainInt gallop = 1;
  while(true) {
    if(check_sac_timeout())
      throw EndOfSearch();
    if(var.getMin() == var.getMax()) {
      return pruned;
    }
    DomainInt minval = var.getMin();
    DomainInt step = minval + gallop;
    bool check = check_fail_range(var, minval, step-1, vararray, prop);
    if(check) {
      pruned = true;
      var.setMin(step);
      prop(vararray);
      if(getState().isFailed())
        return pruned;
      if(everfailed && limit) {
        gallop /= 2;
      }
      else {
        gallop *= 2;
      }
      DomainInt maxstep = var.getMax() - var.getMin();
      if(maxstep == 0)
        return pruned;
      gallop = min(gallop, maxstep);
    }
    else {
      everfailed = true;
      gallop /= 2;
    }
    if(gallop == 0) {
      return pruned;
    }
  }
}

template <typename Var, typename Prop>
void propagateSAC_internal(vector<Var>& vararray, Prop prop, bool onlyCheckBounds, bool limit) {
  getQueue().propagateQueue();
  if(getState().isFailed())
    return;
  bool reduced = true;
  int loops = 0;
  while(reduced) {
    // First loop around bounds as long as possible
    while(reduced) {
      if(limit) {
        loops++;
        if(loops > std::min(5, (int)log2(vararray.size()))) {
          return;
        }
      } 

      reduced = false;
      for(SysInt i = 0; i < (SysInt)vararray.size(); ++i) {
        Var& var = vararray[i];
        if(!var.isAssigned()) {
          if(prune_domain_bottom(var, vararray, prop, limit))
            reduced = true;
          if(getState().isFailed())
            return;
          if(prune_domain_top(var, vararray, prop, limit))
            reduced = true;
          if(getState().isFailed())
            return;
          if(check_sac_timeout())
            throw EndOfSearch();
        }
      }
    }

    // Then try inside domain
    if(!onlyCheckBounds) {
      for(SysInt i = 0; i < (SysInt)vararray.size(); ++i) {
        Var& var = vararray[i];
        if(!var.isBound()) {
          for(DomainInt val = var.getMin() + 1; val <= var.getMax() - 1; ++val) {
            if(check_sac_timeout())
              throw EndOfSearch();
            if(var.inDomain(val) && check_fail(var, val, vararray, prop)) {
              reduced = true;
              var.removeFromDomain(val);
              prop(vararray);
              if(getState().isFailed())
                return;
            }
          }
        }
      }
    }
  }
}

struct PropagateGAC {
  PropagationLevel level;

  PropagateGAC(PropagationLevel _level) :
  level(_level) {}
  
  template <typename Vars>
  void operator()(Vars&) {
    getQueue().propagateQueue();
  }
};

struct PropagateSAC {
  PropagationLevel level;

  PropagateSAC(PropagationLevel _level) :
  level(_level) {}
  
  template <typename Vars>
  void operator()(Vars& vars) {
    propagateSAC_internal(vars, PropagateGAC(level), false, level.limit);
  }
};

struct PropagateSAC_Bounds {
  PropagationLevel level;

  PropagateSAC_Bounds(PropagationLevel _level) :
  level(_level) {}
  
    template <typename Vars>
  void operator()(Vars& vars) {
    propagateSAC_internal(vars, PropagateGAC(level), true, level.limit);
  }
};

struct PropagateSSAC {
  PropagationLevel level;

  PropagateSSAC(PropagationLevel _level) :
  level(_level) {}
  
  template <typename Vars>
  void operator()(Vars& vars) {
    PropagateSAC sac(level);;
    propagateSAC_internal(vars, sac, false, level.limit);
  }
};

struct PropagateSSAC_Bounds {
  PropagationLevel level;

  PropagateSSAC_Bounds(PropagationLevel _level) :
  level(_level) {}

  template <typename Vars>
  void operator()(Vars& vars) {
    PropagateSAC sac(level);
    propagateSAC_internal(vars, sac, true, level.limit);
  }
};

// Class heirarchy to allow virtual function calls to the above.
struct Propagate {
  virtual ~Propagate() {}
  virtual void prop(vector<AnyVarRef>& vars){};
};

struct PropGAC : Propagate {
  PropGAC(PropagationLevel level)
  : prop_obj(level)
  { }

  PropagateGAC prop_obj;
  inline void prop(vector<AnyVarRef>& vars) {
    prop_obj(vars);
  }
};

struct PropSAC : Propagate {
  PropSAC(PropagationLevel level)
  : prop_obj(level)
  { }
  
  PropagateSAC prop_obj;
  inline void prop(vector<AnyVarRef>& vars) {
    prop_obj(vars);
  }
};

struct PropSSAC : Propagate {
  PropSSAC(PropagationLevel level)
  : prop_obj(level)
  { }

  PropagateSSAC prop_obj;
  inline void prop(vector<AnyVarRef>& vars) {
    prop_obj(vars);
  }
};

struct PropSAC_Bounds : Propagate {
  PropSAC_Bounds(PropagationLevel level)
  : prop_obj(level)
  { }

  PropagateSAC_Bounds prop_obj;
  inline void prop(vector<AnyVarRef>& vars) {
    prop_obj(vars);
  }
};

struct PropSSAC_Bounds : Propagate {
  PropSSAC_Bounds(PropagationLevel level)
  : prop_obj(level)
  { }

  PropagateSSAC_Bounds prop_obj;
  inline void prop(vector<AnyVarRef>& vars) {
    prop_obj(vars);
  }
};

void PropogateCSP(PropagationLevel, vector<AnyVarRef>&, bool print_info = false);

#endif
