#ifndef _WRAPPER_H
#define _WRAPPER_H

#include <ostream>

template<typename T>
struct Wrapper
{ 
  T t; 
  
  Wrapper(const T& _t) : t(_t)
  { }
  
  Wrapper() : t()
  {}
  
  Wrapper& operator+=(const Wrapper& w)
  { 
    t += w.t; 
    return *this;
  }

  Wrapper& operator*=(const Wrapper& w)
  { 
    t += w.t; 
    return *this;
  }

  Wrapper& operator-=(const Wrapper& w)
  { 
    t += w.t; 
    return *this;
  }

  Wrapper& operator/=(const Wrapper& w)
  { 
    t += w.t; 
    return *this;
  }
  
  Wrapper& operator-()
  {
	t = -t;
	return *this;
  }
  
  void operator++()
  { t++; }
  
  void operator--()
  { t--; }
  
  friend std::ostream& operator<<(std::ostream& o, Wrapper v)
  { return o << v.t; }
  
};

#define WRAP_BOOL_OPS(op) \
template<typename T> \
bool operator op (const Wrapper<T>& t1, const Wrapper<T>& t2) \
{ return t1.t op t2.t; } \
\
template<typename T, typename U> \
bool operator op (const U& t1, const Wrapper<T>& t2) \
{ return Wrapper<T>(t1) op t2.t; } \
\
template<typename T, typename U> \
bool operator op (const Wrapper<T>& t1, const U& t2) \
{ return t1.t op Wrapper<T>(t2); } \

WRAP_BOOL_OPS(==)
WRAP_BOOL_OPS(!=)
WRAP_BOOL_OPS(<)
WRAP_BOOL_OPS(>)
WRAP_BOOL_OPS(<=)
WRAP_BOOL_OPS(>=)

#define WRAP_ARITHMETIC_OPS(op) \
template<typename T> \
Wrapper<T> operator op (const Wrapper<T>& t1, const Wrapper<T>& t2) \
{ return t1.t op t2.t; } \
\
template<typename T, typename U> \
Wrapper<T> operator op(const U& t1, const Wrapper<T>& t2) \
{ return t1 op t2.t; } \
\
template<typename T, typename U> \
Wrapper<T> operator op(const Wrapper<T>& t1, const U& t2) \
{ return t1.t op t2; } \

WRAP_ARITHMETIC_OPS(+)
WRAP_ARITHMETIC_OPS(-)
WRAP_ARITHMETIC_OPS(*)
WRAP_ARITHMETIC_OPS(/)
WRAP_ARITHMETIC_OPS(%)

template<typename T>
Wrapper<T> abs(const Wrapper<T>& in)
{ return Wrapper<T>(abs(in.t)); }

#endif // _WRAPPER_H
