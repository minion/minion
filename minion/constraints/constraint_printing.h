/*
* Minion http://minion.sourceforge.net
* Copyright (C) 2006-12
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

#include "../variables/mappings/variable_constant.h"
#include "../inputfile_parse/CSPSpec.h"

namespace ConOutput
{
  template<typename T>
  string print_vars(StateObj* stateObj, const T& t)
  {
    if(t.isAssigned())
      return to_string(t.getAssignedValue());
    else
    {
      vector<Mapper> m = t.getMapperStack();
      string prefix = "";
      if(m.size() == 1)
      {
        D_ASSERT(m[0] == Mapper(MAP_NOT));
        prefix = "!";
      }
      else
      {
        D_ASSERT(m.empty());
      }

        return prefix + getState(stateObj).getInstance()->vars.getName(t.getBaseVar());
    }
  }

  inline
  string print_vars(StateObj* stateObj,TupleList* const& t)
  { return t->getName(); }

  inline
  string print_vars(StateObj* stateObj,AbstractConstraint* const& c);

  inline
  string print_vars(StateObj* stateObj, const DomainInt& i)
  { return to_string(i); }

#ifdef MINION_DEBUG
  inline
  string print_vars(StateObj* stateObj, const SysInt& i)
  { return to_string(i); }
#endif

  template<SysInt i>
  string print_vars(StateObj* stateObj, const compiletime_val<i>)
  { return to_string(i); }

  inline
  string print_vars(StateObj* stateObj, const std::vector<AbstractConstraint*>& t)
  {
    ostringstream o;
    o << "{";
    bool first = true;
    for(size_t i = 0; i < t.size(); ++i)
    {
      if(!first)
        o << ",";
      else
        first = false;
      o << ConOutput::print_vars(stateObj, t[i]);
    }
    o << "}";
    return o.str();
  }


  template<typename T>
  string print_vars(StateObj* stateObj, const std::vector<T>& t)
  {
    ostringstream o;
    o << "[";
    bool first = true;
    for(size_t i = 0; i < t.size(); ++i)
    {
      if(!first)
        o << ",";
      else
        first = false;
      o << ConOutput::print_vars(stateObj, t[i]);
    }
    o << "]";
    return o.str();
  }

  template<typename T, size_t len>
  string print_vars(StateObj* stateObj, const array<T,len>& t)
  {
    ostringstream o;
    o << "[";
    bool first = true;
    for(size_t i = 0; i < t.size(); ++i)
    {
      if(!first)
        o << ",";
      else
        first = false;
      o << ConOutput::print_vars(stateObj, t[i]);
    }
    o << "]";
    return o.str();
  }

  template<typename T>
  inline
  vector<DomainInt> filter_constants(T& vars)
  {
    vector<DomainInt> constants;
    for(size_t i = 0; i < vars.size(); ++i)
    {
      if(vars[i].isAssigned())
      {
        constants.push_back(vars[i].getAssignedValue());
        vars.erase(vars.begin() + i);
        --i;
      }
    }
    return constants;
  }


  inline 
  void compress_arrays(StateObj* stateObj, string name, vector<AnyVarRef>& vars, AnyVarRef& result)
  {
    if(name.find("sum") != string::npos)
    {
      vector<DomainInt> res = filter_constants(vars);
      DomainInt sum = 0;
      for(size_t i = 0; i < res.size(); ++i)
        sum += res[i];

      if(sum != 0)
      {
        if(result.isAssigned())
          result = ConstantVar(stateObj, result.getAssignedValue() - sum);
        else
          vars.push_back(ConstantVar(stateObj, sum));
      }
    }

    if(name.find("min") != string::npos)
    {
      vector<DomainInt> res = filter_constants(vars);
      if(!res.empty())
      {
        DomainInt val = res[0];
        for(size_t i = 1; i < res.size(); ++i)
        {
          val = std::min(val, res[i]);
        }
        vars.push_back(ConstantVar(stateObj, val));
      }
    }

    if(name.find("max") != string::npos)
    {
      vector<DomainInt> res = filter_constants(vars);
      if(!res.empty())
      {
        DomainInt val = res[0];
        for(size_t i = 1; i < res.size(); ++i)
        {
          val = std::max(val, res[i]);
        }
        vars.push_back(ConstantVar(stateObj, val));
      }
    }
  }
  
  inline
  string print_con(StateObj* stateObj, string name)
  { return name + "()"; }


  template<typename T>
  string print_con(StateObj* stateObj, string name, const T& args)
  { 
    string s = print_vars(stateObj, args); 
    return name + "(" + s + ")";
  }

  template<typename T1, typename T2>
  string print_con(StateObj* stateObj, string name, const T1& args1, const T2& args2)
  { 

    string s1 = print_vars(stateObj, args1);
    string s2 = print_vars(stateObj, args2); 
    return name + "(" + s1 + "," + s2 + ")";
  }

  inline
  string print_array_var_con(StateObj* stateObj, string name, vector<AnyVarRef> args1, AnyVarRef args2)
  { 
    compress_arrays(stateObj, name, args1, args2);
    string s1 = print_vars(stateObj, args1);
    string s2 = print_vars(stateObj, args2); 
    return name + "(" + s1 + "," + s2 + ")";
  }
  
  template<typename T1, typename T2, typename T3>
  string print_con(StateObj* stateObj, string name, const T1& args1, const T2& args2, const T3& args3)
  { 
    string s1 = print_vars(stateObj, args1); 
    string s2 = print_vars(stateObj, args2);
    string s3 = print_vars(stateObj, args3);
    return name + "(" + s1 + "," + s2 + "," + s3 + ")";
  }

  inline
  string print_weight_array_var_con(StateObj* stateObj, string name, vector<DomainInt> args1,
                                    vector<AnyVarRef> args2, const AnyVarRef& args3)
  { 
    string s1 = print_vars(stateObj, args1); 
    string s2 = print_vars(stateObj, args2);
    string s3 = print_vars(stateObj, args3);
    return name + "(" + s1 + "," + s2 + "," + s3 + ")";
  }


  template<typename T1, typename T2>
  string print_reversible_con(StateObj* stateObj, string name, string neg_name, const T1& vars, const T2& res)
  {
    vector<Mapper> m = res.getMapperStack();
    if(!m.empty() && m.back() == Mapper(MAP_NEG))
    {
      vector<AnyVarRef> pops;
      for(size_t i = 0; i < vars.size(); ++i)
      {
        vector<Mapper> mapi = vars[i].getMapperStack();
        if(mapi.empty() || mapi.back() != Mapper(MAP_NEG))
          FATAL_REPORTABLE_ERROR();
        pops.push_back(vars[i].popOneMapper());
      }
      return print_array_var_con(stateObj, neg_name, pops, AnyVarRef(res.popOneMapper()));
    }
    else
    {
      return print_array_var_con(stateObj, name, make_AnyVarRef(vars), AnyVarRef(res));
    }
  }

template<typename T1, typename T2>
  string print_weighted_con(StateObj* stateObj, string weight, string name, const T1& sumvars, const T2& result)
  {
    if(sumvars.empty())
      return print_con(stateObj, name, sumvars, result);

    vector<Mapper> v = sumvars[0].getMapperStack();
    if(!v.empty() && (v.back().type() == MAP_MULT || v.back().type() == MAP_SWITCH_NEG))
    {
      vector<AnyVarRef> pops;
      vector<DomainInt> weights;
      for(size_t i = 0; i < sumvars.size(); ++i)
      {
        vector<Mapper> mapi = sumvars[i].getMapperStack();
        if(mapi.empty() || (mapi.back().type() != MAP_MULT && mapi.back().type() != MAP_SWITCH_NEG))
          FATAL_REPORTABLE_ERROR();
        pops.push_back(sumvars[i].popOneMapper());
        weights.push_back(mapi.back().val());
      }
      return print_weight_array_var_con(stateObj, weight + name,  weights, pops,  AnyVarRef(result));
    }
    else
    {
      return print_array_var_con(stateObj, name, make_AnyVarRef(sumvars), AnyVarRef(result));
    }
  }

template<typename T1, typename T2>
  string print_weighted_reversible_con(StateObj* stateObj, string weight, string name, string neg_name, const T1& vars, const T2& res)
  {
    vector<Mapper> m = res.getMapperStack();
    if(!m.empty() && m.back() == Mapper(MAP_NEG))
    {
      vector<AnyVarRef> pops;
      for(size_t i = 0; i < vars.size(); ++i)
      {
        vector<Mapper> mapi = vars[i].getMapperStack();
        if(mapi.empty() || mapi.back() != Mapper(MAP_NEG))
          FATAL_REPORTABLE_ERROR();
        pops.push_back(vars[i].popOneMapper());
      }
      return print_weighted_con(stateObj, weight, neg_name, pops, res.popOneMapper());
    }
    else
    {
      return print_weighted_con(stateObj, weight, name, vars, res);
    }
  }

}

#define CONSTRAINT_ARG_LIST0() \
virtual string full_output_name() \
{ return ConOutput::print_con(stateObj, constraint_name()); }

#define CONSTRAINT_ARG_LIST1(x) \
virtual string full_output_name() \
{ return ConOutput::print_con(stateObj, constraint_name(), x); }

#define CONSTRAINT_ARG_LIST2(x, y) \
virtual string full_output_name() \
{ return ConOutput::print_con(stateObj, constraint_name(), x, y); }

#define CONSTRAINT_REVERSIBLE_ARG_LIST2(name, revname, x, y) \
virtual string full_output_name() \
{ return ConOutput::print_reversible_con(stateObj, name, revname, x, y); }

#define CONSTRAINT_WEIGHTED_REVERSIBLE_ARG_LIST2(weight, name, revname, x, y) \
virtual string full_output_name() \
{ return ConOutput::print_weighted_reversible_con(stateObj, weight, name, revname, x, y); }

#define CONSTRAINT_ARG_LIST3(x, y, z) \
virtual string full_output_name() \
{ return ConOutput::print_con(stateObj, constraint_name(), x, y, z); }
