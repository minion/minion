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

#ifndef GCC_MULTILAYER_H
#define GCC_MULTILAYER_H

#include <stdlib.h>
#include <iostream>
#include <vector>
#include <deque>
#include <algorithm>
#include <utility>
#include "gcc_common.h"

//#define GCCPRINT(x) cout << x << endl
#define GCCPRINT(x)

#define SPECIALQUEUE
#define SCC
#define INCREMENTALMATCH

//#define SCCCARDS

//Incremental graph -- maintains adjacency lists for values and vars
#define INCGRAPH

// Does not trigger itself if this is on, and incgraph is on.
#define ONECALL

// use the algorithm from Quimper et al. to prune the target variables.
// requires INCGRAPH and not SCC
//#define QUIMPER

//#define CAPBOUNDSCACHE

// Note on semantics: GCC only restricts those values which are 'of interest',
// it does not put any restriction on the number of other values. 


template<typename VarArray, typename CapArray>
struct GCCMultiLayer : public FlowConstraint<VarArray, true>
{
    using FlowConstraint<VarArray, true>::stateObj;
    using FlowConstraint<VarArray, true>::constraint_locked;
    
    using FlowConstraint<VarArray, true>::adjlist;
    using FlowConstraint<VarArray, true>::adjlistlength;
    using FlowConstraint<VarArray, true>::adjlistpos;
    using FlowConstraint<VarArray, true>::adjlist_remove;
    
    using FlowConstraint<VarArray, true>::dynamic_trigger_start;
    using FlowConstraint<VarArray, true>::var_array;
    using FlowConstraint<VarArray, true>::dom_min;
    using FlowConstraint<VarArray, true>::dom_max;
    using FlowConstraint<VarArray, true>::numvars;
    using FlowConstraint<VarArray, true>::numvals;
    using FlowConstraint<VarArray, true>::varvalmatching;
    
    using FlowConstraint<VarArray, true>::varinlocalmatching;
    using FlowConstraint<VarArray, true>::valinlocalmatching;
    using FlowConstraint<VarArray, true>::invprevious;
    
    using FlowConstraint<VarArray, true>::initialize_hopcroft;
    
    using FlowConstraint<VarArray, true>::hopcroft2_setup;
    using FlowConstraint<VarArray, true>::hopcroft_wrapper2;
    using FlowConstraint<VarArray, true>::hopcroft2;
    
    GCCMultiLayer(StateObj* _stateObj, const VarArray& _var_array, const CapArray& _capacity_array, vector<int> _val_array, vector<int> _varsubsetindices) : 
    FlowConstraint<VarArray, UseIncGraph>(_stateObj, _var_array),
    capacity_array(_capacity_array), val_array(_val_array), varsubsetindices(_varsubsetindices),
    SCCSplit(_stateObj, numvars+numvals)
    {
        D_ASSERT(capacity_array.size()==val_array.size());
        
        for(int i=0; i<val_array.size(); i++)
        {
            for(int j=i+1; j<val_array.size(); j++)
            {
                D_ASSERT(val_array[i]!=val_array[j]);
            }
        }
        usage.resize(numvals, 0);
        
        lower.resize(numvals, 0);
        upper.resize(numvals, numvars);
        
        prev.resize(numvars+numvals);
        
        initialize_hopcroft();
        initialize_tarjan();
        
        // count subsets
        D_ASSERT(varsubsetindices.size()==var_array.size());
        numsubsets=0;
        for(int i=0; i<varsubsetindices.size(); i++)
        {
            D_ASSERT(varsubsetindices[i]>=0);
            if(varsubsetindices[i]+1>numsubsets)
                numsubsets=varsubsetindices[i]+1;
        }
        var_subsets.resize(numsubsets);
        var_subsets_idx.resize(numsubsets);
        for(int i=0; i<numvars; i++)
        {
            var_subsets[varsubsetindices[i]].push_back(var_array[i]);
            var_subsets_idx[varsubsetindices[i]].push_back(i);
        }
        cap_subsets.resize(numsubsets);
        
        // copy blocks of size val_array.size() from capacity_array to cap_subsets
        for(int i=0; i<numsubsets; i++)
        {
            for(int j=0; j<val_array.size(); j++)
            {
                cap_subsets[i].push_back(capacity_array[i*val_array.size()+j]);
            }
        }
        // copy the last block of capacity variables
        for(int i=numsubsets*val_array.size(); i<capacity_array.size(); i++)
        {
            cap_allvars.push_back(capacity_array[i]);
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
        
        #ifdef CAPBOUNDSCACHE
        boundsupported.resize(numvals*2, -1);  
        // does the bound need to be updated? Indexed as validx*2 for lowerbound, validx*2+1 for ub
        // Contains the capacity value which is supported. Reset to -1 if the support is lost.
        #endif
        
        for(int i=0; i<numvars; i++)
            varvalmatching[i]=dom_min-1;
    }
    
    CapArray capacity_array;   // capacities for values of interest
    
    
    int numsubsets;
    vector<VarArray> var_subsets;
    vector<vector<int> > var_subsets_idx;
    vector<CapArray> cap_subsets;   // value capacities for subsets of vars.
    vector<int> varsubsetindices;
    
    CapArray cap_allvars;   // value capacities for all vars.
    
    vector<int> val_array;   // values of interest
    
    vector<int> val_to_cap_index;
    
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
    
    virtual void propagate(int prop_var, DomainDelta)
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
    
    virtual void propagate(DynamicTrigger* trig)
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
            D_ASSERT(trig>= dtstart && trig<dtstart+(2*val_array.size()*(numvars+val_array.size())) );
            boundsupported[trig->trigger_info()]=-1;
        }
        #endif
    }
    
    #ifdef CAPBOUNDSCACHE
    vector<int> boundsupported;  // does the bound need to be updated? Indexed as validx*2 for lowerbound, validx*2+1 for ub
    // Contains the capacity value which is supported. Reset to -1 if the support is lost.
    #endif
    
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
    do_gcc_prop_scc();
    #else
    do_gcc_prop();
    #endif
    
  }
  
  
    void do_gcc_prop()
    {
        // find/ repair the matching.
        #ifndef INCREMENTALMATCH
        varvalmatching.resize(0);
        varvalmatching.resize(numvars, dom_min-1);
        usage.clear();
        usage.resize(numvals, 0);
        usage_subset.clear();
        usage_subset.resize(numsubsets);
        for(int i=0; i<numsubsets; i++) usage_subset[i].resize(numvals);
        #endif
        
        // populate lower and upper
        // should we stop using lower and upper -- just read the card vars??
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
        
        bool flag=bfsmatching_multilayer_gcc();
        GCCPRINT("matching:"<<flag);
        
        if(!flag)
        {
            getState(stateObj).setFailed(true);
            return;
        }
        
        tarjan_recursive(0, upper, lower, varvalmatching, usage);
        
    }
    
    smallset sccs_to_process;
    
    
    deque<int> fifo;
    // deque_fixed_size was not faster.
    //deque_fixed_size fifo;
    vector<int> prev;
    
    vector<int> matchbac;
    
    vector<int> lower;
    vector<int> upper;
    vector<int> usage;
    
    vector<vector<int> > usage_subset;
    
    vector<int> usagebac;
    
    // Incremental SCC data.
    vector<int> SCCs;    // Variable numbers and values as val-dom_min+numvars
    ReversibleMonotonicSet SCCSplit;
    
    vector<int> varToSCCIndex;  // Mirror of the SCCs array.
    
    smallset to_process;
    
    // return value is whether or not an aug path was found and applied. 
    bool bfs(int startnode)
    {
        // startnode is required to pass into apply_augmenting_path.
        while(!fifo.empty())
        {
            // pop a vertex and expand it.
            int curnode=fifo.front();
            fifo.pop_front();
            GCCPRINT("Popped vertex " << (curnode<numvars? "(var)":"(val)") << (curnode<numvars? curnode : curnode+dom_min-numvars ));
            if(curnode<numvars)
            { // it's a variable
                // edges are vals in domain except the matching edge.
                int subset=varsubsetindices[curnode];
                for(int vali=0; vali<adjlistlength[curnode]; vali++)
                {
                    int val=adjlist[curnode][vali];
                    if(varvalmatching[curnode]!=val)
                    {
                        int validx=numvars+subset*numvals+val-dom_min;
                        prev[validx]=curnode;
                        fifo.push_back(validx);
                        visited.insert(validx);
                    }
                }
            }
            else if(curnode < numvars+(numsubsets*numvals))
            {
                // middle value layer.
                // edges are matching edges to the var layer, and
                // edges with capacity to the other value layer.
                int tmp=curnode-numvars;
                int subset=tmp / numvals;
                int validx=tmp % numvals;
                int val= validx+dom_min;
                // matching edges first.
                if(usage_subset[subset][validx]>0)
                {
                    for(int i=0; i<var_subsets_idx[subset].size(); i++)
                    {
                        int var=var_subsets_idx[subset][i];
                        if(varvalmatching[var]==val)
                        {
                            prev[var]=curnode;
                            fifo.push_back(var);
                            visited.insert(var);
                        }
                    }
                }
                
                // other value layer
                if(usage_subset[subset][validx]<cap_subsets[subset][validx].getMax())
                {
                    int newnode=validx+numvars+(numsubsets*numvals);
                    prev[newnode]=curnode;
                    fifo.push_back(newnode);
                    visited.insert(newnode);
                }
            }
            else
            {
                D_ASSERT(curnode < numvars+(numsubsets*numvals)+numvals);
                // value layer adjacent to t.
                int validx=curnode-numvars-(numsubsets*numvals);
                int val=validx+dom_min;
                // if adjacent to t, we are done.
                if(usage[validx]<cap_allvars[validx].getMax())
                {
                    apply_augmenting_path(curnode, startnode);
                    return true;
                }
                
                // edges to middle value layer
                for(int subset=0; subset<numsubsets; subset++)
                {
                    if(usage_subset[subset][validx]>cap_subsets[subset][validx].getMin())
                    {
                        int newnode=validx+numvars+(subset*numvals);
                        prev[newnode]=curnode;
                        fifo.push_back(newnode);
                        visited.insert(newnode);
                    }
                }
            }
        }
        // did not find an augmenting path.
        return false;
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
                // it's a variable
                // Next has to be a middle-layer value.
                int var=augpath[i];
                int subset=varsubsetindices[var];
                int validx=augpath[i+1]-numvars-(subset*numvals);
                
                D_ASSERT(varvalmatching[var]==dom_min-1);
                varvalmatching[var]=validx+dom_min;
                D_ASSERT(var_array.inDomain(varvalmatching[var]));
                usage[validx]++;
                usage_subset[subset][validx]++;
            }
            else if(augpath[i]<(numvars+(numsubsets*numvals)))
            {   // it's a middle layer value. check if it has a var next
                // otherwise ignore, matching does not need to change. 
                if(augpath[i+1]<numvars)
                {
                    int tmp=augpath[i]-numvars;
                    int subset=tmp / numvals;
                    int validx=tmp % numvals;
                    D_ASSERT(augpath[i]>=numvars && augpath[i]<numvars+numvals);
                    varvalmatching[augpath[i+1]]=dom_min-1;
                    usage[validx]--;
                    usage_subset[subset][validx]--;
                }
            }
            // ignores the other value layer entirely.
        }
        
        GCCPRINT("varvalmatching:" << varvalmatching);
    }
    
    inline bool bfsmatching_multilayer_gcc()
    {
        // Assumes incgraph is in use.
        
        // to start with, assume the matching is blank.
        
        // lower and upper are indexed by value-dom_min and provide the capacities.
        // usage is the number of times a value is used in the matching.
        
        
        // If the upper bounds have been changed since last call, it is possible that
        // the usage[val] of some value is greater than upper[val]. This is impossible
        // in the flow graph, so it must be corrected before we run the algorithm.
        // Some values in the matching are changed to blank (dom_min-1).
        for(int valsccindex=0; valsccindex<vals_in_scc.size(); valsccindex++)         // needs adapting.
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
        
        // iterate through the subsets of vars, fixing lowerbounds where necessary.
        for(int subset=0; subset<numsubsets; subset++)
        {
            for(int validx=0; validx<val_array.size(); validx++)
            {
                int value=val_array[validx];
                if(usage_subset[subset][value-dom_min]<cap_subsets[subset][validx].getMin())
                {
                    // find an augmenting path starting with a variable in the subset
                    // and value. 
                    // Check that the val is in domain.
                    // If trying to use a variable that is already assigned,
                    // must un-assign it first and fix the usages..
                    
                    
                    for(int vari=0; vari<var_subsets_idx[subset].size(); vari++)
                    {
                        int var=var_subsets_idx[vari];
                        fifo.clear();
                        visited.clear();
                        visited.insert(var);
                        bool finished=false;
                        prev[numvars+numvals*subset+value-dom_min]=var;
                        fifo.push_back(numvars+numvals*subset+value-dom_min);
                        
                        if(bfs(
                            
                            // but what about aug paths where val is not the second vertex?
                        
                    }
                }
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
                        for(int vartoqueuei=0; vartoqueuei<adjlistlength[curnode]; vartoqueuei++)
                        {
                            int vartoqueue=adjlist[curnode][vartoqueuei];
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
    
    
    
    virtual string constraint_name()
    {
      return "GCCMultiLayer";
    }
    
    virtual triggerCollection setup_internal()
    {
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
        //varinlocalmatching.reserve(numvars);
    }
    
    void tarjan_recursive(int sccindex_start,
        vector<int>& upper, 
        vector<int>& lower, vector<int>& matching, vector<int>& usage)
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
                visit(curnode, true, upper, lower, matching, usage);
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
                visit(curnode, true, upper, lower, matching, usage);
                GCCPRINT("Returned from tarjan's algorithm.");
            }
        }
    }
    
    void visit(int curnode, bool toplevel, vector<int>& upper, vector<int>& lower, vector<int>& matching, vector<int>& usage)
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
                        visit(newnode, false, upper, lower, matching, usage);
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
            int newnode=matching[curnode]-dom_min+numvars;
            D_ASSERT(var_array[curnode].inDomain(newnode+dom_min-numvars));
            
            if(!visited.in(newnode))
            {
                visit(newnode, false, upper, lower, matching, usage);
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
                if(matching[newnode]!=curnode-numvars+dom_min)   // if the value is not in the matching.
                {
                    #ifndef INCGRAPH
                    if(var_array[newnode].inDomain(curnode+dom_min-numvars))
                    #endif
                    {
                        //newnode=varvalmatching[newnode]-dom_min+numvars;  // Changed here for merge nodes
                        if(!visited.in(newnode))
                        {
                            
                            visit(newnode, false, upper, lower, matching, usage);
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
                    visit(newnode, false, upper, lower, matching, usage);
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
                                    if(matching[curvar]!=copynode+dom_min-numvars)
                                    {
                                        GCCPRINT("Removing var: "<< curvar << " val:" << copynode+dom_min-numvars);
                                        if(var_array[curvar].inDomain(copynode+dom_min-numvars))
                                        {
                                            var_array[curvar].removeFromDomain(copynode+dom_min-numvars);
                                            #ifdef INCGRAPH
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
    
    typedef typename CapArray::value_type CapVarRef;
    virtual AbstractConstraint* reverse_constraint()
    {
        // use a watched-or of NotOccurrenceEqualConstraint, i.e. the negation of occurrence
        vector<AbstractConstraint*> con;
        for(int i=0; i<capacity_array.size(); i++)
        {
            NotOccurrenceEqualConstraint<VarArray, DomainInt, CapVarRef>*
                t=new NotOccurrenceEqualConstraint<VarArray, DomainInt, CapVarRef>(
                    stateObj, var_array, val_array[i], capacity_array[i]);
            con.push_back((AbstractConstraint*) t);
        }
        return new Dynamic_OR(stateObj, con);
    }
};

#endif
