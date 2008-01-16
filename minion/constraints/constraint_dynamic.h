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

struct DynamicConstraint;

struct DynamicTrigger;


#define DYNAMIC_PROPAGATE_FUNCTION virtual void propogate


VARDEF_ASSIGN(DynamicTrigger* next_queue_ptr, NULL);

/// This is a trigger to a constraint, which can be dynamically moved around.
struct DynamicTrigger
{
private:
  /// Hidden, as copying a DynamicTrigger is almost certainly an error.
  DynamicTrigger(const DynamicTrigger&);  
public:

  /// In debug mode, a value set to 1234. This allows a check that a DynamicTrigger*
  /// actually points to a valid object.
  D_DATA(int sanity_check); 
  /// The constraint to be triggered.
  DynamicConstraint* constraint;
  /// A small space for constraints to store trigger-specific information.
  int _trigger_info;
  
  /// Wrapper function for _trigger_info.
  int& trigger_info()
  { return _trigger_info; }
  
  DynamicTrigger* prev;
  DynamicTrigger* next;


  DynamicTrigger(DynamicConstraint* c) : constraint(c), prev(NULL), next(NULL)
  { D_DATA(sanity_check = 1234);}
  
  DynamicTrigger() : constraint(NULL)
  { 
    D_DATA(sanity_check = 1234);
    prev = next = this; 
  }
  
 
  /// Remove from whatever list this trigger is currently stored in.
  void remove()
  { 
    D_INFO(1,DI_DYNAMICTRIG,string("Trigger ") + to_string(this) + " removed from list: "
							+ ":" + to_string(prev) + "," + to_string(next));
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
  
  /// Add this trigger after another one in a list.
  /// This function will remove this trigger from any list it currently lives in.
  void add_after(DynamicTrigger* new_prev)
  {
    D_INFO(1, DI_DYNAMICTRIG, string("Trigger ") + to_string(this) + " added to list " + to_string(new_prev));
    D_ASSERT(constraint != NULL);
	D_ASSERT(sanity_check == 1234);
    D_ASSERT(new_prev->sanity_check_list());
    if(prev != NULL)
	{
	  D_INFO(1, DI_DYNAMICTRIG, "Must remove trigger from existing queue.");
	  if(this == next_queue_ptr)
	    next_queue_ptr = next;
	  remove();
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
  
  /// Propogates the constraint stored in the trigger.
  /** Out of line as it needs the full definition of DynamicConstraint */
  void propogate();
  
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

/// Base type from which all dynamic constraints are derived.
struct DynamicConstraint
{
  /// Method to get constraint name for debugging.
  virtual string constraint_name() = 0;
  
  /// Private member of the base class.
#ifdef WATCHEDLITERALS
  MemOffset _DynamicTriggerCache;
#else
  BackTrackOffset _DynamicTriggerCache;
#endif
  /// Returns a point to the first dynamic trigger of the constraint.
  DynamicTrigger* dynamic_trigger_start()
  { return static_cast<DynamicTrigger*>(_DynamicTriggerCache.get_ptr()); }
  
  /// Gives the value of a specific dynamic trigger.
  int dynamic_trigger_num(DynamicTrigger* trig)
  { return trig - static_cast<DynamicTrigger*>(_DynamicTriggerCache.get_ptr()); }
  
  /// Actually creates the dynamic triggers.
  void setup()
  {
    int trigs = dynamic_trigger_count();
	_DynamicTriggerCache.request_bytes(sizeof(DynamicTrigger) * trigs);
	DynamicTrigger* start = dynamic_trigger_start();
	for(int i = 0 ; i < trigs; ++i)
	  new (start+i) DynamicTrigger(this);
  }
  
  /// Defines the number of dynamic triggers the constraint wants.
  virtual int dynamic_trigger_count() = 0;
  
  /// Performs a full round of propagation and sets up any data needs by propogate().
  /** This function can be called during search if the function is reified */
  virtual void full_propogate() = 0;
  
  /// Iterative propagation function.
  /** Can assume full_propagate is always called at least once before propagate */
  DYNAMIC_PROPAGATE_FUNCTION(DynamicTrigger*) = 0;
  
  // Returns the variables of the constraint
  virtual vector<AnyVarRef> get_vars() = 0;
  
  
  /// Checks if an assignment is satisfied.
  /** This takes the variable order returned by, and is mainly only used by, get_table_constraint() */
  virtual BOOL check_assignment(vector<int>) = 0;
    
  virtual ~DynamicConstraint()
  {}
};


inline void DynamicTrigger::propogate()
{ 
  D_ASSERT(sanity_check == 1234);
  constraint->propogate(this); 
}

