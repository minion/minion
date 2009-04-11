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

#ifndef CONSTRAINT_UNARYNEQ_H
#define CONSTRAINT_UNARYNEQ_H

// x = constant
template<typename VarRef, typename Offset>
struct UnaryNeqConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "Unary="; }
  
  //typedef BoolLessSumConstraint<VarArray, VarSum,1-VarToCount> NegConstraintType;
  Offset offset;
  VarRef x;
  
  UnaryNeqConstraint(VarRef _x, Offset _o) :
    offset(_o), x(_x)
  { }
  
  virtual triggerCollection setup_internal()
  {
    triggerCollection t;    
    return t;
  }
  
  //  virtual AbstractConstraint* reverse_constraint()
  
  virtual void propagate(int,DomainDelta)
  { D_FATAL_ERROR("This method should never be called"); }
  
  
  virtual BOOL check_unsat(int,DomainDelta)
  { return !x.inDomain(offset); }
  
  virtual void full_propagate()
  { x.removeFromDomain(offset); }
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    D_ASSERT(v.size() == 1);
    return v[0] != offset;
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> array;
    array.push_back(x);
    return array;
  }
};

template<typename VarRef, typename Offset>
AbstractConstraint*
UnaryNeqCon(VarRef v1,  Offset o)
{ return new UnaryNeqConstraint<VarRef,Offset>(v1,o); }

#endif
