/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

template<typename Var, typename Vars, typename Prop>
bool inline check_fail(Var& var, int val, Vars& vars, Prop prop)
{
  Controller::world_push();
  var.propogateAssign(val);
  prop(vars);
  
  bool check_failed = Controller::failed;
  Controller::failed = false;
  
  Controller::world_pop();
  
  return check_failed;
}

template <typename Var, typename Prop>
void propogateSAC_internal(vector<Var>& vararray, Prop prop)
{
  Controller::propogate_queue();
  if(Controller::failed)
	return;
  
  bool reduced = true;
  while(reduced)
  {
    reduced = false;
    for(int i = 0; i < vararray.size(); ++i)
    {
      Var& var = vararray[i];
      if(var.isBound())
      {
        while(check_fail(var, var.getMax(), vararray, prop))
        {
          reduced = true;
          var.setMax(var.getMax() - 1);
          prop(vararray);
          if(Controller::failed)
            return;
        }
        
        while(check_fail(var, var.getMin(), vararray, prop))
        {
          reduced = true;
          var.setMin(var.getMin() + 1);
          prop(vararray);
          if(Controller::failed)
            return;
        }
      }
      else
      {
        for(int val = var.getMin(); val <= var.getMax(); ++val)
        {
          if(var.inDomain(val) && check_fail(var, val, vararray, prop))
          {
            reduced = true;
            var.removeFromDomain(val);
            prop(vararray);
            if(Controller::failed)
              return;          
          }
        }
      }
    }
  }
}


struct PropogateSAC
{
  template<typename Vars>
  void operator()(Vars& vars)
  {propogateSAC_internal(vars, Controller::propogate_queue_vars<Vars>);}
};


struct PropogateSSAC
{
  template<typename Vars>
  void operator()(Vars& vars)
  {
	PropogateSAC sac;
	propogateSAC_internal(vars, sac);
  }
};
