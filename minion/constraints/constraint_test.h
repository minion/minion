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



#ifndef CONSTRAINT_TEST_H
#define CONSTRAINT_TEST_H

#define PREPARE
#include "generated_constraint_code.h"
#undef PREPARE


#ifdef MINION_DEBUG
#define D(X,V) X.at(V)
#else
#define D(X,V) X[V]
#endif



template<typename VarArray>
struct TestConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "Test"; }
  
   typedef typename VarArray::value_type ArrayVarRef;
  

  int vars_size;

  static const int MaxDomSize = 14;
  static const int MaxVarSize = 13;
  array<ArrayVarRef,MaxVarSize> vars;

#ifdef SYMMETRIC
  array<signed char, MaxVarSize> domain_min;
  array<array<signed char, MaxDomSize>, MaxVarSize> domain_vals;
  array<pair<signed char, signed char>, MaxVarSize * MaxDomSize> literal_map;
  int total_lits;  
#endif 
  
  TestConstraint(StateObj* _stateObj, const VarArray& _vars) :
    AbstractConstraint(_stateObj)
  {
      for(int i=0; i<_vars.size(); i++)
      {
          vars[i] = _vars[i];
      }
      vars_size = _vars.size();

#ifdef SYMMETRIC
      vector<int> vec_mapping = get_mapping_vector();
      int mapping_size = vec_mapping.size();

      int* mapping = &(vec_mapping[0]);

      for(int i = 0; i < MaxDomSize * MaxVarSize; ++i)
        D(literal_map,i) = std::pair<signed char, signed char>(-1,-1);

      for(int i = 0; i < MaxVarSize; ++i)
        for(int j = 0; j < MaxDomSize; ++j)
          D(D(domain_vals,i),j) = -1;

      if(mapping_size == 0)
        return;

      total_lits = mapping_size / 2;

      vector<set<int> > domains(vars_size);
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
        
        domain_min[i] = *domains[i].begin();

        set<int>::iterator last = domains[i].end();
        last--;
        if(*last - domain_min[i] >= MaxDomSize)
        {
          cout << "Go into constraint_test.h and increase MaxDomSize\n";
          FAIL_EXIT();
        }

        for(set<int>::iterator it = domains[i].begin(); it != domains[i].end(); ++it)
        {
          D(D(domain_vals,i), (*it - domain_min[i]) ) = literal;
          D(literal_map,literal) = std::pair<signed char, signed char>(i, *it);
          literal++;
        }
      }

      D_ASSERT(literal == total_lits);
#endif
  }
  
  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    
    for(int i = 0; i < vars_size; ++i)
    { // Have to add 1 else the 0th element will be lost.
      //t.push_back(make_trigger(var_array[i], Trigger(this, 2*i), LowerBound));
      //t.push_back(make_trigger(var_array[i], Trigger(this, 2*i+1), UpperBound));
      t.push_back(make_trigger(vars[i], Trigger(this, 2*i+1), DomainChanged));
    }
    
    return t;
  }
  
#ifdef SYMMETRIC

  inline int get_lit_from_varval(int var, int val)
  {
    return D(D(domain_vals,var), (val - domain_min[var]));
  }

  inline pair<int,int> get_varval_from_lit(int lit)
  {
    D_ASSERT(lit >= 0 && lit < MaxVarSize * MaxDomSize);
    D_ASSERT(vars[literal_map[lit].first].getInitialMin() <= literal_map[lit].second);
    D_ASSERT(vars[literal_map[lit].first].getInitialMax() >= literal_map[lit].second);
    
    return D(literal_map,lit); 
  }

  pair<int,int> apply_perm(compiletime_val<0>, int const* perm, int var, int val)
  {
    int lit = get_lit_from_varval(var, val);
    D_ASSERT(lit >= 0 && lit < total_lits);
    int mapped_lit = perm[lit];
    D_ASSERT(mapped_lit >= 0 && mapped_lit < total_lits);
    return get_varval_from_lit(mapped_lit);
  }
/*
  pair<int,int> apply_perm(compiletime_val<1>, int const* perm, int var, int val)
  { return std::pair<int, int>(perm[var], val); }
*/
  template<typename CVal>
  pair<int,int> get_varval(CVal cval, int const* perm1, int const* perm2, int const* perm3, int var, int val)
  { 
    switch((int)cval)
    {
      case 0:
        return make_pair(var, val); 
      case 1:
      { 
        return apply_perm(compiletime_val<0>(), perm1, var, val);
      }
      case 2:
      {
        return apply_perm(compiletime_val<0>(), perm2, var, val);
      }
      case 3:
      { 
        return apply_perm(compiletime_val<0>(), perm3, var, val);
      }
      default:
        abort();
    }
  }


  template<typename Data, typename CVal>
  inline int applyPermutation(CVal cval, int* perm, int* vals, int* newvals, Data* NewPerm)
  {
      switch((int)cval)
      {
        case 0:
        {
          for(int i = 0; i < total_lits; ++i)
          {
            perm[i] = NewPerm[i];
            D_ASSERT(perm[i] >= 0 && perm[i] < total_lits);
          }
          return 1;
        }
        break;
        case 1:
        {
          for(int i = 0; i < total_lits; ++i)
          {
            vals[i] = perm[NewPerm[i] ];
            D_ASSERT(vals[i] >= 0 && vals[i] < total_lits);
          }
          return 2;
        }
        break;
        case 2:
        {
          for(int i = 0; i < total_lits; ++i)
          {
            newvals[i] = vals[NewPerm[i] ];
            D_ASSERT(newvals[i] >= 0 && newvals[i] < total_lits);
          }
          return 3;
        }
        break;
        case 3:
        {
          for(int i = 0; i < total_lits; ++i)
          {
            vals[i] = newvals[NewPerm[i] ];
            D_ASSERT(vals[i] >= 0 && vals[i] < total_lits);
          }
          return 2;
        }
        break;
        default:
        abort();
      }
  }

  template<typename CV>
  bool permutedInDomain(CV cVal, int const* perm, int const* vals, int const* newvals, int var, int val)
  {
    pair<int, int> varval = get_varval(cVal, perm, vals, newvals, var, val);
    D_ASSERT(vars[varval.first].getInitialMin() <= varval.second);
    D_ASSERT(vars[varval.first].getInitialMax() >= varval.second);
    return vars[varval.first].inDomain(varval.second);
  }

  template<typename CV>
  void permutedRemoveFromDomain(CV cVal, int const* perm, int const* vals, int const* newvals, int var, int val)
  {
    pair<int, int> varval = get_varval(cVal, perm, vals, newvals, var, val);
    D_ASSERT(vars[varval.first].getInitialMin() <= varval.second);
    D_ASSERT(vars[varval.first].getInitialMax() >= varval.second);
    vars[varval.first].removeFromDomain(varval.second);
  }
  
#endif

  virtual void propagate(int lit, DomainDelta)
  {
   full_propagate();
  }

#define PERM_ARGS state, perm, vals, newvals
#define FULL_PROPAGATE_INIT \
int state = 0; \
int vals[total_lits]; \
int newvals[total_lits]; \
int perm[total_lits]; \

//#define PRINT_MACRO(X) std::cout << "Label: " << X << std::endl;
#define PRINT_MACRO(X)

#include "generated_constraint_code.h"



  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
   return true;// return (std::min(v[0], v[1]) == v[2]);
  }

  virtual vector<AnyVarRef> get_vars()
  {
    vector<AnyVarRef> varRet;
    varRet.reserve(vars.size());
    for(unsigned i = 0; i < vars_size; ++i)
      varRet.push_back(AnyVarRef(vars[i]));
    return varRet;
  }
};
#endif
