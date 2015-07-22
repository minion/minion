#ifndef VARIADIC_MACROS_HPP
#define VARIADIC_MACROS_HPP

/* This counts the number of args */
#define MINLIB_NARGS_SEQ(_1, _2, _3, _4, _5, _6, _7, _8, N, ...) N
#define MINLIB_NARGS(...) MINLIB_NARGS_SEQ(__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1)

/* This will let macros expand before concating them */
#define MINLIB_PRIMITIVE_CAT(x, y) x##y
#define MINLIB_CAT(x, y) MINLIB_PRIMITIVE_CAT(x, y)

/* This will call a macro on each argument passed in */
#define MINLIB_APPLY(macro, sep, ...)                                                              \
  MINLIB_CAT(MINLIB_APPLY_, MINLIB_NARGS(__VA_ARGS__))(macro, sep, __VA_ARGS__)
#define MINLIB_APPLY_1(m, sep, x1) m(x1)
#define MINLIB_APPLY_2(m, sep, x1, x2) m(x1) sep() m(x2)
#define MINLIB_APPLY_3(m, sep, x1, x2, x3) m(x1) sep() m(x2) sep() m(x3)
#define MINLIB_APPLY_4(m, sep, x1, x2, x3, x4) m(x1) sep() m(x2) sep() m(x3) sep() m(x4)
#define MINLIB_APPLY_5(m, sep, x1, x2, x3, x4, x5)                                                 \
  m(x1) sep() m(x2) sep() m(x3) sep() m(x4) sep() m(x5)
#define MINLIB_APPLY_6(m, sep, x1, x2, x3, x4, x5, x6)                                             \
  m(x1) sep() m(x2) sep() m(x3) sep() m(x4) sep() m(x5) sep() m(x6)
#define MINLIB_APPLY_7(m, sep, x1, x2, x3, x4, x5, x6, x7)                                         \
  m(x1) sep() m(x2) sep() m(x3) sep() m(x4) sep() m(x5) sep() m(x6) sep() m(x7)
#define MINLIB_APPLY_8(m, sep, x1, x2, x3, x4, x5, x6, x7, x8)                                     \
  m(x1) sep() m(x2) sep() m(x3) sep() m(x4) sep() m(x5) sep() m(x6) sep() m(x7) sep() m(x8)

#define MINLIB_COMMA() ,
#define MINLIB_SEMICOLON() ;
#define MINLIB_EMPTY()
#endif
