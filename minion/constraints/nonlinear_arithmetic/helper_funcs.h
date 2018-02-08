#ifndef NONLINEAR_HELPER_H
#define NONLINEAR_HELPER_H

inline DomainInt round_up_div(DomainInt x, DomainInt y) {
    if(y == 0)
        return 0;
    DomainInt ret = x / y;
    if(x % y != 0)
        ret++;
    return ret;
}

inline DomainInt round_down_div(DomainInt x, DomainInt y) {
    if(y == 0)
        return DomainInt_Max;
    return x / y;
}

#endif
