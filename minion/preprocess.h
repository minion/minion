/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

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

struct PropogateGAC
{
  template<typename Vars>
  void operator()(StateObj* stateObj, Vars&)
  {getQueue(stateObj).propagateQueue();}
};

struct PropagateSAC
{
  template<typename Vars>
  void operator()(StateObj* stateObj, Vars& vars)
  {propagateSAC_internal(stateObj, vars, PropogateGAC(), false);}
};

struct PropagateSAC_Bounds
{
  template<typename Vars>
  void operator()(StateObj* stateObj, Vars& vars)
  {propagateSAC_internal(stateObj, vars, PropogateGAC(), true);}
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


enum VarOrder
{
  ORDER_STATIC,
  ORDER_SDF,
  ORDER_LDF,
  ORDER_ORIGINAL
};

struct MinionArguments
{
  enum PreProcess
  { None, GAC, SAC, SSAC, SACBounds, SSACBounds  };
  
  static PreProcess getPropMethod(string s)
  {
    if(s == "None") return None;
    else if(s == "GAC") return GAC;
    else if(s == "SAC") return SAC;
    else if(s == "SSAC") return SSAC;
    else if(s == "SACBounds") return SACBounds;
    else if(s == "SSACBounds") return SSACBounds;
    else
    {
      cerr << s << " is an invalid propagation method" << endl;
      abort();
    }
  }
  
  VarOrder order;
  enum PreProcess preprocess;
  enum PreProcess prop_method;
  unsigned random_seed;
  MinionArguments() : order(ORDER_ORIGINAL), preprocess(None), prop_method(GAC), random_seed((unsigned)time(NULL))
  { }
  
};

void preprocessCSP(StateObj*, MinionArguments::PreProcess, vector<AnyVarRef>&);
