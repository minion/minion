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

#include "minion.h"

#include "preprocess.h"
/// Apply a high level of consistency to a CSP.
/** This function is not particularly optimised, implementing only the most
 * basic SAC and SSAC algorithms */
void PropogateCSP(PropagationLevel preprocessLevel, vector<AnyVarRef>& vars, bool printInfo) {
  if(preprocessLevel.type == PropLevel_None)
    return;

  PropagateGAC propGAC(preprocessLevel);
  propGAC(vars);

  if(preprocessLevel.type == PropLevel_GAC)
    return;

  DomainInt lits = litCount(vars);
  bool boundsCheck = ((preprocessLevel.type == PropLevel_SACBounds) ||
                      (preprocessLevel.type == PropLevel_SSACBounds));

  if(boundsCheck) {
    PropagateSAC_Bounds prop_SACBounds(preprocessLevel);
    prop_SACBounds(vars);
  } else {
    PropagateSAC prop_SAC(preprocessLevel);
    prop_SAC(vars);
  }

  if(printInfo) {
    cout << "SAC" << (boundsCheck ? "Bounds" : "") << " Removed " << (lits - litCount(vars))
         << " literals" << endl;
  }

  if(preprocessLevel.type == PropLevel_SAC || preprocessLevel.type == PropLevel_SACBounds)
    return;

  lits = litCount(vars);
  if(boundsCheck) {
    PropagateSSAC_Bounds prop_SSACBounds(preprocessLevel);
    prop_SSACBounds(vars);
  } else {
    PropagateSSAC prop_SSAC(preprocessLevel);
    prop_SSAC(vars);
  }
  if(printInfo) {
    cout << "SSAC" << (boundsCheck ? "Bounds" : "") << " Removed " << (lits - litCount(vars))
         << " literals" << endl;
  }
}
