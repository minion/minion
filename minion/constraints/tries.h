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


#include <numeric>

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


//template<typename VarArray>
struct TupleTrie {
  //VarArray scope_vars;
  int arity ;
  int significantIndex ;
  int delim ;
  //vector<int> dom_size;
  //vector<int> offset;   // index array of values with [val+offset[var]]
  
  int* levelLengths ;
  int** trie ;
  
  int* last_tuple_pointer;
  
  TupleList* tuplelist;
  
  /* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	Constructor
  	Assumes tuples is non-empty and each tuple has same length.
    WCase: each level of trie has |tuples| elements.
	Tuples are first sorted lexicographically, then added to trie.
	%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */
  TupleTrie(/*const VarArray& _vars,*/
            const int _significantIndex, const int _delim, 
            /*vector<vector<int> > __tuples,
            const vector<int>& __dom_size,
            const vector<int>& __offset,*/ TupleList* _tuplelist) : 
	//scope_vars(_vars),
	significantIndex(_significantIndex), 
	delim(_delim),
	//dom_size(_dom_size),
    //offset(_offset),
	tuplelist(_tuplelist)
  {
	  int componentIndex, trieLevel, tupleIndex ;
	  int noTuples = tuplelist->size() ;
	  arity = tuplelist->tuple_size() ;
	  // Sort tuples for ease of addition to trie
	  //sort(tuples.begin(), tuples.end(), TupleComparator(significantIndex, arity));
	  
	  // JAVA was:
	  //      int[][] initTrie = new int[arity][tuples.length*4] ;       //val+2ptr+del
	  
	  int** initTrie = new int*[arity];
	  for(int i = 0;i < arity ; i++)
	  {
		initTrie[i] = new int[noTuples*4]; //val+2ptr+del
        // this next line is prob.not necessary
        for(int j = 0; j < noTuples*4; j++) 
		  initTrie[i][j]=delim; 
	  };
	  
	  // currElement, levelLengths(=currIndices in Java). Indexed by trie level.
	  int* currElement = new int[arity] ;
	  levelLengths = new int[arity] ;
	  for (int index = 0; index < arity; index++) {
		currElement[index] = delim ;
		levelLengths[index] = 0 ;
	  }
	  // Iterate over tuples, adding each to the trie.
	  for (tupleIndex = 0; tupleIndex < noTuples; tupleIndex++) {
		vector<int> tuple = tuplelist->get_vector(tupleIndex) ;
		// lev 0 = signif idx. No delimiters/back ptrs on level 0
        D_ASSERT(tuple[significantIndex] != delim);
		if (tuple[significantIndex] != currElement[0]) {
		  currElement[0] = tuple[significantIndex] ;
		  initTrie[0][levelLengths[0]++] = tuple[significantIndex] ;
		  if (arity > 1) {
			// Start new block at next level
			if (levelLengths[1] > 0)
			  initTrie[1][levelLengths[1]++] = delim ;
			// point to new block at next level
			initTrie[0][levelLengths[0]++] = levelLengths[1] ;
			// reset currElement at next level
			currElement[1] = delim ;
		  }
		}
		// Now look at rest of tuple
		for (trieLevel = 1; trieLevel < arity; trieLevel++) { 
		  componentIndex = ((trieLevel <= significantIndex) ?  
							trieLevel - 1 : trieLevel) ;
		    D_ASSERT(tuple[componentIndex] != delim);
            // Only modify this level if this is a new prefix
		  if (tuple[componentIndex] != currElement[trieLevel]) {
			currElement[trieLevel] = tuple[componentIndex] ;
			initTrie[trieLevel][levelLengths[trieLevel]++] =
			  tuple[componentIndex] ;
			if (trieLevel < (arity - 1)) {
			  // Start new block at next level
			  if (levelLengths[trieLevel+1] > 0)
				initTrie[trieLevel+1][levelLengths[trieLevel+1]++] = delim ;
			  // point to new block at next level
			  initTrie[trieLevel][levelLengths[trieLevel]++] =
				levelLengths[trieLevel+1] ;
			  // reset currElement at next level
			  currElement[trieLevel+1] = delim ;
			}
			// Point back to parent
			initTrie[trieLevel][levelLengths[trieLevel]++] =
			  levelLengths[trieLevel-1] - ((trieLevel==1) ? 2 : 3) ;
		  } // end of modified prefix test 
		} // end of trieLevel loop
	  } // end of tuple loop
		// Create final, immutable trie.
	  
	  trie = new int*[arity] ;
	  for (trieLevel = 0; trieLevel < arity; trieLevel++) 
	  {
		trie[trieLevel] = new int[levelLengths[trieLevel]] ;
		// JAVA Was: 
		// System.arraycopy(initTrie[trieLevel], 0, trie[trieLevel], 0, trie[trieLevel].size()) ;
		
		for (int i=0; i < levelLengths[trieLevel]; ++i) 
		{
		  trie[trieLevel][i] = initTrie[trieLevel][i];
		}
	  }
	  // COULD BE WRONG BELOW
	  
	  for(int i=0; i < arity; i++)
	  { delete[] (initTrie[i]); };
	  delete[] (initTrie);           
	  // Just incantations, sorry if I have it wrong.    Obviously this is not necessarily 
	  // optimal as we could keep this around for other constructs.  Not to worry.
      
      // added by pn, make a new array for the place we got up to,
      // index into the bottom level of the trie.
      last_tuple_pointer=new int[(tuplelist->dom_size)[significantIndex] + 1];
      for(int i = 0; i <= (tuplelist->dom_size)[significantIndex]; i++) 
	    last_tuple_pointer[i]=-1; 
      // can safely start at ltp+1
      minlevel=new int[arity];
  }
  
    template<typename VarArray>
    int nextSupportingTuple(int valToSupport, VarArray& vars)
    {
	    
        int valPtr=last_tuple_pointer[valToSupport-(tuplelist->dom_smallest)[significantIndex]];
        int highestLevel = arity, highestIndex = -1;
        if(valPtr==-1)
        {
            highestLevel=1;
            // search along top level to 
            int found=0; 
            for(int i=0; i<levelLengths[0]; i=i+2)
            {
                if(trie[0][i]==valToSupport)
                {
                    highestIndex=trie[0][i+1];
                    found=1;
                    break;
                }
            }
            if(found==0) return -1;
        }
        else
        {
            int index=valPtr;
            for (int trieLevel = arity-1; trieLevel > 0; trieLevel--) {
              if (!inDomain(trie[trieLevel][index], trieLevel, vars))
              {
	            highestLevel = trieLevel ;
		        highestIndex = index ;
	          }
	          index = trie[trieLevel][index+((trieLevel == arity-1)? 1:2)];
	        }
	        // still supported
	        if (highestLevel == arity) return valPtr;
	        // search for next supporting tuple
        }
        int st= searchTrie(highestLevel, highestIndex, valToSupport, vars);
        return st;
    }
    
  /* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	searchTrie
	%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */
    int * minlevel;
    // Another total replacement by pn
    
    template<typename VarArray>
    int searchTrie(int trieLevel, int index, int valToSupport, VarArray& vars) {
        // First search from startIndex to the
        // end, then from baseIndex to the old support.
        
        // search from index to the end
        bool searchUpTo=false;  // while false, store min level for each level. if true, search up to the min level.
        
        for(int i=0; i<arity; i++) minlevel[i]=2000000000;  // should be the biggest +ve integer.
        
	    while(true)
        {  // all ending conditions tested first.
            //System.out.println("trieLevel:"+trieLevel+" index:"+index);
          if(searchUpTo && (trieLevel==0 || index>minlevel[trieLevel] || index==levelLengths[trieLevel])) return -1;
          if(trieLevel==0 || index==levelLengths[trieLevel])
          {     // and not searchUpTo. 
              searchUpTo=true; 
              bool found=false;
                for(int i=0; i<levelLengths[0]; i=i+2) // should be a binary search.
                {
                    if(trie[0][i]==valToSupport)
                    {
                        index=trie[0][i+1];
                        found=true;
                        break;
                    }
                }
                D_ASSERT(found);
                //System.out.println("Resetting and returning to the start. index="+index);
                trieLevel=1; continue;
          }
          
          if(!searchUpTo && minlevel[trieLevel]>index){
            //System.out.println("Changing minlevel["+trieLevel+"] to "+index);
            minlevel[trieLevel]=index;
          }
          
          // Done testing end conditions, now change index and/or trieLevel.
          if (trie[trieLevel][index] == delim)
          {   // Block end, so backtrack.
		      // Point to value after parent of previous value.
              int parent=trie[trieLevel][index-1];
              trieLevel--;
		      index = parent+(trieLevel == (arity-1)? 2:3);
              continue;
		  }
          
          if (inDomain(trie[trieLevel][index], trieLevel, vars)) {
	        // value is in domain. We might be done?
	        if (trieLevel == (arity - 1)){
              last_tuple_pointer[valToSupport-(tuplelist->dom_smallest)[significantIndex]]=index;
	          return index;
            }
	        // Move down to next level.
		    index = trie[trieLevel][index+1];
	        trieLevel++;
            continue;
	      }
          else
          {  // Value not in domain, move on to next value.
	        index += ((trieLevel == (arity-1)? 2:3)) ;
            continue;
	      }
        } // end of trie search
      }
    
    
  
  /* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
reconstructTuple
Follow back ptrs up the tree.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */
void reconstructTuple(int*& supportingTuple, int valPtr) {
  int componentIndex ;
  for (int trieLevel = arity-1; trieLevel > 0; trieLevel--) {
	componentIndex = ((trieLevel <= significantIndex) ?  
					  trieLevel - 1 : trieLevel) ;
	supportingTuple[componentIndex] = trie[trieLevel][valPtr] ;
	// get back ptr
	valPtr = trie[trieLevel][valPtr+((trieLevel == arity-1)? 1:2)] ;
  }
  supportingTuple[significantIndex] = trie[0][valPtr] ;
}

  /* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	inDomain
	%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */
  /*					 
	return true iff val is in domain scope[componentIndex]
	*/
  
private: 
template<typename VarArray>
bool inDomain(int val, int trieLevel, VarArray& vars) {
  int componentIndex = ((trieLevel <= significantIndex) ?  
						trieLevel - 1 : trieLevel) ;
  return vars[componentIndex].inDomain(val);
  // This should be the CSP Var?
  // XXX return scope_vars[componentIndex].inDomain(val);
}
};       // end of TupleTrie struct


//template<typename VarArray>
struct TupleTrieArray {
  
  TupleList* tuplelist;

  int arity; 
  TupleTrie* tupleTries;
  
  TupleTrie & getTrie(int varIndex) 
  {
    return tupleTries[varIndex];
  };
  
  /* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	Constructor
	%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */
  
  TupleTrieArray(//const VarArray& vars,
                 //const vector<vector<int> >& tuples,
				 TupleList* _tuplelist
                 ) :
				 tuplelist(_tuplelist)
  {
     tuplelist->finalise_tuples();
	 arity = tuplelist->tuple_size();
     vector<int> dom_size(arity);
     vector<int> offset(arity);
     int delim=-2000000000;
     
	 
	 for(int i = 0; i < arity; ++i)
	 {
	   // getInitial* should be a const function, but fixing constness of all of minion is a major undertaking
	   // I'm not going to get into right now, so for now we'll get rid of the constness. This isn't illegal
	   // C++ or anything, just a bit nasty looking.
	   // XXX VarArray& non_const_vars = const_cast<VarArray&>(vars);
	   //dom_size[i] = non_const_vars[i].getInitialMax() - non_const_vars[i].getInitialMin() + 1;
       //offset[i] = -non_const_vars[i].getInitialMin();
	  }
      // create	one trie for each element of scope.
	  tupleTries = (TupleTrie*) malloc(sizeof(TupleTrie) * arity);
	  //new TupleTrie[arity];
	  for (unsigned varIndex = 0; varIndex < arity; varIndex++)
	  {
		new (tupleTries + varIndex) TupleTrie(/*vars,*/ varIndex, delim,/* tuples, dom_size, offset,*/ tuplelist);
		//tupleTries[varIndex] = new TupleTrie(vars, varIndex, delim, tuples, dom_size) ;
	  };
	  //
  }
};
