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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

#include "../inputfile_parse/CSPSpec.h"
#include "../variables/mappings/variable_constant.h"

namespace ConOutput {
template <typename T>
string print_vars(const T& t) {
  if(t.isAssigned())
    return tostring(t.assignedValue());
  else {
    vector<Mapper> m = t.getMapperStack();
    string prefix = "";
    if(m.size() == 1) {
      D_ASSERT(m[0] == Mapper(MAP_NOT));
      prefix = "!";
    } else {
      D_ASSERT(m.empty());
    }

    return prefix + getState().getInstance()->vars.getName(t.getBaseVar());
  }
}

inline string print_vars(TupleList* const& t) {
  return t->getName();
}

inline string print_vars(ShortTupleList* const& t) {
  return t->getName();
}

inline string print_vars(AbstractConstraint* const& c);

inline string print_vars(const DomainInt& i) {
  return tostring(i);
}

#ifdef MINION_DEBUG
inline string print_vars(const SysInt& i) {
  return tostring(i);
}
#endif

template <typename T, T i>
string print_vars(const compiletimeVal<T, i>) {
  return tostring(i);
}

inline string print_vars(const std::vector<AbstractConstraint*>& t) {
  ostringstream o;
  o << "{";
  bool first = true;
  for(size_t i = 0; i < t.size(); ++i) {
    if(!first)
      o << ",";
    else
      first = false;
    o << ConOutput::print_vars(t[i]);
  }
  o << "}";
  return o.str();
}

template <typename T>
string print_vars(const std::vector<T>& t) {
  ostringstream o;
  o << "[";
  bool first = true;
  for(size_t i = 0; i < t.size(); ++i) {
    if(!first)
      o << ",";
    else
      first = false;
    o << ConOutput::print_vars(t[i]);
  }
  o << "]";
  return o.str();
}

template <typename T>
string print_vars(const std::vector<std::pair<T, T>>& t) {
  ostringstream o;
  o << "[";
  bool first = true;
  for(size_t i = 0; i < t.size(); ++i) {
    if(!first)
      o << ",";
    else
      first = false;
    o << ConOutput::print_vars(t[i].first);
    o << ",";
    o << ConOutput::print_vars(t[i].second);
  }
  o << "]";
  return o.str();
}

template <typename T, size_t len>
string print_vars(const std::array<T, len>& t) {
  ostringstream o;
  o << "[";
  bool first = true;
  for(size_t i = 0; i < t.size(); ++i) {
    if(!first)
      o << ",";
    else
      first = false;
    o << ConOutput::print_vars(t[i]);
  }
  o << "]";
  return o.str();
}

template <typename T>
inline vector<DomainInt> filter_constants(T& vars) {
  vector<DomainInt> constants;
  for(size_t i = 0; i < vars.size(); ++i) {
    if(vars[i].isAssigned()) {
      constants.push_back(vars[i].assignedValue());
      vars.erase(vars.begin() + i);
      --i;
    }
  }
  return constants;
}

inline void compress_arrays(string name, vector<AnyVarRef>& vars, AnyVarRef& result) {
  if(name.find("sum") != string::npos) {
    vector<DomainInt> res = filter_constants(vars);
    DomainInt sum = 0;
    for(size_t i = 0; i < res.size(); ++i)
      sum += res[i];

    if(sum != 0) {
      if(result.isAssigned())
        result = ConstantVar(result.assignedValue() - sum);
      else
        vars.push_back(ConstantVar(sum));
    }
  }

  if(name.find("min") != string::npos) {
    vector<DomainInt> res = filter_constants(vars);
    if(!res.empty()) {
      DomainInt val = res[0];
      for(size_t i = 1; i < res.size(); ++i) {
        val = std::min(val, res[i]);
      }
      vars.push_back(ConstantVar(val));
    }
  }

  if(name.find("max") != string::npos) {
    vector<DomainInt> res = filter_constants(vars);
    if(!res.empty()) {
      DomainInt val = res[0];
      for(size_t i = 1; i < res.size(); ++i) {
        val = std::max(val, res[i]);
      }
      vars.push_back(ConstantVar(val));
    }
  }
}

inline string printCon(string name) {
  return name + "()";
}

template <typename T>
string printCon(string name, const T& args) {
  string s = print_vars(args);
  return name + "(" + s + ")";
}

template <typename T1, typename T2>
string printCon(string name, const T1& args1, const T2& args2) {

  string s1 = print_vars(args1);
  string s2 = print_vars(args2);
  return name + "(" + s1 + "," + s2 + ")";
}

inline string print_array_var_con(string name, vector<AnyVarRef> args1, AnyVarRef args2) {
  compress_arrays(name, args1, args2);
  string s1 = print_vars(args1);
  string s2 = print_vars(args2);
  return name + "(" + s1 + "," + s2 + ")";
}

template <typename T1, typename T2, typename T3>
string printCon(string name, const T1& args1, const T2& args2, const T3& args3) {
  string s1 = print_vars(args1);
  string s2 = print_vars(args2);
  string s3 = print_vars(args3);
  return name + "(" + s1 + "," + s2 + "," + s3 + ")";
}

template <typename T1, typename T2, typename T3, typename T4, typename T5>
string printCon(string name, const T1& args1, const T2& args2, const T3& args3, const T4& args4,
                 const T5& args5) {
  string s1 = print_vars(args1);
  string s2 = print_vars(args2);
  string s3 = print_vars(args3);
  string s4 = print_vars(args4);
  string s5 = print_vars(args5);
  return name + "(" + s1 + "," + s2 + "," + s3 + "," + s4 + "," + s5 + ")";
}

inline string print_weight_array_var_con(string name, vector<DomainInt> args1,
                                         vector<AnyVarRef> args2, const AnyVarRef& args3) {
  string s1 = print_vars(args1);
  string s2 = print_vars(args2);
  string s3 = print_vars(args3);
  return name + "(" + s1 + "," + s2 + "," + s3 + ")";
}

template <typename T1, typename T2>
string print_reversible_con(string name, string neg_name, const T1& vars, const T2& res) {
  vector<Mapper> m = res.getMapperStack();
  if(!m.empty() && m.back() == Mapper(MAP_NEG)) {
    vector<AnyVarRef> pops;
    for(size_t i = 0; i < vars.size(); ++i) {
      vector<Mapper> mapi = vars[i].getMapperStack();
      if(mapi.empty() || mapi.back() != Mapper(MAP_NEG))
        FATAL_REPORTABLE_ERROR();
      pops.push_back(vars[i].popOneMapper());
    }
    return print_array_var_con(neg_name, pops, AnyVarRef(res.popOneMapper()));
  } else {
    return print_array_var_con(name, make_AnyVarRef(vars), AnyVarRef(res));
  }
}

template <typename T1, typename T2>
string print_weighted_con(string weight, string name, const T1& sumvars, const T2& result) {
  if(sumvars.empty())
    return printCon(name, sumvars, result);

  vector<Mapper> v = sumvars[0].getMapperStack();
  if(!v.empty() && (v.back().type() == MAP_MULT || v.back().type() == MAP_SWITCH_NEG)) {
    vector<AnyVarRef> pops;
    vector<DomainInt> weights;
    for(size_t i = 0; i < sumvars.size(); ++i) {
      vector<Mapper> mapi = sumvars[i].getMapperStack();
      if(mapi.empty() || (mapi.back().type() != MAP_MULT && mapi.back().type() != MAP_SWITCH_NEG))
        FATAL_REPORTABLE_ERROR();
      pops.push_back(sumvars[i].popOneMapper());
      weights.push_back(mapi.back().val());
    }
    return print_weight_array_var_con(weight + name, weights, pops, AnyVarRef(result));
  } else {
    return print_array_var_con(name, make_AnyVarRef(sumvars), AnyVarRef(result));
  }
}

template <typename T1, typename T2>
string print_weighted_reversible_con(string weight, string name, string neg_name, const T1& vars,
                                     const T2& res) {
  vector<Mapper> m = res.getMapperStack();
  if(!m.empty() && m.back() == Mapper(MAP_NEG)) {
    vector<AnyVarRef> pops;
    for(size_t i = 0; i < vars.size(); ++i) {
      vector<Mapper> mapi = vars[i].getMapperStack();
      if(mapi.empty() || mapi.back() != Mapper(MAP_NEG))
        FATAL_REPORTABLE_ERROR();
      pops.push_back(vars[i].popOneMapper());
    }
    return print_weighted_con(weight, neg_name, pops, res.popOneMapper());
  } else {
    return print_weighted_con(weight, name, vars, res);
  }
}
} // namespace ConOutput

#define CONSTRAINT_ARG_LIST0()                                                                     \
  virtual string fullOutputName() {                                                              \
    return ConOutput::printCon(constraintName());                                                \
  }

#define CONSTRAINT_ARG_LIST1(x)                                                                    \
  virtual string fullOutputName() {                                                              \
    return ConOutput::printCon(constraintName(), x);                                             \
  }

#define CONSTRAINT_ARG_LIST2(x, y)                                                                 \
  virtual string fullOutputName() {                                                              \
    return ConOutput::printCon(constraintName(), x, y);                                          \
  }

#define CONSTRAINT_ARG_LIST5(a, b, c, d, e)                                                        \
  virtual string fullOutputName() {                                                              \
    return ConOutput::printCon(constraintName(), a, b, c, d, e);                                 \
  }

#define CONSTRAINT_REVERSIBLE_ARG_LIST2(name, revname, x, y)                                       \
  virtual string fullOutputName() {                                                              \
    return ConOutput::print_reversible_con(name, revname, x, y);                                   \
  }

#define CONSTRAINT_WEIGHTED_REVERSIBLE_ARG_LIST2(weight, name, revname, x, y)                      \
  virtual string fullOutputName() {                                                              \
    return ConOutput::print_weighted_reversible_con(weight, name, revname, x, y);                  \
  }

#define CONSTRAINT_ARG_LIST3(x, y, z)                                                              \
  virtual string fullOutputName() {                                                              \
    return ConOutput::printCon(constraintName(), x, y, z);                                       \
  }
