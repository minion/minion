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

/** @help constraints;mddc Description
Another type of table constraint.
*/

/** @help constraints;mddc Example 

MDDC is an implementation of MDDC(sp) by Cheng and Yap.

mddc([x,y,z])

*/

/** @help constraints;mddc Notes
This constraint enforces generalized arc consistency.
*/

#ifndef CONSTRAINT_MDDC_H
#define CONSTRAINT_MDDC_H

#include <stdlib.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include "../constraints/constraint_checkassign.h"

using namespace std;


// MDD constraint
// Implemented as close as possible to the paper by Cheng and Yap. 
// Uses sparse sets. 

// Copied from STR2 -- version that is Chris-optimised. Used for internal domain sets. 
struct arrayset {
    vector<SysInt> vals;
    vector<SysInt> vals_pos;
    SysInt size;
    DomainInt minval;
    
    void initialise(DomainInt low, DomainInt high) {
        minval=low;
        vals_pos.resize(checked_cast<SysInt>(high-low+1));
        vals.resize(checked_cast<SysInt>(high-low+1));
        for(SysInt i=0; i<checked_cast<SysInt>(high-low+1); i++) {
            vals[i]=checked_cast<SysInt>(i+low);
            vals_pos[i]=i;
        }
        size=0;
    }
    
    void clear() {
        size=0;
    }
    
    bool in(DomainInt val) {
        return vals_pos[checked_cast<SysInt>(val-minval)]<size;
    }
    
    
    // This method looks a bit messy, due to stupid C++ optimisers not being
    // clever enough to realise various things don't alias, and this method
    // being called as much as it is.
    void unsafe_insert(DomainInt val)
    {
        D_ASSERT(!in(val));
        const SysInt minval_cpy = checked_cast<SysInt>(minval);
        const SysInt validx = checked_cast<SysInt>(val-minval_cpy);
        const SysInt size_cpy = checked_cast<SysInt>(size);
        const SysInt swapval = vals[size_cpy];
        const SysInt vpvx = vals_pos[validx];
        vals[vpvx]=swapval;
        vals[size_cpy]=checked_cast<SysInt>(val);
        
        vals_pos[checked_cast<SysInt>(swapval-minval_cpy)]=vpvx;
        vals_pos[validx]=size_cpy;
        
        size++;
    }
    
    void insert(DomainInt val) {
        if(!in(val)) {
            unsafe_insert(val);
        }
    }
    
    void unsafe_remove(DomainInt val)
    {
        // swap to posiition size-1 then reduce size
        D_ASSERT(in(val));
        const SysInt validx=checked_cast<SysInt>(val-minval);
        const SysInt swapval=vals[checked_cast<SysInt>(size-1)];
        vals[vals_pos[validx]]=swapval;
        vals[checked_cast<SysInt>(size-1)]=checked_cast<SysInt>(val);
        
        vals_pos[checked_cast<SysInt>(swapval-minval)]=vals_pos[validx];
        vals_pos[validx]=checked_cast<SysInt>(size-1);
        
        size--;
    }

    void remove(DomainInt val) {
        if(in(val)) {
            unsafe_remove(val);
        }
    }
    
    void fill() {
        size=vals.size();
    }
};

// Backtracking version of above. 
struct arrayset_bt {
    vector<SysInt> vals;
    vector<SysInt> vals_pos;
    ReversibleInt size;
    DomainInt minval;
    
    arrayset_bt(StateObj* _stateObj) : size(_stateObj) { }
    
    void initialise(DomainInt low, DomainInt high) {
        minval=low;
        vals_pos.resize(checked_cast<SysInt>(high-low+1));
        vals.resize(checked_cast<SysInt>(high-low+1));
        for(SysInt i=0; i<checked_cast<SysInt>(high-low+1); i++) {
            vals[i]=checked_cast<SysInt>(i+low);
            vals_pos[i]=i;
        }
        size=0;
    }
    
    void clear() {
        size=0;
    }
    
    bool in(DomainInt val) {
        return vals_pos[checked_cast<SysInt>(val-minval)]<size;
    }
    

    // This method looks a bit messy, due to stupid C++ optimisers not being
    // clever enough to realise various things don't alias, and this method
    // being called as much as it is.
    void unsafe_insert(DomainInt val)
    {
        D_ASSERT(!in(val));
        const SysInt minval_cpy = checked_cast<SysInt>(minval);
        const SysInt validx = checked_cast<SysInt>(val-minval_cpy);
        const SysInt size_cpy = checked_cast<SysInt>(size);
        const SysInt swapval = vals[size_cpy];
        const SysInt vpvx = vals_pos[validx];
        vals[vpvx]=swapval;
        vals[size_cpy]=checked_cast<SysInt>(val);
        
        vals_pos[checked_cast<SysInt>(swapval-minval_cpy)]=vpvx;
        vals_pos[validx]=size_cpy;
        
        size=size+1;
    }
    
    void insert(DomainInt val) {
        if(!in(val)) {
            unsafe_insert(val);
        }
    }
    
    void unsafe_remove(DomainInt val)
    {
        // swap to posiition size-1 then reduce size
        D_ASSERT(in(val));
        const SysInt validx=checked_cast<SysInt>(val-minval);
        const SysInt swapval=vals[checked_cast<SysInt>(size-1)];
        vals[vals_pos[validx]]=swapval;
        vals[checked_cast<SysInt>(size-1)]=checked_cast<SysInt>(val);
        
        vals_pos[checked_cast<SysInt>(swapval-minval)]=vals_pos[validx];
        vals_pos[validx]=checked_cast<SysInt>(size-1);
        
        size=size-1;
    }

    void remove(DomainInt val) {
        if(in(val)) {
            unsafe_remove(val);
        }
    }
    
    void fill() {
        size=vals.size();
    }
};

struct MDDNode {
    SysInt id;  // should really be redundant
    vector<SysInt> links;  // pairs val,id
};

template<typename VarArray>
struct MDDC : public AbstractConstraint
{
    virtual string constraint_name()
    { 
        return "mddc";
    }
    
    virtual string full_output_name()
    {
        return ConOutput::print_con(stateObj, constraint_name(), vars); 
    }
    
    VarArray vars;
    
    bool constraint_locked;
    
    //  gyes is set of mdd nodes that have been visited already in this call.
    arrayset gyes;
    // gno is the set of removed mdd nodes.
    arrayset_bt gno;
    
    // The mdd. Top node is at index 0. 
    vector<MDDNode> mddnodes;
    
    vector<arrayset> gacvalues;   // Opposite of the sets in the Cheng and Yap paper: these start empty and are fille d
    
    
    MDDC(StateObj* _stateObj, const VarArray& _var_array, TupleList* _tuples) : 
    AbstractConstraint(_stateObj), 
    vars(_var_array), constraint_locked(false), gno(_stateObj)
    {
        
        init(_tuples);
        
        // set up the two sets of mdd nodes.
        
        gyes.initialise(0,mddnodes.size()-1); 
        gno.initialise(0,mddnodes.size()-1);
        
        // Set up gacvalues.
        
        gacvalues.resize(vars.size());
        for(SysInt i=0; i<vars.size(); i++) {
            gacvalues[i].initialise(vars[i].getInitialMin(), vars[i].getInitialMax());
        }
        
    }
    
    //
    // This one accepts an mdd written in a fairly difficult format. 
    void old_init(TupleList* tuples) {
        // convert tuples into mdd nodes
        int tlsize=tuples->size();
        
        int tuplelen=tuples->tuple_size();
        
        DomainInt* tupdata=tuples->getPointer();
        
        CHECK(tuplelen%2 == 0, "Tuples must be even length in MDDC");  // Check it is even length. 
        
        mddnodes.resize(tlsize);
        
        for(int nodeid=0; nodeid<tlsize; nodeid++) {
            vector<DomainInt> tup(tupdata+(tuplelen*nodeid), tupdata+(tuplelen*(nodeid+1) ));   // inefficient.
            
            if(nodeid<tlsize-1) {
                // Not final tt node.
                for(int pair=0; pair<tup.size(); pair=pair+2) {
                    if(tup[pair]==-1 && tup[pair+1]==-1) break;
                    
                    CHECK(tup[pair+1] >0 && tup[pair+1]<tlsize, "Links in MDD must be in range 1 up to the number of nodes");
                    
                    mddnodes[nodeid].links.push_back(tup[pair]);  // push the domain value
                    mddnodes[nodeid].links.push_back(tup[pair+1]);  // push the id number for the link.
                }
                
                mddnodes[nodeid].id=nodeid;
                
            }
            else {
                // Final tt node.
                CHECK(tup[0]==-1 && tup[1]==-1, "Final MDD node must be all -1s, cannot link to any other nodes.");
                mddnodes[nodeid].id=-1;
            }
        }
    }
    
    // This one converts a list of tuples (i.e. a standard table constraint) to an mdd. 
    void init(TupleList* tuples) {
        // First build a trie by inserting the tuples one by one.
        
        int tlsize=tuples->size();
        
        int tuplelen=tuples->tuple_size();
        
        DomainInt* tupdata=tuples->getPointer();
        
        // Make the top node.
        MDDNode node0;
        node0.id=0;
        mddnodes.push_back(node0);
        
        // Make the t-terminal node.
        MDDNode node1;
        node1.id=-1;
        mddnodes.push_back(node1);
        
        for(int tupid=0; tupid<tlsize; tupid++) {
            vector<DomainInt> tup(tupdata+(tuplelen*tupid), tupdata+(tuplelen*(tupid+1) ));   // inefficient.
            
            MDDNode* curnode=&(mddnodes[0]);
            for(int i=0; i<tuplelen; i++) {
                
                // Linear search for value.
                // Should be replaced with binary search.
                int idx=-1;
                for(int j=0; j<curnode->links.size(); j=j+2) {
                    if(curnode->links[j]==tup[i]) {
                        idx=j;
                        break;
                    }
                }
                
                if(idx==-1) {
                    // New node needed.
                    MDDNode newnode;
                    if(i<tuplelen-1) {
                        newnode.id=mddnodes.size();
                    }
                    else {
                        newnode.id=-1;   // At the end of the tuple -- make a tt node. 
                    }
                    mddnodes.push_back(newnode);  // copies it into the vector
                    
                    // Make the link
                    for(int j=0; j<curnode->links.size(); j=j+2) {
                        if(curnode->links[j]>tup[i]) {
                            // this is the point to insert it. 
                            curnode->links.insert(curnode->links.begin()+j, tup[i]);
                            curnode->links.insert(curnode->links.begin()+j+1, newnode.id);
                            break;
                        }
                    }
                    
                    // Move to this new node. 
                    curnode=&(mddnodes[newnode.id]);
                }
                else {
                    // Follow the link.
                    curnode=&(mddnodes[curnode->links[idx+1]]);
                }
            }
            
        }
        
        // Now MDDNodes is a trie with lots of tt nodes as the leaves.
        // Start merging from the leaves upwards. 
        
        return;
    }
    
    
    // Set up triggers.
    virtual triggerCollection setup_internal()
    {
        triggerCollection t;
        SysInt array_size = vars.size();
        for(SysInt i = 0; i < array_size; ++i) {
            t.push_back(make_trigger(vars[i], Trigger(this, i), DomainChanged));
        }
        return t;
    }
    
    virtual void full_propagate() {
        // Just propagate. 
        do_prop();
    }
    
    virtual vector<AnyVarRef> get_vars()
    {
      vector<AnyVarRef> ret;
      ret.reserve(vars.size());
      for(unsigned i = 0; i < vars.size(); ++i)
        ret.push_back(vars[i]);
      return ret;
    }
    
    virtual bool check_assignment(DomainInt* tup, SysInt v_size)
    {
        MDDNode* curnode=&(mddnodes[0]);
        for(SysInt i=0; i<v_size; i++) {
            // Linear search for value.
            // Should be replaced with binary search.
            int idx=-1;
            for(int j=0; j<curnode->links.size(); j=j+2) {
                if(curnode->links[j]==tup[i]) {
                    idx=j;
                    break;
                }
            }
            
            if(idx==-1) return false;
            
            curnode=&(mddnodes[curnode->links[idx+1]]);
        }
        D_ASSERT(curnode->id==-1);
        
        return false;
    }

    virtual bool get_satisfying_assignment(box<pair<SysInt,DomainInt> >& assignment)
    {
        // Run the propagator to cut off (add to gno) all parts of the MDD that
        // cannot be in a satisfying tuple. 
        
        
        
        
        /*for(SysInt i = 0; i < sct->compressed_tuples.size(); ++i)
        {
            bool sat = true;
            for(SysInt j = 0; j < sct->compressed_tuples[i].size(); ++j)
            {
                if(!vars[sct->compressed_tuples[i][j].first].inDomain(sct->compressed_tuples[i][j].second))
                {
                    sat = false;
                    break;
                }
            }

            if(sat)
            {
                for(SysInt j = 0; j < sct->compressed_tuples[i].size(); ++j)
                    assignment.push_back(sct->compressed_tuples[i][j]);
                return true;
            }
        }*/

        return false;
    }
    
    virtual AbstractConstraint* reverse_constraint()
    { return forward_check_negation(stateObj, this); }
    
    
    virtual void propagate(DomainInt prop_var, DomainDelta)
    {
        if(!constraint_locked)
        {
            constraint_locked = true;
            getQueue(stateObj).pushSpecialTrigger(this);
        }
    }
    
    virtual void special_unlock() { constraint_locked = false;  }
    
    virtual void special_check()
    {
        constraint_locked = false;
        D_ASSERT(!getState(stateObj).isFailed());
        do_prop();
    }
    
    SysInt delta;
    
    void do_prop() {
        
        // Clear gyes -- gno persists from the last call. 
        gyes.clear();
        
        // Clear gacvalues
        for(SysInt var=0; var<vars.size(); var++) {
            gacvalues[var].clear();
        }
        
        
        delta=vars.size();    // Horizon for the dfs. All vars from delta onwards are considered to have full support. 
        
        
        mddcrecurse(mddnodes[0], 0);
        
        
        // Prune the domains.
        for(SysInt var=0; var<delta; var++) {
            for(DomainInt val=vars[var].getMin(); val<=vars[var].getMax(); val++) {
                if(!gacvalues[var].in(val)) {
                    vars[var].removeFromDomain(val);
                }
            }
        }
        
        // Don't need the other bits of the algorithm because Minion does it for us.
    }
    
    // DFS of the MDD. 
    bool mddcrecurse(MDDNode& curnode, SysInt level) {
        if(curnode.id==-1) {
            //  special value indicating this is node tt.
            if(level<delta) {
                delta=level; // This variable and all >= are now fully supported.
            }
            return true;
        }
        
        if(curnode.id==-2) {
            // special value for ff.  This is never used.
            return false;
        }
        
        if(gyes.in(curnode.id)) {
            // if curnode is in the gyes set
            return true;
        }
        
        if(gno.in(curnode.id)) {
            // if in gno set
            return false;
        }
        
        bool res=false;
        
        // Iterate through links from this MDD node. 
        
        vector<SysInt>& links=curnode.links;
        
        for(int k=0; k<links.size(); k=k+2) {
            DomainInt val=curnode.links[k];
            SysInt idx=curnode.links[k+1];
            
            if(vars[level].inDomain(val)) {
                bool returnvalue=mddcrecurse(mddnodes[idx], level+1);
                
                if(returnvalue) {
                    res=true;
                    gacvalues[level].insert(val);
                    if(level+1 == delta && gacvalues[level].size == vars[level].getDomSize()) {
                        delta=level;
                        break;  // for k loop.
                    }
                }
                
            }
            
        }
        
        if(res) {
            // add to gyes
            D_ASSERT(!gyes.in(curnode.id));
            gyes.insert(curnode.id);
        }
        else {
            // add to gno
            D_ASSERT(!gno.in(curnode.id));
            gno.insert(curnode.id);
        }
        
        return res;
    }
    
};


#endif
