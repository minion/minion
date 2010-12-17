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
  }
  
  int numvars, numvals, dom_min, dom_max;
    
    VarArray var_array;
    
    bool constraint_locked;
    
    
  
    // Incremental adjacency lists
    
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
    
};

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

struct deque_fixed_size
{
    // replacement for stl deque. This one is a fixed size circular array.
    // pluggable for deque in gcc_common.h
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
