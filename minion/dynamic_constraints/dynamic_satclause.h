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

#ifndef CONSTRAINT_DYNAMIC_SAT_CLAUSE_H
#define CONSTRAINT_DYNAMIC_SAT_CLAUSE_H

#include <vector>

template<typename VarArray>
struct BoolOrConstraintDynamic : public AbstractConstraint
{
  typedef typename VarArray::value_type VarRef;

  virtual string constraint_name()
  { return "BoolOr"; }
  
  VarArray var_array;
  vector<int> negs; //negs[i]==0 iff var_array[i] is negated, NB. this
            //is also the value that must be watched
  size_t no_vars;
  int watched[2];
  int last;

  BoolOrConstraintDynamic(StateObj* _stateObj, const VarArray& _var_array,
                   const vector<int>& _negs) :
    AbstractConstraint(_stateObj), var_array(_var_array), negs(_negs), last(0)
  { 
    watched[0] = watched[1] = -2;
    no_vars = _var_array.size();
#ifndef WATCHEDLITERALS
    cerr << "This almost certainly isn't going to work... sorry" << endl;
#endif
  }
  
  int dynamic_trigger_count()
  {
    return 2;
  }

  virtual void full_propagate()
  {
    DynamicTrigger* dt = dynamic_trigger_start();
    int found = 0; //num literals that can be T found so far
    int first_found = -1;
    int next_found = -1;
    for(int i = 0; i < no_vars; i++) {
      if(var_array[i].inDomain(negs[i])) { //can literal be T?
        found++;
        if(found == 1) 
          first_found = i;
        else {
          next_found = i;
          break;
        }
      }
    }
    if(found == 0) {
      getState(stateObj).setFailed(true);
      return;
    }
    if(found == 1) { //detect unit clause
      var_array[first_found].propagateAssign(negs[first_found]);
      return; //don't bother placing any watches on unit clause
    }
    //not failed or unit, place watches
    var_array[first_found].addDynamicTrigger(dt, DomainRemoval, negs[first_found]);
    dt->trigger_info() = first_found;
    watched[0] = first_found;
    dt++;
    var_array[next_found].addDynamicTrigger(dt, DomainRemoval, negs[next_found]);
    dt->trigger_info() = next_found;
    watched[1] = next_found;
  }

  virtual void propagate(DynamicTrigger* dt)
  {
    size_t prev_var = dt->trigger_info();
    size_t other_var = watched[0] == prev_var ? watched[1] : watched[0];
    for(int i = 1; i < no_vars; i++) {
      size_t j = (last + i) % no_vars;
      VarRef& v = var_array[j];
      int neg = negs[j];
      if(j != other_var && v.inDomain(neg)) {
    v.addDynamicTrigger(dt, DomainRemoval, neg);
    dt->trigger_info() = j;
    last = j;
    watched[watched[0] == prev_var ? 0 : 1] = j;
    return;
      } 
    }
    //if we get here, we couldn't find a place to put the watch, do UP
    var_array[other_var].propagateAssign(negs[other_var]);
  }

  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    for(int i = 0; i < no_vars; i++)
      if(v[i] == negs[i])
        return true;
    return false;
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> vars;
    vars.reserve(no_vars);
    for(unsigned i = 0; i < no_vars; ++i)
      vars.push_back(AnyVarRef(var_array[i]));
    return vars;  
  }
  
  virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  {
    for(int i = 0; i < var_array.size(); ++i)
    {
      if(var_array[i].inDomain(negs[i]))
      {
        assignment.push_back(make_pair(i, negs[i]));
        return true;
      }
    }
    return false;
  }
};
#endif
