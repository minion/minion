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

template<typename VarArray>
struct BoundsTableConstraint : public Constraint
{
  virtual string constraint_name()
{ return "BoundTable"; }
  
  typedef typename VarArray::value_type VarRef;
  VarArray vars;
  Constraint* constraint;
  
  BoundsTableConstraint(const VarArray& _vars, Constraint* c) :
    vars(_vars), constraint(c)
  { }
  
  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    typename VarArray::iterator end_it(vars.end());
    for(typename VarArray::iterator it = vars.begin(); it < end_it; ++it)
    {
      t.push_back(make_trigger(*it,Trigger(this, 0), UpperBound));
      t.push_back(make_trigger(*it,Trigger(this, 0), LowerBound));
    }
    return t;
  }
  
  
  BOOL increment(vector<int>& v, unsigned int check_var)
  {
    for(unsigned int i=0;i<v.size();i++)
    {
      if(i == check_var)
        continue;
      if(v[i] == vars[i].getMax())
      {
        v[i] = vars[i].getMin();
      }
      else
      {
        v[i] = vars[i].getMax();
        return true;
      }
    }
    return false;
  }
  
  PROPAGATE_FUNCTION(int, DomainDelta)
  {
    PROP_INFO_ADDONE(BoundTable);
    for(unsigned int check_var = 0; check_var < vars.size(); check_var++)
    {
      int check_dom;
      //cerr << vars[check_var].data.var_num << vars[check_var].getMin() << "```" << vars[check_var].getMax() << vars[check_var].inDomain(0) <<  endl;
      for(check_dom = vars[check_var].getMin();
          check_dom <= vars[check_var].getMax(); check_dom++)
      {
        vector<DomainInt> v(vars.size());
        for(unsigned int i=0;i<vars.size();i++)
          v[i] = vars[i].getMin();
        v[check_var] = check_dom;
        BOOL satisfied = false;
        do
        {
          if(constraint->check_assignment(v))
          { 
            satisfied = true; 
            D_INFO(0,DI_ANDCON,to_string(check_var,check_dom)+to_string(v));
          }
        } while(!satisfied && increment(v, check_var));
        if(satisfied)
          goto end_check_lower;
      }
end_check_lower:
      D_INFO(0,DI_ANDCON,string("Removing up to:")+to_string(check_var,check_dom));
      vars[check_var].setMin(check_dom);
      
      for(check_dom = vars[check_var].getMax();
          check_dom >= vars[check_var].getMax(); check_dom--)
      {
        vector<DomainInt> v(vars.size());
        for(unsigned int i=0;i<vars.size();i++)
          v[i] = vars[i].getMin();
        v[check_var] = check_dom;
        BOOL satisfied = false;
        do
        {
          if(constraint->check_assignment(v))
          { 
            satisfied = true; 
            D_INFO(0,DI_ANDCON,to_string(check_var,check_dom)+to_string(v));
          }
        } while(!satisfied && increment(v, check_var));
        if(satisfied)
          goto end_check_upper;
      }
end_check_upper:
        D_INFO(0,DI_ANDCON,string("Removing up to:")+to_string(check_var,check_dom));
      vars[check_var].setMax(check_dom);
      
      
      
    }
  }  
  
  virtual void full_propagate()
  { propagate(0,0); }
};

