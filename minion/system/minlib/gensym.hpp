/** \weakgroup MinLib
 * @{
 */

#ifndef MINLIB_GENSYM_H
#define MINLIB_GENSYM_H

#include "tostring.hpp"

/// Generate a unique integer
inline int gensym_int() {
  static int i = 1;
  return i++;
}

/// Generate a unique string, with optional prefix
inline std::string gensym(std::string prefix = std::string("X")) {
  return prefix + tostring(gensym_int());
}

#endif

/** @}
 */
