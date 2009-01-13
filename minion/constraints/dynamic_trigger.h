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


/// This is a trigger to a constraint, which can be dynamically moved around.
class DynamicTrigger
{
private:
  /// Hidden, as copying a DynamicTrigger is almost certainly an error.
  DynamicTrigger(const DynamicTrigger&);  
public:

  /// In debug mode, a value set to 1234. This allows a check that a DynamicTrigger*
  /// actually points to a valid object.
  D_DATA(int sanity_check); 
  /// The constraint to be triggered.
  AbstractConstraint* constraint;
  /// A small space for constraints to store trigger-specific information.
  int _trigger_info;
  
  /// Wrapper function for _trigger_info.
  int& trigger_info()
  { return _trigger_info; }
  
  DynamicTrigger* prev;
  DynamicTrigger* next;


  DynamicTrigger(AbstractConstraint* c) : constraint(c), prev(NULL), next(NULL)
  { D_DATA(sanity_check = 1234);}
  
  DynamicTrigger() : constraint(NULL)
  { 
    D_DATA(sanity_check = 1234);
    prev = next = this; 
  }
  
 
  /// Remove from whatever list this trigger is currently stored in.
  void remove(DynamicTrigger*& next_queue_ptr)
  { 
    if(this == next_queue_ptr)
    {
      CON_INFO_ADDONE(DynamicMovePtr);
      next_queue_ptr = next;
    }
	D_ASSERT(constraint != NULL);
    D_ASSERT(sanity_check == 1234);
    D_ASSERT( (prev == NULL) == (next == NULL) );
	DynamicTrigger* old_prev = prev;
	DynamicTrigger* old_next = next;
    if(old_prev != NULL)
	{ old_prev->next = old_next; }
	if(old_next != NULL)
	{ old_next->prev = old_prev; }
	D_ASSERT(old_prev == NULL || old_prev->sanity_check_list(false));
	D_ASSERT(old_next == NULL || old_next->sanity_check_list(false));
	next = NULL;
	prev = NULL;
  }
  
  inline bool isAttached()
  {
      return prev!=NULL;
  }
  
  /// Add this trigger after another one in a list.
  /// This function will remove this trigger from any list it currently lives in.
  // next_queue_ptr is a '*&' as it is a pointer which we want a reference to, so we can change it!
  void add_after(DynamicTrigger* new_prev, DynamicTrigger*& next_queue_ptr)
  {
    D_ASSERT(constraint != NULL);
	D_ASSERT(sanity_check == 1234);
    D_ASSERT(new_prev->sanity_check_list());
    if(prev != NULL)
	{
#ifndef NO_DYN_CHECK
	  if(this == next_queue_ptr)
	  {
		CON_INFO_ADDONE(DynamicMovePtr);
	    next_queue_ptr = next;
	  }
      
#endif
	  remove(next_queue_ptr);
	}
	DynamicTrigger* new_next = new_prev->next;
	prev = new_prev;
	next = new_next;
	new_prev->next = this;
	new_next->prev = this;
	D_ASSERT(prev->next == this);
	D_ASSERT(next->prev == this);
    D_ASSERT(new_prev->sanity_check_list());
  }
  
  /// Propagates the constraint stored in the trigger.
  /** Out of line as it needs the full definition of DynamicConstraint */
  void propagate();
  
  ~DynamicTrigger()
  { D_DATA(sanity_check = -1); }
  
  BOOL sanity_check_list(BOOL is_head_of_list = true)
  {
    if(is_head_of_list)
	{
	  D_ASSERT(this->constraint == NULL);
	}
	D_ASSERT(this->sanity_check == 1234);
	for(DynamicTrigger* it = this->next; it != this; it = it->next)
	{
	  D_ASSERT(it->sanity_check == 1234);
	  if(is_head_of_list)
	  {
	    D_ASSERT(it->constraint != NULL);
	  }
	  D_ASSERT(it->prev->next == it);
	  D_ASSERT(it->next->prev == it);
	}
	return true;
  }
};
