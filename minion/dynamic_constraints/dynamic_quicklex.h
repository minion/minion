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


#ifndef DYNAMIC_QUICKLEX_H
#define DYNAMIC_QUICKLEX_H

#ifdef P
#undef P
#endif

//#define P(x) cout << x << endl
#define P(x)

template<typename VarArray1, typename VarArray2, bool Less = false>
  struct QuickLexDynamic : public AbstractConstraint
{
  virtual string constraint_name()
    { 
        if(Less)
            return "QuickLexLessDynamic";
        else
            return "QuickLexDynamic"; 
    }

  typedef typename VarArray1::value_type VarRef1;
  typedef typename VarArray2::value_type VarRef2;

  VarArray1 var_array1;
  VarArray2 var_array2;


  Reversible<int> alpha;

  QuickLexDynamic(StateObj* _stateObj, const VarArray1& _array1,
    const VarArray2& _array2) :
  AbstractConstraint(_stateObj), var_array1(_array1), var_array2(_array2),
    alpha(_stateObj, -1)
    { D_ASSERT(var_array1.size() == var_array2.size()); }

  int dynamic_trigger_count()
    { return 2; }

  
  void attach_triggers(int i)
  {
      P("Attach Trigger: " << i);
      DynamicTrigger* dt = dynamic_trigger_start();
      var_array1[i].addDynamicTrigger(dt, LowerBound, NoDomainValue BT_CALL_BACKTRACK);
      var_array2[i].addDynamicTrigger(dt + 1, UpperBound, NoDomainValue BT_CALL_BACKTRACK);
  }
  
  void detach_triggers()
  {
      P("Detach Triggers");
      DynamicTrigger* dt = dynamic_trigger_start();
      releaseTrigger(stateObj, dt BT_CALL_BACKTRACK);
      releaseTrigger(stateObj, dt + 1 BT_CALL_BACKTRACK);
  }
  
  virtual void full_propagate()
  {
    P("Full Prop");
    DynamicTrigger* dt = dynamic_trigger_start();

    if(var_array1.size() == 0)
    {
        if(Less)
            getState(stateObj).setFailed(true);
        return;
    }
    
    alpha = 0;
    
    if(Less && var_array1.size() == 1)
    {
      var_array2[0].setMin(var_array1[0].getMin() + 1);
      var_array1[0].setMax(var_array2[0].getMax() - 1);
    }
    else
    {
      var_array2[0].setMin(var_array1[0].getMin());
      var_array1[0].setMax(var_array2[0].getMax());
    }

    // Set these up, just so they are stored.
    var_array1[0].addDynamicTrigger(dt, LowerBound, NoDomainValue BT_CALL_STORE);
    var_array2[0].addDynamicTrigger(dt + 1, UpperBound, NoDomainValue BT_CALL_STORE);
    
    if(var_array1[0].isAssigned() && var_array2[0].isAssigned() &&
       var_array1[0].getAssignedValue() == var_array2[0].getAssignedValue())
    {
        progress();
    }
  }
  
  void progress()
  {
    int a = alpha;
    int n = var_array1.size();
    D_ASSERT(var_array1[a].isAssigned());
    D_ASSERT(var_array2[a].isAssigned());
    D_ASSERT(var_array1[a].getAssignedValue() == var_array2[a].getAssignedValue());
    
    a++;
    
    while(a < n)
    {
      if(Less && a >= n-1)
      {
         var_array2[a].setMin(var_array1[a].getMin() + 1);
         var_array1[a].setMax(var_array2[a].getMax() - 1);
      }
      else
      {
         var_array1[a].setMax(var_array2[a].getMax());
         var_array2[a].setMin(var_array1[a].getMin());
      }

      if(var_array1[a].isAssigned() && var_array2[a].isAssigned() &&
         var_array1[a].getAssignedValue() == var_array2[a].getAssignedValue())
         {
           a++;
         }
         else
         {
           attach_triggers(a);
           alpha = a;
           return;
         }
    }
    
    if(Less)
        getState(stateObj).setFailed(true);
    else
    {
        detach_triggers();
        alpha = n;
    }
  }
  
  virtual void propagate(DynamicTrigger* dt)
  {
    DynamicTrigger* base_dt = dynamic_trigger_start();
    
    P("Trigger Event:" << dt - base_dt << " alpha:" << (int)alpha);

    int a = alpha;

    if(base_dt == dt)
    { // X triggered
      if(Less && a >= var_array1.size() - 1)
        var_array2[a].setMin(var_array1[a].getMin() + 1);
      else
        var_array2[a].setMin(var_array1[a].getMin());
    }
    else
    { // Y triggered
      if(Less && a >= var_array1.size() - 1)
        var_array1[a].setMax(var_array2[a].getMax() - 1);
      else
        var_array1[a].setMax(var_array2[a].getMax());
    }

    if(var_array1[a].isAssigned() && var_array2[a].isAssigned() &&
       var_array1[a].getAssignedValue() == var_array2[a].getAssignedValue())
    {
      progress();
    }
    else
    {
        //if(var_array1[a].getMax() < var_array2[a].getMin())
        //    detach_triggers();
    }
  }

  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    D_ASSERT(v_size == var_array1.size() + var_array2.size());
    size_t x_size = var_array1.size();

    P("Check Assignment: " << (int)alpha);
    for(size_t i = 0;i < x_size; i++)
    {
      if(v[i] < v[i + x_size])
        return true;
      if(v[i] > v[i + x_size])
        return false;
    }
    
    return !Less;
  }
  
  virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
    {
      size_t array_size = var_array1.size();
      for(size_t i = 0; i < array_size; ++i)
      {
        DomainInt x_i_min = var_array1[i].getMin();
        DomainInt y_i_max = var_array2[i].getMax();

        if(x_i_min > y_i_max)
        {
          return false;
        }

        assignment.push_back(make_pair(i         , x_i_min));
        assignment.push_back(make_pair(i + array_size, y_i_max));
        if(x_i_min < y_i_max)
          return true;
      }

      return !Less;
    }

  virtual AbstractConstraint* reverse_constraint()
  {
      return new QuickLexDynamic<VarArray2, VarArray1,!Less>(stateObj,var_array2,var_array1);
  }
    
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> vars;
    vars.reserve(var_array1.size() + var_array2.size());
    for(unsigned i = 0; i < var_array1.size(); ++i)
      vars.push_back(AnyVarRef(var_array1[i]));
    for(unsigned i = 0; i < var_array2.size(); ++i)
      vars.push_back(AnyVarRef(var_array2[i]));
    return vars;  
  }

};
#endif
