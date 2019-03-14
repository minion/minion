#ifndef MINLIB_RANGES_HPP
#define MINLIB_RANGES_HPP
#include <tuple>

template <typename T>
class DomainRange_iter {
  T begin;
  T end;
  T step;

public:
  DomainRange_iter(T _begin, T _end, T _step) : begin(_begin), end(_end), step(_step) {}

  T operator*() {
    return begin;
  }

  void operator++() {
    begin += step;
    if(begin > end)
      begin = end;
  }

  friend bool operator!=(DomainRange_iter lhs, DomainRange_iter rhs) {
    return std::tie(lhs.begin, lhs.end, lhs.step) != std::tie(rhs.begin, rhs.end, rhs.step);
  }
};

template <typename T>
class DomainRange_Impl {
  T begin_m;
  T end_m;
  T step_m;

public:
  DomainRange_Impl(T _end) : begin_m(0), end_m(_end), step_m(1) {
    if(end_m < begin_m)
      end_m = begin_m;
  }

  DomainRange_Impl(T _begin, T _end) : begin_m(_begin), end_m(_end), step_m(1) {
    if(end_m < begin_m)
      end_m = begin_m;
  }

  DomainRange_Impl(T _begin, T _end, T _step) : begin_m(_begin), end_m(_end), step_m(_step) {
    if(end_m < begin_m)
      end_m = begin_m;
  }

  DomainRange_iter<T> begin() const {
    return DomainRange_iter<T>(begin_m, end_m, step_m);
  }

  DomainRange_iter<T> end() const {
    return DomainRange_iter<T>(end_m, end_m, step_m);
  }
};

template <typename T>
DomainRange_Impl<T> Range(const T& e) {
  return DomainRange_Impl<T>(e);
}

template <typename T>
DomainRange_Impl<T> Range(const T& b, const T& e) {
  return DomainRange_Impl<T>(b, e);
}

template <typename T>
DomainRange_Impl<T> Range(const T& b, const T& e, const T& s) {
  return DomainRange_Impl<T>(b, e, s);
}

template <typename T>
class ContainerRange_Impl {
  T maxVec;

public:
  ContainerRange_Impl(T Vec) : maxVec(Vec) {}

  struct Container_iter {
    T maxVec;
    T currentVec;

    Container_iter(T vec) : maxVec(vec), currentVec(vec.size()) {}

    Container_iter(T vec, bool) : maxVec(vec), currentVec(vec) {}

    T operator*() {
      return currentVec;
    }

    void operator++() {
      // We are reusing the existing increment vector,
      // which loops around and returns false at the end
      // of the loop
      if(!incrementVector(currentVec, maxVec))
        currentVec = maxVec;
    }

    friend bool operator!=(Container_iter lhs, Container_iter rhs) {
      return lhs.currentVec != rhs.currentVec;
    }
  };

  Container_iter begin() const {
    return Container_iter(maxVec);
  }

  Container_iter end() const {
    return Container_iter(maxVec, true);
  }
};

template <typename T>
ContainerRange_Impl<T> ContainerRange(const T& e) {
  return ContainerRange_Impl<T>(e);
}

#endif
