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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "minion.h"

/// Apply a high level of consistency to a CSP.
/** This function is not particularly optimised, implementing only the most basic SAC and SSAC algorithms */
void PropogateCSP(StateObj* stateObj, PropagationLevel preprocessLevel, vector<AnyVarRef>& vars, bool print_info)
{
  if(preprocessLevel == PropLevel_None)
    return;
  
  PropagateGAC propGAC;
  propGAC(stateObj, vars);
  
  if(preprocessLevel == PropLevel_GAC)
    return;
  
  BigInt lits = lit_count(vars);
  bool bounds_check = ( (preprocessLevel == PropLevel_SACBounds) ||
                        (preprocessLevel == PropLevel_SSACBounds) );
  
  if(bounds_check)
  {
    PropagateSAC_Bounds prop_SAC_bounds;
    prop_SAC_bounds(stateObj, vars);
  }
  else
  {
    PropagateSAC prop_SAC;
    prop_SAC(stateObj, vars);
  }
  
  if(print_info) 
  { cout << "SAC" << (bounds_check ? "Bounds" : "") << " Removed " << (lits - lit_count(vars)) << " literals" << endl; }
  
  if(preprocessLevel == PropLevel_SAC || preprocessLevel == PropLevel_SACBounds)
    return;

  lits = lit_count(vars);
  if(bounds_check)
  {
    PropagateSSAC_Bounds prop_SSAC_bounds;
    prop_SSAC_bounds(stateObj, vars);
  }
  else
  {
    PropagateSSAC prop_SSAC;
    prop_SSAC(stateObj, vars);
  }
  if(print_info) 
  { cout << "SSAC" << (bounds_check ? "Bounds" : "") << " Removed " << (lits - lit_count(vars)) << " literals" << endl; }

}
