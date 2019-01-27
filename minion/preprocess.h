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

// Copied from dump_state.cpp
template <typename T>
inline string getNameFromVar(const T& v) {
  return getState().getInstance()->vars.getName(v.getBaseVar());
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

#include "constraints/constraint_collect_events.h"

template <typename Var, typename Prop>
void propagateSAC_internal(vector<Var>& vararray, Prop prop, bool onlyCheckBounds, bool limit) {
  getQueue().propagateQueue();
  if(getState().isFailed())
    return;
  bool reduced = true;
  int loops = 0;
  
  int upperlimit=std::min(5, (int)log2(vararray.size()));
  
  while(reduced) {
    // First loop around bounds as long as possible
    while(reduced) {
      if(limit) {
        loops++;
        if(loops > upperlimit) {
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
  
  // Extra pass to collect the mutexes.
  // Populate listbools.
  //  Extra stuff for mutexes
    std::vector<Var> listbools;
  
    for(int i=0; i<vararray.size(); i++) {
      if(vararray[i].getMin()==0 && vararray[i].getMax()==1) {
          listbools.push_back(vararray[i]);
      }
    }
    
    //  Make a 'collect events' constraint and attach it to listbools.
    CollectEvents<std::vector<Var>>* c=new CollectEvents<std::vector<Var>>(listbools);
    getState().addConstraintMidsearch((AbstractConstraint*) c);
    
    std::vector<std::pair<int,int>>& assignments=c->assignments;
    
    std::vector<std::set<int>> edges;   //  mutexes: map from 2*var+val to set of same. Each mutex is inserted in both directions.
    edges.resize(listbools.size()*2);
    
    bool directOutput=true;
    
    if(directOutput) {
        std::cout << "BOOLNAMES ";
        for(int i=0; i<listbools.size(); i++) {
            std::cout << getNameFromVar(listbools[i]);
            if(i<listbools.size()-1) {
                std::cout << " ";
            }
        }
        std::cout << std::endl;
    }
    
    for(SysInt i = 0; i < (SysInt)listbools.size(); ++i) {
        Var& var = listbools[i];
        
        c->liftTriggersLessEqual(i);
        
        Controller::world_push();
        
        var.setMax(0);
        prop(vararray);
        
        string vname1=getNameFromVar(var);
        for(int j=0; j<assignments.size(); j++) {
            if(directOutput) {
                //std::cout << "AMO " << vname1 << " " << 0 << " " << getNameFromVar(listbools[assignments[j].first]) << " " << (1-assignments[j].second) << std::endl;
                std::cout << "AMO " << (-i-1) << " " << ( (assignments[j].second==1)?(-assignments[j].first-1):(assignments[j].first+1) ) << std::endl;
            }
            else {
                edges[2*i+0].insert(2*assignments[j].first+(1-assignments[j].second));
                edges[2*assignments[j].first+(1-assignments[j].second)].insert(2*i+0);
            }
        }
        assignments.clear();
        
        bool check_failed = getState().isFailed();
        getState().setFailed(false);
        
        Controller::world_pop();
        
        Controller::world_push();
        
        var.setMin(1);
        prop(vararray);
        
        for(int j=0; j<assignments.size(); j++) {
            if(directOutput) {
                //std::cout << "AMO " << vname1 << " " << 1 << " " << getNameFromVar(listbools[assignments[j].first]) << " " << (1-assignments[j].second) << std::endl;
                std::cout << "AMO " << i+1 << " " << ( (assignments[j].second==1)?(-assignments[j].first-1):(assignments[j].first+1) ) << std::endl;
            }
            else {
                edges[2*i+1].insert(2*assignments[j].first+(1-assignments[j].second));
                edges[2*assignments[j].first+(1-assignments[j].second)].insert(2*i+1);
            }
        }
        assignments.clear();
        
        check_failed = getState().isFailed();
        getState().setFailed(false);
        
        Controller::world_pop();
    }
    
    if(directOutput) {
        return;
    }
    
    //  Pack into clique cover for the transfer to SR. 
    //  Clique cover must include each edge at least once. 
    //  marks edges as used once they are in some clique, but the same edge can be used again. 
    std::set<std::pair<int,int>> edgesUsed;
    
    for(int idx1 = 0; idx1 < (int)listbools.size()*2; idx1++) {
        // Iterate through all edges involving idx1. 
        std::set<int>& a=edges[idx1];
        
        for(auto it=a.begin(); it!=a.end(); ++it) {
            int idx2=*it;
            
            if( edgesUsed.find(make_pair(idx1, idx2))==edgesUsed.end()) {
                // Edge idx1, idx2 is the starting point of the clique
                
                std::vector<int> clique;
                clique.push_back(idx1);
                clique.push_back(idx2);
                
                //  Go through rest of indices adjacent to idx1. 
                for(auto it2=std::next(it); it2!=a.end(); ++it2) {
                    int idx3=*it2;
                    
                    bool newedgeseen=false;    //  Only add idx3 if it allows us to include a previously unseen edge into the clique.
                    bool connected=true;
                    for(int i=0; i<clique.size(); i++) {
                        if(edges[clique[i]].find(idx3)!=edges[clique[i]].end()) {
                            if( edgesUsed.find(make_pair(clique[i], idx3))==edgesUsed.end()) {
                                newedgeseen=true;
                            }
                        }
                        else {
                            connected=false;
                            break;
                        }
                    }
                    
                    if(connected && newedgeseen) {
                        clique.push_back(idx3);
                    }
                }
                
                //  Add all edges covered by this clique into edgesUsed.
                for(int i=0; i<clique.size(); i++) {
                    for(int j=i+1; j<clique.size(); j++) {
                        edgesUsed.insert(make_pair(clique[i], clique[j]));
                        edgesUsed.insert(make_pair(clique[j], clique[i]));
                    }
                }
                
                //  Dump it.
                std::cout << "AMO ";
                for(int i=0; i<clique.size(); i++) {
                    std::cout << clique[i];
                    if(i<clique.size()-1) {
                        std::cout << " ";
                    }
                }
                std::cout << std::endl;
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
