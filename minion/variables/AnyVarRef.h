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

/** @help variables General
Minion supports 4 different variable types, namely

- 0/1 variables,
- bounds variables,
- sparse bounds variables, and
- discrete variables.

Sub-dividing the variable types in this manner affords the greatest
opportunity for optimisation. In general, we recommend thinking of the
variable types as a hierarchy, where 1 (0/1 variables) is the most
efficient type, and 4 (Discrete variables) is the least. The
user should use the variable which is the highest in the hierarchy,
yet encompasses enough information to provide a full model for the
problem they are attempting to solve.

Minion also supports use of constants in place of variables, and constant
vectors in place of vectors of variables. Using constants will be at least
as efficient as using variables when the variable has a singleton domain.

See the entry on vectors for information on how vectors, matrices and,
more generally, tensors are handled in minion input. See also the
alias entry for information on how to multiply name variables for
convenience.
*/

/** @help variables;constants Description 
Minion supports the use of constants anywhere where a variable can be used. For
example, in a constraint as a replacement for a single variable, or a vector of
constants as a replacement for a vector of variables.
*/

/** @help variables;constants Examples
Use of a constant:

   eq(x,1)

Use of a constant vector:

   element([10,9,8,7,6,5,4,3,2,1],idx,e) 
*/

/** @help variables;vectors Description 
Vectors, matrices and tensors can be declared in minion
input. Matrices and tensors are for convenience, as constraints do not
take these as input; they must first undergo a flattening process to
convert them to a vector before use.
*/

/** @help variables;vectors Examples
A vector of 0/1 variables:

BOOL myvec[5]

A matrix of discrete variables:

DISCRETE sudoku[9,9] {1..9}

A 3D tensor of 0/1s:

BOOL mycube[3,3,2]

One can create a vector from scalars and elements of vectors, etc.:

alldiff([x,y,myvec[1],mymatrix[3,4]])

When a matrix or tensor is constrained, it is treated as a vector
whose entries have been strung out into a vector in index order with
the rightmost index changing most quickly, e.g.

alldiff(sudoku)

is equivalent to

alldiff([sudoku[0,0],...,sudoku[0,8],...,sudoku[8,0],...,sudoku[8,8]])

Furthermore, with indices filled selectively and the remainder filled
with underscores (_) the flattening applies only to the underscore
indices:

alldiff(sudoku[4,_])

is equivalent to

alldiff([sudoku[4,0],...,sudoku[4,8]])

Lastly, one can optionally add square brackets ([]) around an
expression to be flattened to make it look more like a vector:

alldiff([sudoku[4,_]])

is equivalent to

alldiff(sudoku[4,_])
*/

/** @help variables;alias Description
Specifying an alias is a way to give a variable another name. Aliases
appear in the **VARIABLES** section of an input file. It is best
described using some examples:

ALIAS c = a

ALIAS c[2,2] = [[myvar,b[2]],[b[1],anothervar]]
*/

#ifndef _ANYVARREF_H
#define _ANYVARREF_H

#include "../CSPSpec.h"
#include "../constants.h"
#include "../system/system.h"
#include "../solver.h"
#include "../constraints/triggers.h"

class AbstractConstraint;
class DynamicTrigger;


/// Internal type used by AnyVarRef.
struct AnyVarRef_Abstract
{
  virtual BOOL isBound() const = 0;
  virtual BOOL isAssigned() const = 0;  
  virtual DomainInt getAssignedValue() const = 0;
  virtual BOOL isAssignedValue(DomainInt i) const = 0;
  virtual BOOL inDomain(DomainInt b) const = 0;
  virtual BOOL inDomain_noBoundCheck(DomainInt b) const = 0;
  virtual DomainInt getMax() const = 0;
  virtual DomainInt getMin() const = 0;
  virtual DomainInt getInitialMax() const = 0;
  virtual DomainInt getInitialMin() const = 0;
  virtual void setMax(DomainInt i) = 0;
  virtual void setMin(DomainInt i) = 0;
  virtual void uncheckedAssign(DomainInt b) = 0;
  virtual void propagateAssign(DomainInt b) = 0;
  virtual void decisionAssign(DomainInt b) = 0;
  virtual void removeFromDomain(DomainInt b) = 0;
  virtual void addTrigger(Trigger t, TrigType type) = 0;
  virtual vector<AbstractConstraint*>* getConstraints() = 0;
  virtual void addConstraint(AbstractConstraint* c) = 0;
  virtual DomainInt getBaseVal(DomainInt) const = 0;
  virtual Var getBaseVar() const = 0;
#ifdef WDEG
  virtual int getBaseWdeg() = 0;
  virtual void incWdeg() = 0;
#endif

  virtual string virtual_to_string() = 0;
  
  virtual ~AnyVarRef_Abstract()
  {}
  
  virtual int getDomainChange(DomainDelta d) = 0;
#ifdef DYNAMICTRIGGERS
  virtual void addDynamicTrigger(DynamicTrigger* t, TrigType type, DomainInt pos = -999) = 0;
#endif
};

/// Internal type used by AnyVarRef.
template<typename VarRef>
struct AnyVarRef_Concrete : public AnyVarRef_Abstract
{

  virtual BOOL isBound() const
  { return data.isBound();}
  
  VarRef data;
  AnyVarRef_Concrete(const VarRef& _data) : data(_data)
  {}
  
  AnyVarRef_Concrete() 
  {}
  
  AnyVarRef_Concrete(const AnyVarRef_Concrete& b) : data(b.data)
  {}
  
  virtual BOOL isAssigned() const
  { return data.isAssigned(); }
  
  virtual DomainInt getAssignedValue() const
  { return data.getAssignedValue(); }
  
  virtual BOOL isAssignedValue(DomainInt i) const
  { return data.isAssignedValue(i); }
  
  virtual BOOL inDomain(DomainInt b) const
  { return data.inDomain(b); }
  
  virtual BOOL inDomain_noBoundCheck(DomainInt b) const
  { return data.inDomain_noBoundCheck(b); }
  
  virtual DomainInt getMax() const
  { return data.getMax(); }
  
  virtual DomainInt getMin() const
  { return data.getMin(); }

  virtual DomainInt getInitialMax() const
  { return data.getInitialMax(); }
  
  virtual DomainInt getInitialMin() const
  { return data.getInitialMin(); }
  
  virtual void setMax(DomainInt i)
  { data.setMax(i); }
  
  virtual void setMin(DomainInt i)
  { data.setMin(i); }
  
  virtual void uncheckedAssign(DomainInt b)
  { data.uncheckedAssign(b); }
  
  virtual void propagateAssign(DomainInt b)
  { data.propagateAssign(b); }

  virtual void decisionAssign(DomainInt b)
  { data.decisionAssign(b); }

  virtual void removeFromDomain(DomainInt b)
  { data.removeFromDomain(b); }
  
  virtual void addTrigger(Trigger t, TrigType type)
  { data.addTrigger(t, type); }

  virtual vector<AbstractConstraint*>* getConstraints()
  { return data.getConstraints(); }

  virtual void addConstraint(AbstractConstraint* c)
  { data.addConstraint(c); }

  virtual DomainInt getBaseVal(DomainInt v) const
  { return data.getBaseVal(v); }

  virtual Var getBaseVar() const
  { return data.getBaseVar(); }

#ifdef WDEG
  virtual int getBaseWdeg() 
  { return data.getBaseWdeg(); }
  virtual void incWdeg()
  { data.incWdeg(); }
#endif
  
  virtual string virtual_to_string()
  { return to_string(data); }
  
  virtual ~AnyVarRef_Concrete()
  {}
  
  int getDomainChange(DomainDelta d)
  { return data.getDomainChange(d); }

#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(DynamicTrigger* t, TrigType type, DomainInt pos = -999)
  {  data.addDynamicTrigger(t, type, pos); }
#endif
};

/// Provides a method of wrapping any variable type in a general wrapper.
class AnyVarRef
{
public:
  static const BOOL isBool = false;
  static const BoundType isBoundConst = Bound_Maybe;
  shared_ptr<AnyVarRef_Abstract> data;
  
  BOOL isBound() const
  { return data->isBound();}
  
  template<typename VarRef>
    AnyVarRef(const VarRef& _data) 
  { data = shared_ptr<AnyVarRef_Abstract>(new AnyVarRef_Concrete<VarRef>(_data)); }
  
  AnyVarRef() 
  {}
  
  AnyVarRef(const AnyVarRef& b) : data(b.data)
  {}
  
  BOOL isAssigned() const
  { return data->isAssigned(); }
  
  DomainInt getAssignedValue() const
  { return data->getAssignedValue(); }
  
  BOOL isAssignedValue(DomainInt i) const
  { 
    return data->isAssigned() &&
    data->getAssignedValue() == i;
  }
  
  BOOL inDomain(DomainInt b) const
  { return data->inDomain(b); }

  BOOL inDomain_noBoundCheck(DomainInt b) const
  { return data->inDomain_noBoundCheck(b); }
  
  DomainInt getMax() const
  { return data->getMax(); }
  
  DomainInt getMin() const
  { return data->getMin(); }

  DomainInt getInitialMax() const
  { return data->getInitialMax(); }
  
  DomainInt getInitialMin() const
  { return data->getInitialMin(); }
  
  void setMax(DomainInt i)
  { data->setMax(i); }
  
  void setMin(DomainInt i)
  { data->setMin(i); }
  
  void uncheckedAssign(DomainInt b)
  { data->uncheckedAssign(b); }
  
  void propagateAssign(DomainInt b)
  { data->propagateAssign(b); }
  
  void decisionAssign(DomainInt b)
  { data->decisionAssign(b); }
  
  void removeFromDomain(DomainInt b)
  { data->removeFromDomain(b); }

  void addTrigger(Trigger t, TrigType type)
  { data->addTrigger(t, type); }

  vector<AbstractConstraint*>* getConstraints() 
  { return data->getConstraints(); }

  void addConstraint(AbstractConstraint* c)
  { data->addConstraint(c); }

  DomainInt getBaseVal(DomainInt v) const
  { return data->getBaseVal(v); }

  Var getBaseVar() const
  { return data->getBaseVar(); }

#ifdef WDEG
  int getBaseWdeg()
  { return data->getBaseWdeg(); }

  void incWdeg()
  { data->incWdeg(); }
#endif
  
  friend std::ostream& operator<<(std::ostream& o, const AnyVarRef& avr)
  { return o << "AnyVarRef:" << avr.data->virtual_to_string(); }
  
  int getDomainChange(DomainDelta d)
  { return data->getDomainChange(d); }
  
#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(DynamicTrigger* t, TrigType type, DomainInt pos = -999)
  {  data->addDynamicTrigger(t, type, pos); }
#endif
};

#endif
