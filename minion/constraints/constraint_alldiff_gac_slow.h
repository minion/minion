/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id: constraint_alldiff_gac_slow.h 668 2007-09-26 15:14:50Z pete_n $
*/

/* Minion
* Copyright (C) 2006
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

/** @help constraints;alldiffgacslow Description
Forces the input vector of variables to take distinct values.
*/

/** @help constraints;alldiffgacslow Example 
Suppose the input file had the following vector of variables defined:

DISCRETE myVec[9] {1..9}

To ensure that each variable takes a different value include the
following constraint:

alldiffgacslow(myVec)
*/

/** @help constraints;alldiffgacslow Reifiability
Not reifiable.
*/

/** @help constraints;alldiffgacslow Notes
This constraint enforces generalised arc consistency.
*/

#include <stdlib.h>
#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <algorithm>
#include <utility>


// Begin and end markers for iteration in iterative Tarjan's algorithm
// Probably not needed.
#define BEGIN (-1)
#define END (-2)

using namespace std;

struct smallset
{
    // a small set of integers (could be templated?) which
    // clears in constant time, set membership in
    // constant time and direct iteration.
    // Add an item is constant time, remove is not.
    
    unsigned int cert;
    
    vector<unsigned int> membership;
    
    vector<int> list;
    
    void reserve(int size)
    {
        // This must be called before anything is put in the set.
        membership.resize(size, 0);
        list.reserve(size);
        cert=1;
    }
    
    inline bool in(int val)
    {
        return membership[val]==cert;
    }
    
    inline void insert(int val)
    {
        D_ASSERT(membership[val]<cert);
        D_ASSERT(val>=0);
        list.push_back(val);
        membership[val]=cert;
    }
    
    inline int size()
    {
        return list.size();
    }
    
    inline vector<int>& getlist()
    {
        return list;
    }
    
    inline void clear()
    {
        D_ASSERT(cert< 2000000000);
        cert++;
        list.clear();
    }
};


struct smallset_nolist
{
    // a small set of integers (could be templated?) which
    // clears in constant time, set membership in
    // constant time and direct iteration.
    // Add an item is constant time, remove is not.
    
    unsigned int cert;
    
    vector<unsigned int> membership;
    
    void reserve(int size)
    {
        // This must be called before anything is put in the set.
        membership.resize(size, 0);
        cert=1;
    }
    
    inline bool in(int val)
    {
        return membership[val]==cert;
    }
    
    inline void insert(int val)
    {
        D_ASSERT(membership[val]<cert);
        D_ASSERT(val>=0);
        membership[val]=cert;
    }
    
    inline void remove(int val)
    {
        membership[val]=0;
    }
    
    inline void clear()
    {
        D_ASSERT(cert< 2000000000);
        cert++;
    }
};

template<typename VarArray>
struct AlldiffGacSlow : public Constraint
{
  virtual string constraint_name()
  { return "AlldiffGacSlow"; }
  
  typedef typename VarArray::value_type VarRef;  // what for?
  DomainInt dom_min, dom_max;
  VarArray var_array;
  
  int numvars, numvals;
  
  AlldiffGacSlow(StateObj* _stateObj, const VarArray& _var_array) : Constraint(_stateObj),
    var_array(_var_array), constraint_locked(false)
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
      numvars=var_array.size();  // number of variables in the constraint
      numvals=dom_max-dom_min+1;
      
      // Set up data structures
      initialize_hopcroft();
      initialize_tarjan();
  }
  
  virtual triggerCollection setup_internal()
  {
    D_INFO(2, DI_SUMCON, "Setting up Constraint");
    triggerCollection t;
    int array_size = var_array.size();
    for(int i = 0; i < array_size; ++i)
	  t.push_back(make_trigger(var_array[i], Trigger(this, i), DomainChanged));
    return t;
  }
  
  virtual Constraint* reverse_constraint()
  { return new CheckAssignConstraint<VarArray, AlldiffGacSlow>(stateObj, var_array, *this); }

  bool constraint_locked;
  PROPAGATE_FUNCTION(int, DomainDelta)
  { 
    if(constraint_locked) return;
    constraint_locked = true;
    getQueue(stateObj).pushSpecialTrigger(this);
  }

  virtual void special_unlock() { constraint_locked = false; }
  virtual void special_check()
  {
    constraint_locked = false;
    do_prop();
  }
  

  void do_prop()
  {
    PROP_INFO_ADDONE(AlldiffGacSlow);
    
    // Check length of the matching against the number of variables
    var_indices.resize(numvars);
    for(int i=0; i<numvars; i++)
    {
        var_indices[i]=i;
    }
    
    // Check if any part of the matching has been invalidated since last call.
    {vector<int> toiterate = valinlocalmatching.getlist();  // copy. I couldn't think how without copying it.
    valinlocalmatching.clear();
    for(int i=0; i<toiterate.size(); ++i)
    {
        int tempval=toiterate[i];
        int tempvar=localmatching[tempval];
        if(var_array[tempvar].inDomain(tempval+dom_min))
        {
            valinlocalmatching.insert(tempval);
        }
    }
    }
    
    if(valinlocalmatching.size()==0)
    {
        // If we don't have a partial matching left over, 
        // Greedy matching algorithm in here.
        for(int tempvar=0; tempvar<numvars; ++tempvar)
        {
            for(DomainInt j=var_array[tempvar].getMin(); j<=var_array[tempvar].getMax(); j++)
            {
                if(!valinlocalmatching.in(j-dom_min) && var_array[tempvar].inDomain(j))
                {
                    localmatching[j-dom_min]=tempvar;
                    valinlocalmatching.insert(j-dom_min);
                    break;
                }
            }
        }
    }
    //cout << "Matching size:" << valinlocalmatching.size() <<endl;
    if(valinlocalmatching.size()!=numvars)
    {
      if(!hopcroft(var_indices))
      {
        // The constraint is unsatisfiable (no matching).
        getState(stateObj).setFailed(true);
        return;
      }
    }
    
    {
      vector<int>& toiterate=valinlocalmatching.getlist();
      for(int i=0; i<toiterate.size(); ++i)
      {
          // copy into reversed matching
          varvalmatching[localmatching[toiterate[i]]]=toiterate[i]+dom_min;  // store actual value in varvalmatching
      }
    }
    /*cout <<"Success! Final matching:"<<endl;
    for(mapit=matching.begin(); mapit!=matching.end(); mapit++)
    {
        cout<<(*mapit).first<<":"<<(*mapit).second<<endl;
    }*/
    
    include_sink=true;
    
    // spare_values
    spare_values.resize(0);
    for(int i=0; i<numvals; ++i)
    {
        bool found=false;
        bool inmatch=false;
        for(int j=0; j<numvars; j++)
        {
            if(var_array[j].inDomain(i+dom_min))
            {
                found=true;
            }
            if(varvalmatching[j]==(i+dom_min))
            {
                inmatch=true;
                break;
            }
        }
        if(!inmatch && found)
        {
            spare_values.push_back(i+numvars);
        }
    }
    
    tarjan_recursive();
    
    //  print components out
    
    /*cout<<"components:"<<endl;
    for(vector<vector<int> >::iterator compit=components.begin(); compit!=components.end(); compit++)
    {
        vector<int>::iterator cit;
        for(cit=(*compit).begin(); cit!=(*compit).end(); cit++)
        {
            cout<< (*cit)<< endl;
        }
        cout<<"Next component"<<endl;
    }*/
    
    // --------------------------- find residual edges to delete -------------------------------
    
    for(int tempvar=0; tempvar<numvars; tempvar++)
    {
        for(DomainInt tempval=var_array[tempvar].getMin(); tempval<=var_array[tempvar].getMax(); tempval++)
        {
            if(var_array[tempvar].inDomain(tempval))
            {
                bool flag=true;  // tempval should be included in the domain
                
                if(valinlocalmatching.in(tempval-dom_min) || tempvar!=localmatching[tempval-dom_min])
                {
                    // tempval tempvar is a back edge - so not included unless 
                    // both tempvar and tempval are in the same component.
                    flag=false;
                    for(vector<vector<int> >::iterator compit=components.begin(); compit!=components.end(); compit++)
                    {
                        vector<int>& temp=(*compit);
                        if(find(temp.begin(), temp.end(), tempvar)!=temp.end())
                        {
                            if(find(temp.begin(), temp.end(), tempval-dom_min+numvars)!=temp.end())
                            {
                                // tempval tempvar should be included in the domain
                                flag=true;
                            }
                            break;
                        }
                    }
                }
                // read flag here
                if(!flag)
                {
                    //cout<<"Adding "<<tempval<<endl;
                    var_array[tempvar].removeFromDomain(tempval);
                    if(getState(stateObj).isFailed()) return;
                }
            }
        }
    }
    
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
	D_ASSERT(var_array[i].isAssigned());
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
  { do_prop(); }
	
	virtual BOOL check_assignment(vector<DomainInt> v)
	{
	  D_ASSERT(v.size() == var_array.size());
	  int array_size = v.size();
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
    
    // ------------------------------- Targan's algorithm ------------------------------------
    // based on the following pseudocode from wikipedia.
        /*
        Input: Graph G = (V, E), Start node v0

        max_dfs := 0  // Counter for dfs
        U := V        // Collection of unvisited nodes
        S := {}       // An initially empty stack
        tarjan(v0)    // Call the function with the start node
        
        procedure tarjan(v)
        v.dfs := max_dfs;          // Set the depth index
        v.lowlink := max_dfs;      // v.lowlink <= v.dfs
        max_dfs := max_dfs + 1;    // Increment the counter
        S.push(v);                 // Place v on the stack
        U := U \ {v};              // Separate v from U
        forall (v, v') in E do     // Consider the neighboring nodes
          if (v' in U)
            tarjan(v');            // recursive call
            v.lowlink := min(v.lowlink, v'.lowlink);
          // Ask whether v' is on the stack 
          // by a clever constant time method
          // (for example, setting a flag on the node when it is pushed or popped) 
          elseif (v' in S)
            v.lowlink := min(v.lowlink, v'.dfs);
          end if
        end for
        if (v.lowlink = v.dfs)     // the root of a strongly connected component
          print "SZK:";
          repeat
            v' := S.pop;
            print v';
          until (v' = v);
        end if
        */
    
    vector<int> tstack;
    smallset_nolist in_tstack;
    smallset_nolist visited;
    vector<int> dfsnum;
    vector<int> lowlink;
    
    vector<int> iterationstack;
    vector<int> curnodestack;
    
    vector<vector<int> > components;
    
    vector<int> varvalmatching; // For each var, give the matching value.
    // Filled in before calling tarjan's.
    
    int max_dfs;
    
    vector<int> spare_values;
    bool include_sink;
    vector<int> var_indices;
    
    // An integer represents a vertex, where 0 .. numvars-1 represent the vars,
    // numvars .. numvars+numvals-1 represents the values (val-dom_min+numvars),
    // numvars+numvals is the sink,
    // numvars+numvals+1 is the 
    
    void initialize_tarjan()
    {
        int numnodes=numvars+numvals+1;  // One sink node.
        tstack.reserve(numnodes);
        in_tstack.reserve(numnodes);
        visited.reserve(numnodes);
        max_dfs=1;
        dfsnum.resize(numnodes);
        lowlink.resize(numnodes);
        
        iterationstack.resize(numnodes);
        curnodestack.reserve(numnodes);
        
        varvalmatching.resize(numvars);
    }
    
    int tarjan_recursive()
    {
        tstack.resize(0);
        in_tstack.clear();
        
        visited.clear();
        max_dfs=1;
        int curnode=var_indices[0];
        components.resize(0);
        int scccount=0;
        
        for(int i=0; i<var_indices.size(); ++i)
        {
            if(!visited.in(i))
            {
                //cout << "(Re)starting tarjan's algorithm" <<endl;
                visit(var_indices[i]);
                scccount++;  // This is rubbish.
            }
        }
        
        return scccount;
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
            }
        }
        else if(curnode<numvars)
        {
            //cout << "Visiting node variable: "<< curnode<<endl;
            int newnode=varvalmatching[curnode]-dom_min+numvars;
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
        else if(curnode<(numvars+numvals))
        {
            // curnode is a value
            //cout << "Visiting node val: "<< curnode+dom_min-numvars <<endl;
            for(int i=0; i<var_indices.size(); ++i)
            {
                int newnode=var_indices[i];
                if(var_array[newnode].inDomain(curnode+dom_min-numvars))
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
            if(include_sink && 
                find(varvalmatching.begin(), varvalmatching.end(), curnode+dom_min-numvars)!=varvalmatching.end())
            {
                int newnode=numvars+numvals;
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
        
        //cout << "On way back up, curnode:" << curnode<< ", lowlink:"<<lowlink[curnode]<< ", dfsnum:"<<dfsnum[curnode]<<endl;
        if(lowlink[curnode]==dfsnum[curnode])
        {
            // Need to optimize here.
            // we are at the root of a strongly connected component
            vector<int> tempset;
            while(true)
            {
                int copynode=(* --(tstack.end()));  //get last element
                //v.print();
                tstack.pop_back();
                in_tstack.remove(copynode);
                tempset.push_back(copynode);
                
                if(copynode==curnode)
                {
                    break;
                }
            }
            components.push_back(tempset);
        }
    }
    
  // -------------------------Hopcroft-Karp algorithm -----------------------------
  // Can be applied to a subset of var_array as required.
  
  // Each domain value has a label which is numvars+
  
  // localmatching is from val-dom_min to var.
  vector<int> localmatching;   // need to set size somewhere.
  // -1 means unmatched.
  smallset_nolist varinlocalmatching;    // indicates whether a var is recorded in localmatching.
  smallset valinlocalmatching;
  
  // Uprevious (pred) gives (for each CSP value) the value-dom_min
  // it was matched to in the previous layer. If it was unmatched,
  // -1 is used.
  vector<int> uprevious;  // -2 means unset, -1 labelled unmatched. 
  
  vector<vector<int> > vprevious;  // map val-dom_min to vector of vars.
  smallset_nolist invprevious;     // is there a mapping in vprevious for val? Allows fast unset.
  
  smallset layer;
  smallset unmatched;   // contains vals-dom_min.
  
  vector<vector<int> > newlayer;
  smallset innewlayer;
  
  void initialize_hopcroft()
  {
      // Initialize all datastructures to do with hopcroft-karp
      // Surely could reduce the number of arrays etc used for hopcroft-karp??
      int numvals=dom_max-dom_min+1;
      localmatching.resize(numvals);
      varinlocalmatching.reserve(numvars);
      valinlocalmatching.reserve(numvals);
      uprevious.resize(numvars, -2);
      
      vprevious.resize(numvals);
      for(int i=0; i<numvals; ++i)
      {
          vprevious[i].reserve(numvars);
      }
      invprevious.reserve(numvals);
      
      layer.reserve(numvars);
      unmatched.reserve(numvals);
      
      newlayer.resize(numvals);
      for(int i=0; i<numvals; ++i)
      {
          newlayer[i].reserve(numvars);
      }
      innewlayer.reserve(numvals);
  }
  
  inline bool hopcroft(vector<int> var_indices)
    {
        // Domain value convention:
        // Within hopcroft and recurse,
        // a domain value is represented as val-dom_min always.
        
        // Variables are always represented as their index in
        // var_array. var_indices indicates which variables
        // we are allowed to use here.
        
        int localnumvars=var_indices.size();
        
        //valinlocalmatching.clear();  // A partial matching is passed in.
        
        /*# initialize greedy matching (redundant, but faster than full search)
        matching = {}
        for u in graph:
            for v in graph[u]:
                if v not in matching:
                    matching[v] = u
                    break
        */
        
        if(valinlocalmatching.size()==localnumvars)
        {
            return true;
        }
        
        // uprevious == pred
        // vprevious == preds
        
        // need sets u and v
        // u is easy, v is union of domains[0..numvar-1]
        
        while(true)
        {
            /*
            preds = {}
            unmatched = []
            pred = dict([(u,unmatched) for u in graph])
            for v in matching:
                del pred[matching[v]]
            layer = list(pred)
            */
            // Set up layer and uprevious.
            invprevious.clear();
            unmatched.clear();
            
            layer.clear();
            
            // Reconstruct varinlocalmatching here.
            varinlocalmatching.clear();
            {
                vector<int>& toiterate=valinlocalmatching.getlist();
                for(int i=0; i<toiterate.size(); ++i)
                {
                    varinlocalmatching.insert(localmatching[toiterate[i]]);
                }
            }
            
            for(int i=0; i<localnumvars; ++i)
            {
                int tempvar=var_indices[i];
                if(varinlocalmatching.in(tempvar))  // The only use of varinlocalmatching.
                {
                    uprevious[tempvar]=-2;   // Out of uprevious
                }
                else
                {
                    layer.insert(tempvar);
                    uprevious[tempvar]=-1;  // In layer, and set to unmatched in uprevious.
                }
            }
            
            /*cout<< "Uprevious:" <<endl;
            for(int i=0; i<localnumvars; ++i)
            {
                cout<< "for variable "<<var_indices[i]<<" value "<< uprevious[i]<<endl;
            }*/
            
            // we have now calculated layer
            /*
            while layer and not unmatched:
			    newLayer = {}
            */
            
            while(layer.size()!=0 && unmatched.size()==0)
            {
                innewlayer.clear();
                
                /*
                for u in layer:
                    for v in graph[u]:
                        if v not in preds:
                            newLayer.setdefault(v,[]).append(u)
                */
                {
                vector<int>& toiterate=layer.getlist();
                for(int i=0; i<toiterate.size(); ++i)
                {
                    //cout<<"Layer item: "<<(*setit)<<endl;
                    int tempvar=toiterate[i];
                    for(DomainInt realval=var_array[tempvar].getMin(); realval<=var_array[tempvar].getMax(); realval++)
                    {
                        if(var_array[tempvar].inDomain(realval))
                        {
                            int tempval=realval-dom_min;
                            
                            if(!invprevious.in(tempval))  // if tempval not found in vprevious
                            {
                                if(!innewlayer.in(tempval))
                                {
                                    innewlayer.insert(tempval);
                                    newlayer[tempval].clear();
                                }
                                newlayer[tempval].push_back(tempvar);
                            }
                        }
                    }
                }
                }
                /*
                layer = []
                for v in newLayer:
                    preds[v] = newLayer[v]
                    if v in matching:
                        layer.append(matching[v])
                        pred[matching[v]] = v
                    else:
                        unmatched.append(v)
                */
                
                layer.clear();
                
                /*cout<<"Local matching state:"<<endl;
                {
                vector<int>& toiterate = valinlocalmatching.getlist();
                for(int i=0; i<toiterate.size(); ++i)
                {
                    int temp=toiterate[i];
                    D_ASSERT(varinlocalmatching.in(localmatching[temp]));
                    cout << "mapping "<< localmatching[temp] << " to value " << temp <<endl; 
                }
                }*/
                
                {
                vector<int>& toiterate = innewlayer.getlist();
                for(int i=0; i<toiterate.size(); ++i)
                {
                    int tempval = toiterate[i]; // for v in newlayer.
                    //cout << "Looping for value "<< tempval <<endl;
                    
                    D_ASSERT(innewlayer.in(tempval));
                    // insert mapping in vprevious
                    invprevious.insert(tempval);
                    
                    vprevious[tempval]=newlayer[tempval];  // This should be a copy???
                    /*vprevious[tempval].resize(newlayer[tempval].size());
                    for(int x=0; x<newlayer[tempval].size(); x++)
                    {
                        vprevious[tempval][x]=newlayer[tempval][x];
                    }*/
                    
                    if(valinlocalmatching.in(tempval))
                    {
                        int match=localmatching[tempval];
                        //cout << "Matched to variable:" << match << endl;
                        layer.insert(match);
                        uprevious[match]=tempval;
                    }
                    else
                    {
                        //cout<<"inserting value into unmatched:"<<tempval<<endl;
                        unmatched.insert(tempval);
                    }
                }
                }
                
                //cout << "At end of layering loop." << endl;
            }
            //cout << "Out of layering loop."<<endl;
            // did we finish layering without finding any alternating paths?
            // we do not need to calculate unlayered here.
            /*
            # did we finish layering without finding any alternating paths?
            if not unmatched:
                unlayered = {}
                for u in graph:
                    for v in graph[u]:
                        if v not in preds:
                            unlayered[v] = None
                return (matching,list(pred),list(unlayered))
            */
            //cout << "Unmatched size:" << unmatched.size() << endl;
            if(unmatched.size()==0)
            {
                //cout << "Size of matching:" << valinlocalmatching.size() << endl;
                
                if(valinlocalmatching.size()==localnumvars)
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
            
            /*
            for v in unmatched: recurse(v)
            */
            {
            vector<int>& toiterate=unmatched.getlist();
            for(int i=0; i<toiterate.size(); ++i)
            {
                int tempval=toiterate[i];
                //cout<<"unmatched value:"<<tempval<<endl;
                recurse(tempval);
                //cout <<"Returned from recursion."<<endl;
            }
            }
        }
        return false;
    }
    
    /*
    def recurse(v):
			if v in preds:
				L = preds[v]
				del preds[v]
				for u in L:
					if u in pred:
						pu = pred[u]
						del pred[u]
						if pu is unmatched or recurse(pu):
							matching[v] = u
							return 1
			return 0*/

    bool recurse(int val)
    {
        // Again values are val-dom_min in this function.
        // Clearly this should be turned into a loop.
        //cout << "Entering recurse with value " <<val <<endl;
        if(invprevious.in(val))
        {
            vector<int>& listvars=vprevious[val];  //L   // Add ampersand here.
            
            // Remove the value from vprevious.
            invprevious.remove(val);
            
            for(int i=0; i<listvars.size(); ++i)  //for u in L
            {
                int tempvar=listvars[i];
                int pu=uprevious[tempvar];
                if(pu!=-2)   // if u in pred:
                {
                    uprevious[tempvar]=-2;
                    //cout<<"Variable: "<<tempvar<<endl;
                    if(pu==-1 || recurse(pu))
                    {
                        //cout << "Setting "<< tempvar << " to " << val <<endl;
                        if(!valinlocalmatching.in(val))  // If we are not replacing a mapping
                        {
                            valinlocalmatching.insert(val);
                        }
                        localmatching[val]=tempvar;
                        return true;
                    }
                }
            }
        }
        return false;
    }
    
  };



template<typename VarArray>
Constraint*
AlldiffGacSlowCon(StateObj* stateObj, const VarArray& var_array)
{ return new AlldiffGacSlow<VarArray>(stateObj, var_array); }

BUILD_CONSTRAINT1(CT_ALLDIFF_GACSLOW, AlldiffGacSlowCon)



/*
    void tarjan_iterative_unfinished(vector<int> var_indices, bool include_sink)
    {
        // In here values are represented as val-dom_min+numvars or val+valoffset
        int localnumvars=var_indices.size();
        
        tstack.resize(0);
        in_tstack.clear();
        
        visited.clear();
        int max_dfs=0;  //0?
        
        int curnode=var_indices[0];
        bool recurse=true;
        while(true)
        {
            // visit curnode
            if(recurse)
            {
                // This is supposed to be the recursive call. 
                // curnode was changed before setting recurse to 
                // true and continuing.
                recurse=false;
                max_dfs++;
                
                tstack.push_back(curnode);
                in_tstack.insert(curnode);
                visited.insert(curnode);
                dfsnum[curnode]=max_dfs;
                lowlink[curnode]=max_dfs;
                
                // start the inner iteration.
                iterationstack[curnode]=BEGIN;
            }
            
            // Now the loop. Iterate through all edges out of curnode.
            // When recursing for the last value, set iterationstack to END
            // so that it knows to backtrack next.
            int newnode;
            if(curnode<numvars)
            {
                // Curnode represents a variable.
                // Only one outward edge: to its value in the matching.
                D_ASSERT(find(var_indices.begin(), var_indices.end(), curnode)!=var_indices.end());
                if(iterationstack[curnode]==BEGIN)
                {
                    newnode=varvalmatching[curnode]-dom_min+numvars;
                }
                else
                {
                    // done the recursive call, now on the returning path
                    newnode=END;
                }
            }
            else if(curnode<(numvals+numvars))
            {
                // set newnode based on iterationstack[curnode]
            }
            else
            {
                D_ASSERT(curnode==(numvars+numvals));
                D_ASSERT(include_sink);
                
            }
            
            // make the recursive call.
            if(!visited.in(newnode))
            {
                iterationstack[curnode]=newnode;
                curnodestack.push_back(curnode);
                curnode=newnode;
                recurse=true;
                continue;
            }
            else
            {
                // already visited, 
            }
            
            // Now if lowlink == dfsnum, we are at the root of a SCC.
            
            
            
            // Now return from a call. This could be a break or pop_stack and continue..
        }
    }
    */
