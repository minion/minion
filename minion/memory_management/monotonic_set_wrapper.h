

// This class wraps the MonotonicSet and provides easier access for a constraint.

// Everything is in the set to begin with.
// It's like an array of bits, it is allowed to set a 1 to 0 - but not vice versa.

class ReversibleMonotonicSet
{
private:
    MonotonicSet & MS;
    int offset;
    
  #ifndef NO_DEBUG
    int size;
  #endif
    
public:
    // The constructor must be called before the monotonicset is locked.
    ReversibleMonotonicSet(StateObj * stateObj, int _size) : MS(getMemory(stateObj).monotonicSet())
  #ifndef NO_DEBUG  
    , size(_size)
  #endif
    {
        D_ASSERT(&getMemory(stateObj).monotonicSet() !=NULL);
        offset=MS.request_storage(_size);
        #ifndef NO_DEBUG
        cout << "Set up ReversibleMonotonicSet with size "<< _size << " and offset " << offset <<endl;
        #endif
    }
    
    bool isMember(int ref)
    {
        D_ASSERT(ref<size && ref>=0);
        return MS.isMember(ref+offset);
    }
    
    void remove(int ref)
    {
        D_ASSERT(ref<size && ref>=0);
        MS.ifMember_remove(ref+offset);
    }
    
    void unchecked_remove(int ref)
    {
        D_ASSERT(ref<size && ref>=0);
        MS.unchecked_remove(ref+offset);
    }
};


class ReversibleMonotonicBoolean
{
private:
    MonotonicSet & MS;
    int offset;
public:
    // The constructor must be called before the monotonicset is locked.
    ReversibleMonotonicBoolean(StateObj * stateObj) : MS(getMemory(stateObj).monotonicSet())
    {
        offset=MS.request_storage(1);
        #ifndef NO_DEBUG
        cout << "Set up ReversibleMonotonicSet with size "<< 1 << " and offset " << offset <<endl;
        #endif
    }
    
    inline bool isMember()
    {
        return MS.isMember(offset);
    }
    
    inline void remove()
    {
        MS.ifMember_remove(offset);
    }
    
    inline void unchecked_remove()
    {
        MS.unchecked_remove(offset);
    }
};


