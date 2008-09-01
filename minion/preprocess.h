/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
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

template <typename Var, typename Prop>
void propagateSAC_internal(StateObj* stateObj, vector<Var>& vararray, Prop prop, bool checkBounds)
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
      if(var.isBound() || checkBounds)
      {
        while(check_fail(stateObj, var, var.getMax(), vararray, prop))
        {
          reduced = true;
          var.setMax(var.getMax() - 1);
          prop(stateObj, vararray);
          if(getState(stateObj).isFailed())
            return;
        }
        
        while(check_fail(stateObj, var, var.getMin(), vararray, prop))
        {
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


inline PropagationLevel GetPropMethodFromString(string s)
{
  if(s == "None") return PropLevel_None;
  else if(s == "GAC") return PropLevel_GAC;
  else if(s == "SAC") return PropLevel_SAC;
  else if(s == "SSAC") return PropLevel_SSAC;
  else if(s == "SACBounds") return PropLevel_SACBounds;
  else if(s == "SSACBounds") return PropLevel_SSACBounds;
  else
    throw parse_exception(s + " is not a valid Propagation Method");
}

struct MinionArguments
{
  VarOrder order;
  enum PropagationLevel preprocess;
  enum PropagationLevel prop_method;
  unsigned random_seed;
  MinionArguments() : order(ORDER_ORIGINAL), preprocess(PropLevel_None), prop_method(PropLevel_GAC), random_seed((unsigned)time(NULL) ^ getpid())
  { }
  
};

void PropogateCSP(StateObj*, PropagationLevel, vector<AnyVarRef>&, bool print_info = false);

#endif
