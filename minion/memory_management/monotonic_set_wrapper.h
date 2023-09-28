// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

// This class wraps the MonotonicSet and provides easier access for a
// constraint.

// Everything is in the set to begin with.
// It's like an array of bits, it is allowed to set a 1 to 0 - but not vice
// versa.

class ReversibleMonotonicSet {
private:
  MonotonicSet& MS;
  DomainInt offset;

  D_DATA(DomainInt size);

public:
  // The constructor must be called before the monotonicset is locked.
  ReversibleMonotonicSet(DomainInt Size)
      : MS(getMemory().monotonicSet())
#ifndef NO_DEBUG
        ,
        size(Size)
#endif
  {
    D_ASSERT(size >= 0);
    offset = MS.request_storage(Size);
  }

  bool isMember(DomainInt ref) {
    D_ASSERT(ref < size && ref >= 0);
    return MS.isMember(ref + offset);
  }

  void remove(DomainInt ref) {
    D_ASSERT(ref < size && ref >= 0);
    MS.ifMember_remove(ref + offset);
  }

  void unchecked_remove(DomainInt ref) {
    D_ASSERT(ref < size && ref >= 0);
    MS.unchecked_remove(ref + offset);
  }
};
