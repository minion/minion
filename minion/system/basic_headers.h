#ifndef MINION_BASIC_HEADER_H
#define MINION_BASIC_HEADER_H

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

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

// Stupid visual C++ needs a little hacking
#ifdef _MSC_VER
// We don't want no stupid safe library warnings
#define _SCL_SECURE_NO_DEPRECATE
#define DEFAULT_CALL __std_call
#pragma warning(disable: 4715)
// Supress 'size_t -> SysInt' warnings.
#pragma warning(disable: 4267)
// I don't even get this warning.
#pragma warning(disable: 4244)
// I'll buy a pint for anyone who can figure how to fix this..
// 'unsigned long' : forcing value to BOOL 'true' or 'false'. Of course I am, that's what I want to test!
#pragma warning(disable: 4800)
// At some point I might fix these "signed/UnsignedSysInt mismatch" warnings...
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <setjmp.h>


#include <tuple>
#include <utility>
#include <unordered_set>
#include <unordered_map>
#include <array>
#include <memory>

#define MAP_TYPE std::unordered_map
#define SET_TYPE std::unordered_set
#define INPUT_MAP_TYPE std::map
// TODO: Add hashes

using namespace std;

#ifndef IN_MAIN
#define VARDEF_ASSIGN(x,y) extern x
#define VARDEF(x) extern x
#else
#define VARDEF_ASSIGN(x,y) x(y)
#define VARDEF(x) x
#endif

#define BOOL bool

#endif
