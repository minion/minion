// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#include "../BuildVariables.h"
#include "../inputfile_parse/CSPSpec.h"

namespace BuildCon {

/// General case in iteratively build constraints.
/// This isn't inline, as we don't want the compiler to waste time inlining it.
template <ConstraintType constraint, SysInt size>
struct BuildConObj {
  template <typename ConData>
  static AbstractConstraint* build(const ConData& partial_build, ConstraintBlob& b,
                                   SysInt pos) DOM_NOINLINE;
};


SysInt same_type_check(ConstraintBlob& b, SysInt pos);

vector<BoolVarRef> make_boolvarref(const vector<Var>& vars);
vector<VarNot<BoolVarRef>> make_neg_boolref(const vector<Var>& vars);
vector<BoundVarRef> make_boundref(const vector<Var>& vars);
vector<SparseBoundVarRef> make_sparseboundref(const vector<Var>& vars);
vector<BigRangeVarRef> make_bigrangeref(const vector<Var>& vars);
vector<ConstantVar> make_constref(const vector<Var>& vars);
vector<AnyVarRef> make_avrref(const vector<Var>& vars);


template <ConstraintType constraint, SysInt size>
template <typename ConData>
DOM_NOINLINE AbstractConstraint* BuildConObj<constraint, size>::build(const ConData& partial_build,
                                                         ConstraintBlob& b, SysInt pos) {

  const vector<Var>& vars = b.vars[pos];

  SysInt type = same_type_check(b,pos);
  bool same_type = (type != -123);
  (void)type;
  (void)same_type;

#ifndef QUICK_COMPILE
  if(same_type) {
    switch(type) {
    case VAR_BOOL: {
      vector<BoolVarRef> v = make_boolvarref(vars);
      return BuildConObj<constraint, size - 1>::build(make_pair(partial_build, &v), b, pos + 1);
    }
    case VAR_NOTBOOL: {
      vector<VarNot<BoolVarRef>> v = make_neg_boolref(vars);
      return BuildConObj<constraint, size - 1>::build(make_pair(partial_build, &v), b, pos + 1);
    }
    case VAR_BOUND: {
      vector<BoundVarRef> v = make_boundref(vars);
      return BuildConObj<constraint, size - 1>::build(make_pair(partial_build, &v), b, pos + 1);
    }
    case VAR_SPARSEBOUND: {
      vector<SparseBoundVarRef> v = make_sparseboundref(vars);
      return BuildConObj<constraint, size - 1>::build(make_pair(partial_build, &v), b, pos + 1);
    }
    case VAR_DISCRETE: {
      vector<BigRangeVarRef> v = make_bigrangeref(vars);
      return BuildConObj<constraint, size - 1>::build(make_pair(partial_build, &v), b, pos + 1);
    }
    case VAR_SPARSEDISCRETE: INPUT_ERROR("Sparse Discrete Variables current broken. Sorry");

    case VAR_CONSTANT: {
      vector<ConstantVar> v = make_constref(vars);
      return BuildConObj<constraint, size - 1>::build(make_pair(partial_build, &v), b, pos + 1);
    }
    }
  } else
#endif
  {
    vector<AnyVarRef> v = make_avrref(vars);
    return BuildConObj<constraint, size - 1>::build(make_pair(partial_build, &v), b, pos + 1);
  }
  // This FAIL_EXIT is here to stop a "no return in non-void function" warning.
  // It should never be reached.
  INPUT_ERROR("This should never be reached..");
}
} // namespace BuildCon

AbstractConstraint* build_constraint(ProbSpec::ConstraintBlob& b);
