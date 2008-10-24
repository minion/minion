#ifndef ALLDIFF_GAC_SHARED_H
#define ALLDIFF_GAC_SHARED_H

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

#endif
