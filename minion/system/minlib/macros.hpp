/** \weakgroup MinLib
 * @{
 */

#ifndef MACRO_INCLUDE_IJUODQWHK
#define MACRO_INCLUDE_IJUODQWHK

#include <iostream>
#include <ostream>

#include "variadic_macros.hpp"

// These two defines just work around a bug in some compilers.
#define MERGE(x, y) MERGE2(x, y)
#define MERGE2(x, y) x##y

#define DEFINE_DEFAULT_CONSTRUCTORS_AND_OPERATOR_EQ(X)                                             \
  X() = default;                                                                                   \
  X(const X&) = default;                                                                           \
  X(X&&) = default;                                                                                \
  X& operator=(const X&) = default;                                                                \
  X& operator=(X&&) = default;

#define DEFINE_DEFAULT_COPY_MOVE_AND_OPERATOR_EQ(X)                                                \
  X(const X&) = default;                                                                           \
  X(X&&) = default;                                                                                \
  X& operator=(const X&) = default;                                                                \
  X& operator=(X&&) = default;

#define STRINGIFY(x) #x
#define FUNCTION_AND_LINE(a, b) std::string(a) + ":" + std::string(STRINGIFY(b))

#define USER_ERROR(x)                                                                              \
  {                                                                                                \
    std::cerr << "Error in input: " << x << std::endl;                                             \
    exit(1);                                                                                       \
  }

#define INTERNAL_ERROR(x)                                                                          \
  {                                                                                                \
    std::cerr << "Fatal Internal Error:" << x << " at "                                            \
              << FUNCTION_AND_LINE(__FUNCTION__, __LINE__) << std::endl;                           \
    abort();                                                                                       \
  }

#define D_CATCH_FATAL_ERROR(x)                                                                     \
  catch(std::exception & r) {                                                                      \
    std::cerr << r.what() << std::endl;                                                            \
    D_FATAL_ERROR(x);                                                                              \
  }                                                                                                \
  catch(...) {                                                                                     \
    std::cerr << "Unknown error\n";                                                                \
    D_FATAL_ERROR(x);                                                                              \
  }

#define D_CHECK(x)                                                                                 \
  {                                                                                                \
    if(!(x)) {                                                                                     \
      std::cerr << "Assert Failure:" << STRINGIFY(x) << " at "                                     \
                << FUNCTION_AND_LINE(__FUNCTION__, __LINE__) << std::endl;                         \
      abort();                                                                                     \
    }                                                                                              \
  }

#define D_CHECK_EXPLAIN(x, y)                                                                      \
  {                                                                                                \
    if(!(x)) {                                                                                     \
      std::cerr << "Assert Failure:" << STRINGIFY(x) << " at "                                     \
                << FUNCTION_AND_LINE(__FUNCTION__, __LINE__) << std::endl                          \
                << y << std::endl;                                                                 \
      abort();                                                                                     \
    }                                                                                              \
  }

#ifdef DOM_ASSERT

#define D_ASSERT(x) D_CHECK(x)
#define D_ASSERT_EXPLAIN(x, y) D_CHECK_EXPLAIN(x, y)

#define D_DATA(x) x

#define D_THROWS(x)                                                                                \
  {                                                                                                \
    bool throw_b = false;                                                                          \
    try {                                                                                          \
      x;                                                                                           \
    } catch(...) { throw_b = true; }                                                               \
    if(!throw_b) {                                                                                 \
      std::cerr << "Failure to throw:" << STRINGIFY(x) << " at "                                   \
                << FUNCTION_AND_LINE(__FUNCTION__, __LINE__) << std::endl;                         \
      abort();                                                                                     \
    }                                                                                              \
  }

#else

#define D_ASSERT(x)
#define D_DATA(x)
#define D_THROWS(x)
#endif

#ifdef DOM_PRINTINFO
#define D_INFO(x, y, z)                                                                            \
  { std::cerr << #x << ":" << this << ":" << #y << ":" << z << std::endl; }
#else
#define D_INFO(x, y, z)
#endif

#endif

/** @}
 */
