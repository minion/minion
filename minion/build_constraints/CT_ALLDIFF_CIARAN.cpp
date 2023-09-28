// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#include "../minion.h"

#include "../constraints/constraint_alldiff_ciaran.h"

template <typename VarArray>
AbstractConstraint* BuildCT_ALLDIFF_CIARAN(StateObj* stateObj, const VarArray& varArray,
                                           ConstraintBlob&) {
  return new AlldiffCiaran<VarArray>(stateObj, varArray);
}

BUILD_CT(CT_ALLDIFF_CIARAN, 1)
