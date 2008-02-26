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
  
  long long lits = lit_count(vars);
  bool bounds_check = (preprocessLevel == PropLevel_SACBounds) ||
  (preprocessLevel == PropLevel_SSACBounds);
  clock_t start_SAC_time = clock();
  if(bounds_check)
  {
    PropagateSAC prop_SAC;
    prop_SAC(stateObj, vars);
  }
  else
  {
    PropagateSAC_Bounds prop_SAC_bounds;
    prop_SAC_bounds(stateObj, vars);
  }
  if(print_info) cout << "Preprocess Time: " << (clock() - start_SAC_time) / (1.0 * CLOCKS_PER_SEC) << endl;
  if(print_info) cout << "Removed " << (lits - lit_count(vars)) << " literals" << endl;
  
  if(preprocessLevel == PropLevel_SAC || preprocessLevel == PropLevel_SACBounds)
    return;

  lits = lit_count(vars);
  start_SAC_time = clock();
  if(bounds_check)
  {
    PropagateSSAC prop_SSAC;
    prop_SSAC(stateObj, vars);
  }
  else
  {
    PropagateSSAC_Bounds prop_SSAC_bounds;
    prop_SSAC_bounds(stateObj, vars);
  }
  if(print_info) cout << "Preprocess 2 Time: " << (clock() - start_SAC_time) / (1.0 * CLOCKS_PER_SEC) << endl;
  if(print_info) cout << "Removed " << (lits - lit_count(vars)) << " literals" << endl;

}
