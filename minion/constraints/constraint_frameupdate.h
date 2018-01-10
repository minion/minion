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

/** @help constraints;alldiffmatrix Description
For a latin square this constraint is placed on the whole matrix once for each
value.
It ensures there is a bipartite matching between rows and columns where the
edges
in the matching correspond to a pair (row, column) where the variable in
position
(row,column) in the matrix may be assigned to the given value.
*/

/** @help constraints;alldiffmatrix Example

alldiffmatrix(myVec, Value)
*/

/** @help constraints;alldiffmatrix Notes
This constraint adds some extra reasoning in addition to the GAC Alldifferents
on the rows and columns.
*/

#include <stdlib.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <utility>
#include <cmath>

#include "constraint_checkassign.h"

template <typename V1, typename V2, typename V3, typename V4, typename ValueType>
struct FrameUpdateConstraint : public AbstractConstraint {
  V1 source;
  V2 target;
  V3 idx_source;
  V4 idx_target;
  SysInt blocksize;

  //Reversible<SysInt> sourceidx;
  //Reversible<SysInt> targetidx;  //  Left of sourceidx and targetidx have already been copied over. 
  
  Reversible<bool> idxes_assigned;
 
  // These should only be read when idxes_assigned is true
    std::set<DomainInt> idx_source_set;
    std::set<DomainInt> idx_target_set;
    std::vector<SysInt> source_to_target_map;
    std::vector<SysInt> target_to_source_map;
  
  FrameUpdateConstraint(const V1& v1, const V2& v2, const V3& v3, const V4& v4, const ValueType _value)
      : source(v1),
        target(v2),
        idx_source(v3),
        idx_target(v4),
        blocksize(checked_cast<SysInt>(_value)),
        //,sourceidx(-1), targetidx(-1),
        idxes_assigned()
  {
    idxes_assigned = false;
    CHECK(( source.size()==target.size() ),
          "Source and target vectors are different sizes in frameupdate constraint.");
    
    CHECK(( source.size()%blocksize == 0 ),
          "Source and target vector size does not divide by block size in frameupdate constraint.");

    source_to_target_map.assign(source.size(), -1);
    target_to_source_map.assign(target.size(), -1);
  }
  
  virtual string constraint_name() {
    return "frameupdate";
  }
  
  CONSTRAINT_ARG_LIST5(idx_source, idx_target, source, target, blocksize);
  
  void trigger_setup() {
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
    D_ASSERT(trig == dynamic_trigger_count());
    
  }

  virtual SysInt dynamic_trigger_count() {
    return idx_source.size() + idx_target.size() + (source.size() + target.size()) * 2;
  }

  bool check_idx_sets() {
    D_ASSERT(idxes_assigned == false);
    for(unsigned i=0; i<idx_source.size(); ++i) {
        if(!idx_source[i].isAssigned()) {
            return false;
        }
    }
    for(unsigned i=0; i<idx_target.size(); ++i) {
        if(!idx_target[i].isAssigned()) {
            return false;
        }
    }

    idx_source_set.clear();
    idx_target_set.clear();

   for(unsigned i=0; i<idx_source.size(); ++i) {
        DomainInt val = idx_source[i].getAssignedValue();
        if(idx_source_set.count(val) > 0 || val <= 0 || val > source.size())
        {
            getState().setFailed(true);
            return false;
        }
        idx_source_set.insert(val);
    }
    for(unsigned i=0; i<idx_target.size(); ++i) {
        DomainInt val = idx_target[i].getAssignedValue();
        if(idx_target_set.count(val) > 0 || val <= 0 || val > target.size())
        {
            getState().setFailed(true);
            return false;
        }
        idx_target_set.insert(val);
    }

    // Setup mapping arrays
    source_to_target_map.assign(source.size(), -1);
    target_to_source_map.assign(target.size(), -1);

    SysInt numblocks=source.size()/blocksize;
    SysInt idxsource=1;   ///  Index blocks from 1. 
    SysInt idxtarget=1;
    while(idxsource<=numblocks && idxtarget<=numblocks) {
        //  Increment past any skips
        while(idx_source_set.count(idxsource)>0) {
            idxsource++;
        }
        while(idx_target_set.count(idxtarget)>0) {
            idxtarget++;
        }
        if(idxsource<=numblocks && idxtarget<=numblocks) {
            source_to_target_map[idxsource-1] = idxtarget-1;
            target_to_source_map[idxtarget-1] = idxsource-1;
        }
        
        idxsource++;
        idxtarget++;
    }

    idxes_assigned = true;
    return true;
  }

  virtual void full_propagate() {
    trigger_setup();
    propagateDynInt(0, DomainDelta::empty());
  }

  void copy_from_source(SysInt i) {
    SysInt block = i / blocksize;
    if(source_to_target_map[block] >= 0)
    {
        SysInt blockpos = i % blocksize;
        SysInt targetpos = source_to_target_map[block]*blocksize+blockpos;
        D_ASSERT(targetpos >= 0 && targetpos < target.size());
        D_ASSERT(i >= 0 && i < source.size());
        target[targetpos].setMax(source[i].getMax());
        target[targetpos].setMin(source[i].getMin());
    }
  }

  void copy_from_target(SysInt i) {
    SysInt block = i / blocksize;
    if(target_to_source_map[block] >= 0)
    {
        SysInt blockpos = i % blocksize;
        SysInt sourcepos = target_to_source_map[block]*blocksize+blockpos;
        D_ASSERT(sourcepos >= 0 && sourcepos < source.size());
        D_ASSERT(i >= 0 && i < target.size());
        source[sourcepos].setMax(target[i].getMax());
        source[sourcepos].setMin(target[i].getMin());
    }
  }


  virtual void propagateDynInt(SysInt flagin, DomainDelta) {
    SysInt flag = flagin;
    if(flag < idx_source.size() + idx_target.size()) {
        if(idxes_assigned)
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

    if(!idxes_assigned)
        return;

    flag -= idx_source.size() + idx_target.size();
    D_ASSERT(flag >= 0);
    SysInt parity = flag % 2;
    flag /= 2;
    if(flag < source.size())
    {
        copy_from_source(flag);
    }
    else
    {
        flag -= source.size();
        D_ASSERT(flag < target.size());
        copy_from_target(flag);
    }
  }
  
  virtual BOOL check_assignment(DomainInt* v, SysInt v_size) {
    std::set<DomainInt> idx_source_set;
    std::set<DomainInt> idx_target_set;
    
    for(unsigned i=0; i<idx_source.size(); ++i) {
        DomainInt val = v[i];
        if(idx_source_set.count(val) > 0 || val <= 0 || val > source.size())
            return false;

        idx_source_set.insert(val);
    }
    for(unsigned i=0; i<idx_target.size(); ++i) {
        DomainInt val = v[i+idx_source.size()];
        if(idx_target_set.count(val) > 0 || val <= 0 || val > target.size())
            return false;
        idx_target_set.insert(val);
    }
    
    DomainInt* src=v+idx_source.size()+idx_target.size();
    DomainInt* trg=src+source.size();
    
    SysInt numblocks=checked_cast<SysInt>(source.size()/blocksize);
    SysInt idxsource=1;   ///  Index blocks from 1. 
    SysInt idxtarget=1;
    while(idxsource<=numblocks && idxtarget<=numblocks) {
        //  Increment past any skips
        while(idx_source_set.count(idxsource)>0) {
            idxsource++;
        }
        while(idx_target_set.count(idxtarget)>0) {
            idxtarget++;
        }
        if(idxsource<=numblocks && idxtarget<=numblocks) {
            // Copy a block over. 
            for(SysInt i=0; i<blocksize; i++) {
                if(trg[(idxtarget-1)*blocksize + i] != src[(idxsource-1)*blocksize + i]) {
                    return false;
                }
            }
            
            idxsource++;
            idxtarget++;
        }
    }
    return true;
  }
  
  virtual vector<AnyVarRef> get_vars() {
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
  
  virtual bool get_satisfying_assignment(box<pair<SysInt, DomainInt>>& assignment) {
    for(unsigned i = 0; i < idx_source.size(); ++i) {
      if(! idx_source[i].isAssigned()) {
        assignment.push_back(make_pair(i, idx_source[i].getMax()));
        assignment.push_back(make_pair(i, idx_source[i].getMin()));
        return true;
      }
    }
    SysInt skip=idx_source.size();
    for(unsigned i = 0; i < idx_target.size(); ++i) {
      if(! idx_target[i].isAssigned()) {
        assignment.push_back(make_pair(i+skip, idx_target[i].getMax()));
        assignment.push_back(make_pair(i+skip, idx_target[i].getMin()));
        return true;
      }
    }
    skip=skip+idx_target.size();
    for(unsigned i = 0; i < source.size(); ++i) {
      if(! source[i].isAssigned()) {
        assignment.push_back(make_pair(i+skip, source[i].getMax()));
        assignment.push_back(make_pair(i+skip, source[i].getMin()));
        return true;
      }
    }
    skip=skip+source.size();
    for(unsigned i = 0; i < target.size(); ++i) {
      if(! target[i].isAssigned()) {
        assignment.push_back(make_pair(i+skip, target[i].getMax()));
        assignment.push_back(make_pair(i+skip, target[i].getMin()));
        return true;
      }
    }
    
    //  All variables assigned. Check the assignment. 
    std::set<DomainInt> idx_source_set;
    std::set<DomainInt> idx_target_set;
    
    for(unsigned i=0; i<idx_source.size(); ++i) {
        DomainInt val = idx_source[i].getAssignedValue();
        if(idx_source_set.count(val) > 0 || val <= 0 || val > source.size())
            return false;
        idx_source_set.insert(val);
    }
    for(unsigned i=0; i<idx_target.size(); ++i) {
        DomainInt val = idx_target[i].getAssignedValue();
        if(idx_target_set.count(val) > 0 || val <= 0 || val > target.size())
            return false;
        idx_target_set.insert(val);
    }
    
    SysInt numblocks=source.size()/blocksize;
    SysInt idxsource=1;   ///  Index blocks from 1. 
    SysInt idxtarget=1;
    while(idxsource<=numblocks && idxtarget<=numblocks) {
        //  Increment past any skips
        while(idx_source_set.count(idxsource)>0) {
            idxsource++;
        }
        while(idx_target_set.count(idxtarget)>0) {
            idxtarget++;
        }
        if(idxsource<=numblocks && idxtarget<=numblocks) {
            // Copy a block over. 
            for(SysInt i=0; i<blocksize; i++) {
                if(target[(idxtarget-1)*blocksize + i].getAssignedValue() != source[(idxsource-1)*blocksize + i].getAssignedValue()) {
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
  virtual AbstractConstraint* reverse_constraint() {
    return forward_check_negation(this);
  }
  
};



template <typename V1, typename V2, typename V3, typename V4>
AbstractConstraint* BuildCT_FRAMEUPDATE(const V1& v1, const V2& v2, const V3& v3, const V4& v4, ConstraintBlob& b) {
    std::vector<AnyVarRef> avr3 = make_AnyVarRef(v3);
    std::vector<AnyVarRef> avr4 = make_AnyVarRef(v4);
    
 //   return new FrameUpdateConstraint<V3,V4>(v1,v2,v3,v4,b.constants[0][0]);
  return new FrameUpdateConstraint<V1, V2, std::vector<AnyVarRef>, std::vector<AnyVarRef>, decltype(b.constants[0][0])>(
      v1, v2, avr3, avr4, b.constants[0][0]);
}

/* JSON
{ "type": "constraint",
  "name": "frameupdate",
  "internal_name": "CT_FRAMEUPDATE",
  "args": [ "read_list", "read_list", "read_list", "read_list", "read_constant" ]
}
*/
