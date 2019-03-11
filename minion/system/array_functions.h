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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

/// A simple wrapper for a pair of bounds.
struct Bounds {
  DomainInt lower_bound;
  DomainInt upper_bound;
  Bounds(DomainInt _lower, DomainInt _upper) : lower_bound(_lower), upper_bound(_upper) {}

  bool contains(DomainInt i) {
    return lower_bound <= i && i <= upper_bound;
  }

  DomainInt min() const {
    return lower_bound;
  }

  DomainInt max() const {
    return upper_bound;
  }

  bool hasSingleValue() const {
    return min() == max();
  }
};

inline bool operator==(Bounds lhs, Bounds rhs) {
  return lhs.lower_bound == rhs.lower_bound && lhs.upper_bound == rhs.upper_bound;
}

inline Bounds emptyBounds() {
  return Bounds(DomainInt_Max, DomainInt_Min);
}

template <typename T>
inline Bounds getBounds(const T& t) {
  return Bounds(t.getMin(), t.getMax());
}

inline Bounds addValue(Bounds b, DomainInt d) {
  return Bounds(std::min(b.lower_bound, d), std::max(b.upper_bound, d));
}

inline bool checkAllZero(char* begin, char* end) {
  for(char* p = begin; p < end; ++p) {
    if(*p != 0)
      return false;
  }
  return true;
}
