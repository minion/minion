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


#include "../minion.h"

#ifdef P
#undef P
#endif

//#define P(x) cout << x << endl
#define P(x)

#ifdef MINION_DEBUG
#define D(X,V) X.at(checked_cast<SysInt>(V))
#else
#define D(X,V) X[checked_cast<SysInt>(V)]
#endif

//#define SPECIAL_VM

#define UseStatePtr (0==1)
#define UseStatePtrSym (0==1)

#if UseStatePtr
#error Chris broke this in the if instruction in the VM
#endif

// UseStatePtr not finished:at least have to do the Jump instruction and also make sure the vm is not using Perm instructions.

template<typename VarArray, bool UseSymmetricVM>
struct VMConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "VMConstraint"; }

  typedef typename VarArray::value_type VarRef;

  // Note: This is max - min, not the number of values per domain.
  enum { MaxDomSize = 14 };
  enum { MaxVarSize = 13 };
  minion_array<VarRef, MaxVarSize> vars;
  minion_array<signed char, MaxVarSize> domain_min;
  minion_array<minion_array<signed char, MaxDomSize>, MaxVarSize> domain_vals;
  minion_array<pair<signed char, signed char>, MaxVarSize * MaxDomSize> literal_map;
  SysInt total_lits;

  SysInt vars_size;

  DomainInt* VM_data;
  DomainInt VM_size;
  
  #if UseStatePtr
  Reversible<int> StatePtr;
  bool AllChoicesFixed;
  #if UseStatePtrSym
  MoveableArray<char> StatePtrPerm;
  #endif
  #endif

  VMConstraint(StateObj* stateObj, const VarArray& _vars, TupleList* _tuples, TupleList* _mapping_tuples) :
  AbstractConstraint(stateObj), 
  total_lits(0), vars_size(-1),
  VM_data(_tuples->getPointer()), VM_size(_tuples->tuple_size())
  #if UseStatePtr
    ,StatePtr(stateObj, 0)
  #endif
#ifdef SPECIAL_VM
    ,constraint_locked(false)
#endif
  {
      if(_vars.size() > MaxVarSize)
          FAIL_EXIT("Only MaxVarSize (13?) variables allowed!");
      for(int i = 0; i < _vars.size(); ++i)
        D(vars,i) = D(_vars,i);
      vars_size = _vars.size();
      if(_tuples->size() != 1 || _mapping_tuples->size() != 1)
      {
          cout << "VM takes tuplelists containing a single tuple" << endl;
          FAIL_EXIT();
      }
      
      
      
      DomainInt* mapping = _mapping_tuples->getPointer();
      DomainInt mapping_size = _mapping_tuples->tuple_size();
      if(mapping_size % 2 != 0)
      {
        cout << "Mapping must be of even length";
        FAIL_EXIT();
      }

      for(int i = 0; i < MaxDomSize * MaxVarSize; ++i)
        D(literal_map,i) = std::pair<signed char, signed char>(-1,-1);

      for(int i = 0; i < MaxVarSize; ++i)
        for(int j = 0; j < MaxDomSize; ++j)
          D(D(domain_vals,i),j) = -1;

      if(mapping_size == 0)
        return;

      total_lits = checked_cast<SysInt>(mapping_size / 2);
      
      #if UseStatePtr && UseStatePtrSym
      StatePtrPerm=getMemory(stateObj).backTrack().template requestArray<char>(total_lits);
      for(int i=0; i<total_lits; i++) StatePtrPerm[i]=i;
      #endif
      
      vector<set<DomainInt> > domains(vars_size);
      for(int i = 0; i < mapping_size; i+=2)
      {
        D(domains, mapping[i]).insert(mapping[i+1]);
      }

      int literal = 0;

      for(int i = 0; i < vars_size; ++i)
      {
        if(domains[i].size() == 0)
        {
          cout << "empty domain?";
          FAIL_EXIT();
        }
        
        domain_min[i] = checked_cast<char>(*domains[i].begin());

        set<DomainInt>::iterator last = domains[i].end();
        last--;
        if(*last - domain_min[i] >= (DomainInt)MaxDomSize)
        {
          cout << "Go into vm.h and increase MaxDomSize\n";
          FAIL_EXIT();
        }

        for(set<DomainInt>::iterator it = domains[i].begin(); it != domains[i].end(); ++it)
        {
          D(D(domain_vals,i), (*it - domain_min[i]) ) = literal;
          D(literal_map,literal) = std::pair<signed char, signed char>(i, checked_cast<SysInt>(*it));
          literal++;
        }
      }


  }


  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    
    for(int i = 0; i < vars_size; ++i)
      t.push_back(make_trigger(vars[i], Trigger(this, 1), DomainChanged));
   
    return t;
  }


  SysInt dynamic_trigger_count()
    { return 0; }
#ifdef SPECIAL_VM
  virtual void special_unlock()
  { constraint_locked = false; }

  virtual void special_check()
  { constraint_locked = false; full_propagate(); }

  bool constraint_locked;

  virtual void propagate(DomainInt, DomainDelta)
  { 
      if(constraint_locked) 
          return;
      constraint_locked = true; 
      getQueue(stateObj).pushSpecialTrigger(this); 
  }
#else
  virtual void propagate(DomainInt, DomainDelta)
  { full_propagate(); }
#endif

  template<typename Data>
  Data checked_get(Data* VM_start, DomainInt length, DomainInt pos)
  {
      if(pos < 0 || pos >= length)
      {
          std::cerr << "Accessed instruction " << pos << " of " << length << std::endl;
          FAIL_EXIT();
      }
      return VM_start[checked_cast<SysInt>(pos)];
  }

#ifdef MINION_DEBUG
#define get(x) checked_cast<SysInt>(checked_get(VM_start, length, x))
#else
#define get(x) checked_cast<SysInt>(VM_start[checked_cast<SysInt>(x)])
#endif

// MINION-VM
// -1000 : Return true
// -1001 : Remove literals
//         Series of <var, val> finished by '-1'
//         For example:
//             -1001, 1, 2, 1, 1, -1
//             Removes var[1]=2 and var[1]=1
// -1010 : Branch var, val, i2
//         if(!var.inDomain(val)) jump to i2
//          NOTE: jumping to '-3' is a special value,
//                which signifies exiting the program (with true)
// -1100 : goto i1
//         Jump to i1


  template<typename Data>
  void execute_vm(Data* VM_start, DomainInt length)
  {
    int InPtr = 0;
    while(true)
    {
        switch(get(InPtr))
        {
            case -1000:
                return;
            case -1001:
            {
                InPtr++;
                while(get(InPtr) != -1)
                {
                  vars[get(InPtr)].removeFromDomain(get(InPtr+1));
                  InPtr+=2;
                }
                InPtr++;
            }
            break;
            case -1010:
            {
                InPtr++;
                if(vars[get(InPtr)].inDomain(get(InPtr+1)))
                        InPtr += 3;
                else
                        InPtr = get(InPtr+2);
                if(InPtr == -3)
                        return;
            }
            break;
            case -1100:
                InPtr = get(InPtr+1);
                if(InPtr == -3)
                        return;
            break;
            case -2000:
                    FAIL_EXIT("Attempt to use 'perm' instruction in vm. Use vmsym");
            default:
              FAIL_EXIT("Invalid VM Instruction");
        }
    }
  }

  inline DomainInt get_lit_from_varval(DomainInt var, DomainInt val)
  {
    return D(D(domain_vals,var), (val - domain_min[checked_cast<SysInt>(var)]));
  }

  inline pair<SysInt, DomainInt> get_varval_from_lit(DomainInt _lit)
  {
    const SysInt lit = checked_cast<SysInt>(_lit);
    D_ASSERT(lit >= 0 && lit < MaxVarSize * MaxDomSize);
    D_ASSERT(vars[literal_map[lit].first].getInitialMin() <= literal_map[lit].second);
    D_ASSERT(vars[literal_map[lit].first].getInitialMax() >= literal_map[lit].second);
    
    return D(literal_map,lit); 
  }


  template<typename CVal>
  pair<SysInt, DomainInt> get_varval(CVal cval, DomainInt* perm1, DomainInt* perm2, DomainInt* perm3, int var, DomainInt val)
  { 
    switch((int)cval)
    {
      case 0:
        return make_pair(var, val); 
      case 1:
      { 
        SysInt lit = checked_cast<SysInt>(get_lit_from_varval(var, val));
        DomainInt mapped_lit = perm1[lit];
        return get_varval_from_lit(mapped_lit);
      }
      case 2:
      { 
        SysInt lit = checked_cast<SysInt>(get_lit_from_varval(var, val));
        DomainInt mapped_lit = perm2[lit];
        return get_varval_from_lit(mapped_lit);
      }
      case 3:
      { 
        SysInt lit = checked_cast<SysInt>(get_lit_from_varval(var, val));
        DomainInt mapped_lit = perm3[lit];
        return get_varval_from_lit(mapped_lit);
      }
      default:
        abort();
    }
  }

  template<typename Data>
  void execute_symmetric_vm_start(Data* VM_start, DomainInt length)
  {
    #if UseStatePtr
      AllChoicesFixed=true;
    #endif
    
    if(total_lits > 0)
    {
#ifdef _WIN32
// QuickFix: Windows does not support VLAs
abort();
// Just to make code compile
DomainInt* vals = 0;
DomainInt* newvals = 0;
#else
      DomainInt vals[total_lits];
      DomainInt newvals[total_lits];
#endif
      DomainInt* perm = 0;

      #if UseStatePtr && UseStatePtrSym
      for(int i=0; i<total_lits; i++) vals[i]=StatePtrPerm[i];
      
      // 2 means start in the state where the permutation is in 'vals'... I think... 
      execute_symmetric_vm(compiletime_val<2>(), VM_start, length, StatePtr, perm, vals, newvals);
      
      #else
      
      execute_symmetric_vm(compiletime_val<0>(), VM_start, length,  
  #if UseStatePtr
      StatePtr, 
  #else
      0,
  #endif
      perm, vals, newvals);
      
      #endif
      
    }
    else
    {

      execute_symmetric_vm(compiletime_val<0>(), VM_start, length, 
  #if UseStatePtr
      StatePtr, 
  #else
      0,
  #endif
      0, 0, 0);
      
    }
  }


  template<typename Data, typename CVal>
  inline void increment_vm_perm(CVal cval, Data* VM_start, DomainInt length, DomainInt InPtr, DomainInt* perm, DomainInt* vals, DomainInt* newvals)
  {
      switch((int)cval)
      {
        case 0:
        {
          perm = VM_start + checked_cast<SysInt>(InPtr);
          InPtr += total_lits;
          #if UseStatePtr && UseStatePtrSym
          if(AllChoicesFixed) { 
            StatePtr=InPtr;
            for(int i=0; i<total_lits; i++) StatePtrPerm[i]=perm[i];
          }
          #endif
          return execute_symmetric_vm(compiletime_val<1>(), VM_start, length, InPtr, perm, vals, newvals);
        }
        break;
        case 1:
        {
          for(int i = 0; i < total_lits; ++i)
            vals[i] = perm[get(InPtr+i)];
          InPtr += total_lits;
          #if UseStatePtr && UseStatePtrSym
          if(AllChoicesFixed) { 
            StatePtr=InPtr;
            for(int i=0; i<total_lits; i++) StatePtrPerm[i]=vals[i];
          }
          #endif
          return execute_symmetric_vm(compiletime_val<2>(), VM_start, length, InPtr, perm, vals, newvals);
        }
        break;
        case 2:
        {
          for(int i = 0; i < total_lits; ++i)
            newvals[i] = vals[get(InPtr+i)];
          InPtr += total_lits;
          #if UseStatePtr && UseStatePtrSym
          if(AllChoicesFixed) { 
            StatePtr=InPtr;
            for(int i=0; i<total_lits; i++) StatePtrPerm[i]=newvals[i];
          }
          #endif
          return execute_symmetric_vm(compiletime_val<3>(), VM_start, length, InPtr, perm, vals, newvals);
        }
        break;
        case 3:
        {
          for(int i = 0; i < total_lits; ++i)
            vals[i] = newvals[get(InPtr+i)];
          InPtr += total_lits;
          #if UseStatePtr && UseStatePtrSym
          if(AllChoicesFixed) { 
            StatePtr=InPtr;
            for(int i=0; i<total_lits; i++) StatePtrPerm[i]=vals[i];
          }
          #endif
          return execute_symmetric_vm(compiletime_val<2>(), VM_start, length, InPtr, perm, vals, newvals);

        }
        break;
        default:
        abort();
      }
  }

  template<typename Data, SysInt CV>
  inline void execute_symmetric_vm(compiletime_val<CV> cv, Data* VM_start, DomainInt length, DomainInt InPtr, DomainInt* perm, DomainInt* vals, DomainInt* newvals)
  {
    //int state = 0;
    
    while(true)
    {
        if(InPtr == -3)
        {
            P(InPtr << ". -3 return");
            return;
        }
        switch(get(InPtr))
        {
            case -1000:
                P(InPtr << ". Return");
                return;
            case -1001:
            {
                P(InPtr << ". Delete lits");
                InPtr++;
                while(get(InPtr) != -1)
                {
                    pair<SysInt, DomainInt> varval = get_varval(cv, perm, vals, newvals, get(InPtr), get(InPtr+1));
                    P("  Deleting " << varval << ", original " << get(InPtr) << "," << get(InPtr+1));
                    vars[varval.first].removeFromDomain(varval.second);
                    InPtr+=2;
                }
                InPtr++;
            }
            break;
            case -1010:
            {
                P(InPtr << ". If");
                InPtr++;
                pair<SysInt, DomainInt> varval = get_varval(cv, perm, vals, newvals, get(InPtr), get(InPtr+1));
                P(" Jump based on " << varval << ", original " << get(InPtr) << "," << get(InPtr+1));
                if(vars[varval.first].inDomain(varval.second))
                {
                    P(" True, no jump");
                    InPtr += 3;
                    #if UseStatePtr
                        if(AllChoicesFixed)
                        {
                          if(vars[varval.first].isAssigned())
                          {
                            StatePtr=InPtr;
                          }
                          else
                          {
                            AllChoicesFixed = false;  // not assigned; this value could be removed so stop updating state pointer from now on. 
                          }
                        }
                    #endif
                }
                else
                {
                    P(" False, jump to " << get(InPtr+2));
                    InPtr = get(InPtr+2);
                    #if UseStatePtr
                        if(AllChoicesFixed) {
                            StatePtr=InPtr;   // 'domain value out' is always fixed. 
                        }
                    #endif
                }
            }
            break;
            case -1100:
            {
                P(InPtr << ". Jump to " << get(InPtr+1));
                InPtr = get(InPtr+1);
            }
            break;
            case -2000:
            {
                P(InPtr << ". Apply perm");
                InPtr++;
                #if UseStatePtr
                if(AllChoicesFixed) {
                    StatePtr=InPtr;
                }
                #endif
                return increment_vm_perm(cv, VM_start, length, InPtr, perm, vals, newvals);
            }
            break;
            default:
              P(" Invalid instruction: " << get(InPtr) << " at " << InPtr);
              FAIL_EXIT("Invalid VM Instruction ");
        }
    }
  }


#undef get

  virtual void full_propagate()
  {
    if(UseSymmetricVM)
      execute_symmetric_vm_start(VM_data, VM_size); 
    else
      execute_vm(VM_data, VM_size);
  }

  virtual BOOL check_assignment(DomainInt* v, SysInt v_size)
  {
      return true;
  }

  virtual vector<AnyVarRef> get_vars()
  {
    vector<AnyVarRef> anyvars;
    for(unsigned i = 0; i < vars_size; ++i)
      anyvars.push_back(vars[i]);
    return anyvars;
  }

};

template<typename VarArray>
AbstractConstraint*
  VMCon(StateObj* stateObj, const VarArray& vars, TupleList* tuples, TupleList* tuples2)
  { return new VMConstraint<VarArray,false>(stateObj, vars, tuples, tuples2); }

  template<typename VarArray>
AbstractConstraint*
  VMSymCon(StateObj* stateObj, const VarArray& vars, TupleList* tuples, TupleList* tuples2)
  { return new VMConstraint<VarArray,true>(stateObj, vars, tuples, tuples2); }
