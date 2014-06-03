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

/** @help constraints;alldiffmatrix Description
For a latin square this constraint is placed on the whole matrix once for each value.
It ensures there is a bipartite matching between rows and columns where the edges
in the matching correspond to a variable that can be assigned to the given value. 
*/

/** @help constraints;alldiffmatrix Example 

alldiffmatrix(myVec, Value)
*/

/** @help constraints;alldiffmatrix Notes
This constraint adds some extra reasoning in addition to the GAC Alldifferents
on the rows and columns. 
*/



#include <stdlib.h>
#include <iostream>
#include <vector>
#include <deque>
#include <algorithm>
#include <utility>
#include <cmath>

#include "alldiff_gcc_shared.h"


using namespace std;


// for reverse_constraint,
#include "constraint_occurrence.h"
#include "../dynamic_constraints/dynamic_new_or.h"

#define GCCPRINT(x) 


template<typename VarArrayType, typename ValueType>
struct AlldiffMatrixConstraint : public AbstractConstraint
{
    VarArrayType var_array;
    
    bool constraint_locked;
    
    
    AlldiffMatrixConstraint(StateObj* _stateObj, const VarArrayType& _var_array, const ValueType _value) : AbstractConstraint(_stateObj),
    var_array(_var_array),
    constraint_locked(false),
    value(_value)
    
    {
        squaresize = (int) sqrt(var_array.size());
        CHECK( (squaresize*squaresize == var_array.size()), "Length of array is not a square.");
        
        CheckNotBound(var_array, "alldiffmatrix", "no alternative");
        
        rowcolmatching.resize(squaresize, -1);
        colrowmatching.resize(squaresize, -1);
        
        prev.resize(squaresize*2);
        visited.reserve(squaresize*2);
    }
    
    int squaresize;
    
    ValueType value;
    
    vector<SysInt> rowcolmatching; // For each row, the matching column.
    vector<SysInt> colrowmatching;   // For each column, the matching row. 
    
    virtual string constraint_name()
    { 
        return "alldiffmatrix";
    }
    
    CONSTRAINT_ARG_LIST2(var_array, value);
    
    
  SysInt dynamic_trigger_count()
  {
      // Need one per variable on the value of interest, and one on any other value. 
      // Two blocks : first the triggers on the value of interest (for all vars) then triggers on some other value. 
      return var_array.size();
  }
  
  inline DynamicTrigger * get_dt(SysInt var)
  {
      return dynamic_trigger_start() + var;
  }
  
  inline bool hasValue(int row, int col) {
      //   Indexing the latin square from 0 
      return var_array[row*squaresize+col].inDomain(value);
  }
  
  
  
  typedef typename VarArrayType::value_type VarRef;
  
  virtual AbstractConstraint* reverse_constraint()
  {   
      // use a watched-or of NotOccurrenceEqualConstraint, i.e. the negation of occurrence
        vector<AbstractConstraint*> con;
        
        ConstantVar one(stateObj, 1);
        for(SysInt i=0; i<squaresize; i++)
        {
            std::vector<VarRef> row_var_array;
            for(SysInt j=0; j<squaresize; j++) row_var_array.push_back(var_array[i*squaresize+j]);
            
            NotOccurrenceEqualConstraint<VarArrayType, ValueType, ConstantVar>*
                t=new NotOccurrenceEqualConstraint<VarArrayType, ValueType, ConstantVar>(
                    stateObj, row_var_array, value, one);
            con.push_back((AbstractConstraint*) t);
        }
        
        for(SysInt j=0; j<squaresize; j++)
        {
            std::vector<VarRef> col_var_array;
            for(SysInt i=0; i<squaresize; i++) col_var_array.push_back(var_array[i*squaresize+j]);
            
            NotOccurrenceEqualConstraint<VarArrayType, ValueType, ConstantVar>*
                t=new NotOccurrenceEqualConstraint<VarArrayType, ValueType, ConstantVar>(
                    stateObj, col_var_array, value, one);
            con.push_back((AbstractConstraint*) t);
        }
        
        return new Dynamic_OR(stateObj, con);
    
  }
  
  virtual void propagate(DynamicTrigger* trig)
  {
      // One of the value has been pruned somewhere. 
      if(!constraint_locked)
      {
          constraint_locked = true;
          getQueue(stateObj).pushSpecialTrigger(this);
      }
  }
  
  virtual void special_unlock() { constraint_locked = false; }
  
  virtual void special_check()
  {
    constraint_locked = false;
    
    if(getState(stateObj).isFailed())
    {
        return;
    }
    do_prop();
  }
  
  
  virtual void full_propagate()
  {
      // Set up triggers. 
      
      for(int i=0; i<var_array.size(); i++) {
          if(var_array[i].inDomain(value)) {
              var_array[i].addDynamicTrigger(dynamic_trigger_start()+i, DomainRemoval, value);
          }
      }
      
      // Clear the two matching arrays
      
      for(int i=0; i<squaresize; i++) {
          rowcolmatching[i]=-1;
          colrowmatching[i]=-1;
      }
      
      do_prop();
  }
  
    
    virtual BOOL check_assignment(DomainInt* v, SysInt array_size)
    {
        D_ASSERT(array_size == var_array.size());
        for(SysInt i=0; i<squaresize; i++) {
            SysInt count=0;
            for(SysInt j=0; j<squaresize; j++) if(v[i*squaresize+j]==value) count++;
            
            if(count!=1) return false;
            
            count=0; 
            for(SysInt j=0; j<squaresize; j++) if(v[j*squaresize+i]==value) count++;
            if(count!=1) return false;
        }
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
  
  
  
  //   Above is basically interface with Minion, below is the flow algo
  
  void do_prop()
  {
      bool matchok=bfsmatching();
      
      if(!matchok) {
          getState(stateObj).setFailed(true);
      }
      
  }
  
  
    vector<SysInt> augpath;
    deque<SysInt> fifo;
    
    vector<SysInt> prev;
    
    smallset_nolist visited;
    
    inline bool bfsmatching()
    {
        
        // iterate through the matching looking for broken matches. 
        for(SysInt row=0; row<squaresize; row++) {
            if(rowcolmatching[row]>-1  &&  !hasValue(row, rowcolmatching[row])) {
                SysInt col=rowcolmatching[row];
                rowcolmatching[row]=-1;
                colrowmatching[col]=-1;
            }
        }
        
        
        for(SysInt initialrow=0; initialrow<squaresize; initialrow++)
        {
            if(rowcolmatching[initialrow]==-1) {
                augpath.clear();
                
                // starting from a row, find a path to a free column. 
                
                // No adjacency lists.
                
                fifo.clear();  // this should be constant time but probably is not.
                
                fifo.push_back(initialrow);
                
                visited.clear();
                visited.insert(initialrow);
                
                bool finished=false;
                while(!fifo.empty() && !finished) {
                    SysInt curnode=fifo.front();
                    fifo.pop_front();
                    
                    if(curnode<squaresize) {
                        // It's a row. 
                        // Iterate through columns connected to this row. 
                        for(int col=0; col<squaresize; col++) {
                            if(!visited.in(col+squaresize) && hasValue(curnode, col)) {
                                // If we got to curnode along a matching edge, 
                                // then the 
                                visited.insert(col+squaresize);
                                prev[col+squaresize]=curnode;
                                fifo.push_back(col+squaresize);
                            }
                        }
                    }
                    else {
                        // curnode is a column. 
                        SysInt newnode=colrowmatching[curnode-squaresize];
                        
                        if(newnode==-1) {
                            // This column is not matched with anything. 
                            finished=true;
                            apply_augmenting_path(curnode, initialrow);
                        }
                        else {
                            if(!visited.in(newnode)) {
                                visited.insert(newnode);
                                prev[newnode]=curnode;
                                fifo.push_back(newnode);
                            }
                        }
                    }
                    
                }
                
                if(!finished) return false;    // No complete matching
                
                
            }
        }
        return true;
    }
    
    
    inline void apply_augmenting_path(SysInt unwindnode, SysInt startnode)
    {
        augpath.clear();
        // starting at unwindnode, unwind the path and put it in augpath.
        // Then apply it.
        // Assumes prev contains vertex numbers.
        SysInt curnode=unwindnode;
        while(curnode!=startnode)
        {
            augpath.push_back(curnode);
            curnode=prev[curnode];
        }
        augpath.push_back(curnode);
        
        std::reverse(augpath.begin(), augpath.end());
        
        GCCPRINT("Found augmenting path:" << augpath);
        
        // now apply the path.
        for(SysInt i=0; i<augpath.size()-1; i++)
        {
            curnode=augpath[i];
            if(curnode<squaresize)
            {
                // if it's a row:
                
                if(rowcolmatching[curnode]>-1) {
                    // Delete the existing edge. 
                    colrowmatching[rowcolmatching[curnode]]=-1;
                    rowcolmatching[curnode]=-1;   // not necessary.
                }
                
                rowcolmatching[curnode]=augpath[i+1]-squaresize;   // convert next node to a column number.
                colrowmatching[augpath[i+1]-squaresize]=curnode;
            }
            else
            {   
                // it's a column. Do nothing.
            }
        }
        
        GCCPRINT("rowcolmatching: "<<rowcolmatching);
        GCCPRINT("colrowmatching: "<<colrowmatching);
    }
    
  
  
  
  virtual bool get_satisfying_assignment(box<pair<SysInt,DomainInt> >& assignment)
  {
      bool matchok=bfsmatching();
      
      if(!matchok) return false;
      
      for(SysInt row=0; row<squaresize; row++) {
          SysInt col=rowcolmatching[row];
          assignment.push_back(make_pair((row*squaresize)+col, value));
      }
      return true;
  }
  
  
  
  
  
  
};
