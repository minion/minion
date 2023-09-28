// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

/** @help constraints;gccweak Description
The Generalized Cardinality Constraint (GCC) (weak variant) constrains the
number of each value that a set of variables can take.

gccweak([primary variables], [values of interest], [capacity variables])

For each value of interest, there must be a capacity variable, which specifies
the number of occurrences of the value in the primary variables.

This constraint only restricts the number of occurrences of the values in
the value list. There is no restriction on the occurrences of other values.
Therefore the semantics of gccweak are identical to a set of occurrence
constraints:

occurrence([primary variables], val1, cap1)
occurrence([primary variables], val2, cap2)
...

*/





#ifndef CONSTRAINT_GCCWEAK_H_PQWOEI
#define CONSTRAINT_GCCWEAK_H_PQWOEI

#include "gcc_common.h"

template <typename VarArray1, typename VarArray2>
AbstractConstraint* BuildCT_GCCWEAK(const VarArray1& varArray, const VarArray2& capArray,
                                    ConstraintBlob& b) {
  return new GCC<VarArray1, VarArray2, false>(varArray, capArray, b.constants[0]);
}

/* JSON
{ "type": "constraint",
  "name": "gccweak",
  "internal_name": "CT_GCCWEAK",
  "args": [ "read_list", "read_constant_list", "read_list" ]
}
*/

#endif
