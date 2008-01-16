/* Minion
* Copyright (C) 2006
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



template<typename VarType>
struct VarIterator
{
  VarType var;
  int var_max;
  int current_val;
  
  VarIterator(VarType _var) : var(_var), var_max(_var.getMax()), current_val(_var.getMin())
  { find_first_valid_position(); }
  
  VarIterator(VarType _var, int _start_val)
  : var(_var), var_max(_var.getMax()), current_val(_start_val)
  { find_first_valid_position(); }
  
  void find_first_valid_position()
  {
    while(current_val <= var_max && !var.inDomain(current_val))
	  ++current_val;
    D_ASSERT(current_val <= var_max);
  }
  
  BOOL step()
  {
    ++current_val;
	while(current_val <= var_max && !var.inDomain(current_val))
      ++current_val;
	return !(current_val > var_max);
  }
  
  int val()
  { return current_val; }
};

template<typename VarType>
struct VarIterator_looping
{
  VarType var;
  int var_max;
  int current_val;
  int start_val;
  BOOL first_step;
  
  VarIterator_looping(VarType _var) 
  : var(_var), var_max(_var.getMax()), current_val(_var.getMin()), start_val(current_val), first_step(true)
  { find_first_valid_position(); }
  
  VarIterator_looping(VarType _var, int _start_val) 
  : var(_var), var_max(_var.getMax()), current_val(_start_val), start_val(current_val), first_step(true)
  { find_first_valid_position(); }
  
  void find_first_valid_position()
  {
	while(current_val <= var_max && !var.inDomain(current_val))
	  ++current_val;
	if(current_val <= var_max)
	  return;
	  
	current_val = var.getMin();
	first_step = false;
	while(current_val < start_val && !var.inDomain(current_val))
	  ++current_val;
    D_ASSERT(current_val < start_val);
  };
  
  BOOL step()
  {
    ++current_val;
    if(first_step)
	{
      while(current_val <= var_max && !var.inDomain(current_val))
	    ++current_val;
	  if(current_val <= var_max)
	    return true;
	  current_val = var.getMin();
	  first_step = false;
	}
	// This isn't in an 'else' of the above for loop, as in the case
	// where that for loop fails, we want to fall into here.
	while(current_val < start_val && !var.inDomain(current_val))
	  ++current_val;
	return current_val != start_val;
  }
  
  int val()
  { return current_val; }
};

