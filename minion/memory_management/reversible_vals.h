// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef REVERSIBLE_VALS_H
#define REVERSIBLE_VALS_H

#include "MemoryBlock.h"
#include "nonbacktrack_memory.h"

#include "../solver.h"

// \addtogroup Memory
// @{

/// Provides a wrapper around a single backtrackable value.
/** This class aims to act like a normal isntance of 'Type', but is
 *  backtracked.
 */
template <typename Type>
class Reversible {
  void* backtrack_ptr;

public:
  /// Automatic conversion to the type.
  operator Type() const {
    Type* ptr = (Type*)(backtrack_ptr);
    return *ptr;
  }

  /// Assignment operator.
  void operator=(const Type& newval) {
    Type* ptr = (Type*)(backtrack_ptr);
    *ptr = newval;
  }

  void operator++() {
    *this = *this + 1;
  }

  void operator--() {
    *this = *this - 1;
  }

  Reversible() {
    backtrack_ptr = getMemory().backTrack().request_bytes(sizeof(Type));
    D_ASSERT((size_t)(backtrack_ptr) % sizeof(Type) == 0);
  }

  /// Constructs and assigns in one step.
  Reversible(Type t) {
    backtrack_ptr = getMemory().backTrack().request_bytes(sizeof(Type));
    D_ASSERT((size_t)(backtrack_ptr) % sizeof(Type) == 0);
    (*this) = t;
  }

  /// Provide output.
  friend std::ostream& operator<<(std::ostream& o, const Reversible& v) {
    return o << Type(v);
  }
};

class BoolContainer {

  void* backtrack_ptr;
  SysInt offset;

public:
  BoolContainer() : offset(sizeof(SysInt) * 8) {}

  pair<void*, UnsignedSysInt> returnBacktrackBool() {
    if(offset == sizeof(SysInt) * 8) {
      offset = 0;
      backtrack_ptr = getMemory().backTrack().request_bytes(sizeof(SysInt));
    }

    pair<void*, UnsignedSysInt> ret(backtrack_ptr, ((UnsignedSysInt)1) << offset);
    offset++;
    return ret;
  }
};

template <>
class Reversible<bool> {
  void* backtrack_ptr;
  UnsignedSysInt mask;

public:
  /// Automatic conversion to the type.
  operator bool() const {
    UnsignedSysInt* ptr = (UnsignedSysInt*)(backtrack_ptr);
    return (*ptr & mask);
  }

  /// Assignment operator.
  void operator=(const bool& newval) {
    UnsignedSysInt* ptr = (UnsignedSysInt*)(backtrack_ptr);
    if(newval)
      *ptr |= mask;
    else
      *ptr &= ~mask;
  }

  Reversible() {
    pair<void*, UnsignedSysInt> state = getBools().returnBacktrackBool();
    backtrack_ptr = state.first;
    mask = state.second;
  }

  /// Constructs and assigns in one step.
  Reversible(bool b) {
    pair<void*, UnsignedSysInt> state = getBools().returnBacktrackBool();
    backtrack_ptr = state.first;
    mask = state.second;
    (*this) = b;
  }

  /// Provide output.
  // friend std::ostream& operator<<(std::ostream& o, const Reversible<Bool>& v)
  //{ return o << Bool(v); }
};

/// Specialisation for backwards compatability.
typedef Reversible<SysInt> ReversibleInt;

// @}

#endif
