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

#ifndef ALLDIFF_GCC_SHARED_H
#define ALLDIFF_GCC_SHARED_H

#include <vector>
#include "constraint_abstract.h"

#define REVERSELIST   // Is this really necessary now?

#ifdef P
#undef P
#endif

#define P(x)
//#define P(x) cout << x << endl

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
        D_ASSERT(membership.size()==0);
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
    
    inline void remove(int val)
    {
        D_DATA(cout << "Warning: smallset::remove is slow and untested." <<endl );
        if(in(val))
        {
            membership[val]=0;
            list.erase(find(list.begin(), list.end(), val));
        }
    }
    
    inline vector<int>& getlist()
    {
        return list;
    }
    
    inline void clear()
    {
        if(cert>2000000000)
        {
            list.clear();
            cert=1;
            for(int i=0; i<membership.size(); i++)
            {
                membership[i]=0;
            }
        }
        else
        {
            cert++;
            list.clear();
        }
    }
};

struct smallset_nolist
{
    // a small set of integers (could be templated?) which
    // clears in constant time, set membership in
    // constant time, no iteration
    // Add and remove item is constant time
    
    unsigned int cert;
    
    vector<unsigned int> membership;
    
    void reserve(int size)
    {
        D_ASSERT(membership.size()==0);
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
        if(cert>2000000000)
        {
            cert=1;
            for(int i=0; i<membership.size(); i++)
            {
                membership[i]=0;
            }
        }
        else
        {
            cert++;
        }
    }
};


struct smallset_nolist_bt
{
    MoveablePointer membership;
    
    int arraysize;
    
    void reserve(int size, StateObj * stateObj)
    {
        // This must be called before anything is put in the set.
        
        int required_mem = size / 8 + 1;
        // Round up to nearest data_type block
        required_mem += sizeof(int) - (required_mem % sizeof(int));
        
        arraysize=required_mem/sizeof(int);
        
        membership= getMemory(stateObj).backTrack().request_bytes(required_mem);
    }
    
    inline bool in(int val)
    {
        D_ASSERT(val/(sizeof(int)*8) <arraysize);
        int shift_offset = 1 << (val % (sizeof(int)*8));
        return ((int *)membership.get_ptr())[val/(sizeof(int)*8)] & shift_offset;
    }
    
    inline void insert(int val)
    {
        D_ASSERT(val/(sizeof(int)*8) <arraysize);
        
        int shift_offset = 1 << (val % (sizeof(int)*8));
        
        ((int *)membership.get_ptr())[val/(sizeof(int)*8)] |= shift_offset;
    }
    
    inline void remove(int val)
    {
        D_ASSERT(val/(sizeof(int)*8) <arraysize);
        
        int shift_offset = 1 << (val % (sizeof(int)*8));
        
        ((int *)membership.get_ptr())[val/(sizeof(int)*8)] &= shift_offset;
    }
    
    inline void clear()
    {
        for(int i=0; i<arraysize; i++)
        {
            ((int *)membership.get_ptr())[i]=0;
        }
    }
};


struct smallset_list_bt
{
    // This one can only be cleared then populated.
    // Must not be partially populated, then go to
    // a new node, then populated some more.
    // Membership array does not backtrack, clearly.
    
    unsigned int cert;
    
    vector<unsigned int> membership;
    
    MoveablePointer list;
    int maxsize;
    
    void reserve(int size, StateObj * stateObj)
    {
        // This must be called before anything is put in the set.
        maxsize=size;
        membership.resize(size);
        
        for(int i=0; i<size; i++) membership[i]=0;
        
        cert=1;
        list= getMemory(stateObj).backTrack().request_bytes((size+1)*sizeof(short));
        ((short*)list.get_ptr())[maxsize]=0;   // The count is stored in the last element of the array.
    }
    
    inline bool in(int val)
    {
        D_ASSERT(val<maxsize && val>=0);
        
        return membership[val]==cert;
    }
    
    inline void insert(int val)
    {
        D_ASSERT(val<maxsize && val>=0);
        //D_DATA(print());
        
        D_DATA(sanitycheck());
        if(membership[val]==cert)
        {
            return;
        }
        membership[val]=cert;
        short * ptr=((short*) list.get_ptr());
        int count=ptr[maxsize];
        D_ASSERT(count<maxsize);
        ptr[maxsize]=(short)count+1;
        ptr[count]=(short)val;
        //D_DATA(print());
        D_DATA(sanitycheck());
    }
    
    inline void clear()
    {
        D_DATA(cout << "clearing list "<< (list.get_ptr()) << endl);
        D_ASSERT(cert< 2000000000);
        
        if(cert>2000000000)
        {
            cert=1;
            for(int i=0; i<membership.size(); i++)
            {
                membership[i]=0;
            }
        }
        else
        {
            cert++;
        }
        
        ((short *)list.get_ptr())[maxsize]=0;
    }
    
    int size()
    {
        return (int) ((short *)list.get_ptr())[maxsize];
    }
    
    void sanitycheck()
    {
        short* l = (short *) list.get_ptr();
        for(int i=0; i<l[maxsize]; i++)
        {
            for(int j=i+1; j<l[maxsize]; j++)
            {
                D_ASSERT(l[i]!=l[j]);
            }
            D_ASSERT(membership[l[i]]==cert);
        }
        
        
    }
    
    void print()
    {
        short * l = (short *)list.get_ptr();
        cout << "smallset_list_bt length:" << l[maxsize] << " at location "<< (&l[maxsize]) << endl;
        for(int i=0; i<maxsize; i++)
        {
            cout << "smallset_list_bt item:" << l[i] << " at location "<< (&l[i]) << endl;
        
        }
        cout<<"certificate:"<<cert<<endl;
        cout<<membership <<endl;
    }
};


template<typename VarArray, bool UseIncGraph>
struct FlowConstraint : public AbstractConstraint
{
    protected:
    
    // Base class for GAC alldiff and GCC
    FlowConstraint(StateObj* _stateObj, const VarArray& _var_array) : AbstractConstraint(_stateObj),
    numvars(0), numvals(0), dom_min(0), dom_max(0),
    #ifndef REVERSELIST
    var_array(_var_array), 
    #else
    var_array(_var_array.rbegin(), _var_array.rend()),
    #endif
    constraint_locked(false)
    {
        if(var_array.size()>0)
        {
            dom_min=var_array[0].getInitialMin();
            dom_max=var_array[0].getInitialMax();
        }
        for(int i=0; i<var_array.size(); ++i)
        {
            if(var_array[i].getInitialMin()<dom_min)
                dom_min=var_array[i].getInitialMin();
            if(var_array[i].getInitialMax()>dom_max)
                dom_max=var_array[i].getInitialMax();
        }
        numvars=var_array.size();  // number of variables in the constraint
        numvals=dom_max-dom_min+1;
        
        //to_process.reserve(var_array.size()); Could this be shared as well??
        
        if(UseIncGraph)
        {
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
            adjlistlength=getMemory(stateObj).backTrack().template requestArray<int>(numvars+numvals);
            for(int i=0; i<numvars; i++) adjlistlength[i]=numvals;
            for(int i=numvars; i<numvars+numvals; i++) adjlistlength[i]=numvars;
        }
        
        #ifndef BTMATCHING
        varvalmatching.resize(numvars); // maps var to actual value
        valvarmatching.resize(numvals); // maps val-dom_min to var.
        #else
        varvalmatching=getMemory(stateObj).backTrack().template requestArray<int>(numvars);
        valvarmatching=getMemory(stateObj).backTrack().template requestArray<int>(numvals);
        #endif
    }
    
    int numvars, numvals, dom_min, dom_max;
    
    VarArray var_array;
    
    bool constraint_locked;
    
    #ifndef BTMATCHING
    vector<int> varvalmatching; // For each var, give the matching value.
    // valvarmatching is from val-dom_min to var.
    vector<int> valvarmatching;   // need to set size somewhere.
    // -1 means unmatched.
    #else
    MoveableArray<int> varvalmatching;
    MoveableArray<int> valvarmatching;
    #endif
    
    // ------------------ Incremental adjacency lists --------------------------
    
    // adjlist[varnum or val-dom_min+numvars] is the vector of vals in the 
    // domain of the variable, or variables with val in their domain.
    vector<vector<int> > adjlist;
    MoveableArray<int> adjlistlength;
    vector<vector<int> > adjlistpos;   // position of a variable in adjlist.
    
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
    
    
    // -------------------------Hopcroft-Karp algorithm -----------------------------
    // Can be applied to a subset of var_array as required.
    
    // Each domain value has a label which is numvars+
    
    
    // These two are for the valvar version of hopcroft.
    smallset_nolist varinlocalmatching;    // indicates whether a var is recorded in localmatching.
    smallset valinlocalmatching;
    
    //smallset varinlocalmatching;    // indicates whether a var is recorded in localmatching.
    //smallset_nolist valinlocalmatching;
    
    
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
    
    // Hopcroft-Karp which takes start and end indices.
    
    inline bool hopcroft_wrapper(int sccstart, int sccend, vector<int>& SCCs)
    {
        // Call hopcroft for the whole matching.
        if(!hopcroft(sccstart, sccend, SCCs))
        {
            // The constraint is unsatisfiable (no matching).
            P("About to fail. Changed varvalmatching: "<< varvalmatching);
            
            for(int j=0; j<numvars; j++)
            {
                // Restore valvarmatching because it might be messed up by Hopcroft.
                valvarmatching[varvalmatching[j]-dom_min]=j;
            }
            
            getState(stateObj).setFailed(true);
            return false;
        }
        
        // Here, copy from valvarmatching to varvalmatching.
        // Using valinlocalmatching left over from hopcroft.
        // This must not be done when failing, because it might mess
        // up varvalmatching for the next invocation.
        {vector<int>& toiterate=valinlocalmatching.getlist();
            for(int j=0; j<toiterate.size(); j++)
            {
                int tempval=toiterate[j];
                varvalmatching[valvarmatching[tempval]]=tempval+dom_min;
            }
        }
        return true;
    }
    
    inline bool hopcroft(int sccstart, int sccend, vector<int>& SCCs)
    {
        // Domain value convention:
        // Within hopcroft and recurse,
        // a domain value is represented as val-dom_min always.
        
        // Variables are always represented as their index in
        // var_array. sccstart and sccend indicates which variables
        // we are allowed to use here.
        
        int localnumvars=sccend-sccstart+1;
        
        // Construct the valinlocalmatching for this SCC, checking each val
        // to see it's in the relevant domain.
        valinlocalmatching.clear();
        
        for(int i=sccstart; i<=sccend; i++)
        {
            int tempvar=SCCs[i];
            if(var_array[tempvar].inDomain(varvalmatching[tempvar]))
            {
                valinlocalmatching.insert(varvalmatching[tempvar]-dom_min);
                // Check the two matching arrays correspond.
                //D_ASSERT(valvarmatching[varvalmatching[tempvar]-dom_min]==tempvar);
            }
        }
        
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
            // WHY end up with duplicates in valvarmatching here???????
            // Because it's left over from a bad state when do_prop was last invoked, and failed.
            varinlocalmatching.clear();
            {
                vector<int>& toiterate=valinlocalmatching.getlist();
                for(int i=0; i<toiterate.size(); ++i)
                {
                    if(!varinlocalmatching.in(valvarmatching[toiterate[i]]))   // This should not be conditional --BUG
                        varinlocalmatching.insert(valvarmatching[toiterate[i]]);
                }
            }
            
            for(int i=sccstart; i<=sccend; ++i)
            {
                int tempvar=SCCs[i];
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
                        int match=valvarmatching[tempval];
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
    
    
    bool recurse(int val)
    {
        // Again values are val-dom_min in this function.
        // Clearly this should be turned into a loop.
        //cout << "Entering recurse with value " <<val <<endl;
        if(invprevious.in(val))
        {
            vector<int>& listvars=vprevious[val];  //L
            
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
                        
                        valvarmatching[val]=tempvar;
                        //varvalmatching[tempvar]=val+dom_min;  // This will be 
                        return true;
                    }
                }
            }
        }
        return false;
    }
    
    // ----------------------- new hopcroft-karp implementation ----------------
    
    // This one uses dom_min-1 as a marker for 'free variable' in matching.
    // also has upper as the upper bound for value nodes, indexed by val+dom_min
    // usage is the occurrences of each value in the matching. 
    
    // Oh no -- does this work with SCCs??
    // First do it without using SCCs.
    // Also ignore the incremental graph to begin with....
    
    inline bool hopcroft_wrapper2(int sccstart, int sccend, vector<int>& SCCs, vector<int>& matching, vector<int>& upper, vector<int>& usage)
    {
        if(!hopcroft2(sccstart, sccend, SCCs, matching, upper, usage))
        {
            getState(stateObj).setFailed(true);
            return false;
        }
        return true;
    }
    
    inline bool hopcroft2(int sccstart, int sccend, vector<int>& SCCs, vector<int>& matching, vector<int>& upper, vector<int>& usage)
    {
        // The return value is whether the matching is complete over teh variables
        // in the SCC.
        D_ASSERT(sccstart==0 && sccend==(SCCs.size()-1));
        // Clear any values from matching which are no longer in domain.
        for(int i=sccstart; i<=sccend; i++)
        {
            int var=SCCs[i];
            if(matching[var]!=dom_min-1 &&
                !var_array[var].inDomain(matching[var]))
            {
                usage[matching[var]-dom_min]--;
                matching[var]=dom_min-1;
            }
        }
        
        int localnumvars=sccend-sccstart+1;
        
        // This is the wrong datastructure. Shoudl be edges, not vertices.
        // Or perhaps both, because a vertex is removed when the DFS explores it.
        vector<vector<int> > layers; // turn this into a box of boxes??
        
        layers.resize(10000);
        smallset_nolist valinlayer;
        smallset_nolist varused;
        smallset_nolist valused;
        
        valinlayer.reserve(numvals);
        varused.reserve(numvars);
        valused.reserve(numvals);
        
        // Note: multiple nodes representing each domain value. 
        // All nodes for a particular value end up in the same layer
        // because they all have the same set of non-matching edges NOT TRUE.
        
        while(true)
        {
            // Find all free variables in current SCC and insert into layers[0].
            
            // real stuff starts here
            layers[0].clear();
            
            int unmatched=0;
            for(int i=sccstart; i<=sccend; ++i)
            {
                int tempvar=SCCs[i];
                if(matching[tempvar]==dom_min-1)
                {
                    layers[0].push_back(tempvar);
                    unmatched++;
                }
            }
            
            if(unmatched==0)
            {
                return true;
            }
            
            int curlayer=0;
            varused.clear();
            valused.clear();
            
            // continue layering until we see a free value vertex.
            
            bool foundFreeValNode=false;
            while(layers[curlayer].size()!=0)
            {
                layers[curlayer+1].clear();
                
                vector<int>& layer1=layers[curlayer];
                // at a variable layer, extend it to a value layer using 
                // values which are not saturated.
                valinlayer.clear();
                for(int i=0; i<layer1.size(); i++)
                {
                    int tempvar=layer1[i];
                    for(DomainInt realval=var_array[tempvar].getMin(); realval<=var_array[tempvar].getMax(); realval++)
                    {
                        if(var_array[tempvar].inDomain(realval))
                        {
                            int tempval=realval-dom_min;
                            if(!valused.in(tempval)) 
                            {
                                layers[curlayer+1].push_back(tempval);
                                valinlayer.insert(tempval);
                                valused.insert(tempval);
                                
                                if(upper[tempval]>usage[tempval])
                                {
                                    foundFreeValNode=true;
                                }
                            }
                        }
                    }
                }
                curlayer++;
                layers[curlayer+1].clear();
                
                if(foundFreeValNode)
                {
                    break;
                }
                else if(layers[curlayer].size()==0)
                {
                    // no augmenting paths
                    return false;
                }
                
                // at a value layer, extend it to a variable layer using
                // variables which are matched to one of the values.
                // Probably shouldn't iterate through variables like this.
                
                vector<int>& layer2=layers[curlayer];
                
                for(int i=sccstart; i<=sccend; i++)
                {
                    int tempvar=SCCs[i];
                    if(!varused.in(tempvar))
                    {
                        int tempval=matching[tempvar]-dom_min;
                        if(valinlayer.in(tempval))
                        {
                            layers[curlayer+1].push_back(tempvar);
                            varused.insert(tempvar);
                        }
                    }
                }
                
                curlayer++;
                
                if(layers[curlayer].size()==0)
                {
                    // no augmenting paths
                    return false;
                }
            }
            
            if(foundFreeValNode)
            {
                // Find a set of minimal-length augmenting paths using DFS within
                // the layers.
                // starting at layer 0.
                
                vector<int> augpath; // alternating path stored here with vars and val-dom_min
                
                augpath.clear();
                dfs_hopcroft2(augpath
            }
            
        } // end of main loop.
        
        // should not be possible to reach here.
        D_ASSERT(false);
        return false;
    }
    
    bool dfs_hopcroft2(vector<int>& augpath, vector<int>& upper, vector<int>& usage, vector<int>& matching)
    {
        int layer=augpath.size(); // depth of recursion, starting at 0
        
        if(layer & 1)  // if layer is odd
        {   // values layer
            int tempvar=augpath[layer-1];
            
            // going from a var layer to a val layer
            for(int i=0; i<layers[layer].size(); i++)
            {
                int tempval=layers[layer][i];    // problem: is it a real value or a val-dom_min in layers??
                if(matching[tempvar]!=(tempval+dom_min)
                    && var_array[tempvar].inDomain(tempval+dom_min))
                {
                    augpath.push_back(tempval);
                    if(upper[tempval]>usage[tempval])
                    {
                        apply_augmenting_path(augpath); // update matching and usage.
                        if(upper[tempval]==usage[tempval])
                        {
                            // if this value vertex is now saturated, delete it from
                            // the layer.
                            layers[layer][i]=layers[layer][layers[layer].size()-1];
                            layers[layer].pop_back();
                        }
                        return true;
                    }
                    if(upper[tempval]==usage[tempval])
                    {
                        // if this value vertex is saturated, delete it from
                        // the layer.
                        layers[layer][i]=layers[layer][layers[layer].size()-1];
                        layers[layer].pop_back();
                        i--; // we just replaced the item at i, so do position i
                        // again next time around the loop.
                    }
                    
                    if(dfs_hopcroft2(augpath, upper, usage, matching))
                    {
                        return true;
                    }
                    augpath.pop_back();
                }
            }
            // didn't find an unsaturated vertex or a path to one.
            return false;
        }
        else
        {   // going from a val layer to a var layer.
            // Only edges in the matching.
            
            // This is a special case because it ignores the 
            if(layer==0)
            {
                for(int i=0; i<layers[layer].size(); i++)
                {
                    D_ASSERT(augpath.empty());
                    augpath.push_back(layers[layer][i]);
                    
                    
                    
                    augpath.pop_back();
                }
            }
            else
            {
                int tempval=augpath[layer-1];   // what about first layer??
                for(int i=0; i<layers[layer][i].size(); i++)
                {
                    
                }
            }
        }
    }
    
    inline void apply_augmenting_path(vector<int>& augpath, vector<int>& matching, vector<int>& usage)
    {
        D_ASSERT((augpath.size() & 1) == 0);
        for(int i=0; i<augpath.size(); i=i+2)
        {
            int var=augpath[i];
            int val=augpath[i+1];
            if(matching[var]!=dom_min-1)
            {
                usage[matching[var]-dom_min]--;
            }
            matching[var]=val+dom_min;
            usage[val]++;
        }
        augpath.clear();
    }
    
    
};

struct deque_fixed_size
{
    // replacement for stl deque. This one is a fixed size circular array.
    // pluggable for deque in gcc_common.h -- no faster.
    vector<int> list;
    int head, tail;
    
    deque_fixed_size()
    {
        head=tail=0;
    }
    
    void reserve(int size)
    {
        list.resize(size);
    }
    
    inline void clear()
    {
        head=tail=0;
    }
    
    inline bool empty()
    {
        return head==tail;
    }
    
    inline void push_back(int val)
    {
        list[tail]=val;
        if(++tail == list.size())
        {
            tail=0;
        }
    }
    
    inline int front()
    {
        D_ASSERT(head!=tail);
        return list[head];
    }
    
    inline void pop_front()
    {
        D_ASSERT(head!=tail);
        if(++head == list.size())
        {
            head=0;
        }
    }
};

#endif
