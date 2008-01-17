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