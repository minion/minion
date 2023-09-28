// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef _CONSTANTS_H
#define _CONSTANTS_H

#include <string>

enum TrigType { UpperBound, LowerBound, Assigned, DomainChanged, DomainRemoval };

enum TrigOp { TO_Default, TO_Store, TO_Backtrack };

static const SysInt NoDomainValue = -98765;
#define BAD_POINTER (void*)(-1)

enum BoundType { Bound_Yes, Bound_No, Bound_Maybe };

enum PropagationType {
  PropLevel_None,
  PropLevel_GAC,
  PropLevel_SACBounds,
  PropLevel_SAC,
  PropLevel_SSACBounds,
  PropLevel_SSAC
};

inline std::ostream& operator<<(std::ostream& o, PropagationType p) {
  switch(p) {
  case PropLevel_None: o << "none"; break;
  case PropLevel_GAC: o << "GAC"; break;
  case PropLevel_SACBounds: o << "SACBounds"; break;
  case PropLevel_SAC: o << "SAC"; break;
  case PropLevel_SSACBounds: o << "SSACBounds"; break;
  case PropLevel_SSAC: o << "SSAC"; break;
  }
  return o;
}

struct PropagationLevel {
  PropagationType type;
  bool limit;

  PropagationLevel(PropagationType _t = PropLevel_GAC, bool _l = false) : type(_t), limit(_l) {}
};

inline std::ostream& operator<<(std::ostream& o, PropagationLevel pl) {
  o << pl.type;
  if(pl.limit)
    o << "_limit";
  return o;
}

inline bool operator<(PropagationLevel lhs, PropagationLevel rhs) {
  return std::make_pair(lhs.type, lhs.limit) < std::make_pair(rhs.type, rhs.limit);
}

inline PropagationLevel GetPropMethodFromString(std::string instring) {
  int findSplit = instring.find('_');
  string s = instring;
  if(findSplit != string::npos) {
    s = instring.substr(0, findSplit);
  }

  PropagationType type = PropLevel_None;

  if(s == "None")
    type = PropLevel_None;
  else if(s == "GAC")
    type = PropLevel_GAC;
  else if(s == "SAC")
    type = PropLevel_SAC;
  else if(s == "SSAC")
    type = PropLevel_SSAC;
  else if(s == "SACBounds")
    type = PropLevel_SACBounds;
  else if(s == "SSACBounds")
    type = PropLevel_SSACBounds;
  else {
    ostringstream oss;
    oss << "'" << s << "'' is not a valid Propagation Method!" << std::endl;
    oss << "Valid Values: None, GAC, SAC, SSAC, SACBounds, SSACBounds" << std::endl;
    outputFatalError(oss.str());
  }

  bool limit = false;
  if(findSplit != string::npos) {
    string subtype = instring.substr(findSplit + 1, instring.size());
    if(subtype != "limit") {
      outputFatalError(instring + " is not a valid propagation method");
    }
    limit = true;
  }
  return PropagationLevel(type, limit);
}

struct EndOfSearch {};

#endif // _CONSTANTS_H
