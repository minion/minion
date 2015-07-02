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

/** @help constraints;alldiffciaran Description
Forces the input vector of variables to take distinct values. This is for experiment only. 
*/

/** @help constraints;alldiffciaran Notes
This constraint enforces an unknown consistency.
*/

#ifndef CONSTRAINT_ALLDIFF_CIARAN_H
#define CONSTRAINT_ALLDIFF_CIARAN_H

// includes for reverse constraint.
#include "constraint_equal.h"
#include "../dynamic_constraints/dynamic_new_or.h"
#include "../constraints/constraint_checkassign.h"

#include "alldiff_gcc_shared.h"

template<typename VarArray>
struct AlldiffCiaran : public AbstractConstraint
{
    virtual string constraint_name()
    {
        return "alldiffciaran";
    }

    CONSTRAINT_ARG_LIST1(var_array);
    
    VarArray var_array;
    
    AlldiffCiaran(StateObj* _stateObj, const VarArray& _var_array) : AbstractConstraint(_stateObj),
        var_array(_var_array), H(), A(), D(), sortedvars(_var_array.size(), 0)
    {
        for(unsigned int i=0; i<var_array.size(); i++) sortedvars[i]=i;
    }
    
  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    SysInt array_size = var_array.size();
    for(SysInt i = 0; i < array_size; ++i)
      t.push_back(make_trigger(var_array[i], Trigger(this, i), DomainChanged));
    return t;
  }
    
  typedef typename VarArray::value_type VarRef;
  virtual AbstractConstraint* reverse_constraint()
  { // w-or of pairwise equality.

      /// solely for reify exps
      return forward_check_negation(stateObj, this);

      vector<AbstractConstraint*> con;
      for(SysInt i=0; i<(SysInt)var_array.size(); i++)
      {
          for(SysInt j=i+1; j<(SysInt)var_array.size(); j++)
          {
              EqualConstraint<VarRef, VarRef>* t=new EqualConstraint<VarRef, VarRef>(stateObj, var_array[i], var_array[j]);
              con.push_back((AbstractConstraint*) t);
          }
      }
      return new Dynamic_OR(stateObj, con);
  }
  
  smallset H;  
  smallset A;  
  smallset D;

  DomainInt dom_min, dom_max;
  
  vector<SysInt> sortedvars;
  
  virtual void propagate(DomainInt, DomainDelta)
  {
      /*
     let H be the empty set (this will be a union of Hall sets we've found)
     let A be the empty set (this will be the accumulated union of domains not in H)
     let n be 0 (this is the number of domains contributing to A)
     for each domain D, from smallest to largest:
         D gets D \ H (remove previously seen Hall sets from D)
         A gets A union D
         n gets n + 1
         if D = emptyset or |A| < n then fail
         if |A| = n then
             H gets H union A (we've found a new Hall set)
             A gets empty set
             n gets 0
      */
      if(var_array.size()==0) return;
      
      // insertion sort
      for (SysInt i=1; i < (SysInt) var_array.size(); i++) {
        for (SysInt j = i - 1; j >= 0; j--) {
            
            if (var_array[sortedvars[j+1]].getDomSize() < var_array[sortedvars[j]].getDomSize()) {
                // swap
                SysInt tmp=sortedvars[j+1];
                sortedvars[j+1]=sortedvars[j];
                sortedvars[j]=tmp;
            }
            else {
                break;  // break j loop.
            }
        }
      }
      
      H.clear();   //  let H be the empty set (this will be a union of Hall sets we've found)
      A.clear();   //  let A be the empty set (this will be the accumulated union of domains not in H)
      
      SysInt n=0;  //   (this is the number of domains contributing to A)
      
      //std::cout << "Starting main loop." << std::endl;
      //  for each domain D, from smallest to largest:
      for(unsigned int i=0; i<sortedvars.size(); i++) {
          SysInt var=sortedvars[i];
          
          //std::cout << "Processing variable :" << var << " ("<< var_array[var].getMin() << " ," << var_array[var].getMax() << ")" <<std::endl;
          
          //  D gets D \ H (remove previously seen Hall sets from D)
          D.clear();
          for(DomainInt j=var_array[var].getMin(); j<=var_array[var].getMax(); j++) {
              if(var_array[var].inDomain(j)) {
                  if(! H.in(checked_cast<SysInt>(j-dom_min)) ) { 
                      D.insert(checked_cast<SysInt>(j-dom_min));
                  }
                  else {
                      var_array[var].removeFromDomain(j);   //  Value is in the union of known Hall sets. 
                  }
              }
          }
          
          //  A gets A union D
          //  pn: iterate through D and add any missing values to A 
          vector<SysInt>& dlist=D.getlist();
          for(unsigned int diter=0; diter<dlist.size(); diter++) {
              if( ! A.in(dlist[diter]))  A.insert(dlist[diter]);   // These are val-dom_min
          }
          
          // n gets n + 1
          n++;
          
          //  if D = emptyset or |A| < n then fail
          if(D.size()==0 || A.size()<n) {
              getState(stateObj).setFailed(true);
              return;
          }
          
          //  if |A| = n then
          if(A.size()==n) {
              // H gets H union A (we've found a new Hall set)
              vector<SysInt>& alist=A.getlist();
              for(unsigned int aiter=0; aiter<alist.size(); aiter++) {
                  if( ! H.in(alist[aiter]))  H.insert(alist[aiter]);   // These are val-dom_min
              }
              
              // A gets empty set
              A.clear();
              // n gets 0
              n=0;
          }
      }
  }

  virtual void full_propagate()
  {
      if(var_array.size()==0) return;
      
      dom_min=var_array[0].getMin();
      dom_max=var_array[0].getMax();
      for(unsigned int i=1; i<var_array.size(); i++) {
          if(var_array[i].getMin()<dom_min) dom_min=var_array[i].getMin();
          if(var_array[i].getMax()>dom_max) dom_max=var_array[i].getMax();
      }
      
      //  Set up the smallsets
      H.reserve(checked_cast<SysInt>(dom_max-dom_min+1));
      A.reserve(checked_cast<SysInt>(dom_max-dom_min+1));
      D.reserve(checked_cast<SysInt>(dom_max-dom_min+1));
      
      propagate(0,DomainDelta::empty());
  }
  
  virtual bool get_satisfying_assignment(box<pair<SysInt,DomainInt> >& assignment)
  {
      for(SysInt i=0; i< (SysInt) var_array.size(); i++) {
          if(!var_array[i].isAssigned()) {
              assignment.push_back(make_pair(i, var_array[i].getMin()));
              assignment.push_back(make_pair(i, var_array[i].getMax()));
              return true;
          }
      }
      
      // Otherwise, check pairwise assignments
      for(SysInt i=0; i< (SysInt) var_array.size(); i++) {
          for(SysInt j=i+1; j< (SysInt) var_array.size(); j++) {
              if(var_array[i].getAssignedValue()==var_array[j].getAssignedValue()) {
                  return false;
              }
          }
      }
      return true;
  }
    
    
    virtual BOOL check_assignment(DomainInt* v, SysInt array_size)
    {
      D_ASSERT(array_size == (SysInt)var_array.size());
      for(SysInt i=0;i<array_size;i++)
        for( SysInt j=i+1;j<array_size;j++)
          if(v[i]==v[j]) return false;
      return true;
    }

    virtual vector<AnyVarRef> get_vars()
    {
      vector<AnyVarRef> vars;
      vars.reserve(var_array.size());
      for(UnsignedSysInt i = 0; i < var_array.size(); ++i)
        vars.push_back(var_array[i]);
      return vars;
    }
};
#endif
