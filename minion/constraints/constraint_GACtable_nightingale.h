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

#ifndef CONSTRAINT_GACTABLE_NIGHTINGALE_H
#define CONSTRAINT_GACTABLE_NIGHTINGALE_H

struct TupleComparator
{
  SysInt significantIndex;
  SysInt arity;

  TupleComparator(SysInt i, SysInt a) : significantIndex(i), arity(a)
  { }

  // returns tuple1 <= tuple2 under our ordering.
  bool operator()(const vector<DomainInt>& tuple1, const vector<DomainInt>& tuple2)
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

struct TupleN
{
  // tuple class for nightingale's
  SysInt id;  // global array index (can also be used for lex comparison of two tuples.)
           // no need for nextPointer

  SysInt * values;
  SysInt * nextDifferent;    // SysInt index into global array, pointing to the next

  TupleN(SysInt * _values, SysInt _id, SysInt arity, SysInt* _nD) :
    id(_id), values(_values), nextDifferent(_nD)
  {
    for(SysInt i = 0; i < arity; i++)
      nextDifferent[i] = -1;
  }

  TupleN() : id(-1), values(NULL), nextDifferent(NULL)
  { }
};

#ifdef LISTPERLIT
#define listperliteral true
#else
#define listperliteral false
#endif

struct Nightingale
{

 /// Total number of literals in the variables at the start of search.
  SysInt literal_num;

  TupleN * tuplelist;
  TupleN *** tuplelistperlit;

  SysInt ** tuplelistlengths;

  SysInt noTuples;
  SysInt arity;

  TupleList* tuples;
  Nightingale(TupleList* _tuples) : tuples(_tuples)
  {
    tuples->finalise_tuples();

    arity = tuples->tuple_size();
    noTuples = tuples->size();
    if(listperliteral)
    {
      tuplelistperlit=new TupleN**[arity];

      vector<vector<vector<vector<DomainInt> > > > goods;
      // Pass goods to splittuples just to avoid copying it on return.
      splittuples(tuples, goods);
      tuplelistlengths = new SysInt*[arity];

      for(SysInt i = 0; i < arity; i++)
      {
        SysInt varmin = (tuples->dom_smallest)[i];
        SysInt varmax = (tuples->dom_smallest)[i] + (tuples->dom_size)[i];
        SysInt domsize = (tuples->dom_size)[i];
        tuplelistperlit[i]=new TupleN*[domsize];
        tuplelistlengths[i]=new SysInt[domsize];

        for(SysInt val = varmin; val < varmax; val++)
        {
          TupleN* tlpl=buildhologram(goods[i][val-varmin]);
          tuplelistperlit[i][val-varmin]=tlpl;
          tuplelistlengths[i][val-varmin]=goods[i][val-varmin].size();
        }
      }
    }
    else
    {
      tuplelist = buildhologram(*tuples);
    }
  }

  void printhologram(TupleN* tlpl, SysInt size)
  {
    for(SysInt i = 0; i < size; ++i)
    {
      for(SysInt j = 0; j < arity; ++j)
        printf("%d,",tlpl[i].values[j]);
      printf("\n");

      for(SysInt j = 0; j < arity; ++j)
        printf("%d,",tlpl[i].nextDifferent[j]);
      printf("\n\n");
    }
  }

  template<typename T>
  TupleN* buildhologram(T& tupleref)
  {
    // turn a list of SysInt [] into an n-holo
    TupleN* tlist=new TupleN[tupleref.size()];

    SysInt* mem_block = new SysInt[arity * 2 * tupleref.size()];
    for(SysInt tupleIndex = 0; tupleIndex < (SysInt)tupleref.size(); tupleIndex++)
    {
      SysInt* _values = mem_block + arity * (tupleIndex * 2) ;

      // This line is messy, but is here because we want this code to work
      // for both vector<vector<DomainInt> >s and tuple containers. I'll clean it up
      // sometime.
      std::copy(&tupleref[tupleIndex][0], &tupleref[tupleIndex][0] + arity, _values);

      tlist[tupleIndex] = TupleN(_values, tupleIndex, arity,
                                 mem_block + arity * (tupleIndex * 2 + 1) );
      // Now iterate backwards through the tuplelist, setting the appropriate forward pointers
      SysInt numproc=arity;

      // check how many are the same for the last tuple.
      if(tupleIndex>=1)
      {
        for(SysInt valIndex=0; valIndex<arity; valIndex++)
        {
          if(_values[valIndex]==tlist[tupleIndex-1].values[valIndex])
            numproc--;
        }
      }

      for(SysInt i = tupleIndex - 1; i >= 0; i--)
      {
        TupleN* backtuple = &tlist[i];
        // if backtuple has a value i which is different to curtuple, make the forward link.

        // fill in any entries in nextDifferent
        for(SysInt valIndex = 0; valIndex < arity; valIndex++)
        {
          if(backtuple->nextDifferent[valIndex]==-1)
          {
            if(backtuple->values[valIndex]!=_values[valIndex])
            {
              numproc--;
              backtuple->nextDifferent[valIndex]=tupleIndex;
              // now iterate backwards and fill in any others in the same column
              for(SysInt j = i - 1; j >= 0; j--)
              {
                if(tlist[j].nextDifferent[valIndex]==-1)
                  tlist[j].nextDifferent[valIndex]=tupleIndex;
                else
                  break;
              }
            }
          }
        }

        if(numproc==0)
        {
          break;  // suspicious about this.
                  //What about 1 1 5, 1 2 5, 1 2 6. var 3, nextD only set for second tuple, not first.
        }
      }
    }
    return tlist;
  }

  void splittuples(TupleList* tuples, vector<vector<vector<vector<DomainInt> > > >& goods)
  {
    SysInt arity = tuples->tuple_size();
    goods.resize(arity);
    for(SysInt var = 0; var < arity; var++)
    {
      goods[var].resize((tuples->dom_size)[var]);
      for(SysInt val = (tuples->dom_smallest)[var];
          val <= (tuples->dom_smallest)[var] + (tuples->dom_size)[var]; val++)
      {
        for(SysInt tupleindex = 0; tupleindex < (SysInt)tuples->size(); tupleindex++)
        {
          if((*tuples)[tupleindex][var] == val)
            goods[var][val-(tuples->dom_smallest)[var]].push_back(tuples->get_vector(tupleindex));
        }
      }

      SysInt tuple_sum = 0;
      for(SysInt i = 0; i < (SysInt)goods[var].size(); ++i)
        tuple_sum += goods[var][i].size();
      D_ASSERT(tuple_sum == tuples->size());
    }
  }

};



template<typename VarArray>
struct GACTableConstraint : public AbstractConstraint
{
  virtual string extended_name()
  { return "table(nightingale)"; }

  virtual string constraint_name()
  { return "table"; }

  CONSTRAINT_ARG_LIST2(vars, tuples);


  typedef typename VarArray::value_type VarRef;
  VarArray vars;
  TupleList* tuples;

  /// For each literal, the number of the tuple that supports it.
  // This is bad because it might have holes in it, i.e. revints that are not used.

  SysInt ** current_support;

  /// Check if all allowed values in a given tuple are still in the domains of the variables.
  bool check_tuple(const vector<DomainInt>& v)
  {
    for(UnsignedSysInt i = 0; i < v.size(); ++i)
    {
      if(!vars[i].inDomain(v[i]))
        return false;
    }
    return true;
  }

  bool check_tuple(SysInt * v)
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
    for(SysInt i = 0; i < arity; i++)
    {
      if(t1[i] > t2[i])
        return 1;
      if(t1[i] < t2[i])
        return -1;
    }
    return 0;
  }

  SysInt arity;

  Nightingale* nightingale;

  GACTableConstraint(const VarArray& _vars, TupleList* _tuples) :
    vars(_vars), tuples(_tuples)
  {
      nightingale = tuples->getNightingale();
      arity = nightingale->tuples->tuple_size();
      D_ASSERT(_vars.size() == arity);
      current_support=new SysInt*[arity];

      for(SysInt i=0; i<arity; i++)
      {
        current_support[i]= new SysInt[(tuples->dom_size)[i]];
        for(SysInt j=0; j<(tuples->dom_size)[i]; j++)
          current_support[i][j]=-1;
      }
  }

  virtual SysInt dynamic_trigger_count()
  { return (nightingale->tuples->literal_num) * ( (SysInt)vars.size() - 1) ; }

  TupleN* seekNextSupport(SysInt var, SysInt val)
  {
    // find a support which conforms to var and val, and the current domains,
    // and is after the support in watches unless we reach the end and wrap.
    // Else return null.

    SysInt domain_min = (nightingale->tuples->dom_smallest)[var];
    SysInt domain_max = domain_min + (nightingale->tuples->dom_size)[var];
    if(val >= domain_max)
      return 0;
    SysInt ltp = current_support[var][val-domain_min];  // watches must be indexed by the actual value.

    SysInt index = ltp;
    if(index == -1)
      index=0;

    // select the list to search through.
    TupleN * tuplelisthere;
    SysInt listlength;
    if(listperliteral){
      tuplelisthere=nightingale->tuplelistperlit[var][val-domain_min];
      listlength=nightingale->tuplelistlengths[var][val-domain_min];
    }
    else {
      tuplelisthere=nightingale->tuplelist;
      listlength=nightingale->noTuples;
    }


    while(index<listlength && index!=-1)
    {
      TupleN& curtuple=tuplelisthere[index];
      // iterate from most to least significant digit
      // because most sig digit probably allows greatest jump.
      // Remember that var gets treated specially, as if its domain is just {val}

      bool matchAll=true;
      for(SysInt valIndex=0; valIndex<arity; valIndex++)
      {
        SysInt curvalue=curtuple.values[valIndex];

        if( (valIndex!=var && !vars[valIndex].inDomain(curvalue)) ||
            (valIndex==var && curvalue!=val))
        {
          matchAll=false;
          index=curtuple.nextDifferent[valIndex];
          break;
        }
      }

      if(matchAll)
      {
        // success
        // set watch
        current_support[var][val-domain_min]=index;
        //System.out.println("Support found:"+new tuple(curtuple.values));
        return &curtuple;
      }
    }


    index = 0;

    while(index<=ltp && index!=-1)
    {
      TupleN& curtuple=tuplelisthere[index];
      // iterate from most to least significant digit
      // because most sig digit probably allows greatest jump.
      // Remember that var gets treated specially, as if its domain is just {val}

      bool matchAll=true;
      for(SysInt valIndex=0; valIndex<arity; valIndex++)
      {
        SysInt curvalue=curtuple.values[valIndex];

        if( (valIndex!=var && !vars[valIndex].inDomain(curvalue)) ||
            (valIndex==var && curvalue!=val))
        {
          matchAll=false;
          index=curtuple.nextDifferent[valIndex];
          break;
        }
      }

      if(matchAll)
      {
        // success
        // set watch
        current_support[var][val-domain_min]=index;
        //System.out.println("Support found:"+new tuple(curtuple.values));
        return &curtuple;
      }
    }
    return 0;
  }

  // Below is shared with regin-lhomme file.

  bool find_new_support(SysInt literal)
  {
     pair<DomainInt, DomainInt> varval = nightingale->tuples->get_varval_from_literal(literal);
     SysInt var = varval.first;
     SysInt val = varval.second;
     TupleN* new_support = seekNextSupport(var,val);

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
    SysInt propagated_literal = trigger_pos / ((SysInt)vars.size() - 1);

    BOOL is_new_support = find_new_support(propagated_literal);

    pair<DomainInt, DomainInt> varval = nightingale->tuples->get_varval_from_literal(propagated_literal);
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
    SysInt domain_min = (nightingale->tuples->dom_smallest)[var];
    SysInt* tuple;
    if(!listperliteral)
      tuple=nightingale->tuplelist[current_support[var][val-domain_min]].values;
    else
      tuple=nightingale->tuplelistperlit[var][val-domain_min][current_support[var][val-domain_min]].values;

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
    for(SysInt varIndex = 0; varIndex < (SysInt)vars.size(); ++varIndex)
    {
      // Propagate variables so they fit inside domains. This is a minor fix
      SysInt tuple_domain_min = (nightingale->tuples->dom_smallest)[varIndex];
      SysInt tuple_domain_size = (nightingale->tuples->dom_size)[varIndex];

      vars[varIndex].setMin(tuple_domain_min);
      vars[varIndex].setMax(tuple_domain_min + tuple_domain_size);

      if(getState().isFailed())
        return;

      DomainInt max = vars[varIndex].getMax();

      for(DomainInt i = vars[varIndex].getMin(); i <= max; ++i)
      {
        TupleN* _tuple=seekNextSupport(varIndex, i);

        SysInt sup=current_support[varIndex][i - tuple_domain_min];
        //cout <<sup<<endl;
        // cout << "    var " << varIndex << " val: " << i << " sup " << sup << " " << endl;
        if(_tuple==0)
        {
          //cout <<"no support found for var:"<<varIndex<< " and val:"<< i <<endl ;
          vars[varIndex].removeFromDomain(i);
        }
        else
        {
          setup_watches(varIndex, i, nightingale->tuples->get_literal(varIndex, i));
        }
      }
    }
    // cout << endl; cout << "  fp: finished finding supports: " << endl ;
  }

  virtual BOOL check_assignment(DomainInt* v, SysInt v_size)
  {
    for(UnsignedSysInt i = 0; i < (nightingale->tuples)->size(); ++i)
    {
      if( std::equal(v, v + size, (*nightingale->tuples)[i]) )
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

inline Nightingale* TupleList::getNightingale()
{
  if(nightingale == NULL)
    nightingale = new Nightingale(this);
  return nightingale;
}

#endif
