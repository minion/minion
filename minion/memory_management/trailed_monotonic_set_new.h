// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef TRAILED_MONOTONIC_SET_H
#define TRAILED_MONOTONIC_SET_H

#include "../system/system.h"

class TrailedMonotonicSet {
  vector<char> data;

  vector<SysInt> trailstack;

  vector<SysInt> trailstack_marks;

public:
  TrailedMonotonicSet() {
    trailstack_marks.push_back(0);
  }

  DomainInt size() const {
    return data.size();
  }

  void undo() {
    SysInt i = (SysInt)trailstack.size() - 1;

    SysInt j = trailstack_marks.back();
    trailstack_marks.pop_back();

    for(; i >= j; i--) {
      D_ASSERT(!data[trailstack[i]]);
      data[trailstack[i]] = true;
      trailstack.pop_back();
    }
    D_ASSERT((SysInt)trailstack.size() == j);
  }

  bool ifMember_remove(DomainInt index) {
    SysInt i = checked_cast<SysInt>(index);
    if(data[i]) {
      data[i] = false;
      trailstack.push_back(i);
      return true;
    }
    return false;
  }

  bool isMember(DomainInt index) {
    SysInt i = checked_cast<SysInt>(index);
    return data[i];
  }

  void unchecked_remove(DomainInt index) {
    SysInt i = checked_cast<SysInt>(index);
    D_ASSERT(data[i]);
    data[i] = false;
    trailstack.push_back(i);
  }

  void before_branch_left() {
    trailstack_marks.push_back(trailstack.size());
  }

  void after_branch_left() // nothing to do
  {}

  void before_branch_right() // nothing to do
  {}
  void after_branch_right() // nothing to do
  {}

  DomainInt request_storage(DomainInt allocsize) {
    SysInt i = data.size();
    data.resize(i + checked_cast<SysInt>(allocsize), true);
    return i;
  }
};

typedef TrailedMonotonicSet MonotonicSet;
#endif
