


#include <stdlib.h>
#include <iostream>
#include <vector>
#include <deque>
#include <algorithm>
#include <utility>
#include "alldiff_gcc_shared.h"

#ifdef MINION_DEBUG
    #define GCCPRINT(x) cout << x << endl
#else
    #define GCCPRINT(x)
#endif

#define SPECIALQUEUE
#define SCC
#define INCREMENTALMATCH

template<typename VarArray1, typename VarArray2>
struct GCC : public AbstractConstraint
{
    GCC(StateObj* _stateObj, const VarArray1& _var_array, const VarArray2& _capacity_array, ConstraintBlob& b) : AbstractConstraint(_stateObj),
    stateObj(_stateObj), var_array(_var_array), capacity_array(_capacity_array), constraint_locked(false),
    numvals(count_values()),
    SCCSplit(_stateObj, numvars+numvals)
    {
        GCCPRINT("numvars:"<< numvars << ", numvals:"<< numvals);
        val_array = b.constants[0];
        
        D_ASSERT(capacity_array.size()==val_array.size());
        
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
    }
    
    VarArray1 var_array;   // primary variables
    VarArray2 capacity_array;   // capacities for values of interest
    vector<int> val_array;   // values of interest
    int dom_min, dom_max, numvars, numvals;
    
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
        numvars=var_array.size();  // number of main variables in the constraint
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
        #ifdef SCC
        for(int i=0; i<numvars+numvals; i++) to_process.insert(i);  // may need to change.
        do_gcc_prop_scc();
        #else
        do_gcc_prop();
        #endif
    }
    
    PROPAGATE_FUNCTION(int prop_var, DomainDelta)
    {
        if(!to_process.in(prop_var))
        {
            to_process.insert(prop_var);  // even inserts the bound events with trigger 1000000 + capvarid*2 + 1 if upperbound
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
    
    virtual void special_unlock() { constraint_locked = false;  } // to_process.clear();
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
        #endif
        
        // populate lower and upper
        for(int i=0; i<val_array.size(); i++)
        {
            if(val_array[i]>=dom_min && val_array[i]<=dom_max)
            {
                int capi=val_to_cap_index[val_array[i]-dom_min];
                if(capi>-1)
                {
                    lower[val_array[i]-dom_min]=capacity_array[capi].getMin();   // not quite right in the presence of duplicate values.
                    upper[val_array[i]-dom_min]=capacity_array[capi].getMax();
                }
            }
        }
        
        bool flag=bfsmatching_gcc();
<<<<<<< .mine
        GCCPRINT("matching:"<<flag);
=======
        D_DATA(cout << "matching:"<<flag<<endl);
>>>>>>> .r1630
        
        if(!flag)
        {
            getState(stateObj).setFailed(true);
            return;
        }
        
        tarjan_recursive(0);
        
        // now do basic prop from main vars to cap variables. equiv to occurrence constraints I think.
        // untested
        for(int i=0; i<val_array.size(); i++)
        {
            int val=val_array[i];
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
        }
        
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
                sccs_to_process.insert(sccindex_start);
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
                    vars_in_scc.push_back(sccval);
                }
                else
                {
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
            
            // now do basic prop from main vars to cap variables. equiv to occurrence constraints I think.
            // CAN'T do it here because there may be values in this scc which also occur elsewhere, assigned to some var.
            /*for(int scci=0; scci<vals_in_scc.size(); scci++)
            {
                int val=vals_in_scc[scci];
                int mincap=0;
                int maxcap=0;
                for(int var=0; var<numvars; var++)
                {
                    //int var=vars_in_scc[j];
                    if(var_array[var].inDomain(val))
                    {
                        maxcap++;
                        if(var_array[var].isAssigned())
                            mincap++;
                    }
                }
                capacity_array[val-dom_min].setMin(mincap);
                capacity_array[val-dom_min].setMax(maxcap);
            }*/
        }
        }
        
        // now do basic prop from main vars to cap variables. equiv to occurrence constraints I think.
        // untested
        for(int i=0; i<val_array.size(); i++)
        {
            int val=val_array[i];
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
        }
        
    }
    
    StateObj * stateObj;
    
    deque<int> fifo;
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
        matchbac=varvalmatching;
        usagebac=usage;
        
        // clear out unmatched variables
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
        
        // If the upper bounds have been changed since last call, it is possible that
        // the usage[val] of some value is greater than upper[val]. This is impossible
        // in the flow graph, so it must be corrected before we run the algorithm.
        // Some values in the matching are changed to blank (dom_min-1).
<<<<<<< .mine
        for(int valsccindex=0; valsccindex<vals_in_scc.size(); valsccindex++)
=======
        D_DATA(cout << varvalmatching << endl);
        D_DATA(cout << usage << endl);
        for(int valindex=0; valindex<=dom_max-dom_min; valindex++)
>>>>>>> .r1630
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
        
<<<<<<< .mine
=======
        int lowertotal=0;   // add up lower bounds
        for(int i=0; i<dom_max-dom_min; i++)
        {
            lowertotal+=lower[i];
        }
        if(lowertotal>numvars)
        {
            varvalmatching=matchbac;
            usage=usagebac;
            return false;
        }
        D_DATA(cout << "Lower:" <<lower<<endl);
>>>>>>> .r1630
        // iterate through the values looking for ones which are below their lower capacity bound. 
        for(int startvalsccindex=0; startvalsccindex<vals_in_scc.size(); startvalsccindex++)
        {
            int startvalindex=vals_in_scc[startvalsccindex]-dom_min;
            while(usage[startvalindex]<lower[startvalindex])
            {
                // usage of val needs to increase. Construct an augmenting path starting at val.
                int startval=startvalindex+dom_min;
                
                GCCPRINT("Searching for augmenting path for val: " << startval);
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
                        for(int vartoqueuescc=0; vartoqueuescc<vars_in_scc.size(); vartoqueuescc++)
                        {
                            int vartoqueue=vars_in_scc[vartoqueuescc];
                            // For each variable, check if it terminates an odd alternating path
                            // and also queue it if it is suitable.
                            if(!visited.in(vartoqueue)
                                && var_array[vartoqueue].inDomain(stackval) 
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
                    varvalmatching=matchbac;
                    usage=usagebac;
                    return false;
                }
                
            }  // end while below lower bound.
        } // end for each value
        
        // now search for augmenting paths for unmatched vars.
        
<<<<<<< .mine
        GCCPRINT("feasible matching (respects lower & upper bounds):"<<varvalmatching);
=======
        D_DATA(cout << "feasible matching (respects lower & upper bounds)"<<endl);
        D_DATA(cout << varvalmatching <<endl);
>>>>>>> .r1630
        
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
                        for(int valtoqueue=var_array[curnode].getMin(); valtoqueue<=var_array[curnode].getMax(); valtoqueue++)
                        {
                            // For each value, check if it terminates an odd alternating path
                            // and also queue it if it is suitable.
                            int validx=valtoqueue-dom_min+numvars;
                            if(valtoqueue!=varvalmatching[curnode]
                                && var_array[curnode].inDomain(valtoqueue)
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
                        for(int vartoqueuescc=0; vartoqueuescc<vars_in_scc.size(); vartoqueuescc++)
                        {
                            int vartoqueue=vars_in_scc[vartoqueuescc];
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
                    varvalmatching=matchbac;   // no need for this.
                    usage=usagebac;
                    return false;
                }
            }
        }
        
<<<<<<< .mine
        GCCPRINT("maximum matching:" << varvalmatching);
=======
        D_DATA(cout << "maximum matching:"<<endl);
        D_DATA(cout << varvalmatching <<endl);
        
>>>>>>> .r1630
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
        int array_size = var_array.size();
        int capacity_size=capacity_array.size();
        for(int i = 0; i < array_size; ++i)
        {
            t.push_back(make_trigger(var_array[i], Trigger(this, i), DomainChanged));
        }
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
      for(int i=0; i<numvals; i++) augpath[i]=0;
      for(int i=0; i<numvars; i++)
      {   // count the values.
          augpath[v[i]-dom_min]++;
      }
      for(int i=0; i<val_array.size(); i++)
      {
          int val=val_array[i];
          if(val>=dom_min && val<=dom_max && v[i+numvars]!=augpath[val-dom_min])
              return false;
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
    
    //smallset sccs_to_process;   // Indices to the first var in the SCC to process.
    smallset valinlocalmatching;
    smallset varinlocalmatching;
    
    int varcount;
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
                varcount=0;
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
            for(int i=0; i<vars_in_scc.size(); i++)
            {
                int newnode=vars_in_scc[i];
                if(varvalmatching[newnode]!=curnode-numvars+dom_min)   // if the value is not in the matching.
                {
                    if(var_array[newnode].inDomain(curnode+dom_min-numvars))
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
            // !toplevel . Or perhaps we didn't traverse all the variables.
            // I think these two cases cover everything.
            if(!toplevel || varcount<vars_in_scc.size())
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
                    
                                        var_array[curvar].removeFromDomain(copynode+dom_min-numvars);
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
};
