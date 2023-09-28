// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#define GET_CONTAINER() InternalRefType::getCon_Static()
#define GET_LOCAL_CON() getCon_Static()

#include "VarRefType.h"

#ifdef MORE_SEARCH_INFO
#include "../get_info/info_var_wrapper.h"
#endif

#include "containers/booleanvariables.h"
//#include "containers/intvar.h"
#include "containers/intboundvar.h"
#include "containers/long_intvar.h"
#include "containers/sparse_intboundvar.h"

class VariableContainer {
  // Stop copying!
  VariableContainer(const VariableContainer&);
  void operator=(const VariableContainer&);

public:
  BoundVarContainer<> boundVarContainer;
  BoolVarContainer boolVarContainer;
  BigRangeVarContainer<UnsignedSysInt> bigRangeVarContainer;
  SparseBoundVarContainer<> sparseBoundVarContainer;

  VariableContainer()
      : boundVarContainer(),
        boolVarContainer(),
        bigRangeVarContainer(),
        sparseBoundVarContainer() {}

private:
  template <typename Container, typename Func>
  inline void appendVarsFromContainer(Container& container, const Func& apply) {
    for(UnsignedSysInt i = 0; i < container.varCount(); ++i) {
      apply(container.getVarNum(i));
    }
  }

public:
  template <typename Func>
  inline void forAllVars(const Func& apply) {
    appendVarsFromContainer(boundVarContainer, apply);
    appendVarsFromContainer(boolVarContainer, apply);
    appendVarsFromContainer(bigRangeVarContainer, apply);
    appendVarsFromContainer(sparseBoundVarContainer, apply);
  }

  inline std::vector<AnyVarRef> getAllVars() {
    std::vector<AnyVarRef> v;
    forAllVars([&v](const AnyVarRef& var) { v.push_back(var); });
    return v;
  }
};

#include "mappings/variable_constant.h"
#include "mappings/variable_neg.h"
#include "mappings/variable_not.h"
#include "mappings/variable_shift.h"
#include "mappings/variable_stretch.h"
#include "mappings/variable_switch_neg.h"
