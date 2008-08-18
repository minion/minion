/// increments a container of values. Returns 'true' until the maximum value is reached.
template<typename Container>
bool increment_vector(Container& vals, const Container& maxvals)
{
  bool carry = true;
  int position = vals.size() - 1;
  while(position >= 0 && carry == true)
  {
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
