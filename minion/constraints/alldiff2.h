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
    
    
    vector<int>* bfs(int start, int end)
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
    
    
    // This is a mess -- must be a better way
    void apply_path_fraction(vector<int>& path)
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
        
        augamount=augamount/2.0;
        
        cout << "Augmenting flow by: " << augamount <<endl;
        
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
            }
            else if(from<numvars)
            {
                if(to==numvars+numvals)
                {
                    flow_s_var[from]-=augamount;
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
                }
            }
            
        }
        
    }
    
    
    void print_flow()
    {
        double checktotalflow=0.0;
        for(int var=0; var<numvars; var++)
        {
            for(int val=var_array[var].getMin(); val<=var_array[var].getMax(); val++)
            {
                if(var_array[var].inDomain(val))
                {
                    cout << "x" << var << "=" <<val << ", flow: " << flow_var_val[var][val-dom_min] <<endl;
                    checktotalflow+=flow_var_val[var][val-dom_min];
                }
            }
        }
        
        cout << "Total flow:" << totalflow<<endl;
        cout << "Total flow check:" << checktotalflow<<endl;
        D_ASSERT(totalflow==checktotalflow);
    }
    
    // put the prop algo in here.
    
    void do_initial_prop()
    {
        // clear the flow netowrk.
        totalflow=0.0;
        flow_var_val.resize(numvars);
        for(int i=0; i<numvars; i++)
        {
            flow_var_val[i].resize(0);
            flow_var_val[i].resize(numvals, 0.0);
        }
        flow_val_t.resize(0);
        flow_val_t.resize(numvals, 0.0);
        flow_s_var.resize(0);
        flow_s_var.resize(numvars, 0.0);
        
        vector<int>* augpath=bfs(numvars+numvals, numvars+numvals+1);
        while(augpath.size()!=0)
        {
            apply_path_max_zeros(augpath);
            augpath=bfs(numvars+numvals, numvars+numvals+1);
        }
        
        print_flow();
        
        
        
    }
    
    
    void do_prop()
    {
        do_initial_prop();
        
        
        
    }
    
    
    
  
  // Should only be used in non-dynamic version.
  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    int array_size = var_array.size();
    for(int i = 0; i < array_size; ++i)
      t.push_back(make_trigger(var_array[i], Trigger(this, i), DomainChanged));
    return t;
  }
  
  
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
  
  
  virtual void propagate(int prop_var, DomainDelta)
  {
    D_ASSERT(prop_var>=0 && prop_var<var_array.size());
    
    #ifdef STAGED
    if(var_array[prop_var].isAssigned())
    {
        int assignedval=var_array[prop_var].getAssignedValue();
        for(int i=0; i<numvars; i++)
        {
            if(i!=prop_var && var_array[i].inDomain(assignedval))
            {
                var_array[i].removeFromDomain(assignedval);
                adjlist_remove(i, assignedval);
            }
        }
    }
    #endif
    
    // Go thruogh adjacency list and update.
    for(int i=0; i<adjlistlength[prop_var]; i++)
    {
        if(!var_array[prop_var].inDomain(adjlist[i]))
        {
            adjlist_remove(prop_var, adjlist[i]);
            i--;
        }
    }
    
    
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
        do_prop();
        #endif
    }
  }
  
  /*
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
  */
  
  virtual void special_unlock() { constraint_locked = false; to_process.clear(); }
  virtual void special_check()
  {
    constraint_locked = false;
    
    if(getState(stateObj).isFailed())
    {
        to_process.clear();
        return;
    }
    
    do_prop();
    to_process.clear();
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
                    //DynamicTrigger* mydt= dt+(var*numvals)+(i-dom_min);
                    //var_array[var].addDynamicTrigger(mydt, DomainRemoval, i);
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
      
      do_prop();
      
      
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
    
    /*
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
    */
    
};  // end of class


