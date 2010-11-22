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

// The algorithm iGAC or short-supports-gac
// i.e. the short supports algrithm.


template<typename VarArray>
struct ShortSupportsGAC : public AbstractConstraint
{
    virtual string constraint_name()
    {
        return "ShortSupportsGAC";
    }
    
    // Element of doubly-linked list to store supports
    class supElement
    {
        supElement* next;
        supElement* prev;
        vector<int>* support;
    }
    
    
    int supports;
    vector<int> supportsPerVar;
    vector<vector<int> > supportsPerLit;
    
    // 2d array (indexed by var then val) of doubly-linked list of supports.
    vector<vector<supElement*> >  supportListPerLit;
    
    // For each variable, a vector of values with 0 supports (or had 0 supports
    // when added to the vector).
    vector<vector<int> > zeroVals;
    
    vector<int> varsPerSupport;    // Permutation of the variables
    vector<int> varsPerSupInv;   // Inverse mapping of the above.
    
    vector<int> supportNumPtrs;
    
    /*
         MoveableArray<num> flow_s_var;
         vector<MoveableArray<num> > flow_var_val;
         MoveableArray<num> flow_val_t;
         Reversible<num> totalflow;
    */
    
    ShortSupportsGAC(StateObj* _stateObj, const VarArray& _var_array) : 
    #if BtMatch
    ,totalflow(_stateObj)
    #endif
    
    {
        #if BtMatch
            flow_s_var=getMemory(stateObj).backTrack().template requestArray<num>(numvars);
            flow_val_t=getMemory(stateObj).backTrack().template requestArray<num>(numvals);
            for(int i=0; i<numvars; i++)
            {
                flow_var_val.push_back(getMemory(stateObj).backTrack().template requestArray<num>(numvals));
            }
        #else
            flow_var_val.resize(numvars);
            for(int i=0; i<numvars; i++)
            {
                flow_var_val[i].resize(numvals, minflow);
            }
            flow_val_t.resize(numvals, minflow);
            
            flow_s_var.resize(numvars, minflow);
        #endif
        totalflow=minflow;
        
      to_process.reserve(numvars);
      
      // Set up data structures
      prev.resize(numvars+numvals+2, -1);
      
      visited.reserve(numvars+numvals+2);   // vars, vals, s and t.
  }
  
    /////////////////////////////////////////////////////////////////////
    //  FF-BFS
    
    deque<int> fifo;
    vector<int> prev;
    smallset_nolist visited;
    vector<int> augpath;
    
    
    vector<int>* bfs(int start, int end)
    {
        // find any non-saturated path from start to end (including the circulating edge.)
        // Not allowed to use the edge (or reverse edge) start->end !
        // nodes are 0..numvars-1 for variables,
        // numvars..(numvars+numvals-1) for values
        // numvars+numvals    for s
        // numvars+numvals+1  for t.
        
        fifo.clear(); fifo.push_back(start);
        visited.clear();
        visited.insert(start);
        
        while(fifo.size()>0)
        {
            int curnode=fifo.front();
            fifo.pop_front();
            
            /*if(curnode==end)
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
                
                return &augpath;
            }*/
            
            // Now cases for each type of node.
            if(curnode==numvars+numvals)
            {   // s.
                // All variables.  No circulating edge.
                for(int newnode=0;  newnode<numvars; newnode++)
                {
                    if(!visited.in(newnode) && flow_s_var[newnode]<maxflow && (curnode!=start || newnode!=end))
                    {
                        /*fifo.push_back(newnode);
                        visited.insert(newnode);
                        prev[newnode]=curnode;*/
                        VISITNODE
                    }
                }
            }
            else if(curnode==numvars+numvals+1)
            {   // t
                // Link to all vals that are above 0 flow.
                
                for(int i=dom_min; i<=dom_max; i++)
                {
                    int newnode=i-dom_min+numvars;
                    if(!visited.in(newnode) && flow_val_t[newnode-numvars]>minflow && (curnode!=start || newnode!=end))
                    {
                        /*fifo.push_back(newnode);
                        visited.insert(newnode);
                        prev[newnode]=curnode;*/
                        VISITNODE
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
                    int val=adjlist[curnode][adjlistidx];
                    int newnode=val-dom_min+numvars;
                    if(!visited.in(newnode) && flow_var_val[curnode][val-dom_min]<maxflow && (curnode!=start || newnode!=end))
                    {
                        /*fifo.push_back(newnode);
                        visited.insert(newnode);
                        prev[newnode]=curnode;*/
                        VISITNODE
                    }
                }
                
                // Link to s.
                int newnode=numvars+numvals; ///s
                if(!visited.in(newnode) && flow_s_var[curnode]>minflow && (curnode!=start || newnode!=end))
                {
                    /*fifo.push_back(newnode);
                    visited.insert(newnode);
                    prev[newnode]=curnode;*/
                    VISITNODE
                }
            }
            else
            {
                D_ASSERT( curnode>=numvars && curnode<numvars+numvals);
                // a value
                // link to t if edge not saturated
                int newnode=numvars+numvals+1; ///t
                if(!visited.in(newnode) && flow_val_t[curnode-numvars]<maxflow && (curnode!=start || newnode!=end))
                {
                    /*fifo.push_back(newnode);
                    visited.insert(newnode);
                    prev[newnode]=curnode;*/
                    VISITNODE
                }
                
                // All variables where the flow on the edge can be reduced. >0.
                int len=adjlistlength[curnode];
                for(int adjlistidx=0; adjlistidx<len; adjlistidx++)
                {
                    int newnode=adjlist[curnode][adjlistidx];
                    if(!visited.in(newnode) && (flow_var_val[newnode][curnode-numvars]>minflow) && (curnode!=start || newnode!=end))
                    {
                        /*fifo.push_back(newnode);
                        visited.insert(newnode);
                        prev[newnode]=curnode;*/
                        VISITNODE
                    }
                }
            }
        }
        return 0;
    }
    
    /*def balance(valnode, var):
    val=valnode-100
    # Given a 0 edge (var -> valnode), find a suitable unsaturated value (if one exists)
    # and use it to get rid of the 0 edge.
    for unsatval in range(dom_min, dom_max+1):
        if flow[(unsatval+100, t)]<1:
            # unsatval is really unsaturated.
            # Find a variable with valnode and unsatval in domain
            for midvar in range(len(domains)):
                if unsatval in domains[midvar] and val in domains[midvar]:
                    # Now we're in business.
                    # Find any value in domains[var] except val
                    val2 = domains[var][0]
                    if val2==val:
                        val2=domains[var][1]
                    return [valnode, midvar, unsatval+100, t, val2+100, var]
                    
    return False*/
    
    // specialised search for an augmenting path from val to var of length 6 vertices. 
    // Used to increase flow from var to val from 0.
    inline bool balance(int val, int var)
    {
        //val is a real value.
        // Given the 0 edge var->val, find a suitable unsaturated value (if one exists)
        // and use it to get rid of the 0 edge. 
        if(adjlistlength[var]<2)
        {
            return false;
        }
        
        // First go through variables adjacent to val.
        int len=adjlistlength[val-dom_min+numvars];
        for(int i=0; i<len; i++)
        {
            int midvar=adjlist[val-dom_min+numvars][i];
            
            // Is there a value with spare capacity adjacent to midvar
            int len2=adjlistlength[midvar];
            for(int j=0; j<len2; j++)
            {
                int val_sparecap=adjlist[midvar][j];
                if(flow_val_t[val_sparecap-dom_min]<maxflow)
                {
                    // Find any value in dom(var) other than val.
                    int val2=(adjlist[var][0]!=val ? adjlist[var][0] : adjlist[var][1]);
                    // Found the appropriate augmenting path. Now apply it.
                    // Augmenting path is:
                    // [val, midvar, val_sparecap, t, val2, var, val]
                    
                    
                    
                    return true;
                }
            }
            
            
        }
        
        return false;
    }
    
    
    vector<int> zeros;
    
    // This is a mess -- must be a better way
    
    // Warning: both these functions do not adjust totalflow.
    num apply_path_max_zeros(vector<int>& path)
    {
        P("Augmenting path:" << path);
        
        num augamount = maxflow;
        for(int i=0; i<path.size()-1; i++)
        {
            int from=path[i];
            int to=path[i+1];
            num diff=-1000000.0;
            if(from==numvars+numvals)
            {   // edge from s to a var.
                diff=maxflow-flow_s_var[to];
            }
            else if(from==numvars+numvals+1)
            {   // t to a val
                diff=flow_val_t[to-numvars];  // it can be reduced by the flow amount.
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
                    diff=maxflow-flow_var_val[from][to-numvars];
                }
            }
            else
            {   // from is a value.
                if(to==numvars+numvals+1)
                {
                    diff=maxflow-flow_val_t[from-numvars];
                }
                else
                {
                    D_ASSERT(to<numvars);
                    diff=flow_var_val[to][from-numvars];
                }
            }
            D_ASSERT(diff>minflow);
            augamount=( diff<augamount ? diff : augamount );
        }
        
        P("Augmenting flow by: " << augamount);
        
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
                flow_val_t[to-numvars]-=augamount;  // it can be reduced by the flow amount.
                if(flow_val_t[to-numvars]==minflow)
                {
                    zeros.push_back(to); zeros.push_back(from);
                }
            }
            else if(from<numvars)
            {
                if(to==numvars+numvals)
                {
                    flow_s_var[from]=flow_s_var[from]-augamount;
                    if(flow_s_var[from]==minflow)
                    {
                        zeros.push_back(to); zeros.push_back(from);
                    }
                }
                else
                {
                    D_ASSERT(to>=numvars && to<numvars+numvals);
                    flow_var_val[from][to-numvars]=flow_var_val[from][to-numvars]+augamount;
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
                    flow_var_val[to][from-numvars]=flow_var_val[to][from-numvars]-augamount;
                    if(flow_var_val[to][from-numvars]==minflow)
                    {
                        zeros.push_back(to); zeros.push_back(from);
                    }
                }
            }
            
        }
        
        return augamount;
    }
    
    
    // This is a mess -- must be a better way
    num apply_path_fraction(vector<int>& path)
    {
        num augamount = maxflow;
        for(int i=0; i<path.size()-1; i++)
        {
            int from=path[i];
            int to=path[i+1];
            num diff=-1000000.0;
            if(from==numvars+numvals)
            {   // edge from s to a var.
                diff=maxflow-flow_s_var[to];
            }
            else if(from==numvars+numvals+1)
            {   // t to a val
                diff=flow_val_t[to-numvars];  // it can be reduced by the flow amount.
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
                    diff=maxflow-flow_var_val[from][to-numvars];
                }
            }
            else
            {   // from is a value.
                if(to==numvars+numvals+1)
                {
                    diff=maxflow-flow_val_t[from-numvars];
                }
                else
                {
                    D_ASSERT(to<numvars);
                    diff=flow_var_val[to][from-numvars];
                    if(diff==1)
                    {
                        return -1;  // indicates this problem.
                    }
                }
            }
            D_ASSERT(diff>minflow);
            augamount=( diff<augamount ? diff : augamount );
        }
        
        augamount=augamount/ ((num)2);
        
        P("Augmenting flow by: " << augamount);
        
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
                flow_val_t[to-numvars]-=augamount;  // it can be reduced by the flow amount.
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
                    flow_var_val[from][to-numvars]+=augamount;
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
                    flow_var_val[to][from-numvars]-=augamount;
                }
            }
            
        }
        
        return augamount;
    }
    
    
    void print_flow()
    {
        num checktotalflow=minflow;
        
        for(int var=0; var<numvars; var++)
        {
            cout << "S -> " <<var << ", flow: " << flow_s_var[var] <<endl;
            
        }
        
        
        for(int var=0; var<numvars; var++)
        {
            for(int val=var_array[var].getMin(); val<=var_array[var].getMax(); val++)
            {
                if(var_array[var].inDomain(val))
                {
                    cout << "x" << var << " -> " <<val << ", flow: " << flow_var_val[var][val-dom_min] <<endl;
                    checktotalflow+=flow_var_val[var][val-dom_min];
                }
            }
        }
        
        for(int val=dom_min; val<=dom_max; val++)
        {
            cout << val << " -> T, flow: " << flow_val_t[val-dom_min] <<endl;
            
        }
        
        cout << "Total flow:" << totalflow<<endl;
        cout << "Total flow check:" << checktotalflow<<endl;
        D_ASSERT(totalflow==checktotalflow);
    }
    
    void print_flow_nocheck()
    {
        num checktotalflow=minflow;
        
        for(int var=0; var<numvars; var++)
        {
            cout << "S -> " <<var << ", flow: " << flow_s_var[var] <<endl;
            
        }
        
        
        for(int var=0; var<numvars; var++)
        {
            for(int val=var_array[var].getMin(); val<=var_array[var].getMax(); val++)
            {
                if(var_array[var].inDomain(val))
                {
                    cout << "x" << var << " -> " <<val << ", flow: " << flow_var_val[var][val-dom_min] <<endl;
                    checktotalflow+=flow_var_val[var][val-dom_min];
                }
            }
        }
        
        for(int val=dom_min; val<=dom_max; val++)
        {
            cout << val << " -> T, flow: " << flow_val_t[val-dom_min] <<endl;
            
        }
        
        cout << "Total flow:" << totalflow<<endl;
        cout << "Total flow check:" << checktotalflow<<endl;
        
    }
    
    
    void press_the_nuclear_button()
    {
        // Encountered an edge with flow 1 in apply_path_fraction...
        // can't apply the path. Reset the whole network.
        num partflow=maxflow/numvals;  // Assume division truncates.
        D_ASSERT(partflow*numvals<= maxflow);
        
        totalflow=minflow;
        for(int i=0; i<numvars; i++)
        {
            int len=adjlistlength[i];
            for(int j=0; j<len; j++) flow_var_val[i][adjlist[i][j]-dom_min]=partflow;   //!!
            flow_s_var[i]=partflow*adjlistlength[i];
            totalflow=totalflow+partflow*adjlistlength[i];
        }
        for(int i=0; i<numvals; i++) flow_val_t[i]=partflow*adjlistlength[i+numvars];
        
        vector<int>* augpath=bfs(numvars+numvals, numvars+numvals+1);
        while(augpath!=0)
        {
            num diff=apply_path_max_zeros(*augpath);
            totalflow=totalflow+diff;
            //print_flow();
            augpath=bfs(numvars+numvals, numvars+numvals+1);
        }
        
        D_ASSERT(totalflow == ((num)numvars)*maxflow);
    }
    
    void do_initial_prop()
    {
        P("In do_initial_prop()");
        
        press_the_nuclear_button();
        
        if(totalflow < ((num)numvars)*maxflow)
        {
            getState(stateObj).setFailed(true);
            return;
        }
        
        P("Before fractional part:");
        PLONG(print_flow());
        for(int var=0; var<numvars; var++)
        {
            for(int i=0; i<adjlistlength[var]; i++)
            {
                int val=adjlist[var][i];
                if(flow_var_val[var][val-dom_min]==minflow)
                {
                    P("Attempting to find augmenting path for var: " << var << ", val:" << val);
                    vector<int>* augpath=bfs(val+numvars-dom_min, var);
                    if(augpath==0)
                    {
                        P("No aug path found, removing value.");
                        var_array[var].removeFromDomain(val);
                        adjlist_remove(var, val);
                        i--;
                    }
                    else
                    {
                        P2("Fractional augmenting path:" << *augpath );
                        num diff=apply_path_fraction(*augpath);
                        flow_var_val[var][val-dom_min]+=diff;
                    }
                }
            }
        }
        P("After fractional part:");
        PLONG(print_flow());
        P("Leaving do_initial_prop()");
    }
    
    
    void incremental_prop()
    {
        P("Entered incremental_prop()");
        PLONG(print_flow_nocheck());
        // Go through changed vars and update datastructures.
        vector<int>& changed_vars=to_process.getlist();
        for(int varidx=0; varidx<changed_vars.size(); varidx++)
        {
            int var=changed_vars[varidx];
            // Go through adjacency list and update.
            for(int i=0; i<adjlistlength[var]; i++)
            {
                int val=adjlist[var][i];
                if(!var_array[var].inDomain(val))
                {
                    // adjust the flow.
                    num diff=flow_var_val[var][val-dom_min];
                    flow_var_val[var][val-dom_min]=minflow;
                    flow_s_var[var]-=diff;
                    flow_val_t[val-dom_min]-=diff;
                    totalflow=totalflow-diff;
                    
                    // update the adjacency lists. 
                    adjlist_remove(var, val);
                    i--;
                }
            }
        }
        PLONG(print_flow());
        
        // Go through the vars that have lost some flow, and attempt to restore it.
        // Track the edges that are reduced to zero flow.
        zeros.clear();
        
        while(totalflow<((num)numvars)*maxflow)
        {
            vector<int>* augpath=bfs(numvars+numvals, numvars+numvals+1);  // var to t.
            if(augpath==0)
            {
                getState(stateObj).setFailed(true);
                return;
            }
            num diff=apply_path_max_zeros(*augpath);
            totalflow=totalflow+diff;
        }
        
        /*for(int varidx=0; varidx<changed_vars.size(); varidx++)
        {
            int var=changed_vars[varidx];
            
            while(flow_s_var[var]<maxflow)
            {
                vector<int>* augpath=bfs(var, numvars+numvals+1);  // var to t.
                if(augpath==0)
                {
                    getState(stateObj).setFailed(true);
                    return;
                }
                // Put s on the front.
                //cout << (*augpath) << endl;
                augpath->push_back(augpath->back());
                for(int i=augpath->size()-2; i>=0; i--)  (*augpath)[i+1]=(*augpath)[i];
                (*augpath)[0]=numvars+numvals;
                //cout << (*augpath) << endl;
                apply_path_max_zeros(*augpath);
            }
        }*/    /// With this change it's underpropagating slightly.
        
        P("After restoring maximum flow in incremental_prop:");
        PLONG(print_flow());
        
        // Now go through zeros and do.... summat.
        int numzeros=zeros.size()/2;
        for(int i=0; i<numzeros; i++)
        {
            int var=zeros[i*2];
            int valnode=zeros[i*2+1];
            int val=valnode-numvars+dom_min;
            
            P("Zero: var:" << var << ", val:" << val);
            if(flow_var_val[var][valnode-numvars]==minflow)
            {
                vector<int>* augp=bfs(valnode, var);
                if(augp==0)
                {
                    P("Pruning the zero.");
                    var_array[var].removeFromDomain(val);
                    adjlist_remove(var, val);
                }
                else
                {
                    P2("Fractional augmenting path:"<<*augp);
                    augp->push_back(valnode);
                    num diff=apply_path_fraction(*augp);
                    if(diff==-1)
                    {
                        // Found a 1 somewhere.
                        // Reset the flow network and start from scratch.
                        do_initial_prop();
                        return;
                    }
                }
            }
        }
        
        P("About to exit incremental_prop()");
        PLONG(print_flow());
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
      return new CheckAssignConstraint<VarArray, GacAlldiffConstraint2>(stateObj, var_array, *this);
      
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
                //adjlist_remove(i, assignedval);
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
        incremental_prop();
        //do_initial_prop();
        #endif
    }
  }
  
  virtual void special_unlock() { constraint_locked = false; to_process.clear(); }
  virtual void special_check()
  {
    constraint_locked = false;
    
    if(getState(stateObj).isFailed())
    {
        to_process.clear();
        return;
    }
    
    incremental_prop();
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
        
        //check_adjacency_lists();
        
      to_process.clear();
      
      do_initial_prop();
  }
  
    void check_adjacency_lists()
    {
        for(int var=0; var<numvars; var++)
        {
            for(int val=dom_min; val<=dom_max; val++)
            {
                if(var_array[var].inDomain(val))
                {
                    D_ASSERT(adjlistpos[var][val-dom_min]<adjlistlength[var]);
                    D_ASSERT(adjlistpos[val-dom_min+numvars][var]<adjlistlength[val-dom_min+numvars]);
                }
                else
                {
                    D_ASSERT(adjlistpos[var][val-dom_min]>=adjlistlength[var]);
                    D_ASSERT(adjlistpos[val-dom_min+numvars][var]>=adjlistlength[val-dom_min+numvars]);
                }
            }
        }
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
    
    
};  // end of class


