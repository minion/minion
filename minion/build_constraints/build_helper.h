// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#include "BuildConstraintConstructs.h"

using namespace BuildCon;

// These two defines just work around a bug in some compilers.
#define MERGE(x, y) MERGE2(x, y)
#define MERGE2(x, y) x##y

#define TERMINATE_BUILDCON4(CT_NAME)                                                               \
  namespace BuildCon {                                                                             \
  template <>                                                                                      \
  struct BuildConObj<CT_NAME, 0> {                                                                 \
    template <typename T1, typename T2, typename T3, typename T4>                                  \
    static AbstractConstraint*                                                                     \
    build(const pair<pair<pair<pair<EmptyType, vector<T1>*>, vector<T2>*>, vector<T3>*>,           \
                     vector<T4>*>& vars,                                                           \
          ConstraintBlob& b, SysInt) {                                                             \
      return Build##CT_NAME(*(vars.first.first.first.second), *(vars.first.first.second),          \
                            *(vars.first.second), *(vars.second), b);                              \
    }                                                                                              \
  };                                                                                               \
  }

#define TERMINATE_BUILDCON3(CT_NAME)                                                               \
  namespace BuildCon {                                                                             \
  template <>                                                                                      \
  struct BuildConObj<CT_NAME, 0> {                                                                 \
    template <typename T1, typename T2, typename T3>                                               \
    static AbstractConstraint*                                                                     \
    build(const pair<pair<pair<EmptyType, vector<T1>*>, vector<T2>*>, vector<T3>*>& vars,          \
          ConstraintBlob& b, SysInt) {                                                             \
      return Build##CT_NAME(*(vars.first.first.second), *(vars.first.second), *(vars.second), b);  \
    }                                                                                              \
  };                                                                                               \
  }

#define TERMINATE_BUILDCON2(CT_NAME)                                                               \
  namespace BuildCon {                                                                             \
  template <>                                                                                      \
  struct BuildConObj<CT_NAME, 0> {                                                                 \
    template <typename T1, typename T2>                                                            \
    static AbstractConstraint* build(const pair<pair<EmptyType, vector<T1>*>, vector<T2>*>& vars,  \
                                     ConstraintBlob& b, SysInt) {                                  \
      return Build##CT_NAME(*(vars.first.second), *(vars.second), b);                              \
    }                                                                                              \
  };                                                                                               \
  }

#define TERMINATE_BUILDCON1(CT_NAME)                                                               \
  namespace BuildCon {                                                                             \
  template <>                                                                                      \
  struct BuildConObj<CT_NAME, 0> {                                                                 \
    template <typename T1>                                                                         \
    static AbstractConstraint* build(const pair<EmptyType, vector<T1>*>& vars, ConstraintBlob& b,  \
                                     SysInt) {                                                     \
      return Build##CT_NAME(*(vars.second), b);                                                    \
    }                                                                                              \
  };                                                                                               \
  }

#define TERMINATE_BUILDCON0(CT_NAME)                                                               \
  namespace BuildCon {                                                                             \
  template <>                                                                                      \
  struct BuildConObj<CT_NAME, 0> {                                                                 \
    static AbstractConstraint* build(const EmptyType& vars, ConstraintBlob& b, SysInt) {           \
      return Build##CT_NAME(b);                                                                    \
    }                                                                                              \
  };                                                                                               \
  }

#define BUILD_CT(CT_NAME, COUNT)                                                                   \
  MERGE(TERMINATE_BUILDCON, COUNT)                                                                 \
  (CT_NAME) AbstractConstraint* build_constraint_##CT_NAME(ConstraintBlob& b) {                    \
    return BuildConObj<CT_NAME, COUNT>::build(EmptyType(), b, 0);                                  \
  }
