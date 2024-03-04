// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

/** @help constraints;alldiffmatrix Description
For a latin square this constraint is placed on the whole matrix once for each
value.
It ensures there is a bipartite matching between rows and columns where the
edges
in the matching correspond to a pair (row, column) where the variable in
position
(row,column) in the matrix may be assigned to the given value.
*/





#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdlib.h>
#include <utility>
#include <vector>

#include "constraint_checkassign.h"

template <typename V1, typename V2, typename V3, typename V4, typename ValueType>
struct FrameUpdateConstraint : public AbstractConstraint {
  V1 source;
  V2 target;
  V3 idx_source;
  V4 idx_target;
  SysInt blocksize;

  // Reversible<SysInt> sourceidx;
  // Reversible<SysInt> targetidx;  //  Left of sourceidx and targetidx have already been copied
  // over.

  Reversible<bool> idxesAssigned;

  // These should only be read when idxesAssigned is true
  std::set<DomainInt> idx_source_set;
  std::set<DomainInt> idx_target_set;
  std::vector<SysInt> sourceToTarget_map;
  std::vector<SysInt> targetToSource_map;

  FrameUpdateConstraint(const V1& v1, const V2& v2, const V3& v3, const V4& v4,
                        const ValueType _value)
      : source(v1),
        target(v2),
        idx_source(v3),
        idx_target(v4),
        blocksize(checked_cast<SysInt>(_value)),
        //,sourceidx(-1), targetidx(-1),
        idxesAssigned() {
    idxesAssigned = false;
    CHECK((source.size() == target.size()),
          "Source and target vectors are different sizes in frameupdate constraint.");

    CHECK((source.size() % blocksize == 0),
          "Source and target vector size does not divide by block size in frameupdate constraint.");

    sourceToTarget_map.assign(source.size(), -1);
    targetToSource_map.assign(target.size(), -1);
  }

  virtual string constraintName() {
    return "frameupdate";
  }

  CONSTRAINT_ARG_LIST5(idx_source, idx_target, source, target, blocksize);

  void triggerSetup() {
    int trig = 0;

    for(unsigned i = 0; i < idx_source.size(); ++i) {
      moveTriggerInt(idx_source[i], trig++, Assigned);
    }
    for(unsigned i = 0; i < idx_target.size(); ++i) {
      moveTriggerInt(idx_target[i], trig++, Assigned);
    }

    for(unsigned i = 0; i < source.size(); ++i) {
      moveTriggerInt(source[i], trig++, LowerBound);
      moveTriggerInt(source[i], trig++, UpperBound);
    }

    for(unsigned i = 0; i < source.size(); ++i) {
      moveTriggerInt(target[i], trig++, LowerBound);
      moveTriggerInt(target[i], trig++, UpperBound);
    }
    D_ASSERT(trig == dynamicTriggerCount());
  }

  virtual SysInt dynamicTriggerCount() {
    return idx_source.size() + idx_target.size() + (source.size() + target.size()) * 2;
  }

  bool check_idx_sets() {
    D_ASSERT(idxesAssigned == false);
    for(unsigned i = 0; i < idx_source.size(); ++i) {
      if(!idx_source[i].isAssigned()) {
        return false;
      }
    }
    for(unsigned i = 0; i < idx_target.size(); ++i) {
      if(!idx_target[i].isAssigned()) {
        return false;
      }
    }

    idx_source_set.clear();
    idx_target_set.clear();

    for(unsigned i = 0; i < idx_source.size(); ++i) {
      DomainInt val = idx_source[i].assignedValue();
      if(idx_source_set.count(val) > 0 || val <= 0 || val > source.size()) {
        getState().setFailed();
        return false;
      }
      idx_source_set.insert(val);
    }
    for(unsigned i = 0; i < idx_target.size(); ++i) {
      DomainInt val = idx_target[i].assignedValue();
      if(idx_target_set.count(val) > 0 || val <= 0 || val > target.size()) {
        getState().setFailed();
        return false;
      }
      idx_target_set.insert(val);
    }

    // Setup mapping arrays
    sourceToTarget_map.assign(source.size(), -1);
    targetToSource_map.assign(target.size(), -1);

    SysInt numblocks = source.size() / blocksize;
    SysInt idxsource = 1; ///  Index blocks from 1.
    SysInt idxtarget = 1;
    while(idxsource <= numblocks && idxtarget <= numblocks) {
      //  Increment past any skips
      while(idx_source_set.count(idxsource) > 0) {
        idxsource++;
      }
      while(idx_target_set.count(idxtarget) > 0) {
        idxtarget++;
      }
      if(idxsource <= numblocks && idxtarget <= numblocks) {
        sourceToTarget_map[idxsource - 1] = idxtarget - 1;
        targetToSource_map[idxtarget - 1] = idxsource - 1;
      }

      idxsource++;
      idxtarget++;
    }

    idxesAssigned = true;
    return true;
  }

  virtual void fullPropagate() {
    triggerSetup();

    // -1 to force we check the indices
    propagateDynInt(-1, DomainDelta::empty());
  }

  void copy_from_source(SysInt i) {
    SysInt block = i / blocksize;
    if(sourceToTarget_map[block] >= 0) {
      SysInt blockpos = i % blocksize;
      SysInt targetpos = sourceToTarget_map[block] * blocksize + blockpos;
      D_ASSERT(targetpos >= 0 && targetpos < target.size());
      D_ASSERT(i >= 0 && i < source.size());
      target[targetpos].setMax(source[i].max());
      target[targetpos].setMin(source[i].min());
    }
  }

  void copy_from_target(SysInt i) {
    SysInt block = i / blocksize;
    if(targetToSource_map[block] >= 0) {
      SysInt blockpos = i % blocksize;
      SysInt sourcepos = targetToSource_map[block] * blocksize + blockpos;
      D_ASSERT(sourcepos >= 0 && sourcepos < source.size());
      D_ASSERT(i >= 0 && i < target.size());
      source[sourcepos].setMax(target[i].max());
      source[sourcepos].setMin(target[i].min());
    }
  }

  virtual void propagateDynInt(SysInt flagin, DomainDelta) {
    SysInt flag = flagin;
    if(flag < (SysInt)(idx_source.size() + idx_target.size())) {
      if(idxesAssigned)
        return;

      // Check if idx sets incomplete or invalid
      if(!check_idx_sets())
        return;

      for(int i = 0; i < source.size(); ++i)
        copy_from_source(i);

      for(int i = 0; i < target.size(); ++i)
        copy_from_target(i);

      return;
    }

    if(!idxesAssigned)
      return;

    flag -= idx_source.size() + idx_target.size();
    D_ASSERT(flag >= 0);
    // SysInt parity = flag % 2;
    flag /= 2;
    if(flag < source.size()) {
      copy_from_source(flag);
    } else {
      flag -= source.size();
      D_ASSERT(flag < target.size());
      copy_from_target(flag);
    }
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    std::set<DomainInt> idx_source_set;
    std::set<DomainInt> idx_target_set;

    for(unsigned i = 0; i < idx_source.size(); ++i) {
      DomainInt val = v[i];
      if(idx_source_set.count(val) > 0 || val <= 0 || val > source.size())
        return false;

      idx_source_set.insert(val);
    }
    for(unsigned i = 0; i < idx_target.size(); ++i) {
      DomainInt val = v[i + idx_source.size()];
      if(idx_target_set.count(val) > 0 || val <= 0 || val > target.size())
        return false;
      idx_target_set.insert(val);
    }

    DomainInt* src = v + idx_source.size() + idx_target.size();
    DomainInt* trg = src + source.size();

    SysInt numblocks = checked_cast<SysInt>(source.size() / blocksize);
    SysInt idxsource = 1; ///  Index blocks from 1.
    SysInt idxtarget = 1;
    while(idxsource <= numblocks && idxtarget <= numblocks) {
      //  Increment past any skips
      while(idx_source_set.count(idxsource) > 0) {
        idxsource++;
      }
      while(idx_target_set.count(idxtarget) > 0) {
        idxtarget++;
      }
      if(idxsource <= numblocks && idxtarget <= numblocks) {
        // Copy a block over.
        for(SysInt i = 0; i < blocksize; i++) {
          if(trg[(idxtarget - 1) * blocksize + i] != src[(idxsource - 1) * blocksize + i]) {
            return false;
          }
        }

        idxsource++;
        idxtarget++;
      }
    }
    return true;
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> v;
    for(unsigned i = 0; i < idx_source.size(); ++i) {
      v.push_back(idx_source[i]);
    }
    for(unsigned i = 0; i < idx_target.size(); ++i) {
      v.push_back(idx_target[i]);
    }
    for(unsigned i = 0; i < source.size(); ++i) {
      v.push_back(source[i]);
    }
    for(unsigned i = 0; i < target.size(); ++i) {
      v.push_back(target[i]);
    }
    return v;
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    for(unsigned i = 0; i < idx_source.size(); ++i) {
      if(!idx_source[i].isAssigned()) {
        assignment.push_back(make_pair(i, idx_source[i].max()));
        assignment.push_back(make_pair(i, idx_source[i].min()));
        return true;
      }
    }
    SysInt skip = idx_source.size();
    for(unsigned i = 0; i < idx_target.size(); ++i) {
      if(!idx_target[i].isAssigned()) {
        assignment.push_back(make_pair(i + skip, idx_target[i].max()));
        assignment.push_back(make_pair(i + skip, idx_target[i].min()));
        return true;
      }
    }
    skip = skip + idx_target.size();
    for(unsigned i = 0; i < source.size(); ++i) {
      if(!source[i].isAssigned()) {
        assignment.push_back(make_pair(i + skip, source[i].max()));
        assignment.push_back(make_pair(i + skip, source[i].min()));
        return true;
      }
    }
    skip = skip + source.size();
    for(unsigned i = 0; i < target.size(); ++i) {
      if(!target[i].isAssigned()) {
        assignment.push_back(make_pair(i + skip, target[i].max()));
        assignment.push_back(make_pair(i + skip, target[i].min()));
        return true;
      }
    }

    //  All variables assigned. Check the assignment.
    std::set<DomainInt> idx_source_set;
    std::set<DomainInt> idx_target_set;

    for(unsigned i = 0; i < idx_source.size(); ++i) {
      DomainInt val = idx_source[i].assignedValue();
      if(idx_source_set.count(val) > 0 || val <= 0 || val > source.size())
        return false;
      idx_source_set.insert(val);
    }
    for(unsigned i = 0; i < idx_target.size(); ++i) {
      DomainInt val = idx_target[i].assignedValue();
      if(idx_target_set.count(val) > 0 || val <= 0 || val > target.size())
        return false;
      idx_target_set.insert(val);
    }

    SysInt numblocks = source.size() / blocksize;
    SysInt idxsource = 1; ///  Index blocks from 1.
    SysInt idxtarget = 1;
    while(idxsource <= numblocks && idxtarget <= numblocks) {
      //  Increment past any skips
      while(idx_source_set.count(idxsource) > 0) {
        idxsource++;
      }
      while(idx_target_set.count(idxtarget) > 0) {
        idxtarget++;
      }
      if(idxsource <= numblocks && idxtarget <= numblocks) {
        // Copy a block over.
        for(SysInt i = 0; i < blocksize; i++) {
          if(target[(idxtarget - 1) * blocksize + i].assignedValue() !=
             source[(idxsource - 1) * blocksize + i].assignedValue()) {
            return false;
          }
        }

        idxsource++;
        idxtarget++;
      }
    }
    return true;
  }

  // Function to make it reifiable in the lousiest way.
  virtual AbstractConstraint* reverseConstraint() {
    return forwardCheckNegation(this);
  }
};

template <typename V1, typename V2, typename V3, typename V4>
AbstractConstraint* BuildCT_FRAMEUPDATE(const V1& v1, const V2& v2, const V3& v3, const V4& v4,
                                        ConstraintBlob& b) {
  std::vector<AnyVarRef> avr3 = make_AnyVarRef(v3);
  std::vector<AnyVarRef> avr4 = make_AnyVarRef(v4);

  //   return new FrameUpdateConstraint<V3,V4>(v1,v2,v3,v4,b.constants[0][0]);
  return new FrameUpdateConstraint<V1, V2, std::vector<AnyVarRef>, std::vector<AnyVarRef>,
                                   decltype(b.constants[0][0])>(v1, v2, avr3, avr4,
                                                                b.constants[0][0]);
}

/* JSON
{ "type": "constraint",
  "name": "frameupdate",
  "internal_name": "CT_FRAMEUPDATE",
  "args": [ "read_list", "read_list", "read_list", "read_list", "read_constant" ]
}
*/
