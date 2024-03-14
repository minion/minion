// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

// This file deals with general C++ things which aren't specific to Minion.

#ifndef _MINION_SYSTEM_H
#define _MINION_SYSTEM_H

#ifdef MINION_DEBUG
#define DOM_ASSERT
#endif

#include "globals_forward.h"

#include "minlib/minlib.hpp"

#include "box-helper.h"

#include "basic_headers.h"

#include "./wrapper.h"

#include "sys_constants.h"

#include "debug.h"
#include "minlib/tostring.hpp"
#include "tableout.h"
#include "time_keeping.h"

#include "array_functions.h"
#include "trigger_timer.h"

#include "test_functions.h"

#include "minlib/optional.hpp"

#include <sys/types.h>
#ifndef _WIN32
#include <unistd.h>
#endif

// from sha1.cpp
std::string sha1_hash(const std::string& s);

inline void* checked_zeroed_malloc(size_t size) {
  if(size == 0)
    return 0;
  void* ptr = calloc(size, 1);
  if(ptr == 0) {
    std::cerr << "Fatal: Out of memory";
    abort();
  }
  return ptr;
}

inline void* checked_malloc(size_t size) {
  return checked_zeroed_malloc(size);
}

inline void* checked_realloc(void* ptr, size_t size) {
  void* new_ptr = realloc(ptr, size);
  if(new_ptr == 0) {
    std::cerr << "Fatal: Out of memory";
    abort();
  }
  return new_ptr;
}

#include <random>

VARDEF(std::mt19937 global_random_gen);

#endif
