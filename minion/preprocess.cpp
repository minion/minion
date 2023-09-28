// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

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
