/*
* Minion http://minion.sourceforge.net
* Copyright (C) 2006-09
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

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
  
  static void overflow_check(long double d)
  { 
    if(d >= (long double)std::numeric_limits<T>::max()/1.2 ||
       d <= (long double)std::numeric_limits<T>::min()/1.2)
      throw std::runtime_error("Numeric over/underflow");
  }
  Wrapper& operator+=(const Wrapper& w)
  {
    overflow_check((long double)t + (long double)w.t); 
    t += w.t; 
    return *this;
  }

  Wrapper& operator*=(const Wrapper& w)
  { 
    overflow_check((long double)t * (long double)w.t);
    t *= w.t; 
    return *this;
  }

  Wrapper& operator-=(const Wrapper& w)
  { 
    overflow_check((long double)t - (long double)w.t);
    t -= w.t; 
    return *this;
  }

  Wrapper& operator/=(const Wrapper& w)
  { 
    overflow_check((long double)t / (long double)w.t);
    t /= w.t; 
    return *this;
  }
  
  Wrapper operator-() const
  {
    return -t;
  }
  
  void operator++()
  { 
    overflow_check((long double)(t + 1));
    t++; 
  }
  
  void operator--()
  {
    overflow_check((long double)(t + 1));
    t--;
  }

  Wrapper operator++(int)
  {
    Wrapper t(*this);
    ++(*this);
    return t;
  }

  Wrapper operator--(int)
  {
    Wrapper t(*this);
    --(*this);
    return t;
  }

  
  friend std::ostream& operator<<(std::ostream& o, Wrapper v)
  { return o << v.t; }

  friend std::ostream& operator>>(std::ostream& o, Wrapper v)
  { return o >> v.t; }
  
};

#ifdef WRAP_BOOL_OPS
#undef WRAP_BOOL_OPS
#endif

#define WRAP_BOOL_OPS(op) \
template<typename T> \
bool operator op (const Wrapper<T>& t1, const Wrapper<T>& t2) \
{ return t1.t op t2.t; } \
\
template<typename T, typename U> \
bool operator op (const U& t1, const Wrapper<T>& t2) \
{ return t1 op t2.t; } \
\
template<typename T, typename U> \
bool operator op (const Wrapper<T>& t1, const U& t2) \
{ return t1.t op t2; } \

WRAP_BOOL_OPS(==)
WRAP_BOOL_OPS(!=)
WRAP_BOOL_OPS(<)
WRAP_BOOL_OPS(>)
WRAP_BOOL_OPS(<=)
WRAP_BOOL_OPS(>=)

#ifdef WRAP_ARITHMETIC_OPS
#undef WRAP_ARITHMETIC_OPS
#endif

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
