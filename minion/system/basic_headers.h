#ifndef MINION_BASIC_HEADER_H
#define MINION_BASIC_HEADER_H

// On linux systems, _longjmp and _setjmp are faster versions that ignore
// system signals, which Minion doesn't use.
#ifdef __GNUC__
#define SYSTEM_LONGJMP _longjmp
#define SYSTEM_SETJMP _setjmp
#else
#define SYSTEM_LONGJMP longjmp
#define SYSTEM_SETJMP setjmp
#endif

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

// Stupid visual C++ needs a little hacking
#ifdef _MSC_VER
// We don't want no stupid safe library warnings
#define _SCL_SECURE_NO_DEPRECATE
#define DEFAULT_CALL __std_call
#pragma warning(disable : 4715)
// Supress 'size_t -> SysInt' warnings.
#pragma warning(disable : 4267)
// I don't even get this warning.
#pragma warning(disable : 4244)
// I'll buy a pint for anyone who can figure how to fix this..
// 'unsigned long' : forcing value to BOOL 'true' or 'false'. Of course I am,
// that's what I want to test!
#pragma warning(disable : 4800)
// At some point I might fix these "signed/UnsignedSysInt mismatch" warnings...
#pragma warning(disable : 4018)
// Why can't you realise that abort() means the function doesn't have to return?
#pragma warning(disable : 4716)
// Another annoying warning. I'm not sure why Microsoft want to warn about this,
// it's perfectly common
#pragma warning(disable : 4355)
#else
#define DEFAULT_CALL
#endif // _MSC_VER

#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <numeric>
#include <ostream>
#include <set>
#include <sstream>
#include <string>
#include <time.h>
#include <vector>

#include <assert.h>
#include <memory.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

#include <array>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#define MAP_TYPE std::unordered_map
#define SET_TYPE std::unordered_set
#define INPUT_MAP_TYPE std::map
// TODO: Add hashes

using namespace std;

/*
 * When using as a library, store globals on heap.
 * This allows them to be easily reinitialised for multiple runs.
 *
 * This globals struct is initialised in globals.cpp.
 *
 * For simplicity, it contains set fields that should be hardcoded in.
 */

#ifndef IN_MAIN

#ifdef LIBMINION
#define VARDEF_ASSIGN(x, y) extern Globals* globals
#define VARDEF(x) extern Globals* globals
#else
#define VARDEF_ASSIGN(x, y) extern x
#define VARDEF(x) extern x
#endif

#else /* IN MAIN */

#ifdef LIBMINION
#define VARDEF_ASSIGN(x, y)
#define VARDEF(x)
#else 
#define VARDEF_ASSIGN(x, y) x(y)
#define VARDEF(x) x
#endif
#endif

#define BOOL bool

#ifdef LIBMINION
#define GET_GLOBAL(x) (::globals->x)
#else
#define GET_GLOBAL(x) x
#endif
#endif
