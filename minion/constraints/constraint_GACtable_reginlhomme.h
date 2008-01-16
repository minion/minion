/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id$
*/

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
  BOOL operator()(const vector<int>& tuple1, const vector<int>& tuple2)
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

struct TupleH
  {
    // tuple class for Regin/Lhomme's bounding and jumping algorithm
    int id;  // global array index (can also be used for lex comparison of two tuples.)
    // no need for nextPointer
    int arity;
    
    int * values;
    int * nextValue;    // int index into global array, pointing to the next 
    
    int * redundantValues;
    int * redundantNextValue;
    
    TupleH(int * _values, int * _redundantValues, int _id, int _arity)
    {
        values=_values;
        redundantValues=_redundantValues;
        id=_id;
        arity=_arity;
        
        nextValue=new int[arity];
        redundantNextValue=new int[arity];
        for(int i=0; i<arity; i++)
        {
            nextValue[i]=-1;
            redundantNextValue[i]=-1;
        }
    }
  };

template<typename VarArray>
struct GACTableConstraint : public DynamicConstraint
{
  virtual string constraint_name()
  { return "TableRegin"; }
  
  typedef typename VarArray::value_type VarRef;
  VarArray vars;
  
  /// For each literal, the number of the tuple that supports it.
  // This is bad because it might have holes in it, i.e. revints that are not used.
  
  ReversibleInt *** current_support;
  //vector<vector<ReversibleInt> > current_support;  // current_support[var][val+offset[var]];
  
  vector<int> offset;
  
  /// Total number of literals in the variables at the start of search.
  int literal_num;
  
  vector<int> _map_literal_to_var;
  vector<int> _map_literal_to_val;

  int get_var_from_literal(int literal) 
  { return _map_literal_to_var[literal]; }

  int get_val_from_literal(int literal) 
  { return _map_literal_to_val[literal]; }
  
  /// Check if all allowed values in a given tuple are still in the domains of the variables.
  BOOL check_tuple(const vector<int>& v)
  {
	for(unsigned i = 0; i < v.size(); ++i)
	{
	  if(!vars[i].inDomain(v[i]))
		return false;
	}
	return true;
  }
  
  BOOL check_tuple(int * v)
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
  
  TupleH ** tuplelist;
  vector<vector<int> > tuples;
  BOOL listdone;
  
  int noTuples;
  int arity;
  int * upperboundtuple;
  
  
  GACTableConstraint(const VarArray& _vars, const vector<vector<int> >& _tuples) :
	vars(_vars),
    tuples(_tuples)
  {
	  arity = tuples[0].size();	  
	  D_ASSERT(_vars.size() == arity);
	  noTuples=tuples.size();
      //current_support.resize(arity); 
      
      // sort, required for correctness.
      sort(tuples.begin(), tuples.end(), TupleComparator(0, arity));
      
      current_support=new ReversibleInt**[arity];
      
      listdone=false;
      literal_num = 0;
      
      for(unsigned i = 0; i < arity; ++i)
      {
        literal_num += (vars[i].getInitialMax() - vars[i].getInitialMin() + 1);
        // cout << "initialMax: " << vars[i].getInitialMax() ;
        // cout << "initialMin: " << vars[i].getInitialMin() << endl;
      }
      
      offset.resize(arity);
      for(int i=0; i<arity; i++)
      {
        offset[i]=-vars[i].getInitialMin();
        
        current_support[i]= new ReversibleInt*[vars[i].getInitialMax()+offset[i]+1];
        
        //current_support[i].resize(vars[i].getInitialMax()+offset[i]+1);  // do the reversibleints get set up properly???
        
        for(int j=0; j<vars[i].getInitialMax()+offset[i]+1; j++){
            current_support[i][j]=new ReversibleInt();
            current_support[i][j]->set(-1);
        }
      }
      upperboundtuple=new int[arity];
      
      tuplelist=new TupleH*[tuples.size()];
  }
  
  int dynamic_trigger_count()
  { return literal_num * ( vars.size() - 1) ; }
  
  // set up the list
  void setuplist()
  {
    
    listdone=true;
    
    int * redvalues=new int[arity];
        for(int i=0; i<arity; i++){ 
            redvalues[i]=vars[i].getInitialMin();
        }
        
        for(int i=0; i<tuples.size(); i++)
        {
            // copy redvalues
            int * newredvalues= new int[arity]; 
            for(int j=0; j<arity; j++){ 
                newredvalues[j]=redvalues[j];
            }
            
            int * valuesarray= new int[arity];
            for(int j=0; j<arity; j++){
                valuesarray[j]=tuples[i][j];
            }
            
            TupleH * t=new TupleH(valuesarray, newredvalues, i, arity);
            tuplelist[i]=t;
            
            // cross-link with previous tuples.
            int valcountlocal=arity;  // for each value in this tuple, there is one forward reference in nextValue.
            
            for(int j=i-1; j>=0; j--)
            {
                TupleH* prev=tuplelist[j];
                BOOL breakflag=false;
                
                for(int var=0; var<arity; var++)
                {
                    if((prev->values[var])==(t->values[var]) && prev->nextValue[var]==-1)
                    {
                        prev->nextValue[var]=i;
                        valcountlocal--;
                    }
                    if(prev->redundantValues[var]==t->values[var] && prev->redundantNextValue[var]==-1)
                    {
                        prev->redundantNextValue[var]=i;
                    }
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
            for(int var=0; var<arity; var++)
            {
                redvalues[var]++;
                
                while(!(redvalues[var]>vars[var].getMax()) && !vars[var].inDomain(redvalues[var]))
                {
                    redvalues[var]++;
                }
                if(redvalues[var]>vars[var].getMax())
                {
                    redvalues[var]=vars[var].getMin();
                }
            }
        }
    /*cout << "List of tuples" <<endl;
    for(int i=0; i<tuples.size(); i++)
    {
        TupleH* temp=tuplelist[i];
        for(int j=0; j<arity; j++)
        {
            cout << temp->values[j] << ",";
        }
        cout <<endl << "nextValue: ";
        for(int j=0; j<arity; j++)
        {
            cout << temp->nextValue[j] << ",";
        }
    }*/
  }
  
  TupleH* seekNextSupport(int var, int val)
    {
        int last_pointer=current_support[var][val+offset[var]]->get();
        //System.out.println("Finding support for var:"+var+" val:"+val);
        //cout << "seekNextSupport var:" <<var << " val:" <<val <<endl;
        
        int nxvalid=0;
        BOOL testmode=false;
        D_ASSERT(testmode=true);  // Ha! Only tests when assertions are enabled.
        if(testmode)
        {
            while(nxvalid<tuples.size() && (!check_tuple(tuplelist[nxvalid]->values) || tuplelist[nxvalid]->values[var]!=val))
                nxvalid++;
            if(nxvalid==tuples.size())
                nxvalid=-1;
        }
        
        // for other variables, compute the max (over vars) of the min (over vals)
        // which gives the min lower bound of tuples that have been checked already.
        int lowerbound=last_pointer;
        for(int i=0; i<arity; i++)
        {
            if(i!=var)
            {
                int minlb=current_support[i][vars[i].getMin()+offset[i]]->get();
                
                for(int valIndex=vars[i].getMin()+1; valIndex<=vars[i].getMax(); valIndex++)
                {
                    if(vars[i].inDomain(valIndex))
                    {
                        int thisbound=current_support[i][valIndex+offset[i]]->get();
                        
                        if(thisbound<minlb) minlb=thisbound;  // even if thisbound==-1.
                    }
                }
                if(minlb>lowerbound)
                {
                    lowerbound=minlb;
                }
            }
        }  // can't do this because we don't use tuples from the checking of other variables to support this one.??
        
        // now lowerbound contains the index of the tuple which is the greatest checked so far.
        //System.out.println("lowerbound:"+lowerbound);
        // compute the upperbound -- there has to be a better upper bound than this.
        
        for(int i=0; i<arity; i++)
        {
            if(i!=var)
            {
                upperboundtuple[i]=vars[i].getMax();
            }
            else
            {
                upperboundtuple[i]=val;
            }
        }
        
        /*cout <<"Upperboundtuple:" ;
        for(int i=0; i<arity; i++)
        {
            cout << upperboundtuple[i] << ", ";
        }cout <<endl;*/
        
        // now find the next one from lowerbound which contains (var, val)
        int curtupleIndex;
        
        if(lowerbound==-1)
        {
            curtupleIndex=nextin(var, val, 0);
        }
        else
        {
            curtupleIndex=nextin(var, val, lowerbound);
        }
        if(curtupleIndex==-1)
        {   // off the end of the list
            D_ASSERT(nxvalid==-1);
            return 0;
        }
        
        TupleH* curtuple=tuplelist[curtupleIndex];
        
        if(comparetuples(curtuple->values, upperboundtuple)>0)
        {
            //System.out.println("Bigger than upperboundtuple");
            D_ASSERT(nxvalid==-1);
            return 0;
        }
        
        while(!check_tuple(curtuple->values))
        {
            curtupleIndex=curtuple->nextValue[var];  //  curtuple=NEXT((x,a), curtuple);
            if(curtupleIndex==-1){
                D_ASSERT(nxvalid==-1);
                return 0;
            }
            
            int maxjump=curtupleIndex;
            for(int y=0; y<arity; y++)
            {
                if(y!=var)
                {
                    int b=vars[y].getMin();
                    int off=offset[y];
                    int ltp=current_support[y][b+off]->get();
                    //ltp=0; // surely we can't jump based on other current_supports??
                    int nextinminallvals=nextin(y, b, (ltp>curtupleIndex)?ltp:curtupleIndex);
                    for(b=vars[y].getMin()+1; b<=vars[y].getMax(); b++)
                    {
                        if(vars[y].inDomain(b))
                        {
                            ltp=current_support[y][b+off]->get();
                            //ltp=0;
                            int temp=nextin(y, b, (ltp>curtupleIndex)?ltp:curtupleIndex);
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
                //System.out.println("Jumping forward to: "+maxjump);
                curtupleIndex=maxjump;
            }
            
            curtuple=tuplelist[curtupleIndex];
            
            if(comparetuples(curtuple->values, upperboundtuple)>0)  // this wouldn't be necessary if I got an index for upperboundtuple.
            {
                D_ASSERT(nxvalid==-1);
                return 0;
            }
        }
        current_support[var][val+offset[var]]->set(curtupleIndex);
        
        /*cout << "Support for var:"<<var<< " and value:"<<val << " found:";
        for(int i=0; i<arity; i++){
            cout<<curtuple->values[i] <<", ";
        } cout<<endl;*/
        
        //System.out.println("Support found:"+new tuple(curtuple.values));
        D_ASSERT(curtupleIndex==nxvalid);
        return curtuple;
    }
  
  int nextin(int var, int val, int curtuple)
  { // returns curtuple if curtuple contains var,val. So not strictly 'next'. If there is not one, returns -1.
    TupleH* temp=tuplelist[curtuple];
    
    while(temp->values[var]!=val)
    {
        if(temp->redundantValues[var]==val)
        {
            D_ASSERT( temp->redundantNextValue[var]==-1 || tuplelist[temp->redundantNextValue[var]]->values[var]==val);
            return temp->redundantNextValue[var];
        }
        curtuple++;
        if(curtuple>=noTuples) return -1;
        temp=tuplelist[curtuple];
    }
    D_ASSERT( curtuple==-1 || tuplelist[curtuple]->values[var]==val);
    return curtuple;
  }
  
  // End of regin-lhomme code.
  
  BOOL find_new_support(int literal)
  {
	 TupleH* new_support = seekNextSupport(get_var_from_literal(literal), get_val_from_literal(literal));
           
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
	
	BOOL is_new_support = find_new_support(propogated_literal);
	int varIndex = get_var_from_literal(propogated_literal);
	if(is_new_support)
	{
	  D_INFO(1, DI_TABLECON, "Found new support!");
      // cout << "Found new support:" << get_var_from_literal(propogated_literal) << " val:" << get_val_from_literal(propogated_literal) <<endl;
	  setup_watches(varIndex, propogated_literal);
	}
	else
	{
	  D_INFO(1, DI_TABLECON, "Failed to find new support");
      //cout << "Did not find new support:" << get_var_from_literal(propogated_literal) << " val:" << get_val_from_literal(propogated_literal) <<endl;
	  vars[varIndex].removeFromDomain(get_val_from_literal(propogated_literal));
	}
  }
  
  void setup_watches(int var, int lit)
  {
        // cout << "setup_watches lit= "<< lit << endl ; cout << "calling reconstructTuple from setup_watches" << endl ; 
        //cout << "current_support entry:" << current_support[var][get_val_from_literal(lit)+offset[var]]->get() << endl;
        
        int * tuple=tuplelist[current_support[var][get_val_from_literal(lit)+offset[var]]->get()]->values;
    
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
      int literal = 0;
      //cout << "full propagate: " ;
      if(!listdone){
            // compute the list
            setuplist();
      }
      
      _map_literal_to_var.resize(literal_num);      // may not need this many (see comment below)
      _map_literal_to_val.resize(literal_num);
      for(int varIndex = 0; varIndex < vars.size(); ++varIndex) 
      {
        int max = vars[varIndex].getMax();
        for(int i = vars[varIndex].getMin(); i <= max; ++i) 
        { 
            TupleH * _tuple=seekNextSupport(varIndex, i);
            
            int sup=current_support[varIndex][i+offset[varIndex]]->get();
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
                
                _map_literal_to_var[literal] = varIndex;
                _map_literal_to_val[literal] = i;
                //cout<<"calling setup_watches with var "<< varIndex << " val:" << i << "and literal:"<<literal<<" with lookup:"<<get_val_from_literal(literal) << endl;
                setup_watches(varIndex, literal);
                
            }
            ++literal;   // would like to put this inside else to save space, but can lead 
                      // to bugs I don't want to cope with just now.
        }
      }
      // cout << endl; cout << "  fp: finished finding supports: " << endl ;
  }
  
  virtual BOOL check_assignment(vector<int> v)
  {
   for(unsigned i = 0; i < tuples.size(); ++i)
    {
      if(v == tuples[i]) return true;
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
GACTableCon(const VarArray& vars, const vector<vector<int> >& tuples)
{ return new GACTableConstraint<VarArray>(vars, tuples); }

template <typename T>
DynamicConstraint*
BuildCT_WATCHED_TABLE(const T& t1, BOOL reify, const BoolVarRef& reifyVar, ConstraintBlob& b)
{ 
  if(reify) 
  { 
    cerr << "Cannot reify 'watched literal' constraints. Sorry." << endl; 
	exit(0); 
  } 
  else 
  { return GACTableCon(t1, b.tuples); } 
}
