// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

/// A simple wrapper for a pair of bounds.
struct Bounds {
  DomainInt lowerBound;
  DomainInt upperBound;
  Bounds(DomainInt Lower, DomainInt Upper) : lowerBound(Lower), upperBound(Upper) {}

  bool contains(DomainInt i) {
    return lowerBound <= i && i <= upperBound;
  }

  DomainInt min() const {
    return lowerBound;
  }

  DomainInt max() const {
    return upperBound;
  }

  bool hasSingleValue() const {
    return min() == max();
  }
};

inline bool operator==(Bounds lhs, Bounds rhs) {
  return lhs.lowerBound == rhs.lowerBound && lhs.upperBound == rhs.upperBound;
}

inline Bounds emptyBounds() {
  return Bounds(DomainInt_Max, DomainInt_Min);
}

template <typename T>
inline Bounds getBounds(const T& t) {
  return Bounds(t.min(), t.max());
}

inline Bounds addValue(Bounds b, DomainInt d) {
  return Bounds(std::min(b.lowerBound, d), std::max(b.upperBound, d));
}

inline bool checkAllZero(char* begin, char* end) {
  for(char* p = begin; p < end; ++p) {
    if(*p != 0)
      return false;
  }
  return true;
}
