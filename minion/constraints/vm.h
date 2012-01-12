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


#ifdef P
#undef P
#endif

#include "../minion.h"

//#define P(x) cout << x << endl
#define P(x)

#define SPECIAL_VM

#define UseStatePtr false
// UseStatePtr not finished:at least have to do the Jump instruction and also make sure the vm is not using Perm instructions.

template<typename VarArray, bool UseSymmetricVM>
struct VMConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "VMConstraint"; }

  typedef typename VarArray::value_type VarRef;
  array<VarRef, 11> vars;

  // Note: This is max - min, not the number of values per domain.
  static const int MaxDomSize = 7;
  static const int MaxVarSize = 11;
  array<signed char, MaxVarSize> domain_min;
  array<array<signed char, MaxDomSize>, MaxVarSize> domain_vals;
  array<pair<signed char, signed char>, MaxVarSize * MaxDomSize> literal_map;
  int total_lits;

  int vars_size;

  int* VM_data;
  int VM_size;
  
  #if UseStatePtr
  Reversible<int> StatePtr;
  #endif

  VMConstraint(StateObj* stateObj, const VarArray& _vars, TupleList* _tuples, TupleList* _mapping_tuples) :
  AbstractConstraint(stateObj), 
  VM_data(_tuples->getPointer()), VM_size(_tuples->tuple_size())
  #if UseStatePtr
    ,StatePtr(stateObj, 0)
  #endif
#ifdef SPECIAL_VM
    ,constraint_locked(false)
#endif
  {
      if(_vars.size() > MaxVarSize)
          FAIL_EXIT("Only MaxVarSize (11?) variables allowed!");
      for(int i = 0; i < _vars.size(); ++i)
        vars[i] = _vars[i];
      vars_size = _vars.size();
      if(_tuples->size() != 1 || _mapping_tuples->size() != 1)
      {
          cout << "VM takes tuplelists containing a single tuple" << endl;
          FAIL_EXIT();
      }

      int* mapping = _mapping_tuples->getPointer();
      int mapping_size = _mapping_tuples->tuple_size();
      if(mapping_size % 2 != 0)
      {
        cout << "Mapping must be of even length";
        FAIL_EXIT();
      }

      for(int i = 0; i < MaxDomSize * MaxVarSize; ++i)
        literal_map[i] = std::pair<signed char, signed char>(-1,-1);

      for(int i = 0; i < MaxVarSize; ++i)
        for(int j = 0; j < MaxDomSize; ++j)
          domain_vals[i][j] = -1;

      if(mapping_size == 0)
        return;

      total_lits = mapping_size / 2;

      vector<set<int> > domains(vars_size);
      for(int i = 0; i < mapping_size; i+=2)
      {
        domains[mapping[i]].insert(mapping[i+1]);
      }



      int literal = 0;

      for(int i = 0; i < vars_size; ++i)
      {
        if(domains[i].size() == 0)
        {
          cout << "empty domain?";
          FAIL_EXIT();
        }
        
        domain_min[i] = *domains[i].begin();

        set<int>::iterator last = domains[i].end();
        last--;
        if(*last - domain_min[i] >= MaxDomSize)
        {
          cout << "Go into vm.h and increase MaxDomSize\n";
          FAIL_EXIT();
        }

        set<int>::iterator it = domains[i].begin();
        for(set<int>::iterator it = domains[i].begin(); it != domains[i].end(); ++it)
        {
          domain_vals[i][*it] = literal;
          literal_map[literal] = std::pair<signed char, signed char>(i, domain_vals[i][*it]);
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


  int dynamic_trigger_count()
    { return 0; }
#ifdef SPECIAL_VM
  virtual void special_unlock()
  { constraint_locked = false; }

  virtual void special_check()
  { constraint_locked = false; full_propagate(); }

  bool constraint_locked;

  virtual void propagate(int, DomainDelta)
  { 
      if(constraint_locked) 
          return;
      constraint_locked = true; 
      getQueue(stateObj).pushSpecialTrigger(this); 
  }
#else
  virtual void propagate(int, DomainDelta)
  { full_propagate(); }
#endif

  template<typename Data>
  Data checked_get(Data* VM_start, int length, int pos)
  {
      if(pos < 0 || pos >= length)
      {
          std::cerr << "Accessed instruction " << pos << " of " << length << std::endl;
          FAIL_EXIT();
      }
      return VM_start[pos];
  }

#ifdef MINION_DEBUG
#define get(x) checked_get(VM_start, length, x)
#else
#define get(x) VM_start[x]
#endif

// MINION-VM
// -1000 : Return true
// -1001 : Remove literals
//         Series of <var, val> finished by '-1'
//         For example:
//             -1001, 1, 2, 1, 1, -1
//             Removes var[1]=2 and var[1]=1
// -1010 : Branch var, val, i1, i2
//         if(var.inDomain(val)) jump to i1 else jump to i2
//          NOTE: jumping to '-3' is a special value,
//                which signifies exiting the program (with true)
// -1100 : goto i1
//         Jump to i1
  template<typename Data>
  void execute_vm(Data* VM_start, int length)
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
                        InPtr = get(InPtr+2);
                else
                        InPtr = get(InPtr+3);
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

  inline int get_lit_from_varval(int var, int val)
  {
    return domain_vals[var][val - domain_min[val]];
  }

  inline pair<int,int> get_varval_from_lit(int lit)
  {
    D_ASSERT(lit >= 0 && lit < MaxVarSize * MaxDomSize);
    return literal_map[lit]; 
  }


  pair<int,int> get_varval(compiletime_val<0>, int* perm1, int* perm2, int* perm3, int var, int val)
  { return make_pair(var, val); }



  pair<int,int> get_varval(compiletime_val<1>, int* perm1, int* perm2, int* perm3, int var, int val)
  { 
    int lit = get_lit_from_varval(var, val);
    int mapped_lit = perm1[lit];
    return get_varval_from_lit(mapped_lit);
  }

  pair<int,int> get_varval(compiletime_val<2>, int* perm1, int* perm2, int* perm3, int var, int val)
  { 
    int lit = get_lit_from_varval(var, val);
    int mapped_lit = perm2[lit];
    return get_varval_from_lit(mapped_lit);
  }

  pair<int,int> get_varval(compiletime_val<3>, int* perm1, int* perm2, int* perm3, int var, int val)
  { 
    int lit = get_lit_from_varval(var, val);
    int mapped_lit = perm3[lit];
    return get_varval_from_lit(mapped_lit);
  }

  template<typename Data>
  void execute_symmetric_vm_start(Data* VM_start, int length)
  {
    int vals[total_lits];
    int newvals[total_lits];
    int* perm = 0;

    execute_symmetric_vm(compiletime_val<0>(), VM_start, length, 0, perm, vals, newvals);
  }


  template<typename Data, int CVal>
  inline void increment_vm_perm(compiletime_val<CVal>, Data* VM_start, int length, int InPtr, int*& perm, int* vals, int* newvals)
  {
      switch(CVal)
      {
        case 0:
        {
          perm = VM_start + InPtr;
          InPtr += total_lits;
          return execute_symmetric_vm(compiletime_val<1>(), VM_start, length, InPtr, perm, vals, newvals);
        }
        break;
        case 1:
        {
          for(int i = 0; i < total_lits; ++i)
            vals[i] = perm[get(InPtr+i)];
          InPtr += total_lits;
          return execute_symmetric_vm(compiletime_val<2>(), VM_start, length, InPtr, perm, vals, newvals);
        }
        break;
        case 2:
        {
          for(int i = 0; i < total_lits; ++i)
            newvals[i] = vals[get(InPtr+i)];
          InPtr += total_lits;
          return execute_symmetric_vm(compiletime_val<3>(), VM_start, length, InPtr, perm, vals, newvals);
        }
        break;
        case 3:
        {
          for(int i = 0; i < total_lits; ++i)
            vals[i] = newvals[get(InPtr+i)];
          InPtr += total_lits;
          return execute_symmetric_vm(compiletime_val<2>(), VM_start, length, InPtr, perm, vals, newvals);

        }
        default:
        abort();
      }
  }

  template<typename Data, int CV>
  inline void execute_symmetric_vm(compiletime_val<CV> cv, Data* VM_start, int length, int InPtr, int*& perm, int* vals, int* newvals)
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
                    pair<int,int> varval = get_varval(cv, perm, vals, newvals, get(InPtr), get(InPtr+1));
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
                pair<int,int> varval = get_varval(cv, perm, vals, newvals, get(InPtr), get(InPtr+1));
                 P(" Jump based on " << varval << ", original " << get(InPtr) << "," << get(InPtr+1));
                if(vars[varval.first].inDomain(varval.second))
                {
                    P(" True, jump to " << get(InPtr+2));
                    InPtr = get(InPtr+2);
                }
                else
                {
                    P(" False, jump to " << get(InPtr+3));
                    InPtr = get(InPtr+3);
                    #if UseStatePtr
                        if(AllChoicesFixed) {
                            StatePtr=InPtr;
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

  virtual BOOL check_assignment(DomainInt* v, int v_size)
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
