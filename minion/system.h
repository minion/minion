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

// Only GCC has hashtables
#ifdef __GNUC__
#define USE_HASHTABLE
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
#include<setjmp.h>

#include "system/linked_ptr.h"
#include "system/local_array.h"


// Note: The hash table (unordered_map) is broken in many versions of g++,
// so this can't be activated safely.
#ifdef USE_HASHTABLE
#include <ext/hash_map>
namespace __gnu_cxx
{
template<typename T>
    struct hash<T*>
    {
      size_t
      operator()(T* __x) const
      { return (size_t)__x; }
    };
}
#define MAP_TYPE __gnu_cxx::hash_map
#else
#include <map>
#define MAP_TYPE map
#endif

using namespace std;


#ifdef NO_MAIN
#define VARDEF_ASSIGN(x,y) extern x
#define VARDEF(x) extern x
#else
#define VARDEF_ASSIGN(x,y) x = y
#define VARDEF(x) x
#endif

#define BOOL int