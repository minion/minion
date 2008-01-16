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

/// var1 /\ var2 = var3
template<typename VarRef1, typename VarRef2, typename VarRef3>
struct AndConstraint : public Constraint
{
  virtual string constraint_name()
  { return "And"; }
  
  VarRef1 var1;
  VarRef2 var2;
  VarRef3 var3;
  AndConstraint(VarRef1 _var1, VarRef2 _var2, VarRef3 _var3) :
    var1(_var1), var2(_var2), var3(_var3)
  {}
  
  virtual triggerCollection setup_internal()
  {
    D_INFO(2,DI_ANDCON,"Setting up Constraint");
    triggerCollection t;
	t.push_back(make_trigger(var1, Trigger(this, 1), LowerBound));
	t.push_back(make_trigger(var2, Trigger(this, 2), LowerBound));
	t.push_back(make_trigger(var3, Trigger(this, 3), LowerBound));
	t.push_back(make_trigger(var1, Trigger(this, -1), UpperBound));
	t.push_back(make_trigger(var2, Trigger(this, -2), UpperBound));
	t.push_back(make_trigger(var3, Trigger(this, -3), UpperBound));
	return t;
  }
  
  PROPAGATE_FUNCTION(int i, DomainDelta)
  {
    switch(i)
    {
      case 1:
        if(var2.isAssignedValue(true))
          var3.propogateAssign(true);
        else
        {
          if(var3.isAssignedValue(false))
            var2.propogateAssign(false);
        }
          break;
        
      case 2:
        if(var1.isAssignedValue(true))
          var3.propogateAssign(true);
        else
        {
          if(var3.isAssignedValue(false))
            var1.propogateAssign(false);
        }
          break;
        
      case 3:
        var1.propogateAssign(true);
        var2.propogateAssign(true);
        break;
        
        
      case -1:
      case -2:
        var3.propogateAssign(false);
        break;
        
      case -3:
        if(var1.isAssignedValue(true))
          var2.propogateAssign(false);
        else
        {
          if(var2.isAssignedValue(true))
            var1.propogateAssign(false);
        }
          break;
    }
    
  }
  
  virtual void full_propogate()
  {
    if(var1.isAssignedValue(false) || var2.isAssignedValue(false))
      var3.propogateAssign(false);
    
    if(var1.isAssignedValue(true) && var2.isAssignedValue(true))
      var3.propogateAssign(true);
    
    if(var3.isAssignedValue(false))
    {
      if(var1.isAssignedValue(true))
		var2.propogateAssign(false);
      if(var2.isAssignedValue(true))
		var1.propogateAssign(false);
    }
    
    if(var3.isAssignedValue(true))
    {
      var1.propogateAssign(true);
      var2.propogateAssign(true);
    }
    
  }
  
  virtual BOOL check_assignment(vector<int> v)
  {
    D_ASSERT(v.size() == 3);
    return (v[0] && v[1]) == v[2];
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
	vector<AnyVarRef> v;
	v.push_back(var1);
	v.push_back(var2);
	v.push_back(var3);
	return v;
  }
};

template<typename VarRef1, typename VarRef2, typename VarRef3>
Constraint*
AndCon(VarRef1 var1, VarRef2 var2, VarRef3 var3)
{ 
  return (new AndConstraint<VarRef1,VarRef2,VarRef3>(var1,var2,var3)); 
}
