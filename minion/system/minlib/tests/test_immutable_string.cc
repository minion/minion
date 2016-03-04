#include "minlib/immutable_string.hpp"
#include "minlib/minlib.hpp"

int main(void) {
  ImmutableString s("abc");
  D_ASSERT(s == s);
  D_ASSERT(!(s != s));

  ImmutableString t("abc");
  D_ASSERT(s == t);

  s = ImmutableString("ab");
  D_ASSERT(s != t);
  ImmutableString s2 = s;
  D_ASSERT(s2 == s);
  D_ASSERT(s2 != t);
  s2 = t;
  D_ASSERT(s2 == t);
  D_ASSERT(s2 != s);
}