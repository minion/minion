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



#ifndef CONSTRAINT_TEST_H
#define CONSTRAINT_TEST_H

template<typename VarArray>
struct TestConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "Test"; }
  
   typedef typename VarArray::value_type ArrayVarRef;
  
   array<ArrayVarRef,4> var_array;
  
  TestConstraint(StateObj* _stateObj, const VarArray& _var_array) :
    AbstractConstraint(_stateObj)
  {
    var_array[0] = _var_array[0];
    var_array[1] = _var_array[1];
    var_array[2] = _var_array[2];
    var_array[3] = _var_array[3];
  }
  
  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    
    for(int i = 0; i < var_array.size(); ++i)
    { // Have to add 1 else the 0th element will be lost.
      t.push_back(make_trigger(var_array[i], Trigger(this, 2*i), LowerBound));
      t.push_back(make_trigger(var_array[i], Trigger(this, 2*i+1), UpperBound));
    }
    
    return t;
  }
  
  
  virtual void propagate(int lit, DomainDelta)
  {

   full_propagate();
  }
  
    
  virtual void full_propagate()
  {
if(var_array[0].inDomain(0)) {
  if(var_array[0].inDomain(1)) {
    if(var_array[2].inDomain(0)) {
      if(!var_array[2].inDomain(1)) {
        if(!var_array[3].inDomain(1)) {
          if(var_array[1].inDomain(0)) {
            if(!var_array[1].inDomain(1)) {
              var_array[0].removeFromDomain(0);
            }
          } else {
            var_array[0].removeFromDomain(1);
          }
        }
      }
    } else {
      if(!var_array[3].inDomain(0)) {
        if(var_array[1].inDomain(0)) {
          if(!var_array[1].inDomain(1)) {
            var_array[0].removeFromDomain(0);
          }
        } else {
          var_array[0].removeFromDomain(1);
        }
      }
    }
  } else {
    if(var_array[1].inDomain(1)) {
      if(var_array[2].inDomain(0)) {
        if(!var_array[2].inDomain(1)) {
          if(!var_array[3].inDomain(1)) {
            var_array[1].removeFromDomain(0);
          }
        }
      } else {
        if(!var_array[3].inDomain(0)) {
          var_array[1].removeFromDomain(0);
        }
      }
    } else {
      if(var_array[2].inDomain(0)) {
        if(var_array[2].inDomain(1)) {
          if(var_array[3].inDomain(0)) {
            if(!var_array[3].inDomain(1)) {
              var_array[2].removeFromDomain(0);
            }
          } else {
            var_array[2].removeFromDomain(1);
          }
        } else {
          var_array[3].removeFromDomain(0);
        }
      } else {
        var_array[3].removeFromDomain(1);
      }
    }
  }
} else {
  if(var_array[1].inDomain(0)) {
    if(var_array[2].inDomain(0)) {
      if(!var_array[2].inDomain(1)) {
        if(!var_array[3].inDomain(1)) {
          var_array[1].removeFromDomain(1);
        }
      }
    } else {
      if(!var_array[3].inDomain(0)) {
        var_array[1].removeFromDomain(1);
      }
    }
  } else {
    if(var_array[2].inDomain(0)) {
      if(var_array[2].inDomain(1)) {
        if(var_array[3].inDomain(0)) {
          if(!var_array[3].inDomain(1)) {
            var_array[2].removeFromDomain(0);
          }
        } else {
          var_array[2].removeFromDomain(1);
        }
      } else {
        var_array[3].removeFromDomain(0);
      }
    } else {
      var_array[3].removeFromDomain(1);
    }
  }
}      
  }
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    return (std::min(v[0], v[1]) == v[2]);
  }

  virtual vector<AnyVarRef> get_vars()
  {
    vector<AnyVarRef> vars;
    vars.reserve(var_array.size());
    for(unsigned i = 0; i < var_array.size(); ++i)
      vars.push_back(AnyVarRef(var_array[i]));
    return vars;
  }
};
#endif
