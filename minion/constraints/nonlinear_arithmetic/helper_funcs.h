#ifndef NONLINEAR_HELPER_H
#define NONLINEAR_HELPER_H

inline DomainInt roundUpDiv(DomainInt x, DomainInt y) {
  if(y == 0)
    return 0;
  DomainInt ret = x / y;
  if(x % y != 0)
    ret++;
  return ret;
}

inline DomainInt round_downDiv(DomainInt x, DomainInt y) {
  if(y == 0)
    return DomainInt_Max;
  return x / y;
}

// This function does i/j with Minion semantics.
// It will assert if undef==false and j==0
template <bool undef>
DomainInt doDiv(DomainInt i, DomainInt j) {
  if(j == 0) {
    D_ASSERT(undef);
    return 0;
  }

  bool negsign = (i < 0 || j < 0) && (i > 0 || j > 0);
  DomainInt r = i / j;
  if(negsign && r * j != i)
    r--;
  return r;
}

// This function exists for two reasons:
// 1) Make sure we never divide by zero
// 2) Abstract checking div_undefzero
template <bool undef>
bool checkDivResult(DomainInt i, DomainInt j, DomainInt k) {
  if(j == 0) {
    return (undef && k == 0);
  }

  return doDiv<undef>(i, j) == k;
}

#endif
