// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef CONSTRAINT_GACTABLE_MASTER_H_QPQP
#define CONSTRAINT_GACTABLE_MASTER_H_QPQP

#include "new_table.h"

#include "constraint_GACtable_trie.h"

template <typename T>
AbstractConstraint* BuildCT_WATCHED_TABLE(const T& t1, ConstraintBlob& b) {
  return GACTableCon(t1, b.tuples);
}

/* JSON
  { "type": "constraint",
    "name": "table",
    "internal_name": "CT_WATCHED_TABLE",
    "args": [ "read_list", "read_tuples" ]
  }
*/

template <typename T>
AbstractConstraint* BuildCT_WATCHED_NEGATIVE_TABLE(const T& t1, ConstraintBlob& b) {
  return GACNegativeTableCon(t1, b.tuples);
}

/* JSON
  { "type": "constraint",
    "name": "negativetable",
    "internal_name": "CT_WATCHED_NEGATIVE_TABLE",
    "args": [ "read_list", "read_tuples" ]
  }
*/

#endif
