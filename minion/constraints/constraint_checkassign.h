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

// This is a standard fall-back constraint which can be used when a
// constraint has not got a better method of implementing their
// inverse for reification.

#ifndef CONSTRAINT_CHECKASSIGN_H
#define CONSTRAINT_CHECKASSIGN_H

template<typename OriginalConstraint, bool negate = true>
struct CheckAssignConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  {
    if(negate)
      return "!" + originalcon.constraint_name();
    return
      originalcon.constraint_name();
  }

  virtual string full_output_name()
  { return originalcon.full_output_name(); }

  OriginalConstraint originalcon;

  ReversibleInt assigned_vars;

  CheckAssignConstraint(const OriginalConstraint& con)
  : originalcon(con), assigned_vars()
  { }

  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    typename OriginalConstraint::var_type& variables = originalcon.get_vars();
    for(UnsignedSysInt i = 0; i < variables.size(); ++i)
      t.push_back(make_trigger(variables[i], Trigger(this, i), Assigned));
    return t;
  }

  virtual AbstractConstraint* reverse_constraint()
  {
    return new CheckAssignConstraint<OriginalConstraint, !negate>(originalcon);
  }

  virtual void propagate(DomainInt prop_val,DomainDelta delta)
  {
    PROP_INFO_ADDONE(CheckAssign);
    if(check_unsat(checked_cast<SysInt>(prop_val), delta))
      getState().setFailed(true);
  }

  virtual BOOL check_unsat(SysInt,DomainDelta)
  {
    typename OriginalConstraint::var_type& variables = originalcon.get_vars();

    SysInt count = assigned_vars;
    ++count;
    assigned_vars = count;
    SysInt v_size = variables.size();
    D_ASSERT(count <= v_size);

    if(count == v_size)
     return check_full_assignment();
    return false;
  }

  bool check_full_assignment()
  {
    typename OriginalConstraint::var_type& variables = originalcon.get_vars();

    MAKE_STACK_BOX(assignment, DomainInt, variables.size());
    for(UnsignedSysInt i = 0; i < variables.size(); ++i)
      {
        D_ASSERT(variables[i].isAssigned());
        assignment.push_back(variables[i].getAssignedValue());
      }
      if(assignment.size() == 0)
          return !check_assignment(NULL, 0);
      else
          return !check_assignment(&assignment.front(), assignment.size());
  }

  virtual BOOL full_check_unsat()
  {
    typename OriginalConstraint::var_type& variables = originalcon.get_vars();

    UnsignedSysInt counter = 0;
    for(UnsignedSysInt i = 0; i < variables.size(); ++i)
      if(variables[i].isAssigned()) ++counter;
    assigned_vars = counter;

    if(counter == variables.size())
      return check_full_assignment();
    return false;
  }

  virtual void full_propagate()
  {
    if(full_check_unsat())
      getState().setFailed(true);
  }

  virtual BOOL check_assignment(DomainInt* v, SysInt v_size)
  {
    typename OriginalConstraint::var_type& variables = originalcon.get_vars();
    (void)variables;
    D_ASSERT(v_size == (SysInt)variables.size());
    if(negate)
      return !originalcon.check_assignment(v, v_size);
    else
      return originalcon.check_assignment(v, v_size);
  }

  virtual vector<AnyVarRef> get_vars()
  {
    typename OriginalConstraint::var_type& variables = originalcon.get_vars();

    vector<AnyVarRef> vars;
    vars.reserve(variables.size());
    for(UnsignedSysInt i = 0; i < variables.size(); ++i)
      vars.push_back(variables[i]);
    return vars;
  }

  // Getting a satisfying assignment here is too hard.
  // Let's at least try forward checking!
  virtual bool get_satisfying_assignment(box<pair<SysInt,DomainInt> >& ret_box)
  {
    typename OriginalConstraint::var_type& variables = originalcon.get_vars();

    MAKE_STACK_BOX(c, DomainInt, variables.size());
    D_ASSERT(variables.size() == originalcon.get_vars().size());
    D_ASSERT(ret_box.size() == 0);

    SysInt free_var = -1;

    for(UnsignedSysInt i = 0; i < variables.size(); ++i)
    {
      if(!variables[i].isAssigned())
      {
        if(free_var != -1)
        {
          D_ASSERT(variables[i].getMin() != variables[i].getMax());
          ret_box.push_back(make_pair(i, variables[i].getMin()));
          ret_box.push_back(make_pair(i, variables[i].getMax()));
          return true;
        }
        else
        {
          free_var = i;
          c.push_back(-9999); // this value should never be used
        }
      }
      else
        c.push_back(variables[i].getAssignedValue());
    }



    if(free_var == -1)
    {
      if(try_assignment(ret_box, c))
        return true;
      else
        return false;
    }
    else
    {
      D_ASSERT(c[free_var] == -9999);
      D_ASSERT(variables[free_var].getMin() != variables[free_var].getMax());

      DomainInt free_min = variables[free_var].getMin();
      c[free_var]=free_min;
      if(try_assignment(ret_box, c))
        return true;
      DomainInt free_max = variables[free_var].getMax();
      c[free_var]=free_max;
      if(try_assignment(ret_box, c))
        return true;

      if(!variables[free_var].isBound())
      {
        for(DomainInt i = variables[free_var].getMin() + 1; i < variables[free_var].getMax(); ++i)
        {
          if(variables[free_var].inDomain(i))
          {
            c[free_var] = i;
            if(try_assignment(ret_box, c))
              return true;
          }
        }
      }
      else
      {
          D_ASSERT(free_min != free_max);
          ret_box.push_back(make_pair(free_var, free_min));
          ret_box.push_back(make_pair(free_var, free_max));
          return true;
      }
    }
    return false;
  }

  template<typename Box, typename Check>
  bool try_assignment(Box& assign, Check& check)
  {
    if(check_assignment(check.begin(), check.size()))
    {
      // Put the complete assignment in the box.
      for(SysInt i = 0; i < (SysInt)check.size(); ++i)
        assign.push_back(make_pair(i, check[i]));
      return true;
    }
    else
      return false;
  }
};

class AbstractWrapper
{
  AbstractConstraint* c;
public:

  typedef vector<AnyVarRef> var_type;

  AbstractWrapper(AbstractConstraint* _c) : c(_c)
  { }


  string full_output_name() const
  { return ""; }

  string constraint_name() const
  { return c->constraint_name(); }

  vector<AnyVarRef>& get_vars()
  { return *(c->get_vars_singleton()); }

  virtual bool check_assignment(DomainInt* v, SysInt v_size)
  { return c->check_assignment(v, v_size); }

};

inline AbstractConstraint* forward_check_negation(AbstractConstraint* c)
{ return new CheckAssignConstraint<AbstractWrapper>(AbstractWrapper(c)); }
#endif
