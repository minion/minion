struct StaticBranch
{
  template<typename VarType>
  int operator()(vector<VarType>& var_order, int pos)
  {
	unsigned v_size = var_order.size();
	while(pos < v_size && var_order[pos].isAssigned())
	  ++pos;
	return pos;
  }
};

struct SlowStaticBranch
{
  template<typename VarType>
  int operator()(vector<VarType>& var_order, int pos)
  {
	unsigned v_size = var_order.size();
	pos = 0;
	while(pos < v_size && var_order[pos].isAssigned())
	  ++pos;
	return pos;
  }
};

struct SDFBranch
{
  template<typename VarType>
  int operator()(vector<VarType>& var_order, int pos)
  {
	int length = var_order.size();
	int smallest_dom = length;
	DomainInt dom_size = DomainInt_Max;
	
	
	for(int i = 0; i < length; ++i)
	{
	  DomainInt maxval = var_order[i].getMax();
	  DomainInt minval = var_order[i].getMin();
	  
	  if((maxval != minval) && ((maxval - minval) < dom_size) )
	  {
		dom_size = maxval - minval;
		smallest_dom = i;
		if( maxval - minval == 1)
		{ // Binary domain, must be smallest
		  return i;
		}
	  }
	}
	return smallest_dom;
  }
};

struct LDFBranch
{
  template<typename VarType>
  int operator()(vector<VarType>& var_order, int pos)
  {
	int length = var_order.size();
	
	pos = 0;
	while(pos < length && var_order[pos].isAssigned())
	  ++pos;
	if(pos == length)
	  return length;
	
	int largest_dom = pos;
	DomainInt dom_size = var_order[pos].getMax() - var_order[pos].getMin();
	
	++pos;
	
	for(; pos < length; ++pos)
	{
	  DomainInt maxval = var_order[pos].getMax();
	  DomainInt minval = var_order[pos].getMin();
	  
	  if(maxval - minval > dom_size)
	  {
		dom_size = maxval - minval;
		largest_dom = pos;
	  }
	}
	return largest_dom;
  }
};
