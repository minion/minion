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

#ifndef _BUILDCSP_H
#define _BUILDCSP_H

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#endif

struct SearchMethod
{
  VarOrderEnum order;
  enum PropagationLevel preprocess;
  enum PropagationLevel prop_method;
  unsigned random_seed;
  SearchMethod() : order(ORDER_NONE), preprocess(PropLevel_None), prop_method(PropLevel_GAC), random_seed((unsigned)time(NULL) ^ getpid())
  { }
  
};

void BuildCSP(StateObj* stateObj, ProbSpec::CSPInstance& instance);
void SolveCSP(StateObj* stateObj, ProbSpec::CSPInstance& instance, SearchMethod args);

#endif
