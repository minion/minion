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

/// increments a container of values. Returns 'true' until the maximum value is reached.
template<typename Container>
bool increment_vector(Container& vals, const Container& maxvals)
{
  bool carry = true;
  int position = vals.size() - 1;
  while(position >= 0 && carry == true)
  {
    D_ASSERT(maxvals[position] != 0);
    vals[position]++;
    if(vals[position] == maxvals[position])
      vals[position] = 0;
    else
      carry = false;
    --position;
  }
  return !carry;
}

/// A simple wrapper for a pair of bounds.
struct Bounds
{
  int lower_bound;
  int upper_bound;
  Bounds(int _lower, int _upper) : lower_bound(_lower), upper_bound(_upper)
  { }
};

template<typename T>
vector<T> make_vec(const T& t)
{
  vector<T> vec(1);
  vec[0] = t;
  return vec;
}
