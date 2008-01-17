/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id: dynamic_sum.h 830 2007-11-20 10:42:15Z azumanga $
*/

/* Minion
* Copyright (C) 2006
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

struct BoolOrConstraintDynamic : public DynamicConstraint
{
  virtual string constraint_name()
  { return "BoolOr"; }
  
  BoolVarRef[] var_array;
  int[] neg_array; //neg_array[i]==0 iff var_array[i] is negated,
		   //NB. this is also the value that must be watched
  size_t[] watched = {-1, -1};
  
  BoolLessSumConstraintDynamic(StateObj* _stateObj, const BoolVarRef[] _var_array,
			       const bool[] _neg_array) :
    DynamicConstraint(_stateObj), var_array(_var_array), neg_array(_neg_array)
  { 
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

    int found = 0; //number of vars found to watch
    for(int i = 0; i < var_array.size(); i++) {
      BoolVarRef v = var_array[i];
      if(v.getMin() == v.getMax() && v.getAssignedValue()) {
	return; //already satisfied (literal=T), don't do any more setup
      }
      if(v.inDomain(neg_array[i])) {
	v.addDynamicTrigger(dt, neg_array[i]);
	dt->trigger_info() = i; //note what var trigger is watching
	watching[found] = i; //make record of all watched vars
	found++;
	dt++;
	if(found == 2)
	  break;
      }
    }
    if(found != 2) { //couldn't watch at least 2, propagate
      if(found == 0) //couldn't watch any
	getState(stateObj).setFailed(true); 
      else //found one literal to watch, do unit prop to make this T
	neg_array[i] == 1 ? 
	  var_array[watched[0]].setMin(1) : 
	  var_array[watched[0]].setMax(0);
    }
  }
  
  DYNAMIC_PROPAGATE_FUNCTION(DynamicTrigger* dt)
  {
    size_t prev_var = dt->trigger_info();
    size_t other_var = watched[0] == prev_var ? watched[1] : watched[0];
    for(int i = 1; i < var_array.size(); i++) {
      size_t j = (prev_var + i) % var_array.size();
      BoolVarRef cv = var_array[j];
      if(j != other_var && cv.inDomain(neg_array[j])) { //replace trigger
	cv.addDynamicTrigger(dt, neg_array[j]);
	vc->trigger_info() = j;
	watched[watched[0] == prev_var ? 0 : 1] = j;
	return;
      }
    }
    //failed to find another var to watch, do unit propagation
    if(neg_array[other_var]) var_array[other_var].setMin(1);
    else var_array[other_var].setMax(0);
  }
  
  virtual BOOL check_assignment(vector<DomainInt> v)
  {
    for(int i = 0; i < var_array.size(); i++)
      if(var_array[i].inDomain(neg_array[i]))
	return true;
    return false;
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> vars;
    vars.reserve(var_array.size());
    for(unsigned i = 0; i < var_array.size(); ++i)
      vars.push_back(AnyVarRef(var_array[i]));
    return vars;  
  }
};

/* Want only to be able to create this constraint only with boolean
   variables, but when it is called it must be supplied with an array
   of int where arr[i] == 1 iff var_array[i] is not negated in the
   clause. */
