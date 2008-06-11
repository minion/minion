// This file contains a number of simple helpers for making c++0x and c++03 code
// work together.

#ifdef __GXX_EXPERIMENTAL_CXX0X__
#define USE_CXX0X
#define CXXMOVE(member, var) member(std::move(var.member))
#define MOVE(x) std::move(x)
#else
#define MOVE(x) x
#endif

