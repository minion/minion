/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id$
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



// x = constant
template<typename VarRef, typename Offset>
struct UnaryEqualConstraint : public Constraint
{
  virtual string constraint_name()
  { return "UnaryEqual"; }
  
  //typedef BoolLessSumConstraint<VarArray, VarSum,1-VarToCount> NegConstraintType;
  Offset offset;
  VarRef x;
  
  UnaryEqualConstraint(VarRef _x, Offset _o) :
    offset(_o), x(_x)
  { }
  
  virtual triggerCollection setup_internal()
  {
    triggerCollection t;    
    return t;
  }
  
  //  virtual Constraint* reverse_constraint()
  
  PROPAGATE_FUNCTION(int,DomainDelta)
  { D_ASSERT(0); }
  
  
  virtual BOOL check_unsat(int,DomainDelta)
  { return (x.getMin() <= offset) && (x.getMax() >= offset); }
  
  virtual void full_propogate()
  {
    x.propogateAssign(offset);
  }
  
  virtual BOOL check_assignment(vector<DomainInt> v)
  {
    D_ASSERT(v.size() == 1);
    return v[0] == offset;
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> array;
    array.push_back(x);
	return array;
  }
};

template<typename VarRef, typename Offset>
Constraint*
UnaryEqualCon(VarRef v1,  Offset o)
{ 
  return (new UnaryEqualConstraint<VarRef,Offset>(v1,o)); 
}


