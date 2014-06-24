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

#ifndef CONSTRAINT_SUM_H_FGHJ
#define CONSTRAINT_SUM_H_FGHJ

// VarToCount = 1 means leq, = 0 means geq.
template<typename VarArray, typename VarSum, SysInt VarToCount = 1 >
struct BoolLessSumConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { if(VarToCount) return "sumleq"; else return "sumgeq"; }

  CONSTRAINT_ARG_LIST2(var_array, ConstantVar(stateObj, var_sum));


  typedef BoolLessSumConstraint<VarArray, VarSum,1-VarToCount> NegConstraintType;
  typedef typename VarArray::value_type VarRef;

  ReversibleInt count;
  VarArray var_array;

  VarSum var_sum;

  BoolLessSumConstraint(StateObj* _stateObj, const VarArray& _var_array, VarSum _var_sum) :
    AbstractConstraint(_stateObj), count(_stateObj), var_array(_var_array), var_sum(_var_sum)
  { CHECK((VarToCount == 0) || (VarToCount == 1), "Fatal Internal Bug");
      BigInt accumulator=0;
      for(SysInt i=0; i<var_array.size(); i++) {
          accumulator+= checked_cast<SysInt>((DomainInt)max( abs(var_array[i].getInitialMax()), abs(var_array[i].getInitialMin()) ));
          CHECKSIZE(accumulator, "Sum of bounds of variables too large in sum constraint");
      }
      accumulator+= checked_cast<SysInt>((DomainInt)abs(var_sum));
      CHECKSIZE(accumulator, "Sum of bounds of variables too large in sum constraint");


  }

  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    SysInt array_size = var_array.size();

    count = 0;

    for(SysInt i = 0; i < array_size; ++i)
      if(VarToCount)
        t.push_back(make_trigger(var_array[i], Trigger(this, i), LowerBound));
      else
        t.push_back(make_trigger(var_array[i], Trigger(this, i), UpperBound));
    return t;
  }

  virtual AbstractConstraint* reverse_constraint()
  {
    if(VarToCount)
      return new BoolLessSumConstraint<VarArray, SysInt, 0>(stateObj, var_array, var_sum + 1);
    else
      return new BoolLessSumConstraint<VarArray, SysInt, 1>(stateObj, var_array, var_sum - 1);
  }

  DomainInt occ_count()
  {
    if (VarToCount)
      return var_sum;
    else
      return (SysInt)var_array.size() - var_sum;
  }


  void limit_reached()
  {
    SysInt one_vars = 0;
    typename VarArray::value_type* it = &*var_array.begin();
    typename VarArray::value_type* end_it = it + var_array.size();
    for(; it < end_it; ++it)
    {
      if(it->isAssigned())
      { if(it->getAssignedValue() == VarToCount) ++one_vars; }
      else
      { it->uncheckedAssign(1 - VarToCount); }
    }
    //D_ASSERT(one_vars >= occ_count());
    if(one_vars > occ_count())
      getState(stateObj).setFailed(true);
  }

  virtual void propagate(DomainInt i, DomainDelta)
  {
    PROP_INFO_ADDONE(BoolSum);
    D_ASSERT(var_array[checked_cast<SysInt>(i)].getAssignedValue() == 0 ||
             var_array[checked_cast<SysInt>(i)].getAssignedValue() == 1);
    SysInt c = count + 1;
    count = c;
    if(c == occ_count())
      limit_reached();
  }

  virtual void full_propagate()
  {
    SysInt occs = 0;
    SysInt array_size = var_array.size();
    for(SysInt i = 0; i < array_size; ++i)
      if(var_array[i].isAssignedValue(VarToCount))
        occs++;
    count = occs;
    if(occs > occ_count())
      getState(stateObj).setFailed(true);
    if(occs == occ_count())
      limit_reached();
  }

  virtual BOOL check_assignment(DomainInt* v, SysInt v_size)
  {
    D_ASSERT(v_size == (SysInt)var_array.size());
    for(SysInt i = 0; i < v_size; i++)
      D_ASSERT(v[i] == 0 || v[i] == 1);
    if(VarToCount)
      return std::accumulate(v, v + v_size, DomainInt(0)) <= var_sum;
    else
      return std::accumulate(v, v + v_size, DomainInt(0)) >= var_sum;
  }

  /*
  // TODO : Optimise for booleans
  virtual bool get_satisfying_assignment(box<pair<SysInt,DomainInt> >& assignment)
  {
    SysInt sum_value = 0;
    SysInt v_size = var_array.size();
    if(VarToCount)
    {
      for(SysInt i = 0; i < v_size; ++i)
      {
        assignment.push_back(make_pair(i, var_array[i].getMin()));
        sum_value += var_array[i].getMin();
      }
      return (sum_value <= var_sum);
    }
    else
    {
      for(SysInt i = 0; i < v_size; ++i)
      {
        assignment.push_back(make_pair(i, var_array[i].getMax()));
        sum_value += var_array[i].getMax();
      }
      return (sum_value >= var_sum);
    }
  }
  */

  // TODO : Optimise for booleans
  virtual bool get_satisfying_assignment(box<pair<SysInt,DomainInt> >& assignment)
  {
    SysInt v_size = var_array.size();
    DomainInt sum_limit;
    if(VarToCount)
      sum_limit = (SysInt)var_array.size() - var_sum;
    else
      sum_limit = var_sum;

    SysInt ValToFind = 1 - VarToCount;

    SysInt val_count = 0;

    for(SysInt i = 0; i < v_size && val_count < sum_limit; ++i)
    {
      if(var_array[i].inDomain(ValToFind))
      {
        val_count++;
        assignment.push_back(make_pair(i, ValToFind));
      }
    }
    return val_count >= sum_limit;
  }


  virtual vector<AnyVarRef> get_vars()
  {
    vector<AnyVarRef> vars;
    vars.reserve(var_array.size());
    for(UnsignedSysInt i = 0; i < var_array.size(); ++i)
      vars.push_back(var_array[i]);
    return vars;
  }
};

template<typename VarArray,  typename VarSum>
AbstractConstraint*
BoolLessEqualSumCon(StateObj* stateObj, const VarArray& _var_array,  VarSum _var_sum)
{
  return (new BoolLessSumConstraint<VarArray,VarSum>(stateObj, _var_array,_var_sum));
}

template<typename VarArray,  typename VarSum>
AbstractConstraint*
BoolGreaterEqualSumCon(StateObj* stateObj, const VarArray& _var_array,  VarSum _var_sum)
{
  return (new BoolLessSumConstraint<VarArray,VarSum,0>(stateObj, _var_array,_var_sum));
}

#endif
