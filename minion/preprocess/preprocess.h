/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

template<typename Var>
bool inline check_fail(Var& var, int val)
{
  Controller::world_push();
  var.propogateAssign(val);
  Controller::propogate_queue();
  
  bool check_failed = Controller::failed;
  Controller::failed = false;
  
  Controller::world_pop();
  
  return check_failed;
}

template <typename Var>
void propogateSAC(vector<Var>& vararray)
{
  bool reduced = true;
  while(reduced)
  {
    reduced = false;
    for(int i = 0; i < vararray.size(); ++i)
    {
      Var& var = vararray[i];
      if(var.isBound())
      {
        while(check_fail(var,var.getMax()))
        {
          reduced = true;
          var.setMax(var.getMax() - 1);
          Controller::propogate_queue();
          if(Controller::failed)
            return;
        }
        
        while(check_fail(var,var.getMin()))
        {
          reduced = true;
          var.setMin(var.getMin() + 1);
          Controller::propogate_queue();
          if(Controller::failed)
            return;
        }
      }
      else
      {
        for(int val = var.getMin(); val <= var.getMax(); ++val)
        {
          if(var.inDomain(val) && check_fail(var, val))
          {
            reduced = true;
            var.removeFromDomain(val);
            Controller::propogate_queue();
            if(Controller::failed)
              return;          
          }
        }
      }
    }
  }
}