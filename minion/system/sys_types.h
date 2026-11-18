// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef MINION_SYS_TYPES_H
#define MINION_SYS_TYPES_H

#include <stdint.h>

template <typename T>
struct Wrapper;

#ifdef DOMAINS64

typedef Wrapper<int64_t> BigInt;
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

#endif
