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

/** @help constraints Description
Minion supports many constraints and these are regularly being
improved and added to. In some cases multiple implementations of the
same constraints are provided and we would appreciate additional
feedback on their relative merits in your problem.

Minion does not support nesting of constraints, however this can be
achieved by auxiliary variables and reification.

Variables can be replaced by constants. You can find out more on
expressions for variables, vectors, etc. in the section on variables.
*/

/** @help constraints References 
help variables
*/

#ifndef ABSTRACT_CONSTRAINT
#define ABSTRACT_CONSTRAINT

/// Base type from which all constraints are derived.
class AbstractConstraint
{
 public:

  vector<AnyVarRef> vars;
  StateObj* stateObj;
  BOOL full_propagate_done;

  AbstractConstraint(StateObj* _stateObj) : stateObj(_stateObj), full_propagate_done(false)
    {}

  /// Method to get constraint name for debugging.
  virtual string constraint_name() = 0;
  
  /// Performs a full round of propagation and sets up any data needs by propagate().
  /** This function can be called during search if the function is reified */
  virtual void full_propagate() = 0;
  
  // Returns the variables of the constraint
  virtual vector<AnyVarRef> get_vars() = 0;
  
  vector<AnyVarRef>* get_vars_singleton() //piggyback singleton vector on get_vars()
  { 
    if(vars.size() == 0) vars = get_vars(); //for efficiency: no constraint over 0 variables
    return &vars; 
  }
  
  /// Checks if an assignment is satisfied.
  /** This takes the variable order returned by, and is mainly only used by, get_table_constraint() */
  virtual BOOL check_assignment(vector<DomainInt>) = 0;
    
  virtual ~AbstractConstraint()
  {}
};

#endif
