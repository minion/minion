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

#ifndef CONSTRAINT_GACTABLE_REGIN_H
#define CONSTRAINT_GACTABLE_REGIN_H

struct TupleComparator
{
  SysInt significantIndex;
  SysInt arity;
  
  TupleComparator(SysInt i, SysInt a)
  {
    significantIndex = i;
    arity = a; 
  }
  
  // returns tuple1 <= tuple2 under our ordering.
  BOOL operator()(const vector<DomainInt>& tuple1, const vector<DomainInt>& tuple2)
  {
    if(tuple1[significantIndex] != tuple2[significantIndex])
      return tuple1[significantIndex] < tuple2[significantIndex];
    for(SysInt tupleIndex = 0; tupleIndex < arity; tupleIndex++)
    {
      if(tuple1[tupleIndex] != tuple2[tupleIndex])
        return tuple1[tupleIndex] < tuple2[tupleIndex];
    }
    return false;
  }
};

struct TupleH
{
  // tuple class for Regin/Lhomme's bounding and jumping algorithm
  SysInt id;  // global array index (can also be used for lex comparison of two tuples.)
           // no need for nextPointer
  SysInt arity;
  
  SysInt * values;
  SysInt * nextValue;    // SysInt index into global array, pointing to the next 
  
  SysInt * redundantValues;
  SysInt * redundantNextValue;
  
  TupleH(SysInt * _values, SysInt * _redundantValues, SysInt _id, SysInt _arity)
  {
    values=_values;
    redundantValues=_redundantValues;
    id=_id;
    arity=_arity;
    
    nextValue=new SysInt[arity];
    redundantNextValue=new SysInt[arity];
    for(SysInt i=0; i<arity; i++)
    {
      nextValue[i]=-1;
      redundantNextValue[i]=-1;
    }
  }
};

struct Regin
{
  SysInt literal_num;
  SysInt noTuples;
  SysInt arity;
  
  TupleList* tupleList;
  
  Regin(TupleList* _tuples) : tupleList(_tuples)
  {
    tupleList->finalise_tuples();
    arity = tupleList->tuple_size();    
    noTuples = tupleList->size();
    
          tuples.resize(tupleList->size());
          
          // Need a copy so we can sort it and such things.
          for(SysInt i = 0; i < tupleList->size(); ++i)
            tuples[i] = tupleList->get_vector(i);
          
          // sort, required for correctness.
          std::stable_sort(tuples.begin(), tuples.end(), TupleComparator(0, arity));
          
              tuplelist=new TupleH*[tuples.size()];
              setuplist();
  }
  
  TupleH ** tuplelist;
  vector<vector<DomainInt> > tuples;

  // set up the list
  void setuplist()
  {
    
    SysInt * redvalues=new SysInt[arity];
    for(SysInt i=0; i<arity; i++){ 
      redvalues[i]=(tupleList->dom_smallest)[i];
    }
    
    for(SysInt i=0; i<tuples.size(); i++)
    {
      // copy redvalues
      SysInt * newredvalues= new SysInt[arity]; 
      for(SysInt j=0; j<arity; j++)
        newredvalues[j]=redvalues[j];
      
      SysInt * valuesarray= new SysInt[arity];
      for(SysInt j=0; j<arity; j++)
        valuesarray[j]=tuples[i][j];
      
      TupleH * t=new TupleH(valuesarray, newredvalues, i, arity);
      tuplelist[i]=t;
      
      // cross-link with previous tuples.
      SysInt valcountlocal=arity;  // for each value in this tuple, there is one forward reference in nextValue.
      
      for(SysInt j = i - 1; j >= 0; j--)
      {
        TupleH* prev=tuplelist[j];
        BOOL breakflag=false;
        
        for(SysInt var=0; var<arity; var++)
        {
          if((prev->values[var])==(t->values[var]) && prev->nextValue[var]==-1)
          {
            prev->nextValue[var]=i;
            valcountlocal--;
          }
          if(prev->redundantValues[var]==t->values[var] && prev->redundantNextValue[var]==-1)
            prev->redundantNextValue[var]=i;
          if(valcountlocal==0){
            breakflag=true;
            break;
          }
        }
        if(breakflag){
          break;
        }
      }
      
      // increment redvalues
      for(SysInt var=0; var<arity; var++)
      {
        redvalues[var]++;
        SysInt max = (tupleList->dom_smallest)[var] + (tupleList->dom_size)[var];
        while(!(redvalues[var]>max))
          redvalues[var]++;
        if(redvalues[var] > max)
          redvalues[var] = (tupleList->dom_smallest)[var];
      }
    }
  }
  
  
  
};

template<typename VarArray>
struct GACTableConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "TableRegin"; }
  
  typedef typename VarArray::value_type VarRef;
  VarArray vars;
  
  /// For each literal, the number of the tuple that supports it.
  // This is bad because it might have holes in it, i.e. revints that are not used.
  
  ReversibleInt *** current_support;
  //vector<vector<ReversibleInt> > current_support;  // current_support[var][val+offset[var]];
  
  vector<DomainInt> offset;
  
  TupleList* tupleList;
  
  /// Check if all allowed values in a given tuple are still in the domains of the variables.
  BOOL check_tuple(const vector<DomainInt>& v)
  {
    for(UnsignedSysInt i = 0; i < v.size(); ++i)
    {
      if(!vars[i].inDomain(v[i]))
        return false;
    }
    return true;
  }
  
  BOOL check_tuple(SysInt * v)
  {
    for(UnsignedSysInt i = 0; i < arity; ++i)
    {
      if(!vars[i].inDomain(v[i]))
        return false;
    }
    return true;
  }
  
  SysInt comparetuples(SysInt * t1, SysInt * t2)
  {
    for(SysInt i=0; i<arity; i++)
    {
      if(t1[i]>t2[i])
        return 1;
      if(t1[i]<t2[i])
        return -1;
    }
    return 0;
  }
  
  BOOL listdone;
  
  SysInt noTuples;
  SysInt arity;
  SysInt * upperboundtuple;
  
  Regin* regin;
  
  GACTableConstraint(const VarArray& _vars, TupleList* _tuples) :
    vars(_vars),
    tupleList(_tuples)
  {
      tupleList->finalise_tuples();
      regin = tupleList->getRegin();
      arity = tupleList->tuple_size();
      D_ASSERT(_vars.size() == arity);
      noTuples = tupleList->size();
      //current_support.resize(arity); 
      
      
      current_support=new ReversibleInt**[arity];
      
      listdone=false;
      
      offset.resize(arity);
      for(SysInt i=0; i<arity; i++)
      {
        offset[i]=-vars[i].getInitialMin();
        
        current_support[i]= new ReversibleInt*[vars[i].getInitialMax()+offset[i]+1];
        
        for(SysInt j=0; j<vars[i].getInitialMax()+offset[i]+1; j++){
          current_support[i][j]=new ReversibleInt();
          current_support[i][j]->set(-1);
        }
      }
      upperboundtuple=new SysInt[arity];      

  }
  
  virtual SysInt dynamic_trigger_count()
  { return tupleList->literal_num * ( vars.size() - 1) ; }
  
   TupleH* seekNextSupport(SysInt var, SysInt val)
  {
    SysInt last_pointer=current_support[var][val+offset[var]]->get();
    
    // for other variables, compute the max (over vars) of the min (over vals)
    // which gives the min lower bound of tuples that have been checked already.
    SysInt lowerbound=last_pointer;
    for(SysInt i=0; i<arity; i++)
    {
      if(i!=var)
      {
        SysInt minlb=current_support[i][vars[i].getMin()+offset[i]]->get();
        
        for(SysInt valIndex=vars[i].getMin()+1; valIndex<=vars[i].getMax(); valIndex++)
        {
          if(vars[i].inDomain(valIndex))
          {
            SysInt thisbound=current_support[i][valIndex+offset[i]]->get();            
            if(thisbound<minlb) minlb=thisbound;  // even if thisbound==-1.
          }
        }
        if(minlb>lowerbound)
          lowerbound=minlb;
      }
    }  // can't do this because we don't use tuples from the checking of other variables to support this one.??
    
    
    for(SysInt i=0; i<arity; i++)
    {
      if(i!=var)
        upperboundtuple[i]=vars[i].getMax();
      else
        upperboundtuple[i]=val;
    }
    
    // now find the next one from lowerbound which contains (var, val)
    SysInt curtupleIndex;
    
    if(lowerbound==-1)
      curtupleIndex=nextin(var, val, 0);
    else
      curtupleIndex=nextin(var, val, lowerbound);

    if(curtupleIndex==-1)
    {   // off the end of the list
      return 0;
    }
    
    TupleH* curtuple=regin->tuplelist[curtupleIndex];
    
    if(comparetuples(curtuple->values, upperboundtuple)>0)
     return 0;
    
    while(!check_tuple(curtuple->values))
    {
      curtupleIndex=curtuple->nextValue[var];  //  curtuple=NEXT((x,a), curtuple);
      if(curtupleIndex==-1)
        return 0;
      
      SysInt maxjump=curtupleIndex;
      for(SysInt y = 0; y < arity; y++)
      {
        if(y!=var)
        {
          DomainInt b=vars[y].getMin();
          SysInt off=offset[y];
          SysInt ltp=current_support[y][b+off]->get();
          SysInt nextinminallvals=nextin(y, b, (ltp>curtupleIndex)?ltp:curtupleIndex);
          for(b=vars[y].getMin()+1; b<=vars[y].getMax(); b++)
          {
            if(vars[y].inDomain(b))
            {
              ltp=current_support[y][b+off]->get();
              SysInt temp=nextin(y, b, (ltp>curtupleIndex)?ltp:curtupleIndex);
              if(temp<nextinminallvals) nextinminallvals=temp;
            }
          }
          
          if(nextinminallvals>maxjump) maxjump=nextinminallvals;
        }
      }
      
      maxjump=nextin(var, val, maxjump);
      
      if(maxjump>curtupleIndex)
      {
        D_ASSERT( !check_tuple(curtuple->values)); // for some reason the pseudocode assumes this.
        curtupleIndex=maxjump;
      }
      
      curtuple=regin->tuplelist[curtupleIndex];
      
      if(comparetuples(curtuple->values, upperboundtuple)>0)  // this wouldn't be necessary if I got an index for upperboundtuple.
        return 0;
    }
    current_support[var][val+offset[var]]->set(curtupleIndex);
    return curtuple;
  }
  
  SysInt nextin(SysInt var, SysInt val, SysInt curtuple)
  { // returns curtuple if curtuple contains var,val. So not strictly 'next'. If there is not one, returns -1.
    TupleH* temp=regin->tuplelist[curtuple];
    
    while(temp->values[var]!=val)
    {
      if(temp->redundantValues[var]==val)
      {
        D_ASSERT( temp->redundantNextValue[var]==-1 || regin->tuplelist[temp->redundantNextValue[var]]->values[var]==val);
        return temp->redundantNextValue[var];
      }
      curtuple++;
      if(curtuple>=noTuples) return -1;
      temp=regin->tuplelist[curtuple];
    }
    D_ASSERT( curtuple==-1 || regin->tuplelist[curtuple]->values[var]==val);
    return curtuple;
  }
  
  // End of regin-lhomme code.
  
  bool find_new_support(SysInt literal)
  {
    pair<DomainInt, DomainInt> varval = tupleList->get_varval_from_literal(literal);
    SysInt var = varval.first;
    SysInt val = varval.second;
    TupleH* new_support = seekNextSupport(var,val);
    
    if (new_support == 0)
      return false;
    else
      return true;
  }
  
  virtual void propagate(DynamicTrigger* propagated_trig)
  {
    PROP_INFO_ADDONE(DynGACTable);
    DynamicTrigger* dt = dynamic_trigger_start();
    SysInt trigger_pos = propagated_trig - dt;
    SysInt propagated_literal = trigger_pos / (vars.size() - 1);
    
    BOOL is_new_support = find_new_support(propagated_literal);
    
    pair<DomainInt, DomainInt> varval = tupleList->get_varval_from_literal(propagated_literal);
    SysInt varIndex = varval.first;
    SysInt val = varval.second;
    
    if(is_new_support)
    {
      setup_watches(varIndex, val, propagated_literal);
    }
    else
    {
      vars[varIndex].removeFromDomain(val);
    }
  }
  
  void setup_watches(SysInt var, SysInt val, SysInt lit)
  {
    SysInt domain_min = (tupleList->dom_smallest)[var];
    SysInt * tuple=regin->tuplelist[current_support[var][val+offset[var]]->get()]->values;
    
    DynamicTrigger* dt = dynamic_trigger_start();
    
    SysInt vars_size = vars.size();
    dt += lit * (vars_size - 1);
    for(SysInt v = 0; v < vars_size; ++v)
    {
      if(v != var)
      {
        vars[v].addDynamicTrigger(dt, DomainRemoval, tuple[v]);
        ++dt;
      }
    }
  }
  
  
  virtual void full_propagate()
  { 
    for(SysInt varIndex = 0; varIndex < vars.size(); ++varIndex) 
    {
      // Propagate variables so they fit inside domains. This is a minor fix
      SysInt tuple_domain_min = (tupleList->dom_smallest)[varIndex];
      SysInt tuple_domain_size = (tupleList->dom_size)[varIndex];
      
      vars[varIndex].setMin(tuple_domain_min);
      vars[varIndex].setMax(tuple_domain_min + tuple_domain_size);
      
      if(getState(stateObj).isFailed()) 
        return;
      
      DomainInt max = vars[varIndex].getMax();
      
      for(DomainInt i = vars[varIndex].getMin(); i <= max; ++i) 
      { 
        TupleH* _tuple=seekNextSupport(varIndex, i);
        
        SysInt sup=current_support[varIndex][i - tuple_domain_min]->get();
        if(_tuple==0)
        {
          vars[varIndex].removeFromDomain(i);
        }
        else
        {
          setup_watches(varIndex, i, tupleList->get_literal(varIndex, i));
        }
      }
    }
  }
  
  virtual BOOL check_assignment(DomainInt* v, SysInt v_size)
  {
    for(UnsignedSysInt i = 0; i < (tupleList)->size(); ++i)
    {
      if( std::equal(v, v + size, (*tupleList)[i]) )
        return true;
    }
    return false;
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> anyvars;
    for(UnsignedSysInt i = 0; i < vars.size(); ++i)
      anyvars.push_back(vars[i]);
    return anyvars;
  }
  
};


template<typename VarArray>
AbstractConstraint*
GACTableCon(const VarArray& vars, TupleList* tuples)
{ return new GACTableConstraint<VarArray>(vars, tuples); }

inline Regin* TupleList::getRegin()
{
  if(regin == NULL)
    regin = new Regin(this);
  return regin;
}

#endif
