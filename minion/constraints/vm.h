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

template<typename VarArray, bool UseSymmetricVM>
struct VMConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "VMConstraint"; }

  typedef typename VarArray::value_type VarRef;
  VarArray vars;

  int* VM_data;
  int VM_size;

  VMConstraint(StateObj* stateObj, const VarArray& _vars, TupleList* _tuples) :
  AbstractConstraint(stateObj), vars(_vars), 
  VM_data(_tuples->getPointer()), VM_size(_tuples->tuple_size())
  {
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

  virtual void propagate(int lit, DomainDelta)
  { full_propagate(); }

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
        if(InPtr == -3)
            return;
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
            }
            break;
            case -1100:
                    InPtr = get(InPtr+1);
            break;
            case -2000:
                    FAIL_EXIT("Attempt to use 'perm' instruction in vm. Use vmsym");
            default:
              FAIL_EXIT("Invalid VM Instruction");
        }
    }
  }

  pair<int,int> get_varval(int* perm, int domsize, int var, int val)
  {
    int litval = var*domsize + val;
    int mappedlit = perm[litval];
    int final_val = mappedlit % domsize;
    int final_var = mappedlit / domsize;
    return make_pair(final_var, final_val);
  }

    template<typename Data>
  void execute_symmetric_vm(Data* VM_start, int length)
  {
    int InPtr = 0;
    int domsize = vars[0].getInitialMax() - vars[0].getInitialMin() + 1;
    int lits = vars.size() * domsize;
    int vals[lits];
    for(int i = 0; i < lits; ++i)
        vals[i] = i;

    while(true)
    {
        if(InPtr == -3)
            return;
        switch(get(InPtr))
        {
            case -1000:
                return;
            case -1001:
            {
                InPtr++;
                while(get(InPtr) != -1)
                {
                    pair<int,int> varval = get_varval(vals, domsize, get(InPtr), get(InPtr+1));
                    vars[varval.first].removeFromDomain(varval.second);
                    InPtr+=2;
                }
                InPtr++;
            }
            break;
            case -1010:
            {
                InPtr++;
                pair<int,int> varval = get_varval(vals, domsize, get(InPtr), get(InPtr+1));
                if(vars[varval.first].inDomain(varval.second))
                        InPtr = get(InPtr+2);
                else
                        InPtr = get(InPtr+3);
            }
            break;
            case -1100:
                InPtr = get(InPtr+1);
            break;
            case -2000:
            {
                InPtr++;
                int newvals[lits];
                for(int i = 0; i < lits; ++i)
                    newvals[i] = get(InPtr+vals[i]);
                for(int i = 0; i < lits; ++i)
                {
                    vals[i] = newvals[i];
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
    if(UseSymmetricVIM)
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
