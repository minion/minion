// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef BUILDCONSTRAINT_H
#define BUILDCONSTRAINT_H

#include "system/system.h"

#include "inputfile_parse/CSPSpec.h"
#include "variables/AnyVarRef.h"

namespace BuildCon {

/// Helper function used in a few places.
AnyVarRef getAnyVarRefFromVar(Var v);
AnyVarRef getAnyVarRefFromString(const CSPInstance& c, const std::string& s);

std::pair<DomainInt, DomainInt> getInitialBoundsFromVar(Var v);

/// Helper function used in a few places.
vector<AnyVarRef> getAnyVarRefFromVar(const vector<Var>& v);

vector<vector<AnyVarRef>> getAnyVarRefFromVar(const vector<vector<Var>>& v);

/// Create all the variables used in the CSP.
void buildVariables(const ProbSpec::VarContainer& vars);
} // namespace BuildCon

#endif
