/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

template<typename Var, typename Vars, typename Prop>
bool inline check_fail(Var& var, DomainInt val, Vars& vars, Prop prop, bool checkBounds)
{
  Controller::world_push();
  var.propagateAssign(val);
  prop(vars, checkBounds);
  
  bool check_failed = Controller::failed;
  Controller::failed = false;
  
  Controller::world_pop();
  
  return check_failed;
}

template <typename Var, typename Prop>
void propagateSAC_internal(vector<Var>& vararray, Prop prop, bool checkBounds)
{
  Controller::propagate_queue();
  if(Controller::failed)
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
        while(check_fail(var, var.getMax(), vararray, prop, checkBounds))
        {
          reduced = true;
          var.setMax(var.getMax() - 1);
          prop(vararray, checkBounds);
          if(Controller::failed)
            return;
        }
        
        while(check_fail(var, var.getMin(), vararray, prop, checkBounds))
        {
          reduced = true;
          var.setMin(var.getMin() + 1);
          prop(vararray, checkBounds);
          if(Controller::failed)
            return;
        }
      }
      else
      {
        for(DomainInt val = var.getMin(); val <= var.getMax(); ++val)
        {
          if(var.inDomain(val) && check_fail(var, val, vararray, prop, checkBounds))
          {
            reduced = true;
            var.removeFromDomain(val);
            prop(vararray, checkBounds);
            if(Controller::failed)
              return;          
          }
        }
      }
    }
  }
}


struct PropagateSAC
{
  template<typename Vars>
  void operator()(Vars& vars, bool checkBounds)
  {propagateSAC_internal(vars, Controller::propagate_queue_vars<Vars>, checkBounds);}
};


struct PropagateSSAC
{
  template<typename Vars>
  void operator()(Vars& vars, bool checkBounds)
  {
	PropagateSAC sac;
	propagateSAC_internal(vars, sac, checkBounds);
  }
};
