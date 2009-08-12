/** box
 *  box is a general wrapper around a block of memory, turning into an (almost) valid C++ container.
 *  One useful feature of 'box' is that it can be used to wrap a block of 'alloca'ed memory on the stack, allowing
 *  for a variable-sized C++ container withou stack allocation.
 *
 *
 *  This code is derived from the original HP STL, with a number of bug-fixes and additions from libstdc++.
 */
 
 /*
  *
  * Copyright (c) 1994
  * Hewlett-Packard Company
  *
  * Permission to use, copy, modify, distribute and sell this software
  * and its documentation for any purpose is hereby granted without fee,
  * provided that the above copyright notice appear in all copies and
  * that both that copyright notice and this permission notice appear
  * in supporting documentation.  Hewlett-Packard Company makes no
  * representations about the suitability of this software for any
  * purpose.  It is provided "as is" without express or implied warranty.
  *
  *
  * Copyright (c) 1996
  * Silicon Graphics Computer Systems, Inc.
  *
  * Permission to use, copy, modify, distribute and sell this software
  * and its documentation for any purpose is hereby granted without fee,
  * provided that the above copyright notice appear in all copies and
  * that both that copyright notice and this permission notice appear
  * in supporting documentation.  Silicon Graphics makes no
  * representations about the suitability of this  software for any
  * purpose.  It is provided "as is" without express or implied warranty.
  */
 

#include <algorithm>

#ifdef _WIN32
#include <malloc.h>
#define alloc _alloca
#else
#include <alloca.h>
#endif

#include <boost/type_traits.hpp>

using namespace std;

  template<typename T>
class box 
{
  T*           M_start;
  T*           M_finish;
  T*           M_end_of_storage;

  void construct(T* pos, const T& place)
    { *pos = place; }

  void destroy(T* pos)
    { }

  void destroy(T* begin, T* end)
    { }

public:
  typedef T                  value_type;
  typedef T*                   pointer;
  typedef const T*             const_pointer;
  typedef T&                 reference;
  typedef const T&           const_reference;
  typedef pointer iterator;
  typedef const_pointer const_iterator;
  typedef std::reverse_iterator<const_iterator>  const_reverse_iterator;
  typedef std::reverse_iterator<iterator>        reverse_iterator;
  typedef size_t                     size_type;
  typedef ptrdiff_t                  difference_type;
  typedef void       allocator_type;

public:

  enum IsFull { StartEmpty, StartFull};
  
  box(T* start, T* end, IsFull isFull = StartEmpty) : M_start(start),  M_end_of_storage(end)
  {
    if(isFull == StartEmpty)
      M_finish = start;
    else
      M_finish = end;
//  std::uninitialized_fill_n_a(M_start, n, value,
//                    this->get_allocator());
  }

    box(T* start, size_t size, IsFull isFull = StartEmpty) : M_start(start),  M_end_of_storage(start + size)
    {
      if(isFull == StartEmpty)
        M_finish = start;
      else
        M_finish = start + size;
  //    std::uninitialized_fill_n_a(M_start, n, value,
  //                      this->get_allocator());
    }
  
//  box(const box& box) : M_start(box.M_start), M_finish(box.M_start), 
//    M_end_of_storage(box.M_end_of_storage)
//    { }

  ~box()
    { }

//  box&
//    operator=(const box& x);

  void
    assign(size_type n, const value_type& val)
    { M_fill_assign(n, val); }

      template<typename _InputIterator>
  void
    assign(_InputIterator first, _InputIterator last)
  {
    // Check whether it's an integral type.  If so, it's not an iterator.
    typedef typename boost::is_integral<_InputIterator>::__type _Integral;
    M_assign_dispatch(first, last, _Integral());
  }

  iterator
    begin()
    { return M_start; }

  const_iterator
    begin() const
    { return M_start; }

  iterator
    end()
    { return M_finish; }

  const_iterator
    end() const
    { return M_finish; }

  reverse_iterator
    rbegin()
    { return reverse_iterator(end()); }

  const_reverse_iterator
    rbegin() const
    { return const_reverse_iterator(end()); }

  reverse_iterator
    rend()
    { return reverse_iterator(begin()); }

  const_reverse_iterator
    rend() const
    { return const_reverse_iterator(begin()); }

  size_type
    size() const
    { return size_type(end() - begin()); }

  size_type
    max_size() const
    { return M_end_of_storage - M_start; }

  void
    resize(size_type new_size, const value_type& x)
  {
    if (new_size < size())
      erase(begin() + new_size, end());
    else
      insert(end(), new_size - size(), x);
  }

  void
    resize(size_type new_size)
    { resize(new_size, value_type()); }

  size_type
    capacity() const
    { return M_end_of_storage - M_start; }

  bool
    empty() const
    { return M_finish == M_start; }

  // This function is basically ignored in a box
  void
    reserve(size_type n)
    { 
      if (n > capacity())
      throw string("Out of space!"); 
    }

  reference
    operator[](size_type n)
    { return *(begin() + n); }

  const_reference
    operator[](size_type n) const
    { return *(begin() + n); }

  reference
    at(size_type n)
  {
    if(n > size())
      throw std::string("Out of range exception");
    return (*this)[n]; 
  }

  const_reference
    at(size_type n) const
  {
    if(n > size())
      throw std::string("Out of range exception");
    return (*this)[n];
  }

  reference
    front()
    { return *begin(); }

  const_reference
    front() const
    { return *begin(); }

  reference
    back()
    { return *(end() - 1); }

  const_reference
    back() const
    { return *(end() - 1); }

  void
    push_back(const value_type& x)
  {
    if(M_finish == M_end_of_storage)
      throw std::string("Out of space!");
    construct(M_finish, x);
    M_finish++;
  }

  void
    pop_back()
  {
    --M_finish;
    destroy(M_finish);
  }

  iterator
    insert(iterator position, const value_type& x);

  void
    insert(iterator position, size_type n, const value_type& x)
    { M_fill_insert(position, n, x); }

      template<typename _InputIterator>
  void
    insert(iterator position, _InputIterator first,
    _InputIterator last)
  {
    // Check whether it's an integral type.  If so, it's not an iterator.
    typedef typename boost::is_integral<_InputIterator>::__type _Integral;
    M_insert_dispatch(position, first, last, _Integral());
  }

  iterator
    erase(iterator position)
  {
    if (position + 1 != end())
      std::copy(position + 1, end(), position);
    --M_finish;
    destroy(M_finish);
    return position;
  }

  iterator
    erase(iterator first, iterator last)
  {
    iterator i(std::copy(last, end(), first));
    destroy(i, end());
    M_finish = M_finish - (last - first);
    return first;
  }


  void
    swap(box& x)
  {
    std::swap(M_start, x.M_impl.M_start);
    std::swap(M_finish, x.M_impl.M_finish);
    std::swap(M_end_of_storage,
      x.M_impl.M_end_of_storage);
  }

  void
    clear()
    { erase(begin(), end()); }

protected:

      // Called by the range constructor to implement [23.1.1]/9
      template<typename _Integer>
  void
    M_initialize_dispatch(_Integer n, _Integer value, boost::true_type)
  {
    M_start = M_allocate(n);
    M_end_of_storage = M_start + n;
    std::uninitialized_fill_n(M_start, n, value);
    M_finish = M_end_of_storage;
  }

      // Called by the range constructor to implement [23.1.1]/9
      template<typename _InputIterator>
  void
    M_initialize_dispatch(_InputIterator first, _InputIterator last,
    boost::false_type)
  {
    for (; first != last; ++first)
      push_back(*first);

  }

      // Called by the range assign to implement [23.1.1]/9
      template<typename _Integer>
  void
    M_assign_dispatch(_Integer n, _Integer val, boost::true_type)
  {
    M_fill_assign(static_cast<size_type>(n),
      static_cast<value_type>(val));
  }

      // Called by the range assign to implement [23.1.1]/9
      template<typename _InputIterator>
  void
    M_assign_dispatch(_InputIterator first, _InputIterator last,
    boost::false_type)
  {
    iterator cur(begin());
    for (; first != last && cur != end(); ++cur, ++first)
      *cur = *first;
    if (first == last)
      erase(cur, end());
    else
      insert(end(), first, last);
  }

      // Called by assign(n,t), and the range assign when it turns out
      // to be the same thing.
  void
    M_fill_assign(size_type n, const value_type& val);


      // Internal insert functions follow.

      // Called by the range insert to implement [23.1.1]/9
      template<typename _Integer>
  void
    M_insert_dispatch(iterator pos, _Integer n, _Integer val,
    boost::true_type)
  {
    M_fill_insert(pos, static_cast<size_type>(n),
      static_cast<value_type>(val));
  }

      // Called by the range insert to implement [23.1.1]/9
      template<typename _InputIterator>
  void
    M_insert_dispatch(iterator pos, _InputIterator first,
    _InputIterator last, boost::false_type)
  {
    typedef typename iterator_traits<_InputIterator>::iterator_category
      _IterCategory;
    M_range_insert(pos, first, last, _IterCategory());
  }

      // Called by the second insert_dispatch above
      template<typename _InputIterator>
  void
    M_range_insert(iterator pos, _InputIterator first,
    _InputIterator last, input_iterator_tag);

      // Called by the second insert_dispatch above
      template<typename _ForwardIterator>
  void
    M_range_insert(iterator pos, _ForwardIterator first,
    _ForwardIterator last, forward_iterator_tag);

      // Called by insert(p,n,x), and the range insert when it turns out to be
      // the same thing.
  void
    M_fill_insert(iterator pos, size_type n, const value_type& x);

      // Called by insert(p,x)
  void
    M_insert_aux(iterator position, const value_type& x);
};


  template<typename T>
inline bool
  operator==(const box<T>& x, const box<T>& y)
{ return (x.size() == y.size()
  && std::equal(x.begin(), x.end(), y.begin())); }

  template<typename T>
inline bool
  operator<(const box<T>& x, const box<T>& y)
{ return std::lexicographical_compare(x.begin(), x.end(),
  y.begin(), y.end()); }

  /// Based on operator==
  template<typename T>
inline bool
  operator!=(const box<T>& x, const box<T>& y)
  { return !(x == y); }

  /// Based on operator<
  template<typename T>
inline bool
  operator>(const box<T>& x, const box<T>& y)
  { return y < x; }

  /// Based on operator<
  template<typename T>
inline bool
  operator<=(const box<T>& x, const box<T>& y)
  { return !(y < x); }

  /// Based on operator<
  template<typename T>
inline bool
  operator>=(const box<T>& x, const box<T>& y)
  { return !(x < y); }

  /// See std::box::swap().
  template<typename T>
inline void
  swap(box<T>& x, box<T>& y)
  { x.swap(y); }


  template<typename T>
typename box<T>::iterator
  box<T>::
insert(iterator position, const value_type& x)
{
  const size_type n = position - begin();
  if (M_finish != M_end_of_storage && position == end())
  {
    construct(M_finish, x);
    ++M_finish;
  }
  else
  {      
    if (M_finish == M_end_of_storage)
      throw string("Out of space!");

    construct(M_finish, *(M_finish - 1));
    ++M_finish;
    T x_copy = x;
    std::copy_backward(position,
      iterator(M_finish-2),
      iterator(M_finish-1));
    *position = x_copy; 
  }
  return begin() + n;
}


/*
  template<typename T>
box<T>&
  box<T>::
operator=(const box<T>& x)
{
  if (&x != this)
  {
    const size_type xlen = x.size();
    if (xlen > capacity())
      throw std::string("Out of space!");

    if (size() >= xlen)
    {
      iterator i(std::copy(x.begin(), x.end(), begin()));
      destroy(i, end());
    }
    else
    {
      std::copy(x.begin(), x.begin() + size(),
        M_start);
      std::uninitialized_copy(x.begin() + size(),
        x.end(), M_finish);
    }
    M_finish = M_start + xlen;
  }
  return *this;
}
*/

  template<typename T>
void
  box<T>::
M_fill_assign(size_t n, const value_type& val)
{
  if (n > capacity())
    throw std::string("Out of space!");

  if (n > size())
  {
    std::fill(begin(), end(), val);
    std::uninitialized_fill_n(M_finish, n - size(), val);
    M_finish += n - size();
  }
  else
    erase(fill_n(begin(), n, val), end());
}

  template<typename T>
void
  box<T>::
M_fill_insert(iterator position, size_type n, const value_type& x)
{
  if (n == 0)
    return;

  if (size_type(M_end_of_storage - M_finish) < n)
    { throw string("Out of space!"); }

  value_type x_copy = x;
  const size_type elems_after = end() - position;
  iterator old_finish(M_finish);
  if (elems_after > n)
  {
    std::uninitialized_copy(M_finish - n, M_finish, M_finish);
    M_finish += n;
    std::copy_backward(position, old_finish - n,
      old_finish);
    std::fill(position, position + n, x_copy);
  }
  else
  {
    std::uninitialized_fill_n(M_finish, n - elems_after, x_copy);
    M_finish += n - elems_after;
    std::uninitialized_copy(position, old_finish, M_finish);
    M_finish += elems_after;
    std::fill(position, old_finish, x_copy);
  }
}

  template<typename T> template<typename _InputIterator>
void
  box<T>::
M_range_insert(iterator pos, _InputIterator first,
  _InputIterator last, input_iterator_tag)
{
  for (; first != last; ++first)
  {
    pos = insert(pos, *first);
    ++pos;
  }
}

  template<typename T>
    template<typename _ForwardIterator>
void
  box<T>::
M_range_insert(iterator position, _ForwardIterator first,
  _ForwardIterator last, forward_iterator_tag)
{
  if (first == last)
    return;

  const size_type n = std::distance(first, last);

  if (size_type(M_end_of_storage - M_finish) < n)
    throw string("Out of space!");

  const size_type elems_after = end() - position;
  iterator old_finish(M_finish);
  if (elems_after > n)
  {
    std::uninitialized_copy(M_finish - n, M_finish, M_finish);
    M_finish += n;
    std::copy_backward(position, old_finish - n,
      old_finish);
    std::copy(first, last, position);
  }
  else
  {
    _ForwardIterator mid = first;
    std::advance(mid, elems_after);
    std::uninitialized_copy(mid, last, M_finish);
    M_finish += n - elems_after;
    std::uninitialized_copy(position, old_finish, M_finish);
    M_finish += elems_after;
    std::copy(first, mid, position);
  }
}

#define MAKE_STACK_BOX(c, type, size) box<type> c((type*)alloca(sizeof(type) * (size)), (size))

// Now requires bool flag to be declared before the macro is used.
#define GET_ASSIGNMENT(c, constraint) \
const size_t num_vars##c = constraint->get_vars_singleton()->size();\
box<pair<int, DomainInt> > c((pair<int,DomainInt>*)( alloca(sizeof(pair<int, int>) * num_vars##c * 2) ), num_vars##c * 2);\
flag=constraint->get_satisfying_assignment(c); 

