// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef PREPROCESS_H
#define PREPROCESS_H

#include "solver.h"

template <typename Var, typename Vars, typename Prop>
bool inline check_fail(Var& var, DomainInt val, Vars& vars, Prop prop) {
  Controller::worldPush();
  var.assign(val);
  prop(vars);

  bool checkFailed = getState().isFailed();

  Controller::worldPop();

  return checkFailed;
}

// Copied from dump_state.cpp
template <typename T>
inline string getNameFromVar(const T& v) {
  return getState().getInstance()->vars.getName(v.getBaseVar());
}

template <typename Var, typename Vars, typename Prop>
bool inline check_fail_range(Var& var, DomainInt lowval, DomainInt highval, Vars& vars, Prop prop) {
  Controller::worldPush();

  var.setMin(lowval);
  var.setMax(highval);
  prop(vars);

  bool checkFailed = getState().isFailed();
  Controller::worldPop();

  return checkFailed;
}

inline bool checkSACTimeout() {
  if(Parallel::isAlarmActivated()) {
    if(Parallel::isCtrlCPressed()) {
      getState().setFailed();
      return true;
    } else {
      getOptions().printLine("Time out in preprocessing.");
      getTableOut().set("TimeOut", 1);
      return true;
    }
  }
  return false;
}

template <typename Var, typename Prop>
bool pruneDomainTop(Var& var, vector<Var>& vararray, Prop prop, bool limit) {
  bool pruned = false;
  bool everfailed = false;
  DomainInt gallop = 1;
  while(true) {
    if(checkSACTimeout())
      throw EndOfSearch();
    if(var.min() == var.max()) {
      return pruned;
    }
    DomainInt maxval = var.max();
    DomainInt step = maxval - gallop;
    bool check = check_fail_range(var, step + 1, maxval, vararray, prop);
    if(check) {
      pruned = true;
      var.setMax(step);
      prop(vararray);
      if(getState().isFailed())
        return pruned;
      if(everfailed && limit) {
        gallop /= 2;
      } else {
        gallop *= 2;
      }
      DomainInt maxstep = var.max() - var.min();
      if(maxstep == 0)
        return pruned;
      gallop = min(gallop, maxstep);
    } else {
      everfailed = true;
      gallop /= 2;
    }
    if(gallop == 0) {
      return pruned;
    }
  }
}

template <typename Var, typename Prop>
bool pruneDomain_bottom(Var& var, vector<Var>& vararray, Prop prop, bool limit) {
  bool pruned = false;
  bool everfailed = false;
  DomainInt gallop = 1;
  while(true) {
    if(checkSACTimeout())
      throw EndOfSearch();
    if(var.min() == var.max()) {
      return pruned;
    }
    DomainInt minval = var.min();
    DomainInt step = minval + gallop;
    bool check = check_fail_range(var, minval, step - 1, vararray, prop);
    if(check) {
      pruned = true;
      var.setMin(step);
      prop(vararray);
      if(getState().isFailed())
        return pruned;
      if(everfailed && limit) {
        gallop /= 2;
      } else {
        gallop *= 2;
      }
      DomainInt maxstep = var.max() - var.min();
      if(maxstep == 0)
        return pruned;
      gallop = min(gallop, maxstep);
    } else {
      everfailed = true;
      gallop /= 2;
    }
    if(gallop == 0) {
      return pruned;
    }
  }
}

#include "constraints/constraint_collect_events.h"

template <typename Var, typename Prop>
void propagateSAC_internal(vector<Var>& vararray, Prop prop, bool onlyCheckBounds, bool limit) {
  getQueue().propagateQueue();
  if(getState().isFailed())
    return;
  bool reduced = true;
  int loops = 0;

  int upperlimit = std::min(5, (int)log2(vararray.size()));

  while(reduced) {
    //std::cerr << "Loop: " << loops << " " << upperlimit << std::endl;
    // First loop around bounds as long as possible
    while(reduced) {
      if(limit) {
        loops++;
        if(loops > upperlimit) {
          return;
        }
      }
    //std::cerr << "Inner Loop: " << loops << " " << upperlimit << std::endl;
      reduced = false;
      for(SysInt i = 0; i < (SysInt)vararray.size(); ++i) {
        //std::cerr << "Bound loop: " << i << std::endl;
        Var& var = vararray[i];
        if(!var.isAssigned()) {
          if(pruneDomain_bottom(var, vararray, prop, limit))
            reduced = true;
          if(getState().isFailed())
            return;
          if(pruneDomainTop(var, vararray, prop, limit))
            reduced = true;
          if(getState().isFailed())
            return;
          if(checkSACTimeout())
            throw EndOfSearch();
        }
      }
    }

    // Then try inside domain
    if(!onlyCheckBounds) {
     // std::cerr << "Inside bounds" << std::endl;
      for(SysInt i = 0; i < (SysInt)vararray.size(); ++i) {
        Var& var = vararray[i];
        if(!var.isBound()) {
          for(DomainInt val = var.min() + 1; val <= var.max() - 1; ++val) {
            if(checkSACTimeout())
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

  if(getOptions().gatherAMOs) {
    // Extra pass to collect the mutexes.
    // Populate listbools.
    //  Extra stuff for mutexes
    std::vector<Var> listbools;

    for(int i = 0; i < vararray.size(); i++) {
      if(vararray[i].min() == 0 && vararray[i].max() == 1) {
        listbools.push_back(vararray[i]);
      }
    }

    //  Make a 'collect events' constraint and attach it to listbools.
    CollectEvents<std::vector<Var>>* c = new CollectEvents<std::vector<Var>>(listbools);
    getState().addConstraint((AbstractConstraint*)c);

    std::vector<std::pair<int, DomainInt>>& assignments = c->assignments;

    std::vector<int> listallpairs;

    std::cout << "BOOLNAMES ";
    for(int i = 0; i < listbools.size(); i++) {
      std::cout << getNameFromVar(listbools[i]);
      if(i < listbools.size() - 1) {
        std::cout << " ";
      }
    }
    std::cout << std::endl;

    for(SysInt i = 0; i < (SysInt)listbools.size(); ++i) {
      Var& var = listbools[i];

      c->liftTriggersLessEqual(i);

      Controller::worldPush();

      var.setMax(0);
      prop(vararray);

      string vname1 = getNameFromVar(var);
      for(int j = 0; j < assignments.size(); j++) {
        listallpairs.push_back(-i - 1);
        listallpairs.push_back((assignments[j].second == 1) ? (-assignments[j].first - 1)
                                                            : (assignments[j].first + 1));
      }
      assignments.clear();

      Controller::worldPop();

      Controller::worldPush();

      var.setMin(1);
      prop(vararray);

      for(int j = 0; j < assignments.size(); j++) {
        listallpairs.push_back(i + 1);
        listallpairs.push_back((assignments[j].second == 1) ? (-assignments[j].first - 1)
                                                            : (assignments[j].first + 1));
      }
      assignments.clear();

      Controller::worldPop();
    }

    std::cout << "AMO " << listallpairs.size() / 2 << " ";
    for(int i = 0; i < listallpairs.size(); i++) {
      std::cout << listallpairs[i];
      if(i < listallpairs.size() - 1) {
        std::cout << " ";
      }
    }
    std::cout << std::endl;
  }
}

struct PropagateGAC {
  PropagationLevel level;

  PropagateGAC(PropagationLevel _level) : level(_level) {}

  template <typename Vars>
  void operator()(Vars&) {
    getQueue().propagateQueue();
  }
};

struct PropagateSAC {
  PropagationLevel level;

  PropagateSAC(PropagationLevel _level) : level(_level) {}

  template <typename Vars>
  void operator()(Vars& vars) {
    propagateSAC_internal(vars, PropagateGAC(level), false, level.limit);
  }
};

struct PropagateSAC_Bounds {
  PropagationLevel level;

  PropagateSAC_Bounds(PropagationLevel _level) : level(_level) {}

  template <typename Vars>
  void operator()(Vars& vars) {
    propagateSAC_internal(vars, PropagateGAC(level), true, level.limit);
  }
};

struct PropagateSSAC {
  PropagationLevel level;

  PropagateSSAC(PropagationLevel _level) : level(_level) {}

  template <typename Vars>
  void operator()(Vars& vars) {
    PropagateSAC sac(level);
    ;
    propagateSAC_internal(vars, sac, false, level.limit);
  }
};

struct PropagateSSAC_Bounds {
  PropagationLevel level;

  PropagateSSAC_Bounds(PropagationLevel _level) : level(_level) {}

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
  PropGAC(PropagationLevel level) : prop_obj(level) {}

  PropagateGAC prop_obj;
  inline void prop(vector<AnyVarRef>& vars) {
    prop_obj(vars);
  }
};

struct PropSAC : Propagate {
  PropSAC(PropagationLevel level) : prop_obj(level) {}

  PropagateSAC prop_obj;
  inline void prop(vector<AnyVarRef>& vars) {
    prop_obj(vars);
  }
};

struct PropSSAC : Propagate {
  PropSSAC(PropagationLevel level) : prop_obj(level) {}

  PropagateSSAC prop_obj;
  inline void prop(vector<AnyVarRef>& vars) {
    prop_obj(vars);
  }
};

struct PropSAC_Bounds : Propagate {
  PropSAC_Bounds(PropagationLevel level) : prop_obj(level) {}

  PropagateSAC_Bounds prop_obj;
  inline void prop(vector<AnyVarRef>& vars) {
    prop_obj(vars);
  }
};

struct PropSSAC_Bounds : Propagate {
  PropSSAC_Bounds(PropagationLevel level) : prop_obj(level) {}

  PropagateSSAC_Bounds prop_obj;
  inline void prop(vector<AnyVarRef>& vars) {
    prop_obj(vars);
  }
};

void PropogateCSP(PropagationLevel, vector<AnyVarRef>&, bool printInfo = false);

#endif
