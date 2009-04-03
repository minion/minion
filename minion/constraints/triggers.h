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

#ifndef _TRIGGER_H
#define _TRIGGER_H

#include "../system/system.h"
#include "../constants.h"

class AbstractConstraint;


/** @brief Represents a change in domain. 
 *
 * This is used instead of a simple int as the use of various mappers on variables might mean the domain change needs
 * to be corrected. Every variable should implement the function getDomainChange which uses this and corrects the domain.
 */
class DomainDelta
{ 
  int domain_change; 
public:
  /// This function shouldn't be called directly. This object should be passed to a variables, which will do any "massaging" which 
  /// is required.
  int XXX_get_domain_diff()
{ return domain_change; }

  DomainDelta(int i) : domain_change(i)
{}
};

///The classes which are used to build the queue.
class Trigger
{ 
public:
  /// The constraint to be propagated.
  AbstractConstraint* constraint;
  /// The first value to be passed to the propagate function.
  int info;
#ifdef WEIGHTED_TRIGGERS
  int weight;
#endif
  
  template<typename T>
    Trigger(T* _sc, int _info) : constraint(_sc), info(_info)
  {
#ifdef WEIGHTED_TRIGGERS
    weight = _sc->get_vars_singleton()->size();
#endif
  }
  
  Trigger(const Trigger& t) : constraint(t.constraint), info(t.info) 
  {
#ifdef WEIGHTED_TRIGGERS
    weight = t.weight;
#endif
  }
  
  Trigger() : constraint(NULL)
  {
#ifdef WEIGHTED_TRIGGERS
    weight = 0;
#endif
  }
  
  void inline propagate(DomainDelta domain_data);
  void full_propagate();
  // In function_defs.hpp.

#ifdef WEIGHTED_TRIGGERS
  bool operator<(const Trigger &a) const
  {
    return weight > a.weight;
  }
#endif
};

 
/// Container for a range of triggers
class TriggerRange
{
  /// Start of triggers
  Trigger* start;
  /// End of triggers
  Trigger* finish;

public:
	
  Trigger* begin() const
  { return start; }
  
  Trigger* end() const
  { return finish; }
  
  /// The domain delta from the domain change.
  /** This may not contain the actual delta, but contains data from which a variable can
   construct it, by passing it to getDomainChange. */
  int data;
#ifdef WEIGHTED_TRIGGERS
  int weight;
#endif
  TriggerRange(Trigger* s, Trigger* e, int _data) : start(s), finish(e), data(_data)
  { 
    D_ASSERT(data >= DomainInt_Min);
    D_ASSERT(data <= DomainInt_Max);
#ifdef WEIGHTED_TRIGGERS
    std::sort(s, e);
    weight = s->weight;
#endif
  }

#ifdef WEIGHTED_TRIGGERS
  bool operator<(const TriggerRange &a) const
  {
    return weight > a.weight;
  }
#endif
};

/// Abstract Type that represents any Trigger Creator.
struct AbstractTriggerCreator
{
  Trigger trigger;
  TrigType type;
  virtual void post_trigger() = 0;
  AbstractTriggerCreator(Trigger t, TrigType _type) : trigger(t), type(_type) {}
  virtual ~AbstractTriggerCreator()
  { }
};

/**
 * @brief Concrete Trigger Creators.
 * Allows a trigger to be passed around before being imposed. This is here so reification works.
 */
template<typename VarRef>
struct TriggerCreator : public AbstractTriggerCreator
{
  VarRef* ref;
  TriggerCreator(VarRef& v, Trigger t, TrigType _type) :
    AbstractTriggerCreator(t, _type),  ref(&v)
  {}
  
  virtual void post_trigger()
  { ref->addTrigger(trigger, type); }
  
  virtual ~TriggerCreator()
  { }
};

template<typename VarRef>
inline shared_ptr<AbstractTriggerCreator> 
make_trigger(VarRef& v, Trigger t, TrigType trigger_type)
{ return shared_ptr<AbstractTriggerCreator>(new TriggerCreator<VarRef>(v,t, trigger_type));}

#endif

