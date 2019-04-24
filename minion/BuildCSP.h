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

#ifndef _BUILDCSP_H
#define _BUILDCSP_H

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#endif

struct SearchMethod {
  VarOrderEnum order;
  ValOrder valorder;
  unsigned int limit; // for static limited.
  PropagationLevel preprocess;
  PropagationLevel propMethod;
  UnsignedSysInt randomSeed;
  SearchMethod()
      : order(ORDER_NONE),
        valorder(VALORDER_NONE),
        preprocess(PropLevel_None),
        propMethod(PropLevel_GAC),
        randomSeed(std::random_device{}()) {}
};

void SetupCSPOrdering(CSPInstance& instance, SearchMethod args);
void BuildCSP(ProbSpec::CSPInstance& instance);
void PreprocessCSP(ProbSpec::CSPInstance& instance, SearchMethod args);
void SolveCSP(ProbSpec::CSPInstance& instance, SearchMethod args);

#endif
