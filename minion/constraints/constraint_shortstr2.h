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

/** @help constraints;shortstr2 Description
ShortSTR2 is the algorithm described in the IJCAI 2013 paper by Jefferson and
Nightingale. It is an extension of STR2+ by Christophe Lecoutre, adapted for
short supports.

*/

/** @help constraints;shortstr2 Example

Input format is exactly the same as haggisgac. Refer to the haggisgac and
shorttuplelist pages for more information.

Example:

**SHORTTUPLELIST**
mycon 4
[(0,0),(3,0)]
[(1,0),(3,0)]
[(2,0),(3,0)]
[(0,1),(1,1),(2,1),(3,1)]

**CONSTRAINTS**
shortstr2([x1,x2,x3,x4], mycon)

*/

/** @help constraints;shortstr2 Notes
This constraint enforces generalized arc consistency.
*/


/** @help constraints;shortstr2 References
help input shorttuplelist
help constraints table
help constraints negativetable
help constraints haggisgac
help constraints haggisgac-stable
*/



/** @help constraints;str2plus Description
str2plus is an implementation of the STR2+ algorithm by Christophe Lecoutre.
*/

/** @help constraints;str2plus Example

str2plus is invoked in the same way as all other table constraints, such
as table and mddc.

str2plus([x,y,z], {<1,2,3>, <1,3,2>})

*/

/** @help constraints;shortstr2 Notes
This constraint enforces generalized arc consistency.
*/


#ifndef CONSTRAINT_STR2_H
#define CONSTRAINT_STR2_H



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
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include "../constraints/constraint_checkassign.h"

using namespace std;


struct arrayset {
    vector<SysInt> vals;
    vector<SysInt> vals_pos;
    DomainInt size;
    DomainInt minval;

    void initialise(DomainInt low, DomainInt high) {
        minval=low;
        vals_pos.resize(checked_cast<SysInt>(high-low+1));
        vals.resize(checked_cast<SysInt>(high-low+1));
        for(SysInt i=0; i<checked_cast<SysInt>(high-low+1); i++) {
            vals[i]=checked_cast<SysInt>(i+low);
            vals_pos[i]=i;
        }
        size=0;
    }

    void clear() {
        size=0;
    }

    bool in(DomainInt val) {
        return vals_pos[checked_cast<SysInt>(val-minval)]<size;
    }


    // This method looks a bit messy, due to stupid C++ optimisers not being
    // clever enough to realise various things don't alias, and this method
    // being called as much as it is.
    void unsafe_insert(DomainInt val)
    {
        D_ASSERT(!in(val));
        const SysInt minval_cpy = checked_cast<SysInt>(minval);
        const SysInt validx = checked_cast<SysInt>(val-minval_cpy);
        const SysInt size_cpy = checked_cast<SysInt>(size);
        const SysInt swapval = vals[size_cpy];
        const SysInt vpvx = vals_pos[validx];
        vals[vpvx]=swapval;
        vals[size_cpy]=checked_cast<SysInt>(val);

        vals_pos[checked_cast<SysInt>(swapval-minval_cpy)]=vpvx;
        vals_pos[validx]=size_cpy;

        size++;
    }

    void insert(DomainInt val) {
        if(!in(val)) {
            unsafe_insert(val);
        }
    }

    void unsafe_remove(DomainInt val)
    {
        // swap to posiition size-1 then reduce size
        D_ASSERT(in(val));
        const SysInt validx=checked_cast<SysInt>(val-minval);
        const SysInt swapval=vals[checked_cast<SysInt>(size-1)];
        vals[vals_pos[validx]]=swapval;
        vals[checked_cast<SysInt>(size-1)]=checked_cast<SysInt>(val);

        vals_pos[checked_cast<SysInt>(swapval-minval)]=vals_pos[validx];
        vals_pos[validx]=checked_cast<SysInt>(size-1);

        size--;
    }

    void remove(DomainInt val) {
        if(in(val)) {
            unsafe_remove(val);
        }
    }

    void fill() {
        size=vals.size();
    }
};

struct ReversibleArrayset {
    // Only allows deletion.
    vector<SysInt> vals;
    vector<SysInt> vals_pos;
    ReversibleInt size;
    SysInt minval;

    ReversibleArrayset(StateObj * _so) : size(_so) {}

    void initialise(DomainInt low_, DomainInt high_) {
        const SysInt low = checked_cast<SysInt>(low_);
        const SysInt high = checked_cast<SysInt>(high_);
        minval=low;
        vals_pos.resize(high-low+1);
        vals.resize(high-low+1);
        for(SysInt i=0; i<high-low+1; i++) {
            vals[i]=i+low;
            vals_pos[i]=i;
        }
        size=vals.size();
    }

    bool in(DomainInt val) {
        return vals_pos[checked_cast<SysInt>(val-minval)]<size;
    }

    void remove(DomainInt val) {
        // swap to posiition size-1 then reduce size
        if(in(val)) {
            const SysInt validx=checked_cast<SysInt>(val-minval);
            const SysInt swapval=vals[size-1];
            vals[vals_pos[validx]]=swapval;
            vals[size-1]=checked_cast<SysInt>(val);

            vals_pos[swapval-minval]=vals_pos[validx];
            vals_pos[validx]=size-1;

            size=size-1;
        }
    }
};



struct STRData
{
    vector<vector<DomainInt> > tuples;
    vector<vector<pair<SysInt,DomainInt> > > compressed_tuples;

    STRData(ShortTupleList* _tuples, size_t varsize)
    {
        compressed_tuples = *(_tuples->tuplePtr());

        for(SysInt i = 0; i < (SysInt)compressed_tuples.size(); ++i)
        {
            vector<DomainInt> temp(varsize, DomainInt_Skip);
            for(SysInt j = 0; j < (SysInt)compressed_tuples[i].size(); ++j)
            {
                temp[compressed_tuples[i][j].first] = compressed_tuples[i][j].second;
            }
            tuples.push_back(temp);
        }
    }

    STRData(TupleList* _tuples, size_t varsize)
    {
        DomainInt tuple_count = _tuples->size();
        for(SysInt i = 0; i < tuple_count; ++i)
        {
            vector<DomainInt> t = _tuples->get_vector(i);
            vector<pair<SysInt, DomainInt> > comp;
            for(int j = 0; j < (SysInt)t.size(); ++j)
                comp.push_back(std::make_pair(j, t[j]));
            tuples.push_back(t);
            compressed_tuples.push_back(comp);
        }
    }
};


template<typename VarArray, bool UseShort>
struct STR : public AbstractConstraint
{
    virtual string constraint_name()
    {
        if(UseShort)
            return "shortstr2";
        else
            return "str2plus";
    }

//    CONSTRAINT_ARG_LIST2(vars, tupleList);

    virtual string full_output_name()
    {
        if(UseShort)
            return ConOutput::print_con(stateObj, constraint_name(), vars, shortTupleList);
        else
            return ConOutput::print_con(stateObj, constraint_name(), vars, longTupleList);
    }

    ShortTupleList* shortTupleList;
    TupleList* longTupleList;

    VarArray vars;

    bool constraint_locked;

    vector<SysInt> tupindices;

    ReversibleInt limit;   // In tupindices, indices less than limit are not known to be invalid.

    STRData* sct;

    void init()
    {
        if((SysInt)sct->tuples.size() > 0)
        {
            CHECK(sct->tuples[0].size() == vars.size(), "Cannot use same table for two constraints with different numbers of variables!");
        }
        tupindices.resize(sct->tuples.size());
        for(SysInt i=0; i<(SysInt)sct->tuples.size(); i++) {
            tupindices[i]=i;
        }

        ssup.initialise(0, (SysInt)vars.size()-1);
        sval.initialise(0, (SysInt)vars.size()-1);

        //ssup_permanent.initialise(0, (SysInt)vars.size()-1);

        gacvalues.resize(vars.size());
        for(SysInt i=0; i<(SysInt)vars.size(); i++) {
            gacvalues[i].initialise(vars[i].getInitialMin(), vars[i].getInitialMax());
        }

        std::random_shuffle(tupindices.begin(), tupindices.end());
    }

    STR(StateObj* _stateObj, const VarArray& _var_array, ShortTupleList* _tuples) : AbstractConstraint(_stateObj),
    shortTupleList(_tuples), longTupleList(0),
    vars(_var_array), constraint_locked(false), limit(_stateObj), sct(new STRData(_tuples, _var_array.size()))
    //, ssup_permanent(_stateObj)
    {
        CHECK(UseShort, "Internal error in ShortSTR2");
      init();
    }

    STR(StateObj* _stateObj, const VarArray& _var_array, TupleList* _tuples) : AbstractConstraint(_stateObj),
    shortTupleList(0), longTupleList(_tuples),
    vars(_var_array), constraint_locked(false), limit(_stateObj), sct(new STRData(_tuples, _var_array.size()))
    //, ssup_permanent(_stateObj)
    {
        CHECK(!UseShort, "Internal error in str2plus");
      init();
    }


    // Set up triggers.
    virtual triggerCollection setup_internal()
    {
        triggerCollection t;
        SysInt array_size = vars.size();
        for(SysInt i = 0; i < array_size; ++i) {
            t.push_back(make_trigger(vars[i], Trigger(this, i), DomainChanged));
        }
        return t;
    }

    virtual void full_propagate() {
        limit=sct->tuples.size();

        // pretend all variables have changed.
        for(SysInt i=0; i<(SysInt)vars.size(); i++) sval.insert(i);

        do_prop();
    }

    virtual vector<AnyVarRef> get_vars()
    {
      vector<AnyVarRef> ret;
      ret.reserve(vars.size());
      for(unsigned i = 0; i < vars.size(); ++i)
        ret.push_back(vars[i]);
      return ret;
    }

    virtual bool check_assignment(DomainInt* v, SysInt v_size)
    {
        if(UseShort)
        {
            const vector<set<DomainInt> >& doms = shortTupleList->getInitialDomains();
            if(doms.size() > 0)
            {
                for(SysInt i = 0; i < v_size; ++i)
                {
                    if(doms[i].count(v[i]) == 0)
                        return false;
                }
            }
        }

        for(SysInt i = 0; i < (SysInt)sct->compressed_tuples.size(); ++i)
        {
            bool sat = true;
            for(SysInt j = 0; j < (SysInt)sct->compressed_tuples[i].size(); ++j)
            {
                if(v[sct->compressed_tuples[i][j].first] != sct->compressed_tuples[i][j].second)
                {
                    sat = false;
                    break;
                }
            }

            if(sat)
                return true;
        }

        return false;
    }

    virtual bool get_satisfying_assignment(box<pair<SysInt,DomainInt> >& assignment)
    {


        for(SysInt i = 0; i < (SysInt)sct->compressed_tuples.size(); ++i)
        {
            bool sat = true;
            for(SysInt j = 0; j < (SysInt)sct->compressed_tuples[i].size(); ++j)
            {
                if(!vars[sct->compressed_tuples[i][j].first].inDomain(sct->compressed_tuples[i][j].second))
                {
                    sat = false;
                    break;
                }
            }

            if(sat)
            {
                for(SysInt j = 0; j < (SysInt)sct->compressed_tuples[i].size(); ++j)
                    assignment.push_back(sct->compressed_tuples[i][j]);
                return true;
            }
        }

        return false;
    }

    virtual AbstractConstraint* reverse_constraint()
    { return forward_check_negation(stateObj, this); }

    virtual void propagate(DomainInt prop_var, DomainDelta)
    {
        sval.insert(prop_var);

        if(!constraint_locked)
        {
            constraint_locked = true;
            getQueue(stateObj).pushSpecialTrigger(this);
        }
    }

    virtual void special_unlock() { constraint_locked = false; sval.clear(); }

    virtual void special_check()
    {
        constraint_locked = false;
        D_ASSERT(!getState(stateObj).isFailed());
        do_prop();
    }

    // S_sup is the set of unset (by the search procedure) variables with
    // at least one unsupported val.
    // Iterate only on S_Sup in the main loops looking for support.
    // Unfortunately can't do this exactly as in STR2 paper.
    arrayset ssup;

    //ReversibleArrayset ssup_permanent;  // when a var is assigned and after str2 has been run, it is removed from here.

    // S_val is the set of "unassigned" vars whose domain has been reduced since
    // previous call.
    // Unassigned here I think means not assigned by the search procedure.
    // Also contains the last assigned var (i.e. last assigned by the search procedure)
    // if it belongs to the scope of the constraint.

    // Here interpreted as the set of variables that triggered this call.
    arrayset sval;

    // lastSize array dropped. Only need to keep a list of the triggering vars.


    vector<arrayset> gacvalues;


    bool validTuple(SysInt i)
    {
        SysInt index=tupindices[i];
        const vector<DomainInt> & tau=sct->tuples[index];

        for(SysInt j=0; j<sval.size; j++) {
            const SysInt var=sval.vals[j];
            if(UseShort) {
                const DomainInt tv = tau[var];
                if( (tv!=DomainInt_Skip) && !vars[var].inDomain(tv)) {
                    return false;
                }
            }
            else {
                D_ASSERT(tau[var] != DomainInt_Skip);
                if(!vars[var].inDomain(tau[var])) {
                    return false;
                }
            }

        }

        return true;
    }

    void do_prop() {
        const SysInt numvars=vars.size();

        // Basic impl of ssup for now.
        // For 'removing assigned vars' optimization, need them to be both
        // assigned and to have done the table reduction after assignment!

        // Actually this thing below is OK: as soon as we find a valid tuple,
        // any assigned vars will be removed from ssup.

        //ssup.fill();

        // copy ssup_permanent into ssup.
        //ssup.clear();
        //for(int j=0; j<ssup_permanent.size; j++) ssup.insert(ssup_permanent.vals[j]);





        if(UseShort)
        {
            ssup.clear();

            while(limit > 0) {
                const SysInt index=tupindices[0];
                // check validity
                bool isvalid=validTuple(0);

                if(isvalid) {
                    const vector<pair<SysInt, DomainInt> >& compressed_tau = sct->compressed_tuples[index];
                    for(SysInt t = 0; t < (SysInt)compressed_tau.size(); ++t)
                    {
                        const SysInt ctf = compressed_tau[t].first;
                        ssup.unsafe_insert(ctf);
                        gacvalues[ctf].clear();
                    }

                    break;
                }
                else {
                    removeTuple(0);
                }
            }

            if(limit == 0)
            {
                // We found no valid tuples!
                getState(stateObj).setFailed(true);
                return;
            }
        }
        else
        {
            for(SysInt t=0; t<numvars; t++)
                gacvalues[t].clear();
            ssup.fill();
        }

        vector<vector<DomainInt> >::iterator tup_start = sct->tuples.begin();

        // We dealt with the first tuple, if we are in 'Short' mode.
        SysInt i= UseShort?1:0;


        while(i<limit) {
            // check validity
            if(!validTuple(i))
                removeTuple(i);
            else
                i++;
        }

        i=0;
        SysInt lim_cpy = checked_cast<SysInt>(limit);
        while(i<lim_cpy && ssup.size>0)
        {
            const SysInt index=tupindices[i];
                const vector<DomainInt> & tau=tup_start[index];
                // do stuff
                for(SysInt j=0; j<ssup.size; j++) {
                    const SysInt var=ssup.vals[j];

                    if(UseShort && tau[var]==DomainInt_Skip) {
                        ssup.unsafe_remove(var);
                        j--;
                        //if(vars[var].isAssigned()) ssup_permanent.remove(var);
                    }
                    else if(!gacvalues[var].in(tau[var])) {
                        gacvalues[var].unsafe_insert(tau[var]);

                        if(gacvalues[var].size == vars[var].getDomSize()) {
                            ssup.unsafe_remove(var);
                            j--;
                            //if(vars[var].isAssigned()) ssup_permanent.remove(var);
                        }
                    }
                }
            i++;
        }

        // Prune the domains.
        for(SysInt j=0; j<ssup.size; j++) {
            SysInt var=ssup.vals[j];
            for(DomainInt val=vars[var].getMin(); val<=vars[var].getMax(); val++) {
                if(!gacvalues[var].in(val)) {
                    vars[var].removeFromDomain(val);
                }
            }
        }

        sval.clear();
    }

    inline void removeTuple(SysInt i) {
        // Swap to end
        D_ASSERT(i<limit);
        SysInt tmp=tupindices[limit-1];
        tupindices[limit-1]=tupindices[i];
        tupindices[i]=tmp;
        limit=limit-1;
    }

};


#endif
