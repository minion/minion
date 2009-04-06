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


#include "../inputfile_parse/InputVariableDef.h"


#include "BuildConstraintConstructs.h"

using namespace BuildCon;

// These two defines just work around a bug in some compilers.
#define MERGE(x,y) MERGE2(x,y)
#define MERGE2(x,y) x ## y

#define TERMINATE_BUILDCON3(CT_NAME) \
namespace BuildCon \
{ \
template<> \
struct BuildConObj<CT_NAME, 0> \
{ \
  template<typename T1, typename T2, typename T3> \
  static  \
  AbstractConstraint* build(StateObj* stateObj, const pair<pair<pair<EmptyType, vector<T1>* >, vector<T2>* >, vector<T3>*>& vars, ConstraintBlob& b, int) \
  { \
      return Build ## CT_NAME(stateObj, *(vars.first.first.second), *(vars.first.second), *(vars.second), b); \
  } \
}; \
} \

#define TERMINATE_BUILDCON2(CT_NAME) \
namespace BuildCon \
{ \
template<> \
struct BuildConObj<CT_NAME, 0> \
{ \
  template<typename T1, typename T2> \
  static  \
  AbstractConstraint* build(StateObj* stateObj, const pair<pair<EmptyType, vector<T1>* >, vector<T2>* >& vars, ConstraintBlob& b, int) \
  { \
      return Build ## CT_NAME(stateObj, *(vars.first.second), *(vars.second), b); \
  } \
}; \
} \

#define TERMINATE_BUILDCON1(CT_NAME) \
namespace BuildCon \
{ \
template<> \
struct BuildConObj<CT_NAME, 0> \
{ \
  template<typename T1> \
  static  \
  AbstractConstraint* build(StateObj* stateObj, const pair<EmptyType, vector<T1>* >& vars, ConstraintBlob& b, int) \
  { \
      return Build ## CT_NAME(stateObj, *(vars.second), b); \
  } \
}; \
} \

#define TERMINATE_BUILDCON0(CT_NAME) \
namespace BuildCon \
{ \
template<> \
struct BuildConObj<CT_NAME, 0> \
{ \
  static  \
  AbstractConstraint* build(StateObj* stateObj, const EmptyType& vars, ConstraintBlob& b, int) \
  { \
      return Build ## CT_NAME(stateObj, b); \
  } \
}; \
} \

#define BUILD_CT(CT_NAME,COUNT) \
MERGE(TERMINATE_BUILDCON, COUNT)(CT_NAME) \
AbstractConstraint* \
build_constraint_ ## CT_NAME(StateObj* stateObj, ConstraintBlob& b) \
{ return BuildConObj<CT_NAME, COUNT>::build(stateObj, EmptyType(), b, 0); }
