// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef INPUTFILE_PARSE_H
#define INPUTFILE_PARSE_H

#include "CSPSpec.h"

void readInputFromFiles(CSPInstance& inst, vector<string> fnames, bool parserVerbose,
                        MapLongTuplesToShort mltts, bool ensureBranchOnAllVars);

#endif
