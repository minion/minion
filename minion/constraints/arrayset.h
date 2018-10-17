#ifndef ARRAYSET_H
#define ARRAYSET_H

struct arrayset {
  vector<SysInt> vals;
  vector<SysInt> vals_pos;
  DomainInt size;
  DomainInt minval;

  void initialise(DomainInt low, DomainInt high) {
    minval = low;
    vals_pos.resize(checked_cast<SysInt>(high - low + 1));
    vals.resize(checked_cast<SysInt>(high - low + 1));
    for(SysInt i = 0; i < checked_cast<SysInt>(high - low + 1); i++) {
      vals[i] = checked_cast<SysInt>(i + low);
      vals_pos[i] = i;
    }
    size = 0;
  }

  void clear() {
    size = 0;
  }

  bool in(DomainInt val) {
    return vals_pos[checked_cast<SysInt>(val - minval)] < size;
  }

  // This method looks a bit messy, due to stupid C++ optimisers not being
  // clever enough to realise various things don't alias, and this method
  // being called as much as it is.
  void unsafe_insert(DomainInt val) {
    D_ASSERT(!in(val));
    const SysInt minval_cpy = checked_cast<SysInt>(minval);
    const SysInt validx = checked_cast<SysInt>(val - minval_cpy);
    const SysInt size_cpy = checked_cast<SysInt>(size);
    const SysInt swapval = vals[size_cpy];
    const SysInt vpvx = vals_pos[validx];
    vals[vpvx] = swapval;
    vals[size_cpy] = checked_cast<SysInt>(val);

    vals_pos[checked_cast<SysInt>(swapval - minval_cpy)] = vpvx;
    vals_pos[validx] = size_cpy;

    size++;
  }

  void insert(DomainInt val) {
    if(!in(val)) {
      unsafe_insert(val);
    }
  }

  void unsafe_remove(DomainInt val) {
    // swap to posiition size-1 then reduce size
    D_ASSERT(in(val));
    const SysInt validx = checked_cast<SysInt>(val - minval);
    const SysInt swapval = vals[checked_cast<SysInt>(size - 1)];
    vals[vals_pos[validx]] = swapval;
    vals[checked_cast<SysInt>(size - 1)] = checked_cast<SysInt>(val);

    vals_pos[checked_cast<SysInt>(swapval - minval)] = vals_pos[validx];
    vals_pos[validx] = checked_cast<SysInt>(size - 1);

    size--;
  }

  void remove(DomainInt val) {
    if(in(val)) {
      unsafe_remove(val);
    }
  }

  void fill() {
    size = vals.size();
  }
};

#endif
