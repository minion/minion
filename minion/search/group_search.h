/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id$
*/

namespace Controller
{
   template<typename VarOrder, typename Variables, typename Permutation, typename Propogator>
  	inline void group_solve_loop(StateObj* stateObj, VarOrder& original_order, Variables& v, Permutation& perm, Propogator prop = PropagateGAC())
    {
      int sol_count = 0;
      for(int i = 0; i < perm.size(); ++i)
      {
        perm[i].setMin(1);
        perm[i].setMax(perm.size());
      }
      
      getQueue(stateObj).propagateQueue();
      if(getState(stateObj).isFailed())
        return;
      
      for(int i = 0; i < perm.size(); ++i)
      {
        for(int j = i + 2; j <= perm.size(); ++j)
        {
          int world_depth = get_world_depth(stateObj);
          world_push(stateObj);
          for(int k = 0; k < i; ++k)
            perm[k].propagateAssign(k+1);
          perm[i].propagateAssign(j);
          
          getQueue(stateObj).propagateQueue();
          if(!getState(stateObj).isFailed())
          {
            try
            {
              VarOrder order(original_order);
              solve_loop(stateObj, order, v, prop);
            }
            catch(EndOfSearch)
            { 
              sol_count += getState(stateObj).getSolutionCount();
              getState(stateObj).setSolutionCount(0);
              getQueue(stateObj).clearQueues();
            }
          }
          getState(stateObj).setFailed(false);
          // We need this as we can exit search deep in search.
          D_ASSERT(world_depth < get_world_depth(stateObj));
          world_pop_to_depth(stateObj, world_depth);          
        }        
      }
      
      printf("Generators: %d\n", sol_count);
    }  	  
}


  