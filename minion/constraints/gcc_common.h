#include <stdlib.h>
#include <iostream>
#include <vector>
#include <deque>
#include <algorithm>
#include <utility>
#include "alldiff_gcc_shared.h"

//#define GCCPRINT(x) cout << x << endl
#define GCCPRINT(x)

#define SPECIALQUEUE
#define SCC
#define INCREMENTALMATCH

#define SCCCARDS
#define STRONGCARDS

//Incremental graph -- maintains adjacency lists for values
#define INCGRAPH

// Does not trigger itself if this is on, and incgraph is on.
#define ONECALL

//#define CAPBOUNDSCACHE

// Note on semantics: GCC only restricts those values which are 'of interest',
// it does not put any restriction on the number of other values. 

template<typename VarArray1, typename VarArray2>
struct GCC : public AbstractConstraint
{
    GCC(StateObj* _stateObj, const VarArray1& _var_array, const VarArray2& _capacity_array, ConstraintBlob& b) : AbstractConstraint(_stateObj),
    var_array(_var_array), capacity_array(_capacity_array), 
    numvals(count_values()), constraint_locked(false),
    SCCSplit(_stateObj, numvars+numvals)
    {
        GCCPRINT("numvars:"<< numvars << ", numvals:"<< numvals);
        val_array = b.constants[0];
        
        D_ASSERT(capacity_array.size()==val_array.size());
        
        for(int i=0; i<val_array.size(); i++)
        {
            for(int j=i+1; j<val_array.size(); j++)
            {
                D_ASSERT(val_array[i]!=val_array[j]);
            }
        }
        varvalmatching.resize(numvars, dom_min-1);
        usage.resize(numvals, 0);
        
        lower.resize(numvals, 0);
        upper.resize(numvals, numvars);
        
        prev.resize(numvars+numvals);
        initialize_tarjan();
        
        // SCC data structures
        SCCs.resize(numvars+numvals);
        varToSCCIndex.resize(numvars+numvals);
        for(int i=0; i<numvars+numvals; i++)
        {
            SCCs[i]=i;
            varToSCCIndex[i]=i;
        }
        
        vars_in_scc.reserve(numvars);
        vals_in_scc.reserve(numvals);
        // In case we are not using SCCs, fill the var and val arrays.
        vars_in_scc.clear();
        for(int i=0; i<numvars; i++)
        {
            vars_in_scc.push_back(i);
        }
        vals_in_scc.clear();
        for(int i=dom_min; i<=dom_max; i++)
        {
            vals_in_scc.push_back(i);
        }
        
        to_process.reserve(numvars+numvals);
        sccs_to_process.reserve(numvars+numvals);
        
        // Array to let us find the appropriate capacity variable for a value. 
        val_to_cap_index.resize(numvals);
        for(int i=0; i<numvals; i++)
        {
            bool found=false;
            int j;
            for(j=0; j<val_array.size(); j++)
            {
                if(val_array[j]==i+dom_min)
                {
                    found=true; 
                    break;
                }
            }
            
            if(!found)
            {
                val_to_cap_index[i]=-1;
            }
            else
            {
                val_to_cap_index[i]=j;
            }
        }
        GCCPRINT("val_to_cap_index:" << val_to_cap_index);
        
        augpath.reserve(numvars+numvals+1);
        //fifo.reserve(numvars+numvals);
        
        #ifdef INCGRAPH
            // refactor this to use initial upper and lower bounds.
            adjlist.resize(numvars+numvals);
            adjlistpos.resize(numvars+numvals);
            for(int i=0; i<numvars; i++)
            {
                adjlist[i].resize(numvals);
                for(int j=0; j<numvals; j++) adjlist[i][j]=j+dom_min;
                adjlistpos[i].resize(numvals);
                for(int j=0; j<numvals; j++) adjlistpos[i][j]=j;
            }
            for(int i=numvars; i<numvars+numvals; i++)
            {
                adjlist[i].resize(numvars);
                for(int j=0; j<numvars; j++) adjlist[i][j]=j;
                adjlistpos[i].resize(numvars);
                for(int j=0; j<numvars; j++) adjlistpos[i][j]=j;
            }
            adjlistlength=getMemory(stateObj).backTrack().template requestArray<short>(numvars+numvals);
            for(int i=0; i<numvars; i++) adjlistlength[i]=numvals;
            for(int i=numvars; i<numvars+numvals; i++) adjlistlength[i]=numvars;
        #endif
        
        #ifdef CAPBOUNDSCACHE
        boundsupported.resize(numvals*2, -1);  
        // does the bound need to be updated? Indexed as validx*2 for lowerbound, validx*2+1 for ub
        // Contains the capacity value which is supported. Reset to -1 if the support is lost.
        #endif
    }
    
    VarArray1 var_array;   // primary variables
    VarArray2 capacity_array;   // capacities for values of interest
    vector<int> val_array;   // values of interest
    int dom_min, dom_max, numvars, numvals;
    
    #ifdef INCGRAPH
    vector<vector<int> > adjlist;
    MoveableArray<short> adjlistlength;
    vector<vector<int> > adjlistpos;   // position of a variable in adjlist.
    #endif
    
    int count_values()
    {
        // called in initializer list.
        dom_min=var_array[0].getInitialMin();
        dom_max=var_array[0].getInitialMax();
        
        for(int i=0; i<var_array.size(); ++i)
        {
          if(var_array[i].getInitialMin()<dom_min)
              dom_min=var_array[i].getInitialMin();
          if(var_array[i].getInitialMax()>dom_max)
              dom_max=var_array[i].getInitialMax();
        }
        numvars=var_array.size();  // number of target variables in the constraint
        return dom_max-dom_min+1;
    }
    
    vector<int> val_to_cap_index;
    
    bool constraint_locked;
    
    vector<int> vars_in_scc;
    vector<int> vals_in_scc;  // Actual values.
    
    virtual void full_propagate()
    {
        for(int i=0; i<capacity_array.size(); i++)
        {
            if(val_array[i]>=dom_min && val_array[i]<=dom_max)
            {
                capacity_array[i].setMin(0);
                capacity_array[i].setMax(numvars);
            }
            else
            {   // value can't occur.
                capacity_array[i].propagateAssign(0);
            }
        }
        #ifdef INCGRAPH
        {
            // update the adjacency lists. and place dts
            DynamicTrigger* dt=dynamic_trigger_start();
            for(int i=dom_min; i<=dom_max; i++)
            {
                for(int j=0; j<adjlistlength[i-dom_min+numvars]; j++)
                {
                    int var=adjlist[i-dom_min+numvars][j];
                    if(!var_array[var].inDomain(i))
                    {
                        if(varvalmatching[var]==i)
                        {
                            usage[varvalmatching[var]-dom_min]--;
                            varvalmatching[var]=dom_min-1;
                        }
                        // swap with the last element and remove
                        adjlist_remove(var, i);
                        j--; // stay in the same place, dont' skip over the 
                        // value which was just swapped into the current position.
                    }
                    else
                    {
                        // arranged in blocks for each variable, with numvals triggers in each block
                        DynamicTrigger* mydt= dt+(var*numvals)+(i-dom_min);
                        var_array[var].addDynamicTrigger(mydt, DomainRemoval, i);
                    }
                }
            }
        }
        #endif
        
        #ifdef CAPBOUNDSCACHE
            DynamicTrigger* dt=dynamic_trigger_start();
            dt+=(numvars*numvals);
            for(int i=0; i<val_array.size(); i++)
            {
                // lowerbound first
                for(int j=0; j<(val_array.size()+numvars); j++)
                {
                    dt->trigger_info()=(val_array[i]-dom_min)*2;
                    dt++;
                }
                // upperbound
                for(int j=0; j<(val_array.size()+numvars); j++)
                {
                    dt->trigger_info()=(val_array[i]-dom_min)*2+1;
                    dt++;
                }
            }
        #endif
        
        #ifdef SCC
        for(int i=0; i<numvars+numvals; i++) to_process.insert(i);  // may need to change.
        do_gcc_prop_scc();
        #else
        do_gcc_prop();
        #endif
    }
    
    #ifdef INCGRAPH
    inline void adjlist_remove(int var, int val)
    {
        // swap item at position varidx to the end, then reduce the length by 1.
        int validx=val-dom_min+numvars;
        int varidx=adjlistpos[validx][var];
        D_ASSERT(varidx<adjlistlength[validx]);  // var is actually in the list.
        delfromlist(validx, varidx);
        
        delfromlist(var, adjlistpos[var][val-dom_min]);
    }
    
    inline void delfromlist(int i, int j)
    {
        // delete item in list i at position j
        int t=adjlist[i][adjlistlength[i]-1];
        adjlist[i][adjlistlength[i]-1]=adjlist[i][j];
        
        if(i<numvars)
        {
            adjlistpos[i][adjlist[i][j]-dom_min]=adjlistlength[i]-1;
            adjlistpos[i][t-dom_min]=j;
        }
        else
        {
            adjlistpos[i][adjlist[i][j]]=adjlistlength[i]-1;
            adjlistpos[i][t]=j;
        }
        adjlist[i][j]=t;
        adjlistlength[i]=adjlistlength[i]-1;
    }
    #endif
    
    // convert constraint into dynamic. 
    int dynamic_trigger_count()
    {
        #if defined(INCGRAPH) && !defined(CAPBOUNDSCACHE)
            return numvars*numvals; // one for each var-val pair so we know when it is removed.
        #endif
        
        #if !defined(INCGRAPH) && !defined(CAPBOUNDSCACHE)
            return 0;
        #endif
        
        #ifdef CAPBOUNDSCACHE
            // first numvars*numvals triggers are not used when INCGRAPH is not defined.
            // one block of numvars+val_Array.size() for each bound. 
            return numvars*numvals + 2*val_array.size()*(numvars+val_array.size());
        #endif
    }
    
    PROPAGATE_FUNCTION(int prop_var, DomainDelta)
    {
        if(!to_process.in(prop_var))
        {
            to_process.insert(prop_var);  // inserts the number attached to the trigger. For values this is val-dom_min+numvars
        }
        
        if(!constraint_locked)
        {
            #ifdef SPECIALQUEUE
            constraint_locked = true;
            getQueue(stateObj).pushSpecialTrigger(this);
            #else
            #ifdef SCC
            do_gcc_prop_scc();
            #else
            do_gcc_prop();
            #endif
            #endif
        }
    }
    
    PROPAGATE_FUNCTION(DynamicTrigger* trig)
    {
        DynamicTrigger* dtstart=dynamic_trigger_start();
        
        #ifdef CAPBOUNDSCACHE
        if(trig< dtstart+(numvars*numvals))
        #endif
        {
            // which var/val is this trigger attached to?
            #ifdef INCGRAPH
            int diff=trig-dtstart;
            int var=diff/numvals;
            int validx=diff%numvals;
            if(adjlistpos[validx+numvars][var]<adjlistlength[validx+numvars])
            {
                adjlist_remove(var, validx+dom_min); //validx, adjlistpos[validx][var]);
                if(varvalmatching[var]==validx+dom_min) // remove invalid value in the matching.
                {
                    varvalmatching[var]=dom_min-1;
                    usage[validx]--;
                }
                // trigger the constraint here
                #ifdef ONECALL
                if(!to_process.in(var))
                {
                    to_process.insert(var);  // add the var to the queue to be processed.
                }
                if(!constraint_locked)
                {
                    #ifdef SPECIALQUEUE
                    constraint_locked = true;
                    getQueue(stateObj).pushSpecialTrigger(this);
                    #else
                    #ifdef SCC
                    do_gcc_prop_scc();
                    #else
                    do_gcc_prop();
                    #endif
                    #endif
                }
                #endif
            }
            // else the constraint triggered itself.
            #endif
        }
        #ifdef CAPBOUNDSCACHE
        else
        {
            //dtstart=dtstart+(numvars*numvals);
            D_ASSERT(trig>= dtstart && trig<dtstart+(2*val_array.size()*(numvars+val_array.size())) );
            // arranged in blocks per value. Then the first half of the block is for lower bound.
            /*int diff=trig-dtstart;
            int val_arrayidx=diff/((val_array.size()+numvars)*2);
            int value=val_array[val_arrayidx];
            int lbub=(diff/((val_array.size()+numvars)))%2;  // lowerbound or upperbound. SIMPLIFY HERE.
            boundsupported[(value-dom_min)*2+lbub]=-1;*/
            boundsupported[trig->trigger_info()]=-1;
        }
        #endif
    }
    
    #ifdef CAPBOUNDSCACHE
    vector<int> boundsupported;  // does the bound need to be updated? Indexed as validx*2 for lowerbound, validx*2+1 for ub
    // Contains the capacity value which is supported. Reset to -1 if the support is lost.
    #endif
    
    virtual void special_unlock() { constraint_locked = false;  } // to_process.clear(); why commented out?
  virtual void special_check()
  {
    constraint_locked = false;  // should be above the if.
    
    if(getState(stateObj).isFailed())
    {
        to_process.clear();
        return;
    }
    #ifdef SCC
    do_gcc_prop_scc();
    #else
    do_gcc_prop();
    #endif
    
  }
  
  virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  {
      D_ASSERT(dom_max-dom_min+1 == numvals);
      // Check if the matching is OK.
      bool matchok=true;
      for(int i=0; i<numvars; i++)
      {
          if(!var_array[i].inDomain(varvalmatching[i]))
          {
              matchok=false;
              break;
          }
      }
      if(matchok)
      {
          // now check occurrences
          for(int i=0; i<val_array.size(); i++)
          {
              int val=val_array[i];
              if( (val<dom_min || val>dom_max) && !capacity_array[i].inDomain(0))
              {
                  matchok=false;
                  break;
              }
              if( val>=dom_min && val<=dom_max && !capacity_array[i].inDomain(usage[val-dom_min]))  // is usage OK??
              {
                  matchok=false;
                  break;
              }
          }
      }
      
      if(!matchok)
      {
          // run matching algorithm
          // populate lower and upper
          // Also check if bounds are well formed.
        for(int i=0; i<val_array.size(); i++)
        {
            if(val_array[i]>=dom_min && val_array[i]<=dom_max)
            {
                if(capacity_array[i].getMax()<0 || capacity_array[i].getMin()>numvars)
                {
                    return false;
                }
                lower[val_array[i]-dom_min]=capacity_array[i].getMin();
                upper[val_array[i]-dom_min]=capacity_array[i].getMax();
            }
            else
            {
                if(capacity_array[i].getMin()>0 || capacity_array[i].getMax()<0)
                {
                    return false;
                }
            }
        }
        
        #ifdef INCGRAPH
            // update the adjacency lists.
            for(int i=dom_min; i<=dom_max; i++)
            {
                for(int j=0; j<adjlistlength[i-dom_min+numvars]; j++)
                {
                    int var=adjlist[i-dom_min+numvars][j];
                    if(!var_array[var].inDomain(i))
                    {
                        if(varvalmatching[var]==i)
                        {
                            usage[varvalmatching[var]-dom_min]--;
                            varvalmatching[var]=dom_min-1;
                        }
                        // swap with the last element and remove
                        adjlist_remove(var, i);
                        j--; // stay in the same place, dont' skip over the 
                        // value which was just swapped into the current position.
                    }
                }
            }
        #endif
        
        // I'm sure that these four lines are needed, even though
        // if they are taken out, it still passes the random tests.
        // They are needed because the two vectors may be stale after
        // backtracking.
        vars_in_scc.clear();
        for(int i=0; i<numvars; i++) vars_in_scc.push_back(i);
        vals_in_scc.clear();
        for(int i=dom_min; i<=dom_max; i++) vals_in_scc.push_back(i);
        
        matchok=bfsmatching_gcc();
      }
      
      if(!matchok)
      {
          return false;
      }
      else
      {
          for(int i=0; i<numvars; i++)
          {
              D_ASSERT(var_array[i].inDomain(varvalmatching[i]));
              assignment.push_back(make_pair(i, varvalmatching[i]));
          }
          for(int i=0; i<val_array.size(); i++)
          {
              int occ;
              if(val_array[i]<dom_min || val_array[i]>dom_max)
              {
                  occ=0;
              }
              else
              {
                  occ=usage[val_array[i]-dom_min];
              }
              
             if(capacity_array[i].inDomain(occ))
             {
                 assignment.push_back(make_pair(i+numvars, occ));
             }
             else
             {
                 // push upper and lower bounds.
                 assignment.push_back(make_pair(i+numvars, capacity_array[i].getMin()));
                 assignment.push_back(make_pair(i+numvars, capacity_array[i].getMax()));                 
             }
          }
          return true;
      }
  }
  
    void do_gcc_prop()
    {
        // find/ repair the matching.
        #ifndef INCREMENTALMATCH
        varvalmatching.resize(0);
        varvalmatching.resize(numvars, dom_min-1);
        #endif
        
        // populate lower and upper
        for(int i=0; i<val_array.size(); i++)
        {
            if(val_array[i]>=dom_min && val_array[i]<=dom_max)
            {
                lower[val_array[i]-dom_min]=capacity_array[i].getMin();   // not quite right in the presence of duplicate values.
                upper[val_array[i]-dom_min]=capacity_array[i].getMax();
            }
        }
        
        GCCPRINT("lower:"<<lower);
        GCCPRINT("upper:"<<upper);
        
        bool flag=bfsmatching_gcc();
        GCCPRINT("matching:"<<flag);
        
        if(!flag)
        {
            getState(stateObj).setFailed(true);
            return;
        }
        
        tarjan_recursive(0);
        
        prop_capacity();
        
    }
    
    smallset sccs_to_process;
    
    void do_gcc_prop_scc()
    {
        // Assumes triggered on variables in to_process
        #ifndef INCREMENTALMATCH
        varvalmatching.resize(0);
        varvalmatching.resize(numvars, dom_min-1);
        #endif
        
        sccs_to_process.clear(); 
        {
        vector<int>& toiterate = to_process.getlist();
        GCCPRINT("About to loop for to_process.");
        
        // to_process contains var indexes and vals:
        // (val-dom_min +numvars)
        
        for(int i=0; i<toiterate.size(); ++i)
        {
            int tempidx=toiterate[i];
            
            int sccindex_start=varToSCCIndex[tempidx];
            
            while(sccindex_start>0 && SCCSplit.isMember(sccindex_start-1))
            {
                sccindex_start--;   // seek the first item in the SCC.
            }
            
            if(!sccs_to_process.in(sccindex_start) 
                && SCCSplit.isMember(sccindex_start))   // not singleton.
            {
                sccs_to_process.insert(sccindex_start);
            }
        }
        }
        
        {
        vector<int>& toiterate = sccs_to_process.getlist();
        GCCPRINT("About to loop for sccs_to_process:"<< toiterate);
        for(int i=0; i<toiterate.size(); i++)
        {
            int sccindex_start=toiterate[i];
            vars_in_scc.clear();
            vals_in_scc.clear();
            for(int j=sccindex_start; j<(numvars+numvals); j++)
            {
                // copy vars and vals in the scc into two vectors.
                int sccval=SCCs[j];
                if(sccval<numvars)
                {
                    D_ASSERT(sccval>=0);
                    vars_in_scc.push_back(sccval);
                }
                else
                {
                    D_ASSERT(sccval<numvars+numvals);
                    vals_in_scc.push_back(sccval-numvars+dom_min);
                }
                
                if(!SCCSplit.isMember(j)) break;
            }
            GCCPRINT("vars_in_scc:" << vars_in_scc);
            GCCPRINT("vals_in_scc:" << vals_in_scc);
            
            // Might not need to do anything.
            if(vars_in_scc.size()==0)
            {
                GCCPRINT("refusing to process empty scc.");
                continue;
            }
            
            // populate lower and upper
            for(int i=0; i<vals_in_scc.size(); i++)
            {
                int validx=vals_in_scc[i]-dom_min;
                int capi=val_to_cap_index[validx];
                if(capi>-1)
                {
                    lower[validx]=capacity_array[capi].getMin();
                    upper[validx]=capacity_array[capi].getMax();
                }
            }
            
            bool flag=bfsmatching_gcc();
            if(!flag)
            {
                GCCPRINT("Failing because no matching");
                getState(stateObj).setFailed(true);
                return;
            }
            
            tarjan_recursive(sccindex_start);
            
            #if defined(SCCCARDS) && defined(STRONGCARDS)
                // Propagate to capacity variables for all values in vals_in_scc
                for(int valinscc=0; valinscc<vals_in_scc.size(); valinscc++)
                {
                    int v=vals_in_scc[valinscc];
                    if(val_to_cap_index[v-dom_min]!=-1 && lower[v-dom_min]!=upper[v-dom_min])
                    {
                        prop_capacity_strong_scc(v);
                    }
                }
            #endif
            
        }
        }
        
        #if !defined(SCCCARDS) || !defined(STRONGCARDS)
            prop_capacity();
        #endif
        
        // temporary to test without strong upperbound pruning.
        #if defined(SCCCARDS) && defined(STRONGCARDS)
            prop_capacity_simple();
        #endif
    }
    
    deque<int> fifo;
    // deque_fixed_size was not faster.
    //deque_fixed_size fifo;
    vector<int> prev;
    
    vector<int> matchbac;
    
    vector<int> lower;
    vector<int> upper;
    vector<int> usage;
    vector<int> usagebac;
    vector<int> varvalmatching;
    
    // Incremental SCC data.
    vector<int> SCCs;    // Variable numbers and values as val-dom_min+numvars
    ReversibleMonotonicSet SCCSplit;
    
    vector<int> varToSCCIndex;  // Mirror of the SCCs array.
    
    smallset to_process;
    
    inline bool bfsmatching_gcc()
    {
        // lower and upper are indexed by value-dom_min and provide the capacities.
        // usage is the number of times a value is used in the matching.
        
        // current sccs are contained in vars_in_scc and vals_in_scc
        
        // back up the matching to cover failure
        //matchbac=varvalmatching;
        //usagebac=usage;
        
        // clear out unmatched variables -- unless this has already been done 
        // when the adjacency lists were updated.
        #ifndef INCGRAPH
        for(int scci=0; scci<vars_in_scc.size(); scci++)
        {
            int i=vars_in_scc[scci];
            if(varvalmatching[i]!=dom_min-1
                && !var_array[i].inDomain(varvalmatching[i]))
            {
                usage[varvalmatching[i]-dom_min]--;
                varvalmatching[i]=dom_min-1;   // marker for unmatched.
            }
        }
		#endif
        
        // If the upper bounds have been changed since last call, it is possible that
        // the usage[val] of some value is greater than upper[val]. This is impossible
        // in the flow graph, so it must be corrected before we run the algorithm.
        // Some values in the matching are changed to blank (dom_min-1).
        for(int valsccindex=0; valsccindex<vals_in_scc.size(); valsccindex++)
        {
            int valindex=vals_in_scc[valsccindex]-dom_min;
            if(usage[valindex]>upper[valindex] && upper[valindex]>=0)
            {
                for(int i=0; i<vars_in_scc.size() && usage[valindex]>upper[valindex]; i++)
                {
                    int j=vars_in_scc[i];
                    if(varvalmatching[j]==valindex+dom_min)
                    {
                        varvalmatching[j]=dom_min-1;
                        usage[valindex]--;
                    }
                }
                D_ASSERT(usage[valindex]==upper[valindex]);
            }
        }
        
        // iterate through the values looking for ones which are below their lower capacity bound. 
        for(int startvalsccindex=0; startvalsccindex<vals_in_scc.size(); startvalsccindex++)
        {
            int startvalindex=vals_in_scc[startvalsccindex]-dom_min;
            while(usage[startvalindex]<lower[startvalindex])
            {
                // usage of val needs to increase. Construct an augmenting path starting at val.
                GCCPRINT("Searching for augmenting path for val: " << startvalindex+dom_min);
                // Matching edge lost; BFS search for augmenting path to fix it.
                fifo.clear();  // this should be constant time but probably is not.
                fifo.push_back(startvalindex+numvars);
                visited.clear();
                visited.insert(startvalindex+numvars);
                bool finished=false;
                while(!fifo.empty() && !finished)
                {
                    // pop a vertex and expand it.
                    int curnode=fifo.front();
                    fifo.pop_front();
                    GCCPRINT("Popped vertex " << (curnode<numvars? "(var)":"(val)") << (curnode<numvars? curnode : curnode+dom_min-numvars ));
                    if(curnode<numvars)
                    { // it's a variable
                        // follow the matching edge, if there is one.
                        int valtoqueue=varvalmatching[curnode];
                        if(valtoqueue!=dom_min-1 
                            && !visited.in(valtoqueue-dom_min+numvars))
                        {
                            D_ASSERT(var_array[curnode].inDomain(valtoqueue));
                            int validx=valtoqueue-dom_min+numvars;
                            if(usage[valtoqueue-dom_min]>lower[valtoqueue-dom_min])
                            {
                                // can reduce the flow of valtoqueue to increase startval.
                                prev[validx]=curnode;
                                apply_augmenting_path(validx, startvalindex+numvars);
                                finished=true;
                            }
                            else
                            {
                                visited.insert(validx);
                                prev[validx]=curnode;
                                fifo.push_back(validx);
                            }
                        }
                    }
                    else
                    { // popped a value from the stack.
                        D_ASSERT(curnode>=numvars && curnode < numvars+numvals);
                        int stackval=curnode+dom_min-numvars;
                        #ifndef INCGRAPH
                        for(int vartoqueuescc=0; vartoqueuescc<vars_in_scc.size(); vartoqueuescc++)
                        {
                            int vartoqueue=vars_in_scc[vartoqueuescc];
                        #else
                        for(int vartoqueuei=0; vartoqueuei<adjlistlength[stackval-dom_min+numvars]; vartoqueuei++)
                        {
                            int vartoqueue=adjlist[stackval-dom_min+numvars][vartoqueuei];
                        #endif
                            // For each variable, check if it terminates an odd alternating path
                            // and also queue it if it is suitable.
                            if(!visited.in(vartoqueue)
                                #ifndef INCGRAPH
                                && var_array[vartoqueue].inDomain(stackval)
                                #endif
                                && varvalmatching[vartoqueue]!=stackval)   // Need to exclude the matching edges????
                            {
                                // there is an edge from stackval to vartoqueue.
                                if(varvalmatching[vartoqueue]==dom_min-1)
                                {
                                    // vartoqueue terminates an odd alternating path.
                                    // Unwind and apply the path here
                                    prev[vartoqueue]=curnode;
                                    apply_augmenting_path(vartoqueue, startvalindex+numvars);
                                    finished=true;
                                    break;  // get out of for loop
                                }
                                else
                                {
                                    // queue vartoqueue
                                    visited.insert(vartoqueue);
                                    prev[vartoqueue]=curnode;
                                    fifo.push_back(vartoqueue);
                                }
                            }
                        }  // end for.
                    }  // end value
                }  // end while
                if(!finished)
                {   // no augmenting path found
                    GCCPRINT("No augmenting path found.");
                    // restore the matching to its state before the algo was called.
                    //varvalmatching=matchbac;
                    //usage=usagebac;
                    return false;
                }
                
            }  // end while below lower bound.
        } // end for each value
        
        // now search for augmenting paths for unmatched vars.
        
        GCCPRINT("feasible matching (respects lower & upper bounds):"<<varvalmatching);
        
        // Flip the graph around, so it's like the alldiff case now. 
        // follow an edge in the matching from a value to a variable,
        // follow edges not in the matching from variables to values. 
        
        for(int startvarscc=0; startvarscc<vars_in_scc.size(); startvarscc++)
        {
            int startvar=vars_in_scc[startvarscc];
            if(varvalmatching[startvar]==dom_min-1)
            {
                GCCPRINT("Searching for augmenting path for var: " << startvar);
                fifo.clear();  // this should be constant time but probably is not.
                fifo.push_back(startvar);
                visited.clear();
                visited.insert(startvar);
                bool finished=false;
                while(!fifo.empty() && !finished)
                {
                    // pop a vertex and expand it.
                    int curnode=fifo.front();
                    fifo.pop_front();
                    GCCPRINT("Popped vertex " << (curnode<numvars? "(var)":"(val)") << (curnode<numvars? curnode : curnode+dom_min-numvars ));
                    if(curnode<numvars)
                    { // it's a variable
                        // follow all edges other than the matching edge. 
                        #ifndef INCGRAPH
                        for(int valtoqueue=var_array[curnode].getMin(); valtoqueue<=var_array[curnode].getMax(); valtoqueue++)
                        {
                        #else
                        for(int valtoqueuei=0; valtoqueuei<adjlistlength[curnode]; valtoqueuei++)
                        {
                            int valtoqueue=adjlist[curnode][valtoqueuei];
                        #endif
                            // For each value, check if it terminates an odd alternating path
                            // and also queue it if it is suitable.
                            int validx=valtoqueue-dom_min+numvars;
                            if(valtoqueue!=varvalmatching[curnode]
                            #ifndef INCGRAPH
                                && var_array[curnode].inDomain(valtoqueue)
                            #endif
                                && !visited.in(validx) )
                            {
                                //D_ASSERT(find(vals_in_scc.begin(), vals_in_scc.end(), valtoqueue)!=vals_in_scc.end()); // the value is in the scc.
                                // Does this terminate an augmenting path?
                                if(usage[valtoqueue-dom_min]<upper[valtoqueue-dom_min])
                                {
                                    // valtoqueue terminates an alternating path.
                                    // Unwind and apply the path here
                                    prev[validx]=curnode;
                                    apply_augmenting_path_reverse(validx, startvar);
                                    finished=true;
                                    break;  // get out of for loop
                                }
                                else
                                {
                                    // queue valtoqueue
                                    visited.insert(validx);
                                    prev[validx]=curnode;
                                    fifo.push_back(validx);
                                }
                            }
                        }  // end for.
                    }
                    else
                    { // popped a value from the stack.
                        D_ASSERT(curnode>=numvars && curnode < numvars+numvals);
                        int stackval=curnode+dom_min-numvars;
                        #ifndef INCGRAPH
                        for(int vartoqueuescc=0; vartoqueuescc<vars_in_scc.size(); vartoqueuescc++)
                        {
                            int vartoqueue=vars_in_scc[vartoqueuescc];
                        #else
                        for(int vartoqueuei=0; vartoqueuei<adjlistlength[stackval-dom_min+numvars]; vartoqueuei++)
                        {
                            int vartoqueue=adjlist[stackval-dom_min+numvars][vartoqueuei];
                        #endif
                            // For each variable which is matched to stackval, queue it.
                            if(!visited.in(vartoqueue)
                                && varvalmatching[vartoqueue]==stackval)
                            {
                                D_ASSERT(var_array[vartoqueue].inDomain(stackval));
                                // there is an edge from stackval to vartoqueue.
                                // queue vartoqueue
                                visited.insert(vartoqueue);
                                prev[vartoqueue]=curnode;
                                fifo.push_back(vartoqueue);
                            }
                        }  // end for.
                    }  // end value
                }  // end while
                if(!finished)
                {   // no augmenting path found
                    GCCPRINT("No augmenting path found.");
                    // restore the matching to its state before the algo was called.
                    //varvalmatching=matchbac;   // no need for this.
                    //usage=usagebac;
                    return false;
                }
            }
        }
        
        GCCPRINT("maximum matching:" << varvalmatching);
        return true;
    }
    
    vector<int> augpath;
    
    inline void apply_augmenting_path(int unwindnode, int startnode)
    {
        augpath.clear();
        // starting at unwindnode, unwind the path and put it in augpath.
        // Then apply it.
        // Assumes prev contains vertex numbers, rather than vars and values.
        int curnode=unwindnode;
        while(curnode!=startnode)
        {
            augpath.push_back(curnode);
            curnode=prev[curnode];
        }
        augpath.push_back(curnode);
        
        std::reverse(augpath.begin(), augpath.end());
        
        GCCPRINT("Found augmenting path:" << augpath);
        
        // now apply the path.
        for(int i=0; i<augpath.size()-1; i++)
        {
            if(augpath[i]<numvars)
            {
                // if it's a variable
                //D_ASSERT(varvalmatching[augpath[i]]==dom_min-1);
                // varvalmatching[augpath[i]]=dom_min-1; Can't do this, it would overwrite the correct value.
                GCCPRINT("decrementing usage for value " << augpath[i+1]-numvars+dom_min);
                usage[augpath[i+1]-numvars]--;
            }
            else
            {   // it's a value.
                D_ASSERT(augpath[i]>=numvars && augpath[i]<numvars+numvals);
                varvalmatching[augpath[i+1]]=augpath[i]-numvars+dom_min;
                GCCPRINT("incrementing usage for value " << augpath[i]-numvars+dom_min);
                usage[augpath[i]-numvars]++;
            }
        }
        
        GCCPRINT("varvalmatching: "<<varvalmatching);
    }
    
    inline void apply_augmenting_path_reverse(int unwindnode, int startnode)
    {
        augpath.clear();
        // starting at unwindnode, unwind the path and put it in augpath.
        // Then apply it.
        // Assumes prev contains vertex numbers, rather than vars and values.
        int curnode=unwindnode;
        while(curnode!=startnode)
        {
            augpath.push_back(curnode);
            curnode=prev[curnode];
        }
        augpath.push_back(curnode);
        
        std::reverse(augpath.begin(), augpath.end());
        GCCPRINT("Found augmenting path:" << augpath);
        
        // now apply the path.
        for(int i=0; i<augpath.size()-1; i++)
        {
            if(augpath[i]<numvars)
            {
                // if it's a variable
                D_ASSERT(varvalmatching[augpath[i]]==dom_min-1);
                varvalmatching[augpath[i]]=augpath[i+1]-numvars+dom_min;
                usage[augpath[i+1]-numvars]++;
            }
            else
            {   // it's a value.
                D_ASSERT(augpath[i]>=numvars && augpath[i]<numvars+numvals);
                varvalmatching[augpath[i+1]]=dom_min-1;
                usage[augpath[i]-numvars]--;
            }
        }
        
        GCCPRINT("varvalmatching:" << varvalmatching);
    }
    
    virtual string constraint_name()
    {
      return "GCC";
    }
    
    virtual triggerCollection setup_internal()
    {
        D_INFO(2, DI_SUMCON, "Setting up Constraint");
        triggerCollection t;
        int capacity_size=capacity_array.size();
        
        #if !defined(INCGRAPH) || !defined(ONECALL)
            int array_size = var_array.size();
            for(int i = 0; i < array_size; ++i)
            {
                t.push_back(make_trigger(var_array[i], Trigger(this, i), DomainChanged));
            }
        #endif
        
        for(int i=0; i< capacity_size; ++i)
        {
            if(val_array[i]>=dom_min && val_array[i]<=dom_max)
            {
                t.push_back(make_trigger(capacity_array[i], Trigger(this, val_array[i]-dom_min + numvars), UpperBound));
                t.push_back(make_trigger(capacity_array[i], Trigger(this, val_array[i]-dom_min + numvars), LowerBound));
            }
        }
        return t;
    }
    
    virtual vector<AnyVarRef> get_vars()
	{
	  vector<AnyVarRef> vars;
	  vars.reserve(var_array.size());
	  for(unsigned i = 0; i < var_array.size(); ++i)
	    vars.push_back(var_array[i]);
      for(unsigned i = 0; i < capacity_array.size(); ++i)
	    vars.push_back(capacity_array[i]);
	  return vars;
	}
    
    virtual BOOL check_assignment(DomainInt* v, int vsize)
	{
	  D_ASSERT(vsize == var_array.size()+capacity_array.size());
      // borrow augpath array
      GCCPRINT("In check_assignment with array:[");
      for(int i=0; i<vsize; i++) GCCPRINT( v[i] <<",");
      GCCPRINT("]");
      augpath.clear();
      augpath.resize(numvals, 0);
      
      for(int i=0; i<numvars; i++)
      {   // count the values.
          augpath[v[i]-dom_min]++;
      }
      for(int i=0; i<val_array.size(); i++)
      {
          int val=val_array[i];
          if(val>=dom_min && val<=dom_max)
          {
              if(v[i+numvars]!=augpath[val-dom_min])
              {
                  return false;
              }
          }
          else
          {
              if(v[i+numvars]!=0)
              {
                  return false;
              }
          }
      }
      return true;
	}
    
    ////////////////////////////////////////////////////////////////////////////
    //
    // Tarjan's algorithm 
    
    
    vector<int> tstack;
    smallset_nolist in_tstack;
    smallset_nolist visited;
    vector<int> dfsnum;
    vector<int> lowlink;
    
    vector<int> curnodestack;
    
    bool scc_split;
    
    int sccindex;
    
    int max_dfs;
    
    smallset valinlocalmatching;
    smallset varinlocalmatching;
    
    int varcount, valcount;
    //int localmin,localmax;
    
    void initialize_tarjan()
    {
        int numnodes=numvars+numvals+1;  // One sink node.
        tstack.reserve(numnodes);
        in_tstack.reserve(numnodes);
        visited.reserve(numnodes);
        max_dfs=1;
        scc_split=false;
        dfsnum.resize(numnodes);
        lowlink.resize(numnodes);
        
        //iterationstack.resize(numnodes);
        curnodestack.reserve(numnodes);
        
        //valinlocalmatching.reserve(numvals);
        varinlocalmatching.reserve(numvars);
    }
    
    void tarjan_recursive(int sccindex_start)
    {
        tstack.clear();
        in_tstack.clear();
        
        visited.clear();
        max_dfs=1;
        
        scc_split=false;
        sccindex=sccindex_start;
        
        for(int i=0; i<vars_in_scc.size(); ++i)
        {
            int curnode=vars_in_scc[i];
            if(!visited.in(curnode))
            {
                GCCPRINT("(Re)starting tarjan's algorithm, at node:"<< curnode);
                varcount=0; valcount=0;
                visit(curnode, true);
                GCCPRINT("Returned from tarjan's algorithm.");
            }
        }
        
        // Also make sure all vals have been visited, so that values which
        // are in singleton SCCs are removed from all vars. 
        for(int i=0; i<vals_in_scc.size(); ++i)
        {
            int curnode=vals_in_scc[i]-dom_min+numvars;
            if(!visited.in(curnode))
            {
                GCCPRINT("(Re)starting tarjan's algorithm, at node:"<< curnode);
                varcount=0; valcount=0;
                visit(curnode, true);
                GCCPRINT("Returned from tarjan's algorithm.");
            }
        }
    }
    
    void visit(int curnode, bool toplevel)
    {
        // toplevel is true iff this is the top level of the recursion.
        tstack.push_back(curnode);
        in_tstack.insert(curnode);
        dfsnum[curnode]=max_dfs;
        lowlink[curnode]=max_dfs;
        max_dfs++;
        visited.insert(curnode);
        GCCPRINT("Visiting node: " <<curnode);
        
        if(curnode==numvars+numvals)
        {
            //cout << "Visiting sink node." <<endl;
            // It's the sink so it links to all spare values.
            /*
            for(int i=0; i<spare_values.size(); ++i)
            {
                int newnode=spare_values[i];
                //cout << "About to visit spare value: " << newnode-numvars+dom_min <<endl;
                if(!visited.in(newnode))
                {
                    visit(newnode);
                    if(lowlink[newnode]<lowlink[curnode])
                    {
                        lowlink[curnode]=lowlink[newnode];
                    }
                }
                else
                {
                    // Already visited newnode
                    if(in_tstack.in(newnode) && dfsnum[newnode]<lowlink[curnode])
                    {
                        lowlink[curnode]=dfsnum[newnode];
                    }
                }
            }*/
            
            // GCC mod:
            // link to any value which is below its upper cap.
            GCCPRINT("usage:"<<usage);
            GCCPRINT("upper:"<<upper);
            
            for(int j=0; j<vals_in_scc.size(); j++)
            {
                int i=vals_in_scc[j];
                int newnode=i+numvars-dom_min;
                if(usage[i-dom_min]<upper[i-dom_min])
                {
                    GCCPRINT("val "<< i << "below upper cap.");
                    if(!visited.in(newnode))
                    {
                        visit(newnode, false);
                        if(lowlink[newnode]<lowlink[curnode])
                        {
                            lowlink[curnode]=lowlink[newnode];
                        }
                    }
                    else
                    {
                        // Already visited newnode
                        if(in_tstack.in(newnode) && dfsnum[newnode]<lowlink[curnode])
                        {
                            lowlink[curnode]=dfsnum[newnode];
                        }
                    }
                }
            }
        }
        else if(curnode<numvars)  // This case should never occur with merge nodes.
        {
            D_ASSERT(find(vars_in_scc.begin(), vars_in_scc.end(), curnode)!=vars_in_scc.end());
            varcount++;
            int newnode=varvalmatching[curnode]-dom_min+numvars;
            D_ASSERT(var_array[curnode].inDomain(newnode+dom_min-numvars));
            
            if(!visited.in(newnode))
            {
                visit(newnode, false);
                if(lowlink[newnode]<lowlink[curnode])
                {
                    lowlink[curnode]=lowlink[newnode];
                }
            }
            else
            {
                // Already visited newnode
                if(in_tstack.in(newnode) && dfsnum[newnode]<lowlink[curnode])
                {
                    lowlink[curnode]=dfsnum[newnode];  // Why dfsnum not lowlink?
                }
            }
        }
        else
        {
            // curnode is a value
            // This is the only case where watches are set.
            //cout << "Visiting node val: "<< curnode+dom_min-numvars <<endl;
            valcount++;
            D_ASSERT(curnode>=numvars && curnode<(numvars+numvals));
            #ifndef NO_DEBUG
            bool found=false;
            for(int i=0; i<vars_in_scc.size(); i++)
            {
                if(var_array[vars_in_scc[i]].inDomain(curnode+dom_min-numvars))
                {
                    found=true;
                }
            }
            // D_ASSERT(found);  // it is safe to take out this test. But how did we get to this value?
            #endif
            
            int lowlinkvar=-1;
            #ifndef INCGRAPH
            for(int i=0; i<vars_in_scc.size(); i++)
            {
                int newnode=vars_in_scc[i];
            #else
            for(int i=0; i<adjlistlength[curnode]; i++)
            {
                int newnode=adjlist[curnode][i];
            #endif
                if(varvalmatching[newnode]!=curnode-numvars+dom_min)   // if the value is not in the matching.
                {
                    #ifndef INCGRAPH
                    if(var_array[newnode].inDomain(curnode+dom_min-numvars))
                    #endif
                    {
                        //newnode=varvalmatching[newnode]-dom_min+numvars;  // Changed here for merge nodes
                        if(!visited.in(newnode))
                        {
                            
                            visit(newnode, false);
                            if(lowlink[newnode]<lowlink[curnode])
                            {
                                lowlink[curnode]=lowlink[newnode];
                                lowlinkvar=-1;   // Would be placing a watch where there already is one.
                            }
                        }
                        else
                        {
                            // Already visited newnode
                            if(in_tstack.in(newnode) && dfsnum[newnode]<lowlink[curnode])
                            {
                                lowlink[curnode]=dfsnum[newnode];
                                lowlinkvar=newnode;
                            }
                        }
                    }
                }
            }
            
            if(true //include_sink 
                && usage[curnode-numvars]>lower[curnode-numvars])  // adaptation for GCC instead of the following comment.
            //valinlocalmatching.in(curnode-numvars))
            {
                int newnode=numvars+numvals;
                if(!visited.in(newnode))
                {
                    visit(newnode, false);
                    if(lowlink[newnode]<lowlink[curnode])
                    {
                        lowlink[curnode]=lowlink[newnode];
                        lowlinkvar=-1;
                    }
                }
                else
                {
                    // Already visited newnode
                    if(in_tstack.in(newnode) && dfsnum[newnode]<lowlink[curnode])
                    {
                        lowlink[curnode]=dfsnum[newnode];
                        lowlinkvar=-1;
                    }
                }
            }
            
        }
        
        //cout << "On way back up, curnode:" << curnode<< ", lowlink:"<<lowlink[curnode]<< ", dfsnum:"<<dfsnum[curnode]<<endl;
        if(lowlink[curnode]==dfsnum[curnode])
        {
            // Did the SCC split?
            // Perhaps we traversed all vars but didn't unroll the recursion right to the top.
            // !toplevel . Or perhaps we didn't traverse all the variables. (or all values.)
            // I think these two cases cover everything.
            if(!toplevel || varcount<vars_in_scc.size() || valcount<vals_in_scc.size())
            {
                scc_split=true;  // The SCC has split and there is some work to do later.
            }
            
            // Doing something with the components should not be necessary unless the scc has split.
            // The first SCC found is deep in the tree, so the flag will be set to its final value
            // the first time we are here.
            // so it is OK to assume that scc_split has been
            // set correctly before we do the following.
            if(scc_split)
            {
                // For each variable and value, write it to the scc array.
                // If its the last one, flip the bit.
                
                varinlocalmatching.clear();  // Borrow this datastructure for a minute.
                
                GCCPRINT("Writing new SCC:");
                bool containsvars=false, containsvals=false;
                for(vector<int>::iterator tstackit=(--tstack.end());  ; --tstackit)
                {
                    int copynode=(*tstackit);
                    
                    if(copynode!=numvars+numvals) // if it is not t
                    {
                        if(copynode<numvars) containsvars=true;
                        else containsvals=true;
                        
                        int temp=SCCs[sccindex];
                        int tempi=varToSCCIndex[copynode];
                        
                        SCCs[sccindex]=copynode;
                        varToSCCIndex[copynode]=sccindex;
                        
                        SCCs[tempi]=temp;
                        varToSCCIndex[temp]=tempi;
                        sccindex++;
                        
                        if(copynode<numvars)
                        {
                            varinlocalmatching.insert(copynode);
                        }
                    }
                    
                    if(copynode==curnode)
                    {
                        // Beware it might be an SCC containing just one value.
                        // or just t
                        
                        if(containsvars || containsvals)   //containsvars
                        {
                            GCCPRINT("Inserting split point at "<< sccindex-1 << " SCCs:" << SCCs);
                            SCCSplit.remove(sccindex-1);
                        }
                        
                        // The one written last was the last one in the SCC.
                        break;
                    }
                    
                    // Should be no split points in the middle of writing an SCC.
                    //D_ASSERT(copynode==curnode || copynode>=numvars || SCCSplit.isMember(sccindex-1));
                }
                // Just print more stuff here.
                
                
                // For each value, iterate through the current
                // SCC and remove it from any other variable other
                // than the one in this SCC.
                //cout << "Starting loop"<<endl;
                //if(containsvars) // why is this OK? because of bug above, in case where numnode is a variable.
                {
                    while(true)
                    {
                        int copynode=(*(--tstack.end()));
                        
                        tstack.pop_back();
                        in_tstack.remove(copynode);
                        
                        if(copynode>=numvars && copynode!=(numvars+numvals))
                        {
                            // It's a value. Iterate through old SCC and remove it from
                            // any variables not in tempset.
                            //cout << "Trashing value "<< copynode+dom_min-numvars << endl;
                            for(int i=0; i<vars_in_scc.size(); i++)
                            {
                                int curvar=vars_in_scc[i];
                                if(!varinlocalmatching.in(curvar))
                                {
                                    // var not in tempset so might have to do some test against matching.
                                    // Why doing this test? something wrong with the assigned variable optimization?
                                    if(varvalmatching[curvar]!=copynode+dom_min-numvars)
                                    {
                                        GCCPRINT("Removing var: "<< curvar << " val:" << copynode+dom_min-numvars);
                                        if(var_array[curvar].inDomain(copynode+dom_min-numvars))
                                        {
                                            var_array[curvar].removeFromDomain(copynode+dom_min-numvars);
                                            #ifdef INCGRAPH
                                                // swap with the last element and remove
                                                //adjlist_remove(copynode-numvars, adjlistpos[copynode-numvars][curvar]);
                                                adjlist_remove(curvar, copynode-numvars+dom_min);
                                            #endif
                                        }
                                    }
                                }
                            }
                        }
                        
                        if(copynode==curnode)
                        {
                            break;
                        }
                    }
                }
            }
        }
    }
    
    ///////////////////////////////////////////////////////////////////////////////////////////
    // Propagate to capacity variables.
    
    // can only be called when SCCs not used. 
    inline void prop_capacity()
    {
        #ifdef STRONGCARDS
            prop_capacity_strong();
        #else
            prop_capacity_simple();   
        #endif
    }
    
    void prop_capacity_simple()
    {
        // basic prop from main vars to cap variables. equiv to occurrence constraints I think. 
        // NEEDS TO BE IMPROVED. but it would be quadratic (nd) whatever I do.
        for(int i=0; i<val_array.size(); i++)
        {
            int val=val_array[i];
            #ifndef INCGRAPH
                int mincap=0;
                int maxcap=0;
                for(int j=0; j<numvars; j++)
                {
                    if(var_array[j].inDomain(val))
                    {
                        maxcap++;
                        if(var_array[j].isAssigned())
                            mincap++;
                    }
                }
                capacity_array[i].setMin(mincap);
                capacity_array[i].setMax(maxcap);
            #else
                if(val>= dom_min && val<=dom_max)
                {
                    int mincap=0;
                    for(int vari=0; vari<adjlistlength[val-dom_min+numvars]; vari++)
                    {
                        int var=adjlist[val-dom_min+numvars][vari];
                        if(var_array[var].isAssigned())
                            mincap++;
                    }
                    capacity_array[i].setMin(mincap);
                    capacity_array[i].setMax(adjlistlength[val-dom_min+numvars]);
                }  // else the cap will already have been set to 0.
            #endif
            //if(mincap>lower[val-dom_min])
            //    lower[val-dom_min]=mincap;
            //if(maxcap<upper[val-dom_min])
            //    upper[val-dom_min]=maxcap;
        }
    }
    
    void prop_capacity_strong()
    {
        // Lower bounds.
        prop_capacity_simple();
        GCCPRINT("In prop_capacity_strong");
        
        // Temporary measure.
        vars_in_scc.clear();
        for(int i=0; i<numvars; i++)
        {
            vars_in_scc.push_back(i);
        }
        
        for(int validx=0; validx<val_array.size(); validx++)
        {
            int value=val_array[validx];
            if(value>=dom_min && value<=dom_max)
            {
                // use the matching -- change it by lowering flow to value.
                GCCPRINT("Calling bfsmatching_card_lowerbound for value "<< value);
                int newlb=bfsmatching_card_lowerbound(value, lower[value-dom_min]);
                GCCPRINT("bfsmatching_card_lowerbound Returned " << newlb);
                
                if(newlb > capacity_array[validx].getMin())
                {
                    GCCPRINT("Improved lower bound "<< newlb);
                    capacity_array[validx].setMin(newlb);
                }
                
                GCCPRINT("Calling card_upperbound for value "<< value);
                int newub=card_upperbound(value, upper[value-dom_min]);
                GCCPRINT("card_upperbound Returned " << newub);
                
                if(newub < capacity_array[validx].getMax())
                {
                    GCCPRINT("Improved upper bound "<< newub);
                    capacity_array[validx].setMax(newub);
                }
            }
            else
            {// this may not be neecded. Only needed if we're not calling prop_capacity_simple
                capacity_array[validx].propagateAssign(0);
            }
        }
    }
    
    void prop_capacity_strong_scc(int value)
    {
        GCCPRINT("In prop_capacity_strong_scc(value)");
        // use the matching -- change it by lowering flow to value.
        GCCPRINT("Calling bfsmatching_card_lowerbound for value "<< value);
        // assumes vars_in_scc and vals_in_scc are already populated.
        
        int newlb=bfsmatching_card_lowerbound(value, lower[value-dom_min]);
        GCCPRINT("bfsmatching_card_lowerbound Returned " << newlb);
        int validx=val_to_cap_index[value-dom_min];
        if(newlb > capacity_array[validx].getMin())
        {
            GCCPRINT("Improved lower bound "<< newlb);
            capacity_array[validx].setMin(newlb);
        }
        
        GCCPRINT("Calling card_upperbound for value "<< value);
        int newub=card_upperbound(value, upper[value-dom_min]);
        GCCPRINT("card_upperbound Returned " << newub);
        
        if(newub < capacity_array[validx].getMax())
        {
            GCCPRINT("Improved upper bound "<< newub);
            capacity_array[validx].setMax(newub);
        }
    }
    
    // function to re-maximise a matching without using a particular value.
    // Used to find a new lowerbound for the value.
    // Changed copy of bfsmatching_gcc method above.
    
    // should stop when we reach the existing bound.
    inline int bfsmatching_card_lowerbound(int forbiddenval, int existinglb)
    {
        // lower and upper are indexed by value-dom_min and provide the capacities.
        // usage is the number of times a value is used in the matching.
        
        if(existinglb==usage[forbiddenval-dom_min])
        {
            //bound already supported
            return existinglb;
        }
        
        #ifdef CAPBOUNDSCACHE
        if(boundsupported[(forbiddenval-dom_min)*2]==existinglb)
        {
            return existinglb;
        }
        #endif
        
        // current sccs are contained in vars_in_scc and vals_in_scc
        // back up the matching to restore afterwards.
        matchbac=varvalmatching;
        usagebac=usage;
        
        // clear out forbiddenval
        // instead of clearing it out, can we do something else?
        /*usage[forbiddenval-dom_min]=0;
        for(int i=0; i<numvars; i++)
        {
            if(varvalmatching[i]==forbiddenval)
            {
                varvalmatching[i]=dom_min-1;
                newlb++;
            }
        }*/
        int newlb=usage[forbiddenval-dom_min];  // new lower bound. When this passes existinglb, we can stop.
        
        /*for(int startvarscc=0; startvarscc<vars_in_scc.size(); startvarscc++)
        {
            int startvar=vars_in_scc[startvarscc];
            if(varvalmatching[startvar]==forbiddenval)
            {
                varvalmatching[startvar]=dom_min-1;
                usage[forbiddenval-dom_min]--;
            }
        }*/
        
        // Flip the graph around, so it's like the alldiff case now. 
        // follow an edge in the matching from a value to a variable,
        // follow edges not in the matching from variables to values. 
        
        #ifdef INCGRAPH
		for(int startvari=0; startvari<adjlistlength[forbiddenval-dom_min+numvars] && newlb>existinglb; startvari++)
        {
            int startvar=adjlist[forbiddenval-dom_min+numvars][startvari];
        #else
        for(int startvarscc=0; startvarscc<vars_in_scc.size() && newlb>existinglb; startvarscc++)
        {
            int startvar=vars_in_scc[startvarscc];
        #endif
            if(varvalmatching[startvar]==forbiddenval)
            {
                varvalmatching[startvar]=dom_min-1;
                usage[forbiddenval-dom_min]--;
                GCCPRINT("Searching for augmenting path for var: " << startvar);
                fifo.clear();  // this should be constant time but probably is not.
                fifo.push_back(startvar);
                visited.clear();
                visited.insert(startvar);
                bool finished=false;
                while(!fifo.empty() && !finished)
                {
                    // pop a vertex and expand it.
                    int curnode=fifo.front();
                    fifo.pop_front();
                    GCCPRINT("Popped vertex " << (curnode<numvars? "(var)":"(val)") << (curnode<numvars? curnode : curnode+dom_min-numvars ));
                    if(curnode<numvars)
                    { // it's a variable
                        // follow all edges other than the matching edge. 
                        #ifndef INCGRAPH
                        for(int valtoqueue=var_array[curnode].getMin(); valtoqueue<=var_array[curnode].getMax(); valtoqueue++)
                        {
                        #else
                        for(int valtoqueuei=0; valtoqueuei<adjlistlength[curnode]; valtoqueuei++)
                        {
                            int valtoqueue=adjlist[curnode][valtoqueuei];
                        #endif
                            // For each value, check if it terminates an odd alternating path
                            // and also queue it if it is suitable.
                            int validx=valtoqueue-dom_min+numvars;
                            if(valtoqueue!=varvalmatching[curnode]
                                && valtoqueue!=forbiddenval  // added for this method.
                            #ifndef INCGRAPH
                                && var_array[curnode].inDomain(valtoqueue)
                            #endif
                                && !visited.in(validx) )
                            {
                                //D_ASSERT(find(vals_in_scc.begin(), vals_in_scc.end(), valtoqueue)!=vals_in_scc.end()); // the value is in the scc.
                                // Does this terminate an augmenting path?
                                if(usage[valtoqueue-dom_min]<upper[valtoqueue-dom_min])
                                {
                                    // valtoqueue terminates an alternating path.
                                    // Unwind and apply the path here
                                    prev[validx]=curnode;
                                    apply_augmenting_path_reverse(validx, startvar);
                                    finished=true;
                                    newlb--;  // update bound counter
                                    break;  // get out of for loop
                                }
                                else
                                {
                                    // queue valtoqueue
                                    visited.insert(validx);
                                    prev[validx]=curnode;
                                    fifo.push_back(validx);
                                }
                            }
                        }  // end for.
                    }
                    else
                    { // popped a value from the stack.
                        D_ASSERT(curnode>=numvars && curnode < numvars+numvals);
                        int stackval=curnode+dom_min-numvars;
                        #ifdef INCGRAPH
                        for(int vartoqueuei=0; vartoqueuei<adjlistlength[curnode]; vartoqueuei++)
                        {
                            int vartoqueue=adjlist[curnode][vartoqueuei];
                        #else
                        for(int vartoqueuescc=0; vartoqueuescc<vars_in_scc.size(); vartoqueuescc++)
                        {
                            int vartoqueue=vars_in_scc[vartoqueuescc];
                        #endif
                            // For each variable which is matched to stackval, queue it.
                            if(!visited.in(vartoqueue)
                                && varvalmatching[vartoqueue]==stackval)
                            {
                                D_ASSERT(var_array[vartoqueue].inDomain(stackval));
                                // there is an edge from stackval to vartoqueue.
                                // queue vartoqueue
                                visited.insert(vartoqueue);
                                prev[vartoqueue]=curnode;
                                fifo.push_back(vartoqueue);
                            }
                        }  // end for.
                    }  // end value
                }  // end while
            }
        }
        
        GCCPRINT("maximum matching:" << varvalmatching);
        
        if(newlb==existinglb)
        {
            GCCPRINT("Stopped because new lower bound would be less than or equal the existing lower bound.");
        }
        
        #ifdef CAPBOUNDSCACHE
        boundsupported[(forbiddenval-dom_min)*2]=usage[forbiddenval-dom_min];
        DynamicTrigger* dt=dynamic_trigger_start();
        dt+=(numvars*numvals);  // skip over the first block of triggers
        dt+=val_to_cap_index[forbiddenval-dom_min]*(val_array.size()+numvars)*2;  // move to the area for the value.
        //dt+=(val_array.size()+numvars);  // move to upper bound area
        // now put down the triggers for varvalmatching and usage
        for(int i=0; i<numvars; i++)
        {
            D_ASSERT((dt+i)->trigger_info() == (forbiddenval-dom_min)*2);
            var_array[i].addDynamicTrigger(dt+i, DomainRemoval, varvalmatching[i]);
        }
        for(int i=0; i<val_array.size(); i++)
        {
            D_ASSERT((dt+numvars+i)->trigger_info() == (forbiddenval-dom_min)*2);
            
            if(val_array[i]>= dom_min && val_array[i]<= dom_max)
            {
                capacity_array[i].addDynamicTrigger(dt+numvars+i, DomainRemoval, usage[val_array[i]-dom_min]);
            }
            else
            {
                capacity_array[i].addDynamicTrigger(dt+numvars+i, DomainRemoval, 0);
            }
        }
        #endif
        
        varvalmatching=matchbac;
        usage=usagebac;
        
        return newlb;
    }
    
    inline int card_upperbound(int value, int existingub)
    {
        // lower and upper are indexed by value-dom_min and provide the capacities.
        // usage is the number of times a value is used in the matching.
        
        if(existingub==usage[value-dom_min])
        {
            // bound is already supported
            return existingub;
        }
        
        #ifdef CAPBOUNDSCACHE
        if(boundsupported[(value-dom_min)*2+1]==existingub)
        {
            return existingub;
        }
        #endif
        
        // current sccs are contained in vars_in_scc and vals_in_scc
        
        int startvalindex=value-dom_min;
        while(usage[startvalindex]<existingub)
        {
            // usage of value needs to increase. Construct an augmenting path starting at value.
            GCCPRINT("Searching for augmenting path for val: " << value);
            // Matching edge lost; BFS search for augmenting path to fix it.
            fifo.clear();  // this should be constant time but probably is not.
            fifo.push_back(startvalindex+numvars);
            visited.clear();
            visited.insert(startvalindex+numvars);
            bool finished=false;
            while(!fifo.empty() && !finished)
            {
                // pop a vertex and expand it.
                int curnode=fifo.front();
                fifo.pop_front();
                GCCPRINT("Popped vertex " << (curnode<numvars? "(var)":"(val)") << (curnode<numvars? curnode : curnode+dom_min-numvars ));
                if(curnode<numvars)
                { // it's a variable
                    // follow the matching edge, if there is one.
                    int valtoqueue=varvalmatching[curnode];
                    if(valtoqueue!=dom_min-1 
                        && !visited.in(valtoqueue-dom_min+numvars))
                    {
                        D_ASSERT(var_array[curnode].inDomain(valtoqueue));
                        int validx=valtoqueue-dom_min+numvars;
                        if(usage[valtoqueue-dom_min]>lower[valtoqueue-dom_min])
                        {
                            // can reduce the flow of valtoqueue to increase startval.
                            prev[validx]=curnode;
                            apply_augmenting_path(validx, startvalindex+numvars);
                            finished=true;
                        }
                        else
                        {
                            visited.insert(validx);
                            prev[validx]=curnode;
                            fifo.push_back(validx);
                        }
                    }
                }
                else
                { // popped a value from the stack.
                    D_ASSERT(curnode>=numvars && curnode < numvars+numvals);
                    int stackval=curnode+dom_min-numvars;
                    #ifndef INCGRAPH
                    for(int vartoqueuescc=0; vartoqueuescc<vars_in_scc.size(); vartoqueuescc++)
                    {
                        int vartoqueue=vars_in_scc[vartoqueuescc];
                    #else
                    for(int vartoqueuei=0; vartoqueuei<adjlistlength[curnode]; vartoqueuei++)
                    {
                        int vartoqueue=adjlist[curnode][vartoqueuei];
                    #endif
                        // For each variable, check if it terminates an odd alternating path
                        // and also queue it if it is suitable.
                        if(!visited.in(vartoqueue)
                            #ifndef INCGRAPH
                            && var_array[vartoqueue].inDomain(stackval)
                            #endif
                            && varvalmatching[vartoqueue]!=stackval)   // Need to exclude the matching edges????
                        {
                            // there is an edge from stackval to vartoqueue.
                            if(varvalmatching[vartoqueue]==dom_min-1)
                            {
                                // vartoqueue terminates an odd alternating path.
                                // Unwind and apply the path here
                                prev[vartoqueue]=curnode;
                                apply_augmenting_path(vartoqueue, startvalindex+numvars);
                                finished=true;
                                break;  // get out of for loop
                            }
                            else
                            {
                                // queue vartoqueue
                                visited.insert(vartoqueue);
                                prev[vartoqueue]=curnode;
                                fifo.push_back(vartoqueue);
                            }
                        }
                    }  // end for.
                }  // end value
            }  // end while
            if(!finished)
            {   // no augmenting path found
                GCCPRINT("No augmenting path found.");
                // restore the matching to its state before the algo was called.
                break;
            }
            
        }  // end while
        
        //varvalmatching=matchbac;
        //usage=usagebac;
        #ifdef CAPBOUNDSCACHE
        boundsupported[(value-dom_min)*2+1]=usage[startvalindex];
        DynamicTrigger* dt=dynamic_trigger_start();
        dt+=(numvars*numvals);  // skip over the first block of triggers
        dt+=val_to_cap_index[value-dom_min]*(val_array.size()+numvars)*2;  // move to the area for the value.
        dt+=(val_array.size()+numvars);  // move to upper bound area
        // now put down the triggers for varvalmatching and usage
        for(int i=0; i<numvars; i++)
        {
            D_ASSERT((dt+i)->trigger_info() == (value-dom_min)*2+1);
            var_array[i].addDynamicTrigger(dt+i, DomainRemoval, varvalmatching[i]);
        }
        for(int i=0; i<val_array.size(); i++)
        {
            D_ASSERT((dt+numvars+i)->trigger_info() == (value-dom_min)*2+1);
            if(val_array[i]>= dom_min && val_array[i]<= dom_max)
            {
                capacity_array[i].addDynamicTrigger(dt+numvars+i, DomainRemoval, usage[val_array[i]-dom_min]);
            }
            else
            {
                capacity_array[i].addDynamicTrigger(dt+numvars+i, DomainRemoval, 0);
            }
        }
        #endif
        
        return usage[startvalindex];
    }
    
    // Function to make it reifiable in the most minimal way.
    virtual AbstractConstraint* reverse_constraint()
    {
        vector<AnyVarRef> t;
        for(int i=0; i<var_array.size(); i++)
            t.push_back(var_array[i]);
        for(int i=0; i<capacity_array.size(); i++)
            t.push_back(capacity_array[i]);
        
        return new CheckAssignConstraint<vector<AnyVarRef>, GCC>(stateObj, t, *this);
    }
};
