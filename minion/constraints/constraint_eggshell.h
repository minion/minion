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
Another type of table constraint.
*/

/** @help constraints;shortstr2 Example 

ShortSTR2 is an implementation of STR2 by Christophe Lecoutre, adapted for
short supports.

shortstr2([x,y,z], [[1,2,3], [1,3,2]])

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

using namespace std;


struct arrayset {
    vector<int> vals;
    vector<int> vals_pos;
    int size;
    int minval;
    
    void initialise(int low, int high) {
        minval=low;
        vals_pos.resize(high-low+1);
        vals.resize(high-low+1);
        for(int i=0; i<high-low+1; i++) {
            vals[i]=i+low;
            vals_pos[i]=i;
        }
        size=0;
    }
    
    void clear() {
        size=0;
    }
    
    bool in(int val) {
        return vals_pos[val-minval]<size;
    }
    

    void unsafe_insert(int val)
    {
        D_ASSERT(!in(val));
        int validx=val-minval;
        int swapval=vals[size];
        vals[vals_pos[validx]]=swapval;
        vals[size]=val;
        
        vals_pos[swapval-minval]=vals_pos[validx];
        vals_pos[validx]=size;
        
        size++;

    }

    void insert(int val) {
        if(!in(val)) {
            unsafe_insert(val);
        }
    }
    
    void unsafe_remove(int val)
    {
        // swap to posiition size-1 then reduce size
        D_ASSERT(in(val));
        int validx=val-minval;
        int swapval=vals[size-1];
        vals[vals_pos[validx]]=swapval;
        vals[size-1]=val;
        
        vals_pos[swapval-minval]=vals_pos[validx];
        vals_pos[validx]=size-1;
        
        size--;
    }

    void remove(int val) {
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
    vector<int> vals;
    vector<int> vals_pos;
    ReversibleInt size;
    int minval;
    
    ReversibleArrayset(StateObj * _so) : size(_so) {}
    
    void initialise(int low, int high) {
        minval=low;
        vals_pos.resize(high-low+1);
        vals.resize(high-low+1);
        for(int i=0; i<high-low+1; i++) {
            vals[i]=i+low;
            vals_pos[i]=i;
        }
        size=vals.size();
    }
    
    bool in(int val) {
        return vals_pos[val-minval]<size;
    }
    
    void remove(int val) {
        // swap to posiition size-1 then reduce size
        if(in(val)) {
            int validx=val-minval;
            int swapval=vals[size-1];
            vals[vals_pos[validx]]=swapval;
            vals[size-1]=val;
            
            vals_pos[swapval-minval]=vals_pos[validx];
            vals_pos[validx]=size-1;
            
            size=size-1;
        }
    }
};

#define UseShort true

struct EggShellData
{
    vector<vector<int> > tuples;
    vector<vector<pair<int,int> > > compressed_tuples;

    EggShellData(TupleList* _tuples, size_t varsize)
    {
        // Decode the tuples, they are all encoded in one tuple.
        if(UseShort)
        {
            D_ASSERT(_tuples->size()==1);

            vector<DomainInt> encoded = _tuples->get_vector(0);

            vector<int> temp;
            temp.resize(varsize, -1000000);

            vector<pair<int,int> > compressed_temp;

            for(int i=0; i<encoded.size(); i=i+2) {
                if(encoded[i]==-1) {
                // end of a short support.
                    if(encoded[i+1]!=-1) {
                        cout << "Split marker is -1,-1 in tuple for supportsgac." << endl;
                        abort();
                    }
                    tuples.push_back(temp);

                    temp.clear();
                    temp.resize(varsize, -1000000);
                    compressed_tuples.push_back(compressed_temp);
                    compressed_temp.clear();
                }
                else
                {
                    if(encoded[i]<0 || encoded[i]>=varsize) {
                        cout << "Tuple passed into supportsgac does not correctly encode a set of short supports." << endl;
                        abort();
                    }
                    temp[encoded[i]]=encoded[i+1]; 
                    compressed_temp.push_back(make_pair(encoded[i], encoded[i+1]));
                }
            }

            if(encoded[encoded.size()-2]!=-1 || encoded[encoded.size()-1]!=-1) {
                cout << "Last -1,-1 marker missing from tuple in supportsgac."<< endl;
                abort();
            }   
        }
        else {
            // Normal table constraint.             
            // Hacky hacky hack -- copy the tuples.
            for(int i=0; i<_tuples->size(); i++) {
                tuples.push_back(_tuples->get_vector(i));
            }
        }
    }
};

inline EggShellData* TupleList::getEggShellData(size_t varcount)
{
    if(egg == NULL)
        egg = new EggShellData(this, varcount);

    return egg;
}

template<typename VarArray>
struct EggShell : public AbstractConstraint
{
    VarArray vars;
    
    bool constraint_locked;
    
    vector<int> tupindices;
    
    ReversibleInt limit;   // In tupindices, indices less than limit are not known to be invalid.
    
    EggShellData* sct;

    EggShell(StateObj* _stateObj, const VarArray& _var_array, TupleList* _tuples) : AbstractConstraint(_stateObj), 
    vars(_var_array), constraint_locked(false), limit(_stateObj), sct(_tuples->getEggShellData(_var_array.size()))
    //, ssup_permanent(_stateObj)
    {   
        if(sct->tuples.size() > 0)
        {
            CHECK(sct->tuples[0].size() == vars.size(), "Cannot use same table for two constraints with different numbers of variables!");
        }
        tupindices.resize(sct->tuples.size());
        for(int i=0; i<sct->tuples.size(); i++) {
            tupindices[i]=i;
        }
        
        ssup.initialise(0, vars.size()-1);
        sval.initialise(0, vars.size()-1);
        
        //ssup_permanent.initialise(0, vars.size()-1);
        
        gacvalues.resize(vars.size());
        for(int i=0; i<vars.size(); i++) {
            gacvalues[i].initialise(vars[i].getInitialMin(), vars[i].getInitialMax());
        }

        std::random_shuffle(tupindices.begin(), tupindices.end());
    }
    
    virtual string constraint_name()
    {
        return "ShortSTR2";
    }
    
    
    // Set up triggers.
    virtual triggerCollection setup_internal()
    {
        triggerCollection t;
        int array_size = vars.size();
        for(int i = 0; i < array_size; ++i) {
            t.push_back(make_trigger(vars[i], Trigger(this, i), DomainChanged));
        }
        return t;
    }
    
    virtual void full_propagate() {
        limit=sct->tuples.size();
        
        // pretend all variables have changed.
        for(int i=0; i<vars.size(); i++) sval.insert(i);
        
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
    
    virtual bool check_assignment(DomainInt* v, int v_size) {
        D_ASSERT(v_size == vars.size());
        vector<int> temp;
        for(int i=0; i<v_size; i++) temp.push_back(v[i]);
        return std::find(sct->tuples.begin(), sct->tuples.end(), temp)!=sct->tuples.end();
    }
    
    virtual void propagate(int prop_var, DomainDelta)
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
        
        if(getState(stateObj).isFailed())
        {
            return;
        }
        
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
    

    bool validTuple(int i)
    {
             int index=tupindices[i];
            vector<int> & tau=sct->tuples[index];

        bool isvalid=true;

        for(int j=0; j<sval.size; j++) {
            int var=sval.vals[j];
            if(UseShort) {
                if( (tau[var]!=-1000000) && !vars[var].inDomain(tau[var])) {
                    isvalid=false;
                    break;
                }
            }
            else {
                if(!vars[var].inDomain(tau[var])) {
                    isvalid=false;
                    break;
                }
            }
            
        }

        return isvalid;
    }

    void do_prop() {
        int numvars=vars.size();
        
        // Basic impl of ssup for now. 
        // For 'removing assigned vars' optimization, need them to be both
        // assigned and to have done the table reduction after assignment!
        
        // Actually this thing below is OK: as soon as we find a valid tuple,
        // any assigned vars will be removed from ssup.
        
        //ssup.fill();
        
        // copy ssup_permanent into ssup.
        //ssup.clear();
        //for(int j=0; j<ssup_permanent.size; j++) ssup.insert(ssup_permanent.vals[j]); 
        
        for(int i=0; i<numvars; i++) {
            gacvalues[i].clear();
        }
        
        int i=0;


        if(UseShort)
        {
            ssup.clear();
            
            bool pass_first_loop=false;
            while(i<limit) {
                int index=tupindices[i];
                // check validity
                bool isvalid=validTuple(i);
                
                if(isvalid) {
                    vector<pair<int,int> >& compressed_tau = sct->compressed_tuples[index];
                    for(int i = 0; i < compressed_tau.size(); ++i)
                        ssup.insert(compressed_tau[i].first);
                    pass_first_loop = true;
                    break;
                }
                else {
                    removeTuple(i);
                }
            }

            if(!pass_first_loop)
            {
                // We found no valid tuples!
                getState(stateObj).setFailed(true);
                return;
            }
        }
        else
        {
            ssup.fill();
        }

        while(i<limit) {
            int index=tupindices[i];
            vector<int> & tau=sct->tuples[index];
            
            // check validity
            bool isvalid=validTuple(i);
            
            if(isvalid) {
                
                // do stuff
                for(int j=0; j<ssup.size; j++) {
                    int var=ssup.vals[j];
                    
                    if(UseShort && tau[var]==-1000000) {
                        ssup.unsafe_remove(var);
                        j--;
                        //if(vars[var].isAssigned()) ssup_permanent.remove(var);
                    }
                    else if(!gacvalues[var].in(tau[var])) {
                        gacvalues[var].unsafe_insert(tau[var]);
                        
                        // Next line NOT the correct implementation!
                        // Dominion has dom size
                        if(gacvalues[var].size == vars[var].getDomSize()) {
                            ssup.unsafe_remove(var);
                            j--;
                            //if(vars[var].isAssigned()) ssup_permanent.remove(var);
                        }
                    }
                }
                
                i++;
            }
            else {
                removeTuple(i);
            }
        }

        // Prune the domains.
        for(int j=0; j<ssup.size; j++) {
            int var=ssup.vals[j];
            for(int val=vars[var].getMin(); val<=vars[var].getMax(); val++) {
                if(!gacvalues[var].in(val)) {
                    vars[var].removeFromDomain(val);
                }
            }
        }
        
        sval.clear();
    }
    
    inline void removeTuple(int i) {
        // Swap to end
        D_ASSERT(i<limit);
        int tmp=tupindices[limit-1];
        tupindices[limit-1]=tupindices[i];
        tupindices[i]=tmp;
        limit=limit-1;
    }
    
};


#endif
