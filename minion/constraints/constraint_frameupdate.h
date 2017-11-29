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
#include <deque>
#include <algorithm>
#include <utility>
#include <cmath>

#include "constraint_checkassign.h"

#define ADMPRINT(x)
//#define ADMPRINT(x) std::cout << x << std::endl;

template <typename V1, typename V2, typename V3, typename V4, typename ValueType>
struct FrameUpdateConstraint : public AbstractConstraint {
  V1 idx_source;
  V2 idx_target;
  V3 source;
  V4 target;
  ValueType blocksize;

  //Reversible<SysInt> sourceidx;
  //Reversible<SysInt> targetidx;  //  Left of sourceidx and targetidx have already been copied over. 
  
  FrameUpdateConstraint(const V1& v1, const V2& v2, const V3& v3, const V4& v4, const ValueType _value)
      : idx_source(v1),
        idx_target(v2),
        source(v3),
        target(v4),
        blocksize(_value)
        //,sourceidx(-1), targetidx(-1)
  {
    CHECK(( source.size()==target.size() ),
          "Source and target vectors are different sizes in frameupdate constraint.");
    
    CHECK(( source.size()%blocksize == 0 ),
          "Source and target vector size does not divide by block size in frameupdate constraint.");
  }
  
  virtual string constraint_name() {
    return "frameupdate";
  }
  
  CONSTRAINT_ARG_LIST5(idx_source, idx_target, source, target, blocksize);
  
  void trigger_setup() {
    for(unsigned i = 0; i < idx_source.size(); ++i) {
      moveTriggerInt(idx_source[i], i, Assigned);
    }
    for(unsigned i = 0; i < idx_target.size(); ++i) {
      moveTriggerInt(idx_target[i], i+idx_source.size(), Assigned);
    }
  }

  virtual SysInt dynamic_trigger_count() {
    return idx_source.size() + idx_target.size();
  }

  virtual void propagateDynInt(SysInt flag, DomainDelta) {
    full_propagate();
  }

  virtual void full_propagate() {
    trigger_setup();
    
    for(unsigned i=0; i<idx_source.size(); ++i) {
        if(!idx_source[i].isAssigned()) {
            return;
        }
    }
    for(unsigned i=0; i<idx_target.size(); ++i) {
        if(!idx_target[i].isAssigned()) {
            return;
        }
    }
    
    std::set<DomainInt> idx_source_set;
    std::set<DomainInt> idx_target_set;
    
    for(unsigned i=0; i<idx_source.size(); ++i) {
        idx_source_set.insert(idx_source[i].getAssignedValue());
    }
    for(unsigned i=0; i<idx_target.size(); ++i) {
        idx_target_set.insert(idx_target[i].getAssignedValue());
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
                target[(idxtarget-1)*blocksize + i].assign(source[(idxsource-1)*blocksize + i].getAssignedValue());
            }
            
            idxsource++;
            idxtarget++;
        }
    }
  }
  
  virtual BOOL check_assignment(DomainInt* v, SysInt v_size) {
    std::set<DomainInt> idx_source_set;
    std::set<DomainInt> idx_target_set;
    
    for(unsigned i=0; i<idx_source.size(); ++i) {
        idx_source_set.insert(v[i]);
    }
    for(unsigned i=0; i<idx_target.size(); ++i) {
        idx_target_set.insert(v[i+idx_source.size()]);
    }
    
    DomainInt* src=v+idx_source.size()+idx_target.size();
    DomainInt* trg=src+source.size();
    
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
  
  //  get_satisfying_assignment missing.
  
  // Function to make it reifiable in the lousiest way.
  virtual AbstractConstraint* reverse_constraint() {
    return forward_check_negation(this);
  }
  
};



template <typename V1, typename V2, typename V3, typename V4>
AbstractConstraint* BuildCT_FRAMEUPDATE(const V1& v1, const V2& v2, const V3& v3, const V4& v4, ConstraintBlob& b) {
    std::vector<AnyVarRef> avr1 = make_AnyVarRef(v1);
    std::vector<AnyVarRef> avr2 = make_AnyVarRef(v2);
    
 //   return new FrameUpdateConstraint<V3,V4>(v1,v2,v3,v4,b.constants[0][0]);
  return new FrameUpdateConstraint<std::vector<AnyVarRef>, std::vector<AnyVarRef>, V3, V4, decltype(b.constants[0][0])>(
      avr1, avr2, v3, v4, b.constants[0][0]);
}

/* JSON
{ "type": "constraint",
  "name": "frameupdate",
  "internal_name": "CT_FRAMEUPDATE",
  "args": [ "read_list", "read_list", "read_list", "read_list", "read_constant" ]
}
*/
