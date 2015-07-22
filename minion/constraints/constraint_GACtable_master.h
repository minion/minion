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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
* USA.
*/

#ifndef CONSTRAINT_GACTABLE_MASTER_H_QPQP
#define CONSTRAINT_GACTABLE_MASTER_H_QPQP

#include "new_table.h"

#include "constraint_GACtable_trie.h"

template <typename T>
AbstractConstraint *BuildCT_WATCHED_TABLE(const T &t1, ConstraintBlob &b) {
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
AbstractConstraint *BuildCT_WATCHED_NEGATIVE_TABLE(const T &t1, ConstraintBlob &b) {
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
