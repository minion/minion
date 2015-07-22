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

#ifndef _CONSTANTS_H
#define _CONSTANTS_H

#include <string>

enum TrigType { UpperBound, LowerBound, Assigned, DomainChanged, DomainRemoval };

enum TrigOp { TO_Default, TO_Store, TO_Backtrack };

static const SysInt NoDomainValue = -98765;
#define BAD_POINTER (void *)(-1)

enum BoundType { Bound_Yes, Bound_No, Bound_Maybe };

enum PropagationLevel {
  PropLevel_None,
  PropLevel_GAC,
  PropLevel_SACBounds,
  PropLevel_SAC,
  PropLevel_SSACBounds,
  PropLevel_SSAC
};

inline PropagationLevel GetPropMethodFromString(std::string s) {
  if (s == "None")
    return PropLevel_None;
  else if (s == "GAC")
    return PropLevel_GAC;
  else if (s == "SAC")
    return PropLevel_SAC;
  else if (s == "SSAC")
    return PropLevel_SSAC;
  else if (s == "SACBounds")
    return PropLevel_SACBounds;
  else if (s == "SSACBounds")
    return PropLevel_SSACBounds;
  else {
    ostringstream oss;
    oss << "'" << s << "'' is not a valid Propagation Method!" << std::endl;
    oss << "Valid Values: None, GAC, SAC, SSAC, SACBounds, SSACBounds" << std::endl;
    output_fatal_error(oss.str());
  }
}

struct EndOfSearch {};

#endif // _CONSTANTS_H
