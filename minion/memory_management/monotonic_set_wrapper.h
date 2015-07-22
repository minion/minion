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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
* USA.
*/

// This class wraps the MonotonicSet and provides easier access for a
// constraint.

// Everything is in the set to begin with.
// It's like an array of bits, it is allowed to set a 1 to 0 - but not vice
// versa.

class ReversibleMonotonicSet {
private:
  MonotonicSet &MS;
  DomainInt offset;

  D_DATA(DomainInt size);

public:
  // The constructor must be called before the monotonicset is locked.
  ReversibleMonotonicSet(DomainInt _size)
      : MS(getMemory().monotonicSet())
#ifndef NO_DEBUG
        ,
        size(_size)
#endif
  {
    D_ASSERT(size >= 0);
    offset = MS.request_storage(_size);
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

class ReversibleMonotonicBoolean {
private:
  MonotonicSet &MS;
  DomainInt offset;

public:
  // The constructor must be called before the monotonicset is locked.
  ReversibleMonotonicBoolean() : MS(getMemory().monotonicSet()) {
    offset = MS.request_storage(1);
#ifndef NO_DEBUG
    cout << "Set up ReversibleMonotonicSet with size " << 1 << " and offset " << offset << endl;
#endif
  }

  inline bool isMember() { return MS.isMember(offset); }

  inline void remove() { MS.ifMember_remove(offset); }

  inline void unchecked_remove() { MS.unchecked_remove(offset); }
};
