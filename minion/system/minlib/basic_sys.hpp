#ifndef BASIC_SYS_CQP
#define BASIC_SYS_CQP

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include <algorithm>
#include <array>
#include <exception>
#include <fstream>
#include <functional>
#include <iostream>
#include <istream>
#include <list>
#include <map>
#include <memory>
#include <ostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <assert.h>
#include <limits>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SHARED_PTR std::shared_ptr
#define TUPLE std::tuple
#define TUPLE_GET std::get
#define TUPLE_TIE std::tie

/// Stores typedefs for containers.
/// The reason we do this is to templates in templates in
/// the containers, which is horrible
template <typename... Class>
struct TypeDefs;

#define DOM_NORETURN __attribute__((noreturn))
#define DOM_NOINLINE __attribute__((noinline))

#endif
