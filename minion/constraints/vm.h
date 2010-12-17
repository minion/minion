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

//#define P(x) cout << x << endl
#define P(x)

#define SPECIAL_VM

template<typename VarArray, bool UseSymmetricVM>
struct VMConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "VMConstraint"; }

  typedef typename VarArray::value_type VarRef;
  array<VarRef, 10> vars;

  int* VM_data;
  int VM_size;

  VMConstraint(StateObj* stateObj, const VarArray& _vars, TupleList* _tuples) :
  AbstractConstraint(stateObj), 
  VM_data(_tuples->getPointer()), VM_size(_tuples->tuple_size())
#ifdef SPECIAL_VM
    ,constraint_locked(false)
#endif
  {
      if(_vars.size() != 10)
	  FAIL_EXIT("Invalid constraint length");
      for(int i = 0; i < _vars.size(); ++i)
	vars[i] = _vars[i];
      if(_tuples->size() != 1)
      {
          cout << "VM takes a single tuple" << endl;
          FAIL_EXIT();
      }
  }

   virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    
    for(int i = 0; i < vars.size(); ++i)
      t.push_back(make_trigger(vars[i], Trigger(this, 1), DomainChanged));
   
    return t;
  }


  int dynamic_trigger_count()
    { return 0; }
#ifdef SPECIAL_VM
  virtual void special_unlock()
  { constraint_locked = false; }

  virtual void special_check()
  { full_propagate(); }

  bool constraint_locked;

  virtual void propagate(int, DomainDelta)
  { constraint_locked = true; getQueue(stateObj).pushSpecialTrigger(this); }
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

  inline pair<int,int> get_varval(int state, int* perm1, int* perm2, int* perm3, int domsize, int var, int val)
  {
    switch(state)
    {
      case 0:
        return make_pair(var, val);
      case 1: {
                int litval = var*domsize + val;
                int mappedlit = perm1[litval];
                int final_val = mappedlit % domsize;
                int final_var = mappedlit / domsize;
                return make_pair(final_var, final_val); }
      case 2: {
                int litval = var*domsize + val;
                int mappedlit = perm2[litval];
                int final_val = mappedlit % domsize;
                int final_var = mappedlit / domsize;
                return make_pair(final_var, final_val); }
      case 3: {
                int litval = var*domsize + val;
                int mappedlit = perm3[litval];
                int final_val = mappedlit % domsize;
                int final_var = mappedlit / domsize;
                return make_pair(final_var, final_val); }
      default:
                abort();
    }
  }

    template<typename Data>
  void execute_symmetric_vm(Data* VM_start, int length)
  {
    int InPtr = 0;
    int domsize = 2;
    int lits = 20;
    int state = 0;
    int vals[lits];
    int newvals[lits];
    int* perm = 0;

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
                    pair<int,int> varval = get_varval(state, perm, vals, newvals, domsize, get(InPtr), get(InPtr+1));
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
                pair<int,int> varval = get_varval(state, perm, vals, newvals, domsize, get(InPtr), get(InPtr+1));
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
                
                switch(state)
                {
                  case 0:
                  {
                    state = 1;
                    perm = VM_start + InPtr;
                  }
                  break;
                  case 1:
                  {
                    state = 2;
                    for(int i = 0; i < lits; ++i)
                      vals[i] = perm[get(InPtr+i)];
                  }
                  break;
                  case 2:
                  {
                    state = 3;
                    for(int i = 0; i < lits; ++i)
                      newvals[i] = vals[get(InPtr+i)];
                  }
                  break;
                  case 3:
                  {
                    state = 2;
                    for(int i = 0; i < lits; ++i)
                      vals[i] = newvals[get(InPtr+i)];
                  }
                  break;
                }
                InPtr += lits;
            }
            break;
            default:
              FAIL_EXIT("Invalid VM Instruction");
        }
    }
  }


#undef get

  virtual void full_propagate()
  {
    if(UseSymmetricVM)
      execute_symmetric_vm(VM_data, VM_size); 
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
    for(unsigned i = 0; i < vars.size(); ++i)
      anyvars.push_back(vars[i]);
    return anyvars;
  }

};

template<typename VarArray>
AbstractConstraint*
  VMCon(StateObj* stateObj, const VarArray& vars, TupleList* tuples)
  { return new VMConstraint<VarArray,false>(stateObj, vars, tuples); }

  template<typename VarArray>
AbstractConstraint*
  VMSymCon(StateObj* stateObj, const VarArray& vars, TupleList* tuples)
  { return new VMConstraint<VarArray,true>(stateObj, vars, tuples); }
