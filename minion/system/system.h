/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id$
*/

/* Minion
* Copyright (C) 2006
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

// This file deals with general C++ things which aren't specific to Minion.

#ifndef _MINION_SYSTEM_H
#define _MINION_SYSTEM_H

// On linux systems, _longjmp and _setjmp are faster versions that ignore
// system signals, which Minion doesn't use.
#ifdef __GNUC__
#define SYSTEM_LONGJMP _longjmp
#define SYSTEM_SETJMP  _setjmp

#define _NORETURN __attribute__ ((noreturn))
#define _NOINLINE __attribute__((noinline))
#else
#define SYSTEM_LONGJMP longjmp
#define SYSTEM_SETJMP  setjmp

#define _NORETURN
#define _NOINLINE
#endif

// Stupid visual C++ needs a little hacking
#ifdef _MSC_VER
// We don't want no stupid safe library warnings
#define _SCL_SECURE_NO_DEPRECATE
#define DEFAULT_CALL __std_call
// Supress 'size_t -> int' warnings.
#pragma warning(disable: 4267)
// I don't even get this warning.
#pragma warning(disable: 4244)
// I'll buy a pint for anyone who can figure how to fix this..
// 'unsigned long' : forcing value to BOOL 'true' or 'false'. Of course I am, that's what I want to test!
#pragma warning(disable: 4800)
// At some point I might fix these "signed/unsigned mismatch" warnings...
#pragma warning(disable: 4018)
// Why can't you realise that abort() means the function doesn't have to return?
#pragma warning(disable: 4716)
// Another annoying warning. I'm not sure why Microsoft want to warn about this, it's perfectly common
#pragma warning(disable: 4355)
#else
#define DEFAULT_CALL
#endif // _MSC_VER

#include <time.h>
#include <vector>
#include <set>
#include <limits>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <numeric>
#include <ostream>
#include <sstream>
#include <string>

#include<stdio.h>
#include<stdlib.h>
#include<memory.h>
#include<setjmp.h>
#include<sys/types.h>
#include<unistd.h>

#include "cxx0x-helper.h"

#ifdef USE_BOOST
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>

#define MAP_TYPE boost::unordered_map
#define SET_TYPE boost::unordered_set

#else

#ifdef USE_TR1_HASH_MAP_AND_SET
#include <tr1/unordered_set>
#include <tr1/unordered_map>

#define MAP_TYPE std::tr1::unordered_map
#define SET_TYPE std::tr1::unordered_set

#else

#include <map>
#define MAP_TYPE map
#include <set>
#define SET_TYPE set
#endif
#endif

using namespace std;


#ifndef IN_MAIN
#define VARDEF_ASSIGN(x,y) extern x
#define VARDEF(x) extern x
#else
#define VARDEF_ASSIGN(x,y) x(y)
#define VARDEF(x) x
#endif

#define BOOL bool

#ifdef USE_BOOST
#include <boost/shared_ptr.hpp>
using boost::shared_ptr;
#else
#include "linked_ptr.h"
#endif

#include "wrapper.h"
#include "to_string.h"
#include "local_array.h"
#include "tableout.h"
#include "time_keeping.h"

#include "sys_constants.h"
#include "debug.h"
#include "array_functions.h"
#include "trigger_timer.h"
#include "box.h"
//#include "light_vector.h"

#endif
