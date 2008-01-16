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




struct TupleComparator
{
  int significantIndex;
  int arity;
  
  TupleComparator(int i, int a)
  {
    significantIndex = i;
    arity = a; 
  }
  
  // returns tuple1 <= tuple2 under our ordering.
  bool operator()(const vector<int>& tuple1, const vector<int>& tuple2)
  {
    if(tuple1[significantIndex] != tuple2[significantIndex])
	  return tuple1[significantIndex] < tuple2[significantIndex];
    for(int tupleIndex = 0; tupleIndex < arity; tupleIndex++)
    {
      if(tuple1[tupleIndex] != tuple2[tupleIndex])
        return tuple1[tupleIndex] < tuple2[tupleIndex];
    }
	return true;
  }
};



struct TupleN
{
    // tuple class for nightingale's
    int id;  // global array index (can also be used for lex comparison of two tuples.)
    // no need for nextPointer
    int arity;
    
    int * values;
    int * nextDifferent;    // int index into global array, pointing to the next 
    
    TupleN(int * _values, int _id, int _arity)
    {
        values=_values;
        id=_id;
        arity=_arity;
        
        nextDifferent=new int[arity];
        for(int i=0; i<arity; i++)
        {
            nextDifferent[i]=-1;
        }
    }
};

#ifdef LISTPERLIT
#define listperliteral  true
#else
#define listperliteral false
#endif
  
struct Nightingale
{

 /// Total number of literals in the variables at the start of search.
   int literal_num;
  
//  vector<int> _map_literal_to_var;
//  vector<int> _map_literal_to_val;
  
//  int get_var_from_literal(int literal) 
//  { return _map_literal_to_var[literal]; }

//  int get_val_from_literal(int literal) 
//  { return _map_literal_to_val[literal]; }
  
  TupleN ** tuplelist;
  TupleN **** tuplelistperlit;
  
  int ** tuplelistlengths;

 int noTuples;
  int arity;
  

  TupleList* tuples;
  Nightingale(TupleList* _tuples) : tuples(_tuples)
  {
    tuples->finalise_tuples();
    arity = tuples->tuple_size();	
    noTuples = tuples->size();
    /* if(listperliteral)
        {
            tuplelistperlit=new TupleN***[arity];
            
            vector<vector<vector<vector<int> > > > goods;
			// Pass goods to splittuples just to avoid copying it on return.
			splittuples(tuples, goods);
            tuplelistlengths=new int*[arity];
            
            for(int i=0; i<arity; i++)
            {
			    int varmin = (tuples->dom_smallest)[i];
				int varmax = (tuples->dom_smallest)[i] + (tuples->dom_size)[i];
				int domsize = (tuples->dom_size)[i];
                tuplelistperlit[i]=new TupleN**[domsize];
                tuplelistlengths[i]=new int[domsize];
                
                for(int val=varmin; val < varmax ; val++)
                {   
                    TupleN** tlpl=buildhologram(goods[i][val-varmin]);
                    tuplelistperlit[i][val-varmin]=tlpl;
                    tuplelistlengths[i][val-varmin]=goods[i][val-varmin].size();
                }
            }
        }
        else*/
        {
            tuplelist=buildhologram(tuples);
        }
  
  }
  
  TupleN ** buildhologram(TupleList* _input_tuples)
  {
    TupleList& tupleref = *_input_tuples;
    // turn a list of int [] into an n-holo
    TupleN ** tlist=new TupleN*[tupleref.size()];
        
        for(int tupleIndex = 0; tupleIndex < tupleref.size(); tupleIndex++)
        {
            const vector<int>& values = tupleref.get_vector(tupleIndex);
            
            int * _values=new int[arity];
			std::copy(values.begin(), values.begin() + arity, _values);
//            for(int i=0; i<arity; i++){
//              _values[i]=values[i];
//            }
            
            tlist[tupleIndex]=new TupleN(_values, tupleIndex, arity);
            if(true)
            {
                // Now iterate backwards through the tuplelist, setting the appropriate forward pointers
                int numproc=arity;
                
                // check how many are the same for the last tuple.
                if(tupleIndex>=1)
                {
                for(int valIndex=0; valIndex<arity; valIndex++)
                {
                    if(values[valIndex]==tlist[tupleIndex-1]->values[valIndex])
                    {
                        numproc--;
                    }
                }
                }
                
                for(int i=tupleIndex-1; i>=0; i--)
                {
                    TupleN* backtuple=tlist[i];
                    // if backtuple has a value i which is different to curtuple, make the forward link.
                    
                    // fill in any entries in nextDifferent
                    for(int valIndex=0; valIndex<arity; valIndex++)
                    {
                        if(backtuple->nextDifferent[valIndex]==-1)
                        {
                            if(backtuple->values[valIndex]!=values[valIndex])
                            {
                                numproc--;
                                backtuple->nextDifferent[valIndex]=tupleIndex;
                                // now iterate backwards and fill in any others in the same column
                                for(int j=i-1; j>=0; j--)
                                {
                                    if(tlist[j]->nextDifferent[valIndex]==-1)
                                    {
                                        tlist[j]->nextDifferent[valIndex]=tupleIndex;
                                    }
                                    else
                                    {
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    
                    if(numproc==0)//allfilled)
                    {
                        break;  // suspicious about this. 
                        //What about 1 1 5, 1 2 5, 1 2 6. var 3, nextD only set for second tuple, not first.
                    }
                }
            }
        }
        return tlist;
  }
  /*
     void splittuples(TupleList* alltuples, vector<vector<vector<vector<int> > > >& goods)
    {
	    int arity = tupleslist->tuple_size();	
        goods.resize(arity);
        for(int var=0; var<arity; var++)
        {
            goods[var].resize(vars[var].getInitialMax()-vars[var].getInitialMin()+1);
            for(int val=vars[var].getInitialMin(); val<=vars[var].getInitialMax(); val++)
            {
                for(int tupleindex=0; tupleindex<alltuples.size(); tupleindex++)
                {
                    if(alltuples[tupleindex][var]==val)
                    {
                        // matching tuple
                        goods[var][val+offset[var]].push_back(alltuples[tupleindex]);
                    }
                }
            }
        }
    }
	*/
};



template<typename VarArray>
struct GACTableConstraint : public DynamicConstraint
{
  virtual string constraint_name()
  { return "TableNightingale"; }
  
  typedef typename VarArray::value_type VarRef;
  VarArray vars;
  
  /// For each literal, the number of the tuple that supports it.
  // This is bad because it might have holes in it, i.e. revints that are not used.
  
  int ** current_support;
  
  //bool listperliteral;
  
  //vector<int> offset;
  
  /// Total number of literals in the variables at the start of search.
  // int literal_num;
  
  //vector<int> _map_literal_to_var;
  //vector<int> _map_literal_to_val;
  
  //int get_var_from_literal(int literal) 
  //{ return _map_literal_to_var[literal]; }

  //int get_val_from_literal(int literal) 
  //{ return _map_literal_to_val[literal]; }
  
  /// Check if all allowed values in a given tuple are still in the domains of the variables.
  bool check_tuple(const vector<int>& v)
  {
	for(unsigned i = 0; i < v.size(); ++i)
	{
	  if(!vars[i].inDomain(v[i]))
		return false;
	}
	return true;
  }
  
  bool check_tuple(int * v)
  {
	for(unsigned i = 0; i < arity; ++i)
	{
	  if(!vars[i].inDomain(v[i]))
		return false;
	}
	return true;
  }
  
  int comparetuples(int * t1, int * t2)
  {
    for(int i=0; i<arity; i++)
    {
        if(t1[i]>t2[i])
        {
            return 1;
        }
        if(t1[i]<t2[i])
        {
            return -1;
        }
    }
    return 0;
  }
  
  //vector<vector<int> > tuplesbackup;
  // XXX remove this at some point.
 int arity;
  
  Nightingale* nightingale;
  
  GACTableConstraint(const VarArray& _vars, TupleList* tuples) :
	vars(_vars)
    //tuplesbackup(tuples)
    //tuples(_tuples)
  {
     nightingale = tuples->getNightingale();
	 
	  arity = nightingale->tuples->tuple_size();	 
	   
	  D_ASSERT(_vars.size() == arity);
	  
	  //noTuples=tuples.size();
      //current_support.resize(arity);
      
     // #ifdef LISTPERLIT
     //   listperliteral=true;
     // #else
     //   listperliteral=false;
     // #endif  
      
      // sort, required for correctness.
      //sort(tuplesbackup.begin(), tuplesbackup.end(), TupleComparator(0, arity));
      
      current_support=new int*[arity];
      
    /*  literal_num = 0;
      
      for(unsigned i = 0; i < arity; ++i)
      {
        literal_num += (vars[i].getInitialMax() - vars[i].getInitialMin() + 1);
        // cout << "initialMax: " << vars[i].getInitialMax() ;
        // cout << "initialMin: " << vars[i].getInitialMin() << endl;
      }*/
     // XXX : Base this off tuples? 
    //  offset.resize(arity);
      for(int i=0; i<arity; i++)
      {
        //offset[i]=-vars[i].getInitialMin();
        
        current_support[i]= new int[(tuples->dom_size)[i]];
        
        //current_support[i].resize(vars[i].getInitialMax()+offset[i]+1);  // do the reversibleints get set up properly???
        
        for(int j=0; j<(tuples->dom_size)[i]; j++){
            current_support[i][j]=-1;
        }
      }
      /*
      if(listperliteral)
        {
            tuplelistperlit=new TupleN***[arity];
            
            vector<vector<vector<vector<int> > > > goods;
			// Pass goods to splittuples just to avoid copying it on return.
			splittuples(tuplesbackup, goods);
            tuplelistlengths=new int*[arity];
            
            for(int i=0; i<arity; i++)
            {
                tuplelistperlit[i]=new TupleN**[vars[i].getInitialMax()-vars[i].getInitialMin()+1];
                tuplelistlengths[i]=new int[vars[i].getInitialMax()-vars[i].getInitialMin()+1];
                
                for(int val=vars[i].getInitialMin(); val<=vars[i].getInitialMax(); val++)
                {   
                    TupleN** tlpl=buildhologram(goods[i][val+offset[i]]);
                    tuplelistperlit[i][val+offset[i]]=tlpl;
                    tuplelistlengths[i][val+offset[i]]=goods[i][val+offset[i]].size();
                }
            }
        }
        else
        {
            tuplelist=buildhologram(tuplesbackup);
        }*/
        
  }
  
  int dynamic_trigger_count()
  { return (nightingale->tuples->literal_num) * ( vars.size() - 1) ; }
  
  /*
  TupleN ** buildhologram(const vector<vector<int> >& tuples)
  {
    // turn a list of int [] into an n-holo
    TupleN ** tlist=new TupleN*[tuples.size()];
        
        for(int tupleIndex = 0; tupleIndex < tuples.size(); tupleIndex++)
        {
            const vector<int>& values = tuples[tupleIndex];
            
            int * _values=new int[arity];
			std::copy(values.begin(), values.begin() + arity, _values);
//            for(int i=0; i<arity; i++){
//              _values[i]=values[i];
//            }
            
            tlist[tupleIndex]=new TupleN(_values, tupleIndex, arity);
            if(true)
            {
                // Now iterate backwards through the tuplelist, setting the appropriate forward pointers
                int numproc=arity;
                
                // check how many are the same for the last tuple.
                if(tupleIndex>=1)
                {
                for(int valIndex=0; valIndex<arity; valIndex++)
                {
                    if(values[valIndex]==tlist[tupleIndex-1]->values[valIndex])
                    {
                        numproc--;
                    }
                }
                }
                
                for(int i=tupleIndex-1; i>=0; i--)
                {
                    TupleN* backtuple=tlist[i];
                    // if backtuple has a value i which is different to curtuple, make the forward link.
                    
                    // fill in any entries in nextDifferent
                    for(int valIndex=0; valIndex<arity; valIndex++)
                    {
                        if(backtuple->nextDifferent[valIndex]==-1)
                        {
                            if(backtuple->values[valIndex]!=values[valIndex])
                            {
                                numproc--;
                                backtuple->nextDifferent[valIndex]=tupleIndex;
                                // now iterate backwards and fill in any others in the same column
                                for(int j=i-1; j>=0; j--)
                                {
                                    if(tlist[j]->nextDifferent[valIndex]==-1)
                                    {
                                        tlist[j]->nextDifferent[valIndex]=tupleIndex;
                                    }
                                    else
                                    {
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    
                    if(numproc==0)//allfilled)
                    {
                        break;  // suspicious about this. 
                        //What about 1 1 5, 1 2 5, 1 2 6. var 3, nextD only set for second tuple, not first.
                    }
                }
            }
        }
        return tlist;
  }
  */
  
  /*
   void splittuples(const vector<vector<int> >& alltuples, vector<vector<vector<vector<int> > > >& goods)
    {
        goods.resize(arity);
        for(int var=0; var<arity; var++)
        {
            goods[var].resize(vars[var].getInitialMax()-vars[var].getInitialMin()+1);
            for(int val=vars[var].getInitialMin(); val<=vars[var].getInitialMax(); val++)
            {
                for(int tupleindex=0; tupleindex<alltuples.size(); tupleindex++)
                {
                    if(alltuples[tupleindex][var]==val)
                    {
                        // matching tuple
                        goods[var][val+offset[var]].push_back(alltuples[tupleindex]);
                    }
                }
            }
        }
    }
  */
  
  TupleN* seekNextSupport(int var, int val)
  {
    //System.out.println("Finding support for var:"+var+" val:"+val);
    // find a support which conforms to var and val, and the current domains,
    // and is after the support in watches unless we reach the end and wrap.
    // Else return null.
    
	int domain_min = (nightingale->tuples->dom_smallest)[var];
    int ltp=current_support[var][val-domain_min];  // watches must be indexed by the actual value.
    
    int index=ltp; 
    if(index==-1){ index=0;}
    
    // select the list to search through.
    TupleN ** tuplelisthere;
    int listlength;
    if(listperliteral){
        tuplelisthere=nightingale->tuplelistperlit[var][val-domain_min];
        listlength=nightingale->tuplelistlengths[var][val-domain_min];
    }
    else {
        tuplelisthere=nightingale->tuplelist;
        listlength=nightingale->noTuples;
    }
    
    bool firstpass=true;
    for(int pass=0; pass<2; pass++)
    {
        firstpass=(pass==0)?true:false;
        if(!firstpass) index=0;
        while((firstpass && index<listlength && index!=-1) || (!firstpass && index<=ltp && index!=-1))
        {
            TupleN* curtuple=tuplelisthere[index];
            //System.out.println("Looking at tuple "+curtuple);
            // iterate from most to least significant digit
            // because most sig digit probably allows greatest jump.
            // Remember that var gets treated specially, as if its domain is just {val}
            
            bool matchAll=true;
            for(int valIndex=0; valIndex<arity; valIndex++)
            {
                int curvalue=curtuple->values[valIndex];
                
                if( (valIndex!=var && !vars[valIndex].inDomain(curvalue)) || 
                    (valIndex==var && curvalue!=val))
                {
                    matchAll=false;
                    if(true)  // hologramlist
                    {
                        // attempt to jump forward
                        index=curtuple->nextDifferent[valIndex];
                    }
                    else
                    {
                        index++;
                    }
                    break;
                }
            }
            
            if(matchAll)
            {
                // success
                // set watch
                current_support[var][val-domain_min]=index;
                //System.out.println("Support found:"+new tuple(curtuple.values));
                return curtuple;
            }
        }
    }
    
    return 0;
  }
  
  // Below is shared with regin-lhomme file.
  
  bool find_new_support(int literal)
  {
     pair<int,int> varval = nightingale->tuples->get_varval_from_literal(literal);
	 int var = varval.first;
	 int val = varval.second;
	 TupleN* new_support = seekNextSupport(var,val);
           
         if (new_support == 0)
         { // cout << "find_new_support failed literal: " << literal << " var: " << varIndex << " val: " << get_val_from_literal(literal) << endl ;
           
             return false;
         }
         // cout << "find_new_support sup= "<< new_support << " literal: " << literal << " var: " << varIndex << " val: " << get_val_from_literal(literal) << endl;
         return true;
  }
  
  DYNAMIC_PROPAGATE_FUNCTION(DynamicTrigger* propogated_trig)
  {
	D_INFO(1, DI_TABLECON, "Propogation Triggered: " + to_string(propogated_trig));
	DynamicTrigger* dt = dynamic_trigger_start();
	int trigger_pos = propogated_trig - dt;
	int propogated_literal = trigger_pos / (vars.size() - 1);
	
	bool is_new_support = find_new_support(propogated_literal);
	
	pair<int,int> varval = nightingale->tuples->get_varval_from_literal(propogated_literal);
	int varIndex = varval.first;
	int val = varval.second;

	
	if(is_new_support)
	{
	  D_INFO(1, DI_TABLECON, "Found new support!");
      // cout << "Found new support:" << get_var_from_literal(propogated_literal) << " val:" << get_val_from_literal(propogated_literal) <<endl;
	  setup_watches(varIndex, val, propogated_literal);
	}
	else
	{
	  D_INFO(1, DI_TABLECON, "Failed to find new support");
      //cout << "Did not find new support:" << get_var_from_literal(propogated_literal) << " val:" << get_val_from_literal(propogated_literal) <<endl;
	  vars[varIndex].removeFromDomain(val);
	}
  }
  
  void setup_watches(int var, int val, int lit)
  {
  int domain_min = (nightingale->tuples->dom_smallest)[var];
        // cout << "setup_watches lit= "<< lit << endl ; cout << "calling reconstructTuple from setup_watches" << endl ; 
        //cout << "current_support entry:" << current_support[var][get_val_from_literal(lit)+offset[var]] << endl;
        int * tuple;
        if(!listperliteral)
        {
            tuple=nightingale->tuplelist[current_support[var][val-domain_min]]->values;
        }
        else
        {
            tuple=nightingale->tuplelistperlit[var][val-domain_min][current_support[var][val-domain_min]]->values;
        }
    
	DynamicTrigger* dt = dynamic_trigger_start();
	
	int vars_size = vars.size();
	dt += lit * (vars_size - 1);
	for(int v = 0; v < vars_size; ++v)
	{
	  if(v != var)
	  {
		vars[v].addDynamicTrigger(dt, DomainRemoval, tuple[v]);
		++dt;
	  }
	}
  }
  
  
  virtual void full_propogate()
  { 
      //int literal = 0;
	  
      //cout << "full propagate: " ;
      
      //_map_literal_to_var.resize(literal_num);      // may not need this many (see comment below)
      //_map_literal_to_val.resize(literal_num);
      for(int varIndex = 0; varIndex < vars.size(); ++varIndex) 
      {
	    // Propagate variables so they fit inside domains. This is a minor fix
	    int tuple_domain_min = (nightingale->tuples->dom_smallest)[varIndex];
        int tuple_domain_size = (nightingale->tuples->dom_size)[varIndex];
		
		vars[varIndex].setMin(tuple_domain_min);
		vars[varIndex].setMax(tuple_domain_min + tuple_domain_size);
		
		if(Controller::failed) return;
		
		int max = vars[varIndex].getMax();
        
		for(int i = vars[varIndex].getMin(); i <= max; ++i) 
        { 
            TupleN* _tuple=seekNextSupport(varIndex, i);
            
            int sup=current_support[varIndex][i - tuple_domain_min];
            //cout <<sup<<endl;
            // cout << "    var " << varIndex << " val: " << i << " sup " << sup << " " << endl;
            if(_tuple==0)
            {
                D_INFO(2, DI_TABLECON, "No valid support for " + to_string(x) + " in var " + to_string(i));
                //cout <<"no support found for var:"<<varIndex<< " and val:"<< i <<endl ;
                vars[varIndex].removeFromDomain(i);
            }
            else
            {
               // _map_literal_to_var[literal] = varIndex;
               // _map_literal_to_val[literal] = i;
                //cout<<"calling setup_watches with var "<< varIndex << " val:" << i << "and literal:"<<literal<<" with lookup:"<<get_val_from_literal(literal) << endl;
                setup_watches(varIndex, i, nightingale->tuples->get_literal(varIndex, i));
                
            }
           // ++literal;   // would like to put this inside else to save space, but can lead 
                      // to bugs I don't want to cope with just now.
        }
      }
      // cout << endl; cout << "  fp: finished finding supports: " << endl ;
  }
  
  virtual bool check_assignment(vector<int> v)
  {
    for(unsigned i = 0; i < (nightingale->tuples)->size(); ++i)
	{
	    if( std::equal(v.begin(), v.end(), (*nightingale->tuples)[i]) )
	    return true;
	}
	return false;
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> anyvars;
    for(unsigned i = 0; i < vars.size(); ++i)
	  anyvars.push_back(vars[i]);
	return anyvars;
  }
};


template<typename VarArray>
DynamicConstraint*
GACTableCon(const VarArray& vars, TupleList* tuples)
{ return new GACTableConstraint<VarArray>(vars, tuples); }

inline Nightingale* TupleList::getNightingale()
{
  if(nightingale == NULL)
    nightingale = new Nightingale(this);
  return nightingale;
}