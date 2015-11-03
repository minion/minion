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

#ifndef _SYS_CONSTANTS_H
#define _SYS_CONSTANTS_H

//#define DOMAINS64

#ifdef DOMAINS64

// work out if we need to use the Wrapper<int64_t>
#ifdef _WIN32
// Windows does not have a 128-bit integer
#define USE_WRAPPER
#endif
#ifdef __EMSCRIPTEN__
// Javascript does not either
#define USE_WRAPPER
#endif

#ifdef USE_WRAPPER
typedef Wrapper<int64_t> BigInt;
#else
typedef __int128_t BigInt;
#endif
typedef int64_t SysInt;
typedef uint64_t UnsignedSysInt;
#else
typedef int64_t BigInt;
typedef int32_t SysInt;
typedef unsigned int UnsignedSysInt;
#endif

#ifdef MINION_DEBUG
typedef Wrapper<SysInt> DomainInt;
#else
typedef SysInt DomainInt;
#endif

// Put a ' -1, +1 ' just to have some slack
const DomainInt DomainInt_Max = std::numeric_limits<SysInt>::max() / 2 - 1;
const DomainInt DomainInt_Min = std::numeric_limits<SysInt>::min() / 2 + 1;

/// A big constant, when such a thing is needed.
static const DomainInt DomainInt_Skip = std::numeric_limits<SysInt>::max();

template <typename To, typename From>
To checked_cast(const From& t) {
  return static_cast<To>(t);
}

template <typename To, typename From>
To checked_cast(const Wrapper<From>& t) {
  return static_cast<To>(t.t);
}

template <typename T>
T const_negminusone(T t) {
  return -1 - t;
}

template <typename T, T i>
inline compiletime_val<T, -1 - i> const_negminusone(compiletime_val<T, i>) {
  return compiletime_val<T, -1 - i>();
}

template <typename T>
T const_neg(T t) {
  return -t;
}

template <typename T, T i>
inline compiletime_val<T, (T)0 - i> const_neg(compiletime_val<T, i>) {
  return compiletime_val<T, (T)0 - i>();
}

template <typename T>
inline T mymin(T t1, T t2) {
  if(t1 <= t2)
    return t1;
  else
    return t2;
}

template <typename T>
inline T mymax(T t1, T t2) {
  if(t1 <= t2)
    return t2;
  else
    return t1;
}

enum MapLongTuplesToShort { MLTTS_NoMap, MLTTS_KeepLong, MLTTS_Eager, MLTTS_Lazy };

#endif // _SYS_CONSTANTS_H
