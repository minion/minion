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





#ifndef CONSTRAINT_ALLDIFF_CIARAN_H
#define CONSTRAINT_ALLDIFF_CIARAN_H

// includes for reverse constraint.
#include "../constraints/constraint_checkassign.h"
#include "../dynamic_constraints/dynamic_new_or.h"
#include "constraint_equal.h"

#include "alldiff_gcc_shared.h"

template <typename VarArray>
struct AlldiffCiaran : public AbstractConstraint {
  virtual string constraintName() {
    return "alldiffciaran";
  }

  CONSTRAINT_ARG_LIST1(varArray);

  VarArray varArray;

  AlldiffCiaran(StateObj* _stateObj, const VarArray& _varArray)
      : AbstractConstraint(_stateObj),
        varArray(_varArray),
        H(),
        A(),
        D(),
        sortedvars(_varArray.size(), 0) {
    for(unsigned int i = 0; i < varArray.size(); i++)
      sortedvars[i] = i;
  }

  virtual triggerCollection setup_internal() {
    triggerCollection t;
    SysInt arraySize = varArray.size();
    for(SysInt i = 0; i < arraySize; ++i)
      t.push_back(makeTrigger(varArray[i], Trigger(this, i), DomainChanged));
    return t;
  }

  typedef typename VarArray::value_type VarRef;
  virtual AbstractConstraint* reverseConstraint() { // w-or of pairwise equality.

    /// solely for reify exps
    return forwardCheckNegation(stateObj, this);

    vector<AbstractConstraint*> con;
    for(SysInt i = 0; i < (SysInt)varArray.size(); i++) {
      for(SysInt j = i + 1; j < (SysInt)varArray.size(); j++) {
        EqualConstraint<VarRef, VarRef>* t =
            new EqualConstraint<VarRef, VarRef>(stateObj, varArray[i], varArray[j]);
        con.push_back((AbstractConstraint*)t);
      }
    }
    return new Dynamic_OR(stateObj, con);
  }

  smallset H;
  smallset A;
  smallset D;

  DomainInt domMin, domMax;

  vector<SysInt> sortedvars;

  virtual void propagate(DomainInt, DomainDelta) {
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
    if(varArray.size() == 0)
      return;

    // insertion sort
    for(SysInt i = 1; i < (SysInt)varArray.size(); i++) {
      for(SysInt j = i - 1; j >= 0; j--) {

        if(varArray[sortedvars[j + 1]].domSize() < varArray[sortedvars[j]].domSize()) {
          // swap
          SysInt tmp = sortedvars[j + 1];
          sortedvars[j + 1] = sortedvars[j];
          sortedvars[j] = tmp;
        } else {
          break; // break j loop.
        }
      }
    }

    H.clear(); //  let H be the empty set (this will be a union of Hall sets we've found)
    A.clear(); //  let A be the empty set (this will be the accumulated union of domains not in H)

    SysInt n = 0; //   (this is the number of domains contributing to A)

    // std::cout << "Starting main loop." << std::endl;
    //  for each domain D, from smallest to largest:
    for(unsigned int i = 0; i < sortedvars.size(); i++) {
      SysInt var = sortedvars[i];

      // std::cout << "Processing variable :" << var << " ("<< varArray[var].min() << " ," <<
      // varArray[var].max() << ")" <<std::endl;

      //  D gets D \ H (remove previously seen Hall sets from D)
      D.clear();
      for(DomainInt j = varArray[var].min(); j <= varArray[var].max(); j++) {
        if(varArray[var].inDomain(j)) {
          if(!H.in(checked_cast<SysInt>(j - domMin))) {
            D.insert(checked_cast<SysInt>(j - domMin));
          } else {
            varArray[var].removeFromDomain(j); //  Value is in the union of known Hall sets.
          }
        }
      }

      //  A gets A union D
      //  pn: iterate through D and add any missing values to A
      vector<SysInt>& dlist = D.getlist();
      for(unsigned int diter = 0; diter < dlist.size(); diter++) {
        if(!A.in(dlist[diter]))
          A.insert(dlist[diter]); // These are val-domMin
      }

      // n gets n + 1
      n++;

      //  if D = emptyset or |A| < n then fail
      if(D.size() == 0 || A.size() < n) {
        getState(stateObj).setFailed(true);
        return;
      }

      //  if |A| = n then
      if(A.size() == n) {
        // H gets H union A (we've found a new Hall set)
        vector<SysInt>& alist = A.getlist();
        for(unsigned int aiter = 0; aiter < alist.size(); aiter++) {
          if(!H.in(alist[aiter]))
            H.insert(alist[aiter]); // These are val-domMin
        }

        // A gets empty set
        A.clear();
        // n gets 0
        n = 0;
      }
    }
  }

  virtual void fullPropagate() {
    if(varArray.size() == 0)
      return;

    domMin = varArray[0].min();
    domMax = varArray[0].max();
    for(unsigned int i = 1; i < varArray.size(); i++) {
      if(varArray[i].min() < domMin)
        domMin = varArray[i].min();
      if(varArray[i].max() > domMax)
        domMax = varArray[i].max();
    }

    //  Set up the smallsets
    H.reserve(checked_cast<SysInt>(domMax - domMin + 1));
    A.reserve(checked_cast<SysInt>(domMax - domMin + 1));
    D.reserve(checked_cast<SysInt>(domMax - domMin + 1));

    propagate(0, DomainDelta::empty());
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    for(SysInt i = 0; i < (SysInt)varArray.size(); i++) {
      if(!varArray[i].isAssigned()) {
        assignment.push_back(make_pair(i, varArray[i].min()));
        assignment.push_back(make_pair(i, varArray[i].max()));
        return true;
      }
    }

    // Otherwise, check pairwise assignments
    for(SysInt i = 0; i < (SysInt)varArray.size(); i++) {
      for(SysInt j = i + 1; j < (SysInt)varArray.size(); j++) {
        if(varArray[i].assignedValue() == varArray[j].assignedValue()) {
          return false;
        }
      }
    }
    return true;
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt arraySize) {
    D_ASSERT(arraySize == (SysInt)varArray.size());
    for(SysInt i = 0; i < arraySize; i++)
      for(SysInt j = i + 1; j < arraySize; j++)
        if(v[i] == v[j])
          return false;
    return true;
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> vars;
    vars.reserve(varArray.size());
    for(UnsignedSysInt i = 0; i < varArray.size(); ++i)
      vars.push_back(varArray[i]);
    return vars;
  }
};
#endif
