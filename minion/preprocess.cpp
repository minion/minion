/*
 *  preprocess.cpp
 *  Minion
 *
 *  Created by Chris Jefferson on 28/10/2007.
 *  Copyright 2007 __MyCompanyName__. All rights reserved.
 *
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
  { cout << "SAC" << (bounds_check ? "Bounds" : ".") << " Removed " << (lits - lit_count(vars)) << " literals" << endl; }
  
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
  { cout << "SSAC" << (bounds_check ? "Bounds" : ".") << " Removed " << (lits - lit_count(vars)) << " literals" << endl; }

}
