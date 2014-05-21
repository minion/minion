/** \weakgroup MinLib
 * @{
 */
#ifndef TINY_TEMPLATE_DCDSCQ
#define TINY_TEMPLATE_DCDSCQ

#include <ostream>

/// A placeholder type.
struct EmptyType
{};

struct AnyGrab
{ 
    template<typename T>
    AnyGrab(const T&);
};

/// A constant chosen at compile time.
/// Create with the notation compiletime_val<6>().
template<int i>
struct compiletime_val
{ 
  operator int() const
{ return i; }
  
  compiletime_val<-i-1> negminusone() const
{ return compiletime_val<-i-1>(); }
  
  friend std::ostream& operator<<(std::ostream& o, const compiletime_val&)
{ return o << "CompiletimeConst:" << i; }

  compiletime_val<-i> operator-() const
{ return compiletime_val<-i>(); }
};

template<int i, int j>
compiletime_val<i+j> operator+(compiletime_val<i>, compiletime_val<j>)
{ return compiletime_val<i+j>(); }

template<int i, int j>
compiletime_val<i-j> operator-(compiletime_val<i>, compiletime_val<j>)
{ return compiletime_val<i-j>(); }

/** @}
 */

#endif