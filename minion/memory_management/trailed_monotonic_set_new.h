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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef TRAILED_MONOTONIC_SET_H
#define TRAILED_MONOTONIC_SET_H

#include "../system/system.h"



class TrailedMonotonicSet
{
    vector<char> data;

    vector<SysInt> trailstack;

    vector<SysInt> trailstack_marks;

public:
    TrailedMonotonicSet( )
    {
        trailstack_marks.push_back(0);
    }

    DomainInt size() const
    {
        return data.size();
    }

    void undo() {
        SysInt i=(SysInt)trailstack.size()-1;

        SysInt j=trailstack_marks.back();
        trailstack_marks.pop_back();

        for( ; i>=j ; i--) {
            D_ASSERT(!data[trailstack[i]]);
            data[trailstack[i]]=true;
            trailstack.pop_back();
        }
        D_ASSERT(trailstack.size()==j);
    }

    bool ifMember_remove(DomainInt index) {
        SysInt i=checked_cast<SysInt>(index);
        if(data[i]) {
            data[i]=false;
            trailstack.push_back(i);
            return true;
        }
        return false;
    }

    bool isMember(DomainInt index) {
        SysInt i=checked_cast<SysInt>(index);
        return data[i];
    }

    void unchecked_remove(DomainInt index) {
        SysInt i=checked_cast<SysInt>(index);
        D_ASSERT(data[i]);
        data[i]=false;
        trailstack.push_back(i);
    }

    void before_branch_left() {
        trailstack_marks.push_back(trailstack.size());
    }

    void after_branch_left()  // nothing to do
    { }

    void  before_branch_right()  // nothing to do
    { }
    void after_branch_right()  // nothing to do
    { }

    DomainInt request_storage(DomainInt allocsize) {
        SysInt i=data.size();
        data.resize(i+checked_cast<SysInt>(allocsize), true);
        return i;
    }

    void lock(StateObj * stateObj) { }

};

typedef TrailedMonotonicSet MonotonicSet;
#endif
