


#include <stdlib.h>
#include <iostream>
#include <vector>
#include <deque>
#include <algorithm>
#include <utility>
#include "alldiff_common.h"

template<typename VarArray1, typename VarArray2>
struct GCC : public AbstractConstraint
{
    GCC(StateObj* _stateObj, const VarArray1& _var_array, const VarArray2& _capacity_array) : AbstractConstraint(_stateObj),
    stateObj(_stateObj), var_array(_var_array), capacity_array(_capacity_array), constraint_locked(false),
    SCCSplit(_stateObj, _var_array.size()), SCCSplitVals(_stateObj, _capacity_array.size())
    {
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
        numvals=dom_max-dom_min+1;
        if(capacity_array.size()>numvals)
        {
            numvals=capacity_array.size();
        }
        D_ASSERT(capacity_array.size()==numvals);
        
        varvalmatching.resize(numvars, dom_min-1);
        usage.resize(numvals, 0);
        
        lower.resize(numvals);
        upper.resize(numvals);
        
        prev.resize(numvars+numvals);
        initialize_tarjan();
    }
    
    VarArray1 var_array;
    VarArray2 capacity_array;
    
    bool constraint_locked;
    
    virtual void full_propagate()
    {
        D_ASSERT(capacity_array.size()==numvals);
        for(int i=0; i<numvals; i++)
        {
            capacity_array[i].setMin(0);
            capacity_array[i].setMax(numvars);
        }
        do_gcc_prop();
    }
    
    PROPAGATE_FUNCTION(int prop_var, DomainDelta)
    {
        if(!constraint_locked)
        {
            constraint_locked = true;
            getQueue(stateObj).pushSpecialTrigger(this);
        }
    }
    
    virtual void special_unlock() { constraint_locked = false;  } // to_process.clear();
  virtual void special_check()
  {
    constraint_locked = false;  // should be above the if.
    
    if(getState(stateObj).isFailed())
    {
        //to_process.clear();
        return;
    }
    
    do_gcc_prop();
  }
    
    void do_gcc_prop()
    {
        // find/ repair the matching.
        
        // this should not be necessary -- remove eventually
        //varvalmatching.resize(0);
        //varvalmatching.resize(numvars, dom_min-1);
        
        // populate lower and upper
        for(int i=0; i<numvals; i++)
        {
            lower[i]=capacity_array[i].getMin();
            upper[i]=capacity_array[i].getMax();
            //usage[i]=0;
        }
        
        bool flag=bfsmatching_gcc();
        cout << "matching:"<<flag<<endl;
        
        if(!flag)
        {
            getState(stateObj).setFailed(true);
            return;
        }
        
        var_indices.clear();
        for(int i=0; i<numvars; i++)
        {
            var_indices.push_back(i);
        }
        
        tarjan_recursive(0);
        
        // now do basic prop from main vars to cap variables. equiv to occurrence constraints I think.
        // untested
        for(int i=0; i<numvals; i++)
        {
            int val=i+dom_min;
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
    
    int dom_min, dom_max, numvars, numvals;
    deque<int> fifo;
    vector<int> prev;
    
    vector<int> matchbac;
    
    vector<int> lower;
    vector<int> upper;
    vector<int> usage;
    vector<int> usagebac;
    vector<int> varvalmatching;
    
    // Incremental SCC data.
    // For variables.
    vector<int> SCCs;    // Variable numbers
    ReversibleMonotonicSet SCCSplit;
    
    vector<int> varToSCCIndex;  // Mirror of the SCCs array.
    
    // For values.
    vector<int> SCCsVals;    // Values
    ReversibleMonotonicSet SCCSplitVals;
    vector<int> valToSCCIndex;  // value-dom_min -> index in SCCsVals.
    
    
    inline bool bfsmatching_gcc()
    {
        // lower and upper are indexed by value-dom_min and provide the capacities.
        // usage is the number of times a value is used in the matching.
        
        // back up the matching to cover failure
        matchbac=varvalmatching;
        usagebac=usage;
        
        // clear out unmatched variables
        for(int i=0; i<numvars; i++)
        {
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
        cout << varvalmatching << endl;
        cout << usage << endl;
        for(int valindex=0; valindex<=dom_max-dom_min; valindex++)
        {
            if(usage[valindex]>upper[valindex] && upper[valindex]>=0)
            {
                for(int i=0; i<numvars && usage[valindex]>upper[valindex]; i++)
                {
                    if(varvalmatching[i]==valindex+dom_min)
                    {
                        varvalmatching[i]=dom_min-1;
                        usage[valindex]--;
                    }
                }
                D_ASSERT(usage[valindex]<=upper[valindex]);
            }
        }
        
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
        cout << "Lower:" <<lower<<endl;
        // iterate through the values looking for ones which are below their lower capacity bound. 
        for(int startvalindex=0; startvalindex<numvals; startvalindex++)
        {
            while(usage[startvalindex]<lower[startvalindex])
            {
                // usage of val needs to increase. Construct an augmenting path starting at val.
                int startval=startvalindex+dom_min;
                
                D_DATA(cout << "Searching for augmenting path for val: " << startval <<endl );
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
                    D_DATA(cout << "Popped vertex " << (curnode<numvars? "(var)":"(val)") << (curnode<numvars? curnode : curnode+dom_min-numvars ) <<endl);
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
                        for(int vartoqueue=0; vartoqueue<numvars; vartoqueue++)
                        {
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
                    D_DATA(cout << "No augmenting path found."<<endl);
                    // restore the matching to its state before the algo was called.
                    varvalmatching=matchbac;
                    usage=usagebac;
                    return false;
                }
                
            }  // end while below lower bound.
        } // end for each value
        
        // now search for augmenting paths for unmatched vars.
        
        cout << "feasible matching (respects lower & upper bounds)"<<endl;
        cout << varvalmatching <<endl;
        
        // Flip the graph around, so it's like the alldiff case now. 
        // follow an edge in the matching from a value to a variable,
        // follow edges not in the matching from variables to values. 
        
        for(int startvar=0; startvar<numvars; startvar++)
        {
            if(varvalmatching[startvar]==dom_min-1)
            {
            // attempt to find an augmenting path starting with startvar.
                D_DATA(cout << "Searching for augmenting path for var: " << startvar <<endl );
                
                // done up to here.
                
                // Matching edge lost; BFS search for augmenting path to fix it.
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
                    D_DATA(cout << "Popped vertex " << (curnode<numvars? "(var)":"(val)") << (curnode<numvars? curnode : curnode+dom_min-numvars ) <<endl);
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
                        for(int vartoqueue=0; vartoqueue<numvars; vartoqueue++)
                        {
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
                    D_DATA(cout << "No augmenting path found."<<endl);
                    // restore the matching to its state before the algo was called.
                    varvalmatching=matchbac;
                    usage=usagebac;
                    return false;
                }
            }
        }
        
        cout << "maximum matching:"<<endl;
        cout << varvalmatching <<endl;
        
        return true;
    }
    
    inline void apply_augmenting_path(int unwindnode, int startnode)
    {
        D_DATA(cout << "Found augmenting path:" <<endl);
        vector<int> augpath;
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
        
        D_DATA(cout << "augpath:" << augpath<<endl);
        
        // now apply the path.
        for(int i=0; i<augpath.size()-1; i++)
        {
            if(augpath[i]<numvars)
            {
                // if it's a variable
                //D_ASSERT(varvalmatching[augpath[i]]==dom_min-1);
                // varvalmatching[augpath[i]]=dom_min-1; Can't do this, it would overwrite the correct value.
                D_DATA(cout << "decrementing usage for value " << augpath[i+1]-numvars+dom_min <<endl);
                usage[augpath[i+1]-numvars]--;
            }
            else
            {   // it's a value.
                D_ASSERT(augpath[i]>=numvars && augpath[i]<numvars+numvals);
                varvalmatching[augpath[i+1]]=augpath[i]-numvars+dom_min;
                D_DATA(cout << "incrementing usage for value " << augpath[i]-numvars+dom_min <<endl);
                usage[augpath[i]-numvars]++;
            }
        }
        
        
        #ifndef NO_DEBUG
        cout << "varvalmatching:";
        for(int i=0; i<numvars; i++)
        {
            if(var_array[i].inDomain(varvalmatching[i]))
                cout << i << "->" << varvalmatching[i] << ", ";
        }
        cout << endl;
        #endif
    }
    
    inline void apply_augmenting_path_reverse(int unwindnode, int startnode)
    {
        D_DATA(cout << "Found augmenting path:" <<endl);
        vector<int> augpath;
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
        
        
        #ifndef NO_DEBUG
        cout << "augpath:" << augpath<<endl;
        cout << "varvalmatching:";
        for(int i=0; i<numvars; i++)
        {
            if(var_array[i].inDomain(varvalmatching[i]))
                cout << i << "->" << varvalmatching[i] << ", ";
        }
        cout << endl;
        #endif
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
          t.push_back(make_trigger(var_array[i], Trigger(this, i), DomainChanged));
        for(int i=0; i< capacity_size; ++i)
        {
            t.push_back(make_trigger(capacity_array[i], Trigger(this, i), UpperBound));
            t.push_back(make_trigger(capacity_array[i], Trigger(this, i), LowerBound));
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
      vector<int> occ;
      occ.resize(numvals,0);
      for(int i=0; i<numvars; i++)
      {   // count the values.
          occ[v[i]-dom_min]++;
      }
      for(int i=dom_min; i<=dom_max; i++)
      {
          if(v[i+numvars-dom_min]!=occ[i-dom_min])
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
    
    vector<int> spare_values;
    bool include_sink;
    vector<int> var_indices;  // Should be a pointer so it can be changed.
    
    //smallset sccs_to_process;   // Indices to the first var in the SCC to process.
    smallset valinlocalmatching;
    smallset varinlocalmatching;
    
    int varcount;
    int localmin,localmax;
    
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
        //valinlocalmatching.clear();
        
        localmax=var_array[var_indices[0]].getMax();
        localmin=var_array[var_indices[0]].getMin();
        //valinlocalmatching.insert(varvalmatching[var_indices[0]]-dom_min);
        
        for(int i=1; i<var_indices.size(); i++)
        {
            int tempvar=var_indices[i];
            int tempmax=var_array[tempvar].getMax();
            int tempmin=var_array[tempvar].getMin();
            if(tempmax>localmax) localmax=tempmax;
            if(tempmin<localmin) localmin=tempmin;
            //valinlocalmatching.insert(varvalmatching[var_indices[i]]-dom_min);
        }
        
        include_sink=true;
        
        tstack.clear();
        in_tstack.clear();
        
        visited.clear();
        max_dfs=1;
        
        scc_split=false;
        sccindex=sccindex_start;
        
        for(int i=0; i<var_indices.size(); ++i)
        {
            int curnode=var_indices[i];
            if(!visited.in(curnode))
            {
                D_DATA(cout << "(Re)starting tarjan's algorithm, value:"<< curnode <<endl);
                varcount=0;
                visit(curnode);
                D_DATA(cout << "Returned from tarjan's algorithm." << endl);
            }
        }
        
    }
    
    void visit(int curnode)
    {
        tstack.push_back(curnode);
        in_tstack.insert(curnode);
        dfsnum[curnode]=max_dfs;
        lowlink[curnode]=max_dfs;
        max_dfs++;
        visited.insert(curnode);
        //cout << "Visiting node: " <<curnode<<endl;
        
        if(curnode==numvars+numvals)
        {
            //cout << "Visiting sink node." <<endl;
            D_ASSERT(include_sink);
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
            for(int i=localmin; i<=localmax; i++)
            {
                int newnode=i+numvars-dom_min;
                if(usage[i-dom_min]<upper[i-dom_min])
                {
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
                }
            }
        }
        else if(curnode<numvars)  // This case should never occur with merge nodes.
        {
            D_ASSERT(find(var_indices.begin(), var_indices.end(), curnode)!=var_indices.end());
            varcount++;
            //cout << "Visiting node variable: "<< curnode<<endl;
            int newnode=varvalmatching[curnode]-dom_min+numvars;
            D_ASSERT(var_array[curnode].inDomain(newnode+dom_min-numvars));
            
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
            for(int i=0; i<var_indices.size(); i++)
            {
                if(var_array[var_indices[i]].inDomain(curnode+dom_min-numvars))
                {
                    found=true;
                }
            }
            // D_ASSERT(found);  // it is safe to take out this test. But how did we get to this value?
            #endif
            
            int lowlinkvar=-1;
            for(int i=0; i<var_indices.size(); i++)
            {
                int newnode=var_indices[i];
                if(varvalmatching[newnode]!=curnode-numvars+dom_min)   // if the value is not in the matching.
                {
                    if(var_array[newnode].inDomain(curnode+dom_min-numvars))
                    {
                        //newnode=varvalmatching[newnode]-dom_min+numvars;  // Changed here for merge nodes
                        if(!visited.in(newnode))
                        {
                            
                            visit(newnode);
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
            
            
            
            // Why the find? Can we not use some DS which is already lying around?
            // And why does it include the whole matching in the find??
            if(include_sink 
                && usage[curnode-numvars]>lower[curnode-numvars])  // adaptation for GCC instead of the following comment.
            //valinlocalmatching.in(curnode-numvars))
            //    find(varvalmatching.begin(), varvalmatching.end(), curnode+dom_min-numvars)!=varvalmatching.end())
            {
                int newnode=numvars+numvals;
                if(!visited.in(newnode))
                {
                    visit(newnode);
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
            // Then lowlink[curnode]!=1. Or perhaps we didn't traverse all the variables.
            // I think these two cases cover everything.
            if(lowlink[curnode]!=1 || varcount<var_indices.size())
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
                // For each variable, write it to the scc array.
                // If its the last one, flip the bit.
                
                varinlocalmatching.clear();  // Borrow this datastructure for a minute.
                
                //vector<int> tempset;  // pretend this is SCCs for the time being.
                //tempset.reserve(numvars);
                D_DATA(cout <<"Writing new SCC:"<<endl );
                bool containsvars=false;
                for(vector<int>::iterator tstackit=(--tstack.end());  ; --tstackit)
                {
                    int copynode=(*tstackit);
                    //cout << "SCC element: "<< copynode<<endl;
                    if(copynode<numvars)
                    {
                        containsvars=true;
                        //SCCs[sccindex]=copynode;
                        //varToSCCIndex[copynode]=sccindex;
                        sccindex++;
                        //tempset.push_back(copynode);  // need to write into sccs instead.
                        varinlocalmatching.insert(copynode);
                        D_DATA(cout << "Stored SCC element "<< copynode<<endl);
                    }
                    
                    if(copynode==curnode)
                    {
                        // Beware it might be an SCC containing just one value.
                        
                        if(containsvars)
                        {
                            D_DATA(cout << "Inserting split point." <<endl);
                            //SCCSplit.remove(sccindex-1);
                            //D_DATA(((char*)SCCSplit2.get_ptr())[sccindex-1]=0);
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
                            for(int i=0; i<var_indices.size(); i++)
                            {
                                int curvar=var_indices[i];
                                if(!varinlocalmatching.in(curvar))
                                {
                                    // var not in tempset so might have to do some test against matching.
                                    // Why doing this test? something wrong with the assigned variable optimization?
                                    if(varvalmatching[curvar]!=copynode+dom_min-numvars)
                                    {
                                        D_DATA(cout << "Removing var: "<< curvar << " val:" << copynode+dom_min-numvars <<endl);
                    
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
