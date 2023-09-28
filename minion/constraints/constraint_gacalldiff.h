// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0







#ifndef CONSTRAINT_GACALLDIFF_H
#define CONSTRAINT_GACALLDIFF_H

#include "alldiff_common.h"

template <typename VarArray>
AbstractConstraint* BuildCT_GACALLDIFF(const VarArray& varArray, ConstraintBlob&) {
  return new GacAlldiffConstraint<VarArray>(varArray);
}

/* JSON
{ "type": "constraint",
  "name": "gacalldiff",
  "internal_name": "CT_GACALLDIFF",
  "args": [ "read_list" ]
}
*/
#endif
