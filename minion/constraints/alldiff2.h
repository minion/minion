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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/*

This is the real-valued network flow algorithm, not Regin's algorithm.

*/


#include <stdlib.h>
#include <iostream>
#include <vector>
#include <deque>
#include <algorithm>
#include <utility>

using namespace std;

#include "alldiff_gcc_shared.h"

// includes for reverse constraint.
#include "constraint_equal.h"
#include "../dynamic_constraints/dynamic_new_or.h"

// The implementation of the alldiff GAC algorithm,
// shared between gacalldiff and the (now defunct) dynamicalldiff.


// whether to backtrack the matching.
// This one is a slowdown.
// #define BTMATCHING

// This one seems to break even.
// Whether to use a TMS to track whether spare values are present in the scc
// #define SPAREVALUESOPT

// Reverse the var list. Leave this switched on.
#define REVERSELIST

// Check domain size -- if it is greater than numvars, then no need to wake the constraint.
//#define CHECKDOMSIZE

// Process SCCs independently
#define SCC

// Warning: if this is not defined true, then watchedalldiff probably won't do anything.
#define UseWatches false

// Optimize the case where a value was assigned. Only works in the presence of SCC
#define ASSIGNOPT 

// Use the special queue
#define SPECIALQUEUE

// store matching from one run to the next.
#define INCREMENTALMATCH

// Use BFS instead of HK
#define BFSMATCHING

// Use the new hopcroft-karp implementation.
//#define NEWHK

// Incremental graph stored in adjlist
#define UseIncGraph true

// Use staging a la Schulte and Stuckey
#define STAGED

#ifdef P
#undef P
#endif

#ifdef PLONG
#undef PLONG
#endif

#define P(x)
//#define P(x) cout << x << endl
//#define PLONG

template<typename VarArray>
struct GacAlldiffConstraint2 : public FlowConstraint<VarArray, true>
{
    using FlowConstraint<VarArray, true>::stateObj;
    using FlowConstraint<VarArray, true>::constraint_locked;
    using FlowConstraint<VarArray, true>::adjlist;
    using FlowConstraint<VarArray, true>::adjlistlength;
    using FlowConstraint<VarArray, true>::adjlistpos;
    using FlowConstraint<VarArray, true>::dynamic_trigger_start;
    using FlowConstraint<VarArray, true>::adjlist_remove;
    using FlowConstraint<VarArray, true>::var_array;
    using FlowConstraint<VarArray, true>::dom_min;
    using FlowConstraint<VarArray, true>::dom_max;
    using FlowConstraint<VarArray, true>::numvars;
    using FlowConstraint<VarArray, true>::numvals;
    using FlowConstraint<VarArray, true>::varvalmatching;
    using FlowConstraint<VarArray, true>::valvarmatching;
    
    
    using FlowConstraint<VarArray, true>::varinlocalmatching;
    using FlowConstraint<VarArray, true>::valinlocalmatching;
    using FlowConstraint<VarArray, true>::invprevious;
    
    using FlowConstraint<VarArray, true>::initialize_hopcroft;
    using FlowConstraint<VarArray, true>::hopcroft_wrapper;
    using FlowConstraint<VarArray, true>::hopcroft2_setup;
    
    virtual string constraint_name()
    { 
        return "GacAlldiff2";
    }
    
    vector<double> flow_s_var;
    vector<vector<double> > flow_var_val;  // flow_varval[variable][value] is the 0-1 flow from variable to value.
    vector<double> flow_val_t;    // flow_val_t[value] is the 0-1 flow from a value to t.
    double totalflow;      // If circular edge, this is the flow from t to s. 
    
    GacAlldiffConstraint2(StateObj* _stateObj, const VarArray& _var_array) : FlowConstraint<VarArray, true>(_stateObj, _var_array)
    {
        totalflow=0.0;
        
        flow_var_val.resize(numvars);
        for(int i=0; i<numvars; i++)
        {
            flow_var_val[i].resize(numvals, 0.0);
        }
        flow_val_t.resize(numvals, 0.0);
        
        flow_s_var.resize(numvars, 0.0);
        
        
        
      to_process.reserve(var_array.size());
      
      // Set up data structures
      prev.resize(numvars+numvals, -1);
      
      
      // Initialize matching to satisfy the invariant
      // that the values are all different in varvalmatching.
      // watches DS is used in alldiffgacslow and in debugging.
      #if !defined(DYNAMICALLDIFF) || !defined(NO_DEBUG)
      if(UseWatches) watches.resize(numvars);
      #endif
      
      for(int i=0; i<numvars; i++) //&& i<numvals
      {
          varvalmatching[i]=i+dom_min;
          if(i<numvals) valvarmatching[i]=i;
          
          #if !defined(DYNAMICALLDIFF) || !defined(NO_DEBUG)
          if(UseWatches) watches[i].reserve(numvals, stateObj);
          #endif
      }
      
  }

    /////////////////////////////////////////////////////////////////////
    //  BFS
    
    deque<int> fifo;
    vector<int> prev;
    
    
    vector<int>& bfs(int start, int end)
    {
        // find any non-saturated path from start to end (including the circulating edge.)
        // nodes are 0..numvars-1 for variables,
        // numvars..(numvars+numvals-1) for values
        // numvars+numvals    for s
        // numvars+numvals+1  for t.
        
        fifo.clear(); fifo.push_back(start);
        visited.clear();
        visited.insert(start);
        
        while(fifo.size()>0):
        {
            int curnode=fifo.front();
            fifo.pop_front();
            
            if(curnode==end)
            {   // found a path, extract it from prev
                // This needs to go into the cases below.
                augpath.clear();
                augpath.push_back(end);
                while(curnode!=start)
                {
                    curnode=prev[curnode];
                    augpath.push_back(curnode);
                }
                std::reverse(augpath.begin(), augpath.end());
                
                return augpath;
            }
            
            // Now cases for each type of node.
            if(curnode==numvars+numvals)
            {   // s.
                // All variables.  No circulating edge.
                for(int newnode=0;  newnode<numvars; newnode++)
                {
                    if(!visited.in(newnode) && flow_s_var[newnode]<1.0)
                    {
                        fifo.push_back(newnode);
                        visited.insert(newnode);
                        prev[newnode]=curnode;
                    }
                }
            }
            else if(curnode==numvars+numvals+1)
            {   // t
                // Link to all vals that are above 0 flow.
                
                for(int i=dom_min; i<=dom_max; i++)
                {
                    int newnode=i-dom_min+numvals;
                    if(!visited.in(newnode) && flow_val_t[newnode]>0.0)
                    {
                        fifo.push_back(newnode);
                        visited.insert(newnode);
                        prev[newnode]=curnode;
                    }
                }
                // No circulating edge.
            }
            else if(curnode<numvars)
            {
                // a variable
                // link to all values 
                int len=adjlistlength[curnode];
                for(int adjlistidx=0; adjlistidx<len; adjlistidx++)
                {
                    int val=adjlist[curnode][adjlistidx]
                    int newnode=val-dom_min+numvals;
                    if(!visited.in(newnode) && flow_var_val[curnode][val-dom_min]<1.0)
                    {
                        fifo.push_back(newnode);
                        visited.insert(newnode);
                        prev[newnode]=curnode;
                    }
                }
                
                // Link to s.
                int newnode=numvars+numvals; ///s
                if(!visited.in(newnode) && flow_s_var[curnode]>0.0)
                {
                    fifo.push_back(newnode);
                    visited.insert(newnode);
                    prev[newnode]=curnode;
                }
            }
            else
            {
                assert curnode>=numvars && curnode<numvars+numvals;
                // a value
                int validx=curnode-numvars;  // 0-based value index.
                // link to t if edge not saturated
                int newnode=numvars+numvals+1; ///t
                if(!visited.in(newnode) && flow_val_t[validx]<1.0)
                {
                    fifo.push_back(newnode);
                    visited.insert(newnode);
                    prev[newnode]=curnode;
                }
                
                // All variables where the flow on the edge can be reduced. >0.
                int len=adjlistlength[curnode];
                for(int adjlistidx=0; adjlistidx<len; adjlistidx++)
                {
                    int newnode=adjlist[curnode][adjlistidx];
                    if(!visited.in(newnode) && flow_var_val[newnode, curnode]<1.0)
                    {
                        fifo.push_back(newnode);
                        visited.insert(newnode);
                        prev[newnode]=curnode;
                    }
                }
            }
        }
        return null;
    }
    
    // This is a mess -- must be a better way
    void apply_path_max_zeros(vector<int>& path)
    {
        int augamount = 1.0;
        for(int i=0; i<path.size()-1; i++)
        {
            int from=path[i];
            int to=path[i+1];
            double diff=-1000000.0;
            if(from==numvars+numvals)
            {   // edge from s to a var.
                diff=1.0-flow_s_var[to];
            }
            else if(from==numvars+numvals+1)
            {   // t to a val
                diff=flow_val_t[to];  // it can be reduced by the flow amount.
            }
            else if(from<numvars)
            {
                if(to==numvars+numvals)
                {
                    diff=flow_s_var[from]; 
                }
                else
                {
                    D_ASSERT(to>=numvars && to<numvars+numvals);
                    diff=1.0-flow_var_val[from][to];
                }
            }
            else
            {   // from is a value.
                if(to==numvars+numvals+1)
                {
                    diff=1.0-flow_val_t[from-numvars];
                }
                else
                {
                    D_ASSERT(to<numvars);
                    diff=flow_var_val[to, from];
                }
            }
            D_ASSERT(diff>0.0);
            augamount=( diff<augamount ? diff : augamount );
        }
        
        cout << "Augmenting flow by: " << augamount <<endl;
        zeros.clear();
        
        for(int i=0; i<path.size()-1; i++)
        {
            int from=path[i];
            int to=path[i+1];
            if(from==numvars+numvals)
            {   // edge from s to a var.
                flow_s_var[to]+=augamount;
            }
            else if(from==numvars+numvals+1)
            {   // t to a val
                flow_val_t[to]-=augamount;  // it can be reduced by the flow amount.
                if(flow_val_t[to]==0.0)
                {
                    zeros.push_back(to); zeros.push_back(from);
                }
            }
            else if(from<numvars)
            {
                if(to==numvars+numvals)
                {
                    flow_s_var[from]-=augamount;
                    if(flow_s_var[from]==0.0)
                    {
                        zeros.push_back(to); zeros.push_back(from);
                    }
                }
                else
                {
                    D_ASSERT(to>=numvars && to<numvars+numvals);
                    flow_var_val[from][to]+=augamount;
                }
            }
            else
            {   // from is a value.
                if(to==numvars+numvals+1)
                {
                    flow_val_t[from-numvars]+=augamount;
                }
                else
                {
                    D_ASSERT(to<numvars);
                    flow_var_val[to, from]-=augamount;
                    if(flow_var_val[to, from]==0.0)
                    {
                        zeros.push_back(to); zeros.push_back(from);
                    }
                }
            }
            
        }
        
    }
    
    
    
    
    
    
    
    
    
    
    
    
    
  
  #ifdef DYNAMICALLDIFF
  inline DynamicTrigger * get_dt(int var, int counter)
  {
      // index the square array of dynamic triggers from dt
      D_ASSERT(bt_triggers_start == dynamic_trigger_start()+numvars);
      D_ASSERT(counter<numvals);
      D_ASSERT(var<numvars);
      D_ASSERT((bt_triggers_start+(var*numvals)+counter) >= dynamic_trigger_start());
      D_ASSERT((bt_triggers_start+(var*numvals)+counter) < dynamic_trigger_start() + dynamic_trigger_count());
      
      return bt_triggers_start+(var*numvals)+counter;
  }
  #endif
  
  // Should only be used in non-dynamic version.
  #ifndef DYNAMICALLDIFF
  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    int array_size = var_array.size();
    for(int i = 0; i < array_size; ++i)
      t.push_back(make_trigger(var_array[i], Trigger(this, i), DomainChanged));
    return t;
  }
  #endif
  
  typedef typename VarArray::value_type VarRef;
  virtual AbstractConstraint* reverse_constraint()
  { // w-or of pairwise equality.
      
      /// solely for reify exps
      return new CheckAssignConstraint<VarArray, GacAlldiffConstraint>(stateObj, var_array, *this);
      
      vector<AbstractConstraint*> con;
      for(int i=0; i<var_array.size(); i++)
      {
          for(int j=i+1; j<var_array.size(); j++)
          {
              EqualConstraint<VarRef, VarRef>* t=new EqualConstraint<VarRef, VarRef>(stateObj, var_array[i], var_array[j]);
              con.push_back((AbstractConstraint*) t);
          }
      }
      return new Dynamic_OR(stateObj, con);
  }
  
  smallset to_process;  // set of vars to process.
  
  #if !defined(DYNAMICALLDIFF) || !defined(NO_DEBUG)
  vector<smallset_list_bt> watches;
  #endif
  
  virtual void propagate(int prop_var, DomainDelta)
  {
    D_ASSERT(prop_var>=0 && prop_var<var_array.size());
    
    // return if all the watches are still in place.
    #ifndef BTMATCHING 
    if(UseWatches && !to_process.in(prop_var) 
        && var_array[prop_var].inDomain(varvalmatching[prop_var]))     // This still has to be here, because we still wake up if the matchingis broken.
    #else
    if(UseWatches && !to_process.in(prop_var))
    #endif
    {
        smallset_list_bt& watch = watches[prop_var];
        short * list = ((short *) watch.list.get_ptr());
        int count=list[watch.maxsize];
        bool valout=false;
        
        for(int i=0; i<count; i++)
        {
            P("Checking var "<< prop_var << " val " << list[i]+dom_min);
            if(!var_array[prop_var].inDomain(list[i]+dom_min))
            {
                valout=true;
                break;
            }
        }
        if(!valout)
        {
            P("None of the watches were disturbed. Saved a call with watches.");
            return;
        }
    }
    #ifdef CHECKDOMSIZE
    // If the domain size is >= numvars, then return.
    //if(!constraint_locked)
    {
        int count=0;
        for(int i=var_array[prop_var].getMin(); i<=var_array[prop_var].getMax(); i++)
        {
            if(var_array[prop_var].inDomain(i))
            {
                count++;
            }
        }
        if(count>=numvars)
            return;
    }
    // could improve domain counting by also applying it when
    // the constraint is already queued, DONE since it might avoid calls to Tarjan's.
    // i.e. some vars may not need to be queued in to_process.
    #endif
    
    #ifdef STAGED
    if(var_array[prop_var].isAssigned())
    {
        int assignedval=var_array[prop_var].getAssignedValue();
        for(int i=0; i<numvars; i++)
        {
            if(i!=prop_var && var_array[i].inDomain(assignedval))
            {
                var_array[i].removeFromDomain(assignedval);
                #if UseIncGraph
                    adjlist_remove(i, assignedval);
                #endif
            }
        }
    }
    #endif
    
    if(!to_process.in(prop_var))
    {
        to_process.insert(prop_var);
    }
    
    if(!constraint_locked)
    {
        #ifdef SPECIALQUEUE
        constraint_locked = true;
        getQueue(stateObj).pushSpecialTrigger(this);
        #else
        #ifndef SCC
        do_prop_noscc();
        #else
        do_prop();
        #endif
        #endif
    }
  }
  
  virtual void propagate(DynamicTrigger* trig)
  {
      #if UseIncGraph
      DynamicTrigger* dtstart=dynamic_trigger_start();
      #ifdef DYNAMICALLDIFF
      dtstart+=numvars+numvars*numvals;
      #endif
      if(trig>=dtstart && trig< dtstart+numvars*numvals)  // does this trigger belong to incgraph?
      {
          int diff=trig-dtstart;
            int var=diff/numvals;
            int validx=diff%numvals;
            if(adjlistpos[validx+numvars][var]<adjlistlength[validx+numvars])
            {
                P("Removing var, val " << var << ","<< (validx+dom_min) << " from adjacency list.");
                adjlist_remove(var, validx+dom_min); //validx, adjlistpos[validx][var]);
            }
            return;
      }
      #endif
      
      // get variable number from the trigger
    int prop_var = trig->trigger_info();
    #ifdef PLONG
    // check that some value has been disturbed; otherwise the watches are malfunctioning.
    if(var_array[prop_var].inDomain(varvalmatching[prop_var]))
    {
        smallset_list_bt& watch = watches[prop_var];
        short * list = ((short *) watch.list.get_ptr());
        int count=list[watch.maxsize];
        bool valout=false;
        
        for(int i=0; i<count; i++)
        {
            P("Checking var "<< prop_var << " val " << list[i]+dom_min);
            if(!var_array[prop_var].inDomain(list[i]+dom_min))
            {
                valout=true;
                break;
            }
        }
        if(!valout)
        {
            // none of the watches were disturbed.
            cout << "None of the watches in the DS were disturbed. BT triggers must not match with watches DS." <<endl;
            cout << "Variable " << prop_var <<", val in matching: " << varvalmatching[prop_var] << endl;
            D_ASSERT(false);
        }
    }
    #endif
    
    #ifdef CHECKDOMSIZE
    // If the domain size is >= numvars, then return.
    // WHY IS THIS HERE WHEN checkdomsize and dynamic triggers don't work together??
    int count=0;
    for(int i=var_array[prop_var].getMin(); i<=var_array[prop_var].getMax(); i++)
    {
        if(var_array[prop_var].inDomain(i))
        {
            count++;
        }
    }
    if(count>=numvars)
        return;
    #endif
    
    #ifdef STAGED
    if(var_array[prop_var].isAssigned())
    {
        int assignedval=var_array[prop_var].getAssignedValue();
        for(int i=0; i<numvars; i++)
        {
            if(i!=prop_var && var_array[i].inDomain(assignedval))
            {
                var_array[i].removeFromDomain(assignedval);
                #if UseIncGraph
                    adjlist_remove(i, assignedval);
                #endif
            }
        }
    }
    #endif
    
    if(!to_process.in(prop_var))
    {
        to_process.insert(prop_var);
    }
    
    if(!constraint_locked)
    {
        #ifdef SPECIALQUEUE
        constraint_locked = true;
        getQueue(stateObj).pushSpecialTrigger(this);
        #else
        #ifndef SCC
        do_prop_noscc();
        #else
        do_prop();
        #endif
        #endif
    }
  }
  
  
  virtual void special_unlock() { constraint_locked = false; to_process.clear(); }
  virtual void special_check()
  {
    constraint_locked = false;  // should be above the if.
    
    if(getState(stateObj).isFailed())
    {
        to_process.clear();
        return;
    }
    
    #ifdef SCC
    do_prop();
    #else
    do_prop_noscc();
    #endif
    to_process.clear();
  }
  
  
  void do_prop()
  {
    PROP_INFO_ADDONE(AlldiffGacSlow);
    
    #ifdef DYNAMICALLDIFF
    bt_triggers_start=dynamic_trigger_start()+numvars;
    #endif
    
    #ifdef PLONG
    cout << "Entering do_prop."<<endl;
    cout << "Varvalmatching:" <<varvalmatching<<endl;
    cout << "SCCs:" << SCCs <<endl;
    cout << "SCCSplit: ";
    for(int i=0; i<numvars; i++)
    {
        cout << (SCCSplit.isMember(i)?"1, ":"0, ");
    }
    cout <<endl;
    cout << "varToSCCIndex: "<< varToSCCIndex<<endl;
    cout << "Domains (remember that var_array is reversed):" <<endl;
    for(int i=0; i<numvars; i++)
    {
        cout << "var:" << i << " vals:" ;
        for(int j=dom_min; j<=dom_max; j++)
        {
            if(var_array[i].inDomain(j))
            {
                cout << j <<", ";
            }
        }
        cout << endl;
    }
    // Check the matching is valid.
    for(int i=0; i<numvars; i++)
    {
        if(!var_array[i].inDomain(varvalmatching[i]))
        {
            cout << "val in matching removed: " << i << ", " << varvalmatching[i] <<endl;
        }
        for(int j=i+1; j<numvars; j++)
        {
            D_ASSERT(varvalmatching[i]!=varvalmatching[j]);
            D_ASSERT(SCCs[i]!=SCCs[j]);
        }
        D_ASSERT(SCCSplit.isMember(i) == (((char*)SCCSplit2.get_ptr())[i]==1));
        cout << (int)SCCSplit.isMember(i) << ", " << (int) (((char*)SCCSplit2.get_ptr())[i]==1) << endl;
        // The matches correspond.
        #ifndef BFSMATCHING
        D_ASSERT(valvarmatching[varvalmatching[i]-dom_min]==i);
        #endif
    }
    
    #ifdef DYNAMICALLDIFF
    // Can't yet check all the watches are in the right place, but can check
    // there are the right number of them on each var.
    for(int i=0; i<var_array.size(); i++)
    {
        int count=0;
        DynamicTrigger* trig=get_dt(i, 0);
        while(trig->queue!=NULL && count<numvals)
        {
            count++;
            if(count<numvals) trig=get_dt(i,count);
        }
        D_ASSERT(count==watches[i].size());
    }
    #endif
    
    // Check that if an element of the matching is removed, then the var is 
    // in to_process.
    for(int i=0; i<var_array.size(); i++)
    {
        if(!var_array[i].inDomain(varvalmatching[i]))
        {
            D_ASSERT(to_process.in(i));
        }
    }
    #endif
    // end of debug.
    
    #ifndef INCREMENTALMATCH
    // clear the matching.
    for(int i=0; i<numvars && i<numvals; i++)
    {
      varvalmatching[i]=i+dom_min;
      valvarmatching[i]=i;
    }
    #endif
    
    sccs_to_process.clear();
    {
    vector<int>& toiterate = to_process.getlist();
    P("About to loop for to_process variables.");
    
    for(int i=0; i<toiterate.size(); ++i)
    {
        int tempvar=toiterate[i];
        
        
        int sccindex_start=varToSCCIndex[tempvar];
        int sccindex_end=varToSCCIndex[tempvar];
        
        while(sccindex_start>0 && SCCSplit.isMember(sccindex_start-1))
        {
            sccindex_start--;   // seek the first item in the SCC.
        }
        
        while(SCCSplit.isMember(sccindex_end) && sccindex_end<(numvars-1))
        {
            sccindex_end++;   // seek the last item in the SCC
        }
        
        if(!var_array[tempvar].inDomain(varvalmatching[tempvar]))
        {
            // Find the start of the SCC in SCCs
            var_indices.clear();
            
            // Add a greedy repair here.
            
            // Actually should queue the SCCs which need to be hopcrofted,
            // and make greedy repairs to var matchings as we process them here.
            
            P("Varvalmatching:" <<varvalmatching);
            
            P("start:" << sccindex_start << " end:"<< sccindex_end);
            
            if(!matching_wrapper(sccindex_start, sccindex_end))
                return;
            
            #ifdef DYNAMICALLDIFF
            // sync the watches to the matching, 
            // since the matching has just changed.
            #ifndef BTMATCHING
            DynamicTrigger * trig = dynamic_trigger_start();
            for(int j=0; j<numvars; j++)
            {
                var_array[j].addWatchTrigger(trig + j, DomainRemoval, varvalmatching[j]);
                P("Adding watch for var " << j << " val " << varvalmatching[j]);
            }
            #endif
            #endif
            
            P("Fixed varvalmatching:" << varvalmatching);
            
            // now both varvalmatching and valvarmatching contain 
            // a complete matching for the SCC.
            // Also, valinlocalmatching contains the domain values
            // used in this SCC.
        }
        
        #ifdef ASSIGNOPT
        if(var_array[tempvar].isAssigned())  // Optimize the case where it is assigned.
        #else
        if(false)
        #endif
        {
            // Split tempvar off from the rest of the SCC
            if(SCCSplit.isMember(sccindex_start))
            {
                // Swap it to the front.
                //cout <<"Before swap:" <<SCCs<<endl;
                sccs_to_process.remove(sccindex_start);
                
                int swapvar=SCCs[sccindex_start];
                SCCs[sccindex_start]=SCCs[varToSCCIndex[tempvar]];
                SCCs[varToSCCIndex[tempvar]]=swapvar;
                
                varToSCCIndex[swapvar]=varToSCCIndex[tempvar];
                varToSCCIndex[tempvar]=sccindex_start;
                
                //cout <<"After swap:" <<SCCs <<endl;
                //cout <<"varToSCCIndex:" << varToSCCIndex <<endl;
                
                // Split the SCCs
                SCCSplit.remove(sccindex_start);
                D_DATA(((char*)SCCSplit2.get_ptr())[sccindex_start]=0);
                
                sccindex_start++;
                int tempval=var_array[tempvar].getAssignedValue();
                
                // Now remove the value from the reduced SCC
                for(int i=sccindex_start; i<=sccindex_end; i++)
                {
                    if(var_array[SCCs[i]].inDomain(tempval))
                    {
                        P("Removing var: "<< SCCs[i] << " val:" << tempval);
                        var_array[SCCs[i]].removeFromDomain(tempval);
                        #if UseIncGraph
                            adjlist_remove(SCCs[i], tempval);
                        #endif
                        if(getState(stateObj).isFailed()) return;
                    }
                }
                
                if(sccindex_start<sccindex_end)
                {
                    D_ASSERT(!sccs_to_process.in(sccindex_start));
                    sccs_to_process.insert(sccindex_start);
                }
            }
            // Else: The SCC is unit anyway. Should be no work to do, and do not insert into sccs_to_process.
            
        }
        else
        {
            // Not assigned, just some vals removed, so 
            if(!sccs_to_process.in(sccindex_start) && sccindex_start<sccindex_end)
            {
                sccs_to_process.insert(sccindex_start);
            }
        }
        
    } // end of loop.
    }
    
    
    #ifndef NO_DEBUG
    // Check the matching is valid.
    for(int i=0; i<numvars; i++)
    {
        D_ASSERT(var_array[i].inDomain(varvalmatching[i]));
        for(int j=i+1; j<numvars; j++)
        {
            D_ASSERT(varvalmatching[i]!=varvalmatching[j]);
            D_ASSERT(SCCs[i]!=SCCs[j]);
        }
        D_ASSERT(SCCSplit.isMember(i) == (((char*)SCCSplit2.get_ptr())[i]==1));
        D_ASSERT(SCCs[varToSCCIndex[i]]==i);
    }
    #endif
    
    // Call Tarjan's for each disturbed SCC.
    {
    vector<int> & toiterate=sccs_to_process.getlist();
    for(int i=0; i<toiterate.size(); i++)
    {
        int j=toiterate[i];
        
        // remake var_indices for this SCC.
        var_indices.clear();
        for(int k=j; k<numvars; k++)
        {
            #ifdef CHECKDOMSIZE
            if(!var_array[SCCs[k]].inDomain(varvalmatching[SCCs[k]]))
            {
                int l=j; while(SCCSplit.isMember(l) && l<(numvars-1)) l++;
                if(!matching_wrapper(j, l))
                    return;
            }
            #endif
            
            var_indices.push_back(SCCs[k]);
            if(!SCCSplit.isMember(k)) break;
        }
        
        //cout << "Running tarjan's on component "<< var_indices <<endl;
        tarjan_recursive(j);
    }
    }
    
    
    //  print components out
    
    /*
    cout<<"SCCs:"<<endl;
    for(int i=0; i<SCCs.size(); i++)
    {
        cout<< SCCs[i] <<endl;
    }
    cout<<"SCCSplit:"<<endl;
    for(int i=0; i<SCCs.size(); i++)
    {
        cout << ((SCCSplit.isMember(i))?"Nosplit":"Split") <<endl;
    }*/
    
    return;
  }
  
  // Simpler version which does not maintain SCCs and simply calls hopcroft and
  // visit for the whole set of variables.
  void do_prop_noscc()
  {
    PROP_INFO_ADDONE(AlldiffGacSlow);
    
    #ifdef DYNAMICALLDIFF
    bt_triggers_start=dynamic_trigger_start()+numvars;
    #endif
    
    #ifndef INCREMENTALMATCH
    // clear the matching.
    for(int i=0; i<numvars && i<numvals; i++)
    {
      varvalmatching[i]=i+dom_min;
      valvarmatching[i]=i;
    }
    #endif
    
    #ifdef PLONG
    cout << "Entering do_prop."<<endl;
    cout << "Varvalmatching:" <<varvalmatching<<endl;
    cout << "SCCs:" << SCCs <<endl;
    cout << "SCCSplit: ";
    for(int i=0; i<numvars; i++)
    {
        cout << (SCCSplit.isMember(i)?"1, ":"0, ");
    }
    cout <<endl;
    cout << "varToSCCIndex: "<< varToSCCIndex<<endl;
    cout << "Domains (remember that the var array is reversed):" <<endl;
    for(int i=0; i<numvars; i++)
    {
        cout << "var:" << i << " vals:" ;
        for(int j=dom_min; j<=dom_max; j++)
        {
            if(var_array[i].inDomain(j))
            {
                cout << j <<", ";
            }
        }
        cout << endl;
    }
    
    // Check the matching is valid.
    for(int i=0; i<numvars; i++)
    {
        if(!var_array[i].inDomain(varvalmatching[i]))
        {
            cout << "val in matching removed, var: " << i << ", val:" << varvalmatching[i] <<endl;
        }
        for(int j=i+1; j<numvars; j++)
        {
            D_ASSERT(varvalmatching[i]!=varvalmatching[j]);
            D_ASSERT(SCCs[i]!=SCCs[j]);
        }
        D_ASSERT(SCCSplit.isMember(i) == (((char*)SCCSplit2.get_ptr())[i]==1));
        cout << (int)SCCSplit.isMember(i) << ", " << (int) (((char*)SCCSplit2.get_ptr())[i]==1) << endl;
        // The matches correspond.
        D_ASSERT(valvarmatching[varvalmatching[i]-dom_min]==i);
    }
    
    #ifdef DYNAMICALLDIFF
    // Can't yet check all the watches are in the right place, but can check
    // there are the right number of them on each var.
    for(int i=0; i<var_array.size(); i++)
    {
        int count=0;
        DynamicTrigger* trig=get_dt(i, 0);
        while(trig->queue!=NULL && count<numvals)
        {
            count++;
            if(count<numvals) trig=get_dt(i,count);
        }
        D_ASSERT(count==watches[i].size());
    }
    #endif
    
    // Check that if an element of the matching is removed, then the var is 
    // in to_process.
    for(int i=0; i<var_array.size(); i++)
    {
        if(!var_array[i].inDomain(varvalmatching[i]))
        {
            D_ASSERT(to_process.in(i));
        }
    }
    #endif
    
    // Call hopcroft for the whole matching.
    if(!matching_wrapper(0, numvars-1))
        return;
    
    #ifdef DYNAMICALLDIFF
    // sync the watches to the matching, 
    // since the matching has just changed.
    #ifndef BTMATCHING
    DynamicTrigger * trig = dynamic_trigger_start();
    for(int j=0; j<numvars; j++)
    {
        var_array[j].addWatchTrigger(trig + j, DomainRemoval, varvalmatching[j]);
    }
    #endif
    #endif
    
    P("Fixed varvalmatching:" << varvalmatching);
    
    // now both varvalmatching and valvarmatching contain 
    // a complete matching for the SCC.
    // Also, valinlocalmatching contains the domain values
    // used in this SCC.
    
    #ifndef NO_DEBUG
    // Check the matching is valid.
    for(int i=0; i<numvars; i++)
    {
        D_ASSERT(var_array[i].inDomain(varvalmatching[i]));
        for(int j=i+1; j<numvars; j++)
        {
            D_ASSERT(varvalmatching[i]!=varvalmatching[j]);
            D_ASSERT(SCCs[i]!=SCCs[j]);
        }
        D_ASSERT(SCCSplit.isMember(i) == (((char*)SCCSplit2.get_ptr())[i]==1));
        D_ASSERT(SCCs[varToSCCIndex[i]]==i);
    }
    #endif
    
    
    // Call Tarjan's for all vars
    
    var_indices.clear();
    for(int i=0; i<numvars; i++) var_indices.push_back(i);
    
    if(numvars>0)
        tarjan_recursive(0);
    
    return;
  }
  
  virtual BOOL full_check_unsat()
  { 
    int v_size = var_array.size();
    for(int i = 0; i < v_size; ++i)
    {
      if(var_array[i].isAssigned())
      {
      
        for(int j = i + 1; j < v_size; ++j)
        {
          if(var_array[j].isAssigned())
          {
            if(var_array[i].getAssignedValue() == var_array[j].getAssignedValue())
              return true;
          }
        }
        
      }
    }
    
    return false;
  }
  
  virtual BOOL check_unsat(int i, DomainDelta)
  {
    int v_size = var_array.size();
    if(!var_array[i].isAssigned()) return false;
    
    DomainInt assign_val = var_array[i].getAssignedValue();
    for(int loop = 0; loop < v_size; ++loop)
    {
      if(loop != i)
      {
        if(var_array[loop].isAssigned() && 
           var_array[loop].getAssignedValue() == assign_val)
        return true;
      }
    }
    return false;
  }
  
  virtual void full_propagate()
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
        
        
        
        
      // Is this guaranteed to be called before do_prop is ever called??
      // I hope so, because the following test has to be done first.
      if(numvars>numvals)
      {
          getState(stateObj).setFailed(true);
          return;
      }
      
      // process all variables.
      to_process.clear();    // It seems like this is called twice at the top of the tree, so the clear is necessary.
      
      for(int i=0; i<numvars; i++)
      {
          to_process.insert(i);
      }
      
      #ifdef DYNAMICALLDIFF
        // Clear all the BT triggers. This is so that the tests at
        // the start of do_prop will work.
        bt_triggers_start=dynamic_trigger_start()+numvars;
        for(int i=0; i<var_indices.size(); i++)
        {
            int var=var_indices[i];
            for(int j=triggercount[var]; j<numvals; j++)
            {
                if( get_dt(var, j)->queue == NULL)
                {
                    break;
                }
                get_dt(var, j)->sleepDynamicTriggerBT(stateObj);
            }
        }
        
      // set the variable numbers in the watches.
      DynamicTrigger * trig=dynamic_trigger_start();
      for(int i=0; i<numvars; i++)
      {
          (trig+i)->trigger_info() = i;
      }
      for(int i=0; i<numvars; i++)
      {
          for(int j=0; j<numvals; j++)
          {
              (trig+numvars+(i*numvals)+j)->trigger_info() = i;
          }
      }
      
      #ifndef BTMATCHING
      // set the watches here so that they start synced to the varvalmatching.
        trig = dynamic_trigger_start();
        for(int j=0; j<numvars; j++)
        {
            var_array[j].addWatchTrigger(trig + j, DomainRemoval, varvalmatching[j]);
            P("Adding watch for var " << j << " val " << varvalmatching[j]);
        }
      #endif
      #endif
      
      #ifdef SCC
      do_prop();
      #else
      do_prop_noscc();
      #endif
  }
    
    virtual BOOL check_assignment(DomainInt* v, int array_size)
    {
      D_ASSERT(array_size == var_array.size());
      for(int i=0;i<array_size;i++)
        for( int j=i+1;j<array_size;j++)
          if(v[i]==v[j]) return false;
      return true;
    }
    
    virtual vector<AnyVarRef> get_vars()
    {
      vector<AnyVarRef> vars;
      vars.reserve(var_array.size());
      for(unsigned i = 0; i < var_array.size(); ++i)
        vars.push_back(var_array[i]);
      return vars;
    }
    
    
  virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  {
      bool matchok=true;
      for(int i=0; i<numvars; i++)
      {
          if(!var_array[i].inDomain(varvalmatching[i]))
          {
              matchok=false;
              break;
          }
      }
      
      if(!matchok)
      {
          if(numvals<numvars) return false; // there can't be a matching.
          #if UseIncGraph
            // update the adjacency lists.
            for(int i=dom_min; i<=dom_max; i++)
            {
                for(int j=0; j<adjlistlength[i-dom_min+numvars]; j++)
                {
                    int var=adjlist[i-dom_min+numvars][j];
                    if(!var_array[var].inDomain(i))
                    {
                        // swap with the last element and remove
                        adjlist_remove(var, i);
                        j--; // stay in the same place, dont' skip over the 
                        // value which was just swapped into the current position.
                    }
                }
            }
          #endif
          
          matchok=bfsmatching(0, numvars-1);
      }
      
      if(!matchok)
      {
          return false;
      }
      else
      {
          for(int i=0; i<numvars; i++)
          {
              assignment.push_back(make_pair(i, varvalmatching[i]));
          }
          return true;
      }
  }
    
    
};  // end of class


