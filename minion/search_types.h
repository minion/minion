// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef MINION_SEARCH_TYPES_H
#define MINION_SEARCH_TYPES_H

// Value types shared by the solver and its library interface. Keep this header
// independent of solver implementations and standard-library containers.
enum VarOrderEnum {
  ORDER_NONE,

  ORDER_STATIC,
  ORDER_SDF,
  ORDER_SRF,
  ORDER_LDF,
  ORDER_ORIGINAL,
  ORDER_WDEG,
  ORDER_DOMOVERWDEG,
  ORDER_CONFLICT,
};

enum ValOrderEnum { VALORDER_NONE, VALORDER_ASCEND, VALORDER_DESCEND, VALORDER_RANDOM };

enum PropagationType {
  PropLevel_None,
  PropLevel_GAC,
  PropLevel_SACBounds,
  PropLevel_SAC,
  PropLevel_SSACBounds,
  PropLevel_SSAC
};

struct PropagationLevel {
  PropagationType type;
  bool limit;

  PropagationLevel(PropagationType _t = PropLevel_GAC, bool _l = false) : type(_t), limit(_l) {}
};

#endif
