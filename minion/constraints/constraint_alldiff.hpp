/******************************************************************************
File: alldiff.cpp

Implementation of the algorithm for bounds consistency of the alldifferent
constraint from:

A. Lopez-Ortiz, C.-G. Quimper, J. Tromp, and P.  van Beek.
A fast and simple algorithm for bounds consistency of the
alldifferent constraint. IJCAI-2003.

By: John Tromp
******************************************************************************/

#include <stdlib.h>
//#include <ilsolver/ilcint.h>

//#include "alldiff.h"

/*============================================================
*  User defined propagator for enforcing bounds consistency
*  on the alldiff constraint.
*/

//class IlcNewAllDiffI : public IlcConstraintI
//{
//  public:
//    IlcNewAllDiffI( IlcManager m, IlcIntVarArray vars, PropType prop );
//    ~IlcNewAllDiffI();
//    void post();
//    void propagate();
//    void propagateValue();
//  private:
//    IlcIntVarArray _vars;
//    IlcRevInt currentLevel(m);
//    int lastLevel;
//    propType _prop;
//    int n;
//    int *t;		// tree links
//    int *d;		// diffs between critical capacities
//    int *h;		// hall interval links
//    interval *iv;
//    interval **minsorted;
//    interval **maxsorted;
//    int *bounds;
//    int nb;
//    void sortit();
//    int filterlower();
//    int filterupper();
//};
  
  
  const int INCONSISTENT = 0;
  const int CHANGES = 1;
  const int NO_CHANGES = 2;
  
  template<typename VarArray>
    triggerCollection
    AllDiffConstraint<VarArray>::setup_internal()
  {
      int i;
      
      n = _vars.size();
      
      currentLevel.set( 1 );
      lastLevel = -1;
      
      iv        = (interval  *)calloc(n, sizeof(interval  ));
      minsorted = (interval **)calloc(n, sizeof(interval *));
      maxsorted = (interval **)calloc(n, sizeof(interval *));
      bounds    = (int *)calloc(2*n+2, sizeof(int));
      
      for( i = 0; i < n; i++ ) {
		minsorted[i] = maxsorted[i] = &iv[i];
      }
      
      t = (int *)calloc(2*n+2, sizeof(int));
      d = (int *)calloc(2*n+2, sizeof(int));
      h = (int *)calloc(2*n+2, sizeof(int));
      
      triggerCollection t;
      for(i=0;i<n;i++)
      {
		t.push_back(make_trigger(_vars[i], Trigger(this, -i-1), Assigned));
		t.push_back(make_trigger(_vars[i], Trigger(this, i), LowerBound));
		t.push_back(make_trigger(_vars[i], Trigger(this, i), UpperBound));
      }
      
      return t;
      /*  if( _prop == WithValueRemoval ) {
		_vars.whenValue(propValue(getManager(),this));
      }*/
      //  _vars.whenRangeInterval(this);
  }
  
  template<typename VarArray>
    AllDiffConstraint<VarArray>::~AllDiffConstraint()
  {
	  free(bounds);
	  free(maxsorted);
	  free(minsorted);
	  free(iv);
	  free(h);
	  free(d);
	  free(t);
  }
  
  
  /*ILCDEMON1(propValue, IlcNewAllDiffI*, ct)
  {
    ct->propagateValue();
  }*/
  
  
  /*void
    IlcNewAllDiffI::post()
  {
      
  }*/
  
  template<typename VarArray>
	void
    AllDiffConstraint<VarArray>::propogate(int prop_val,int)
  {
      if(prop_val<0)
      {
		prop_val = (-prop_val - 1);
		int j, l;
		
		//i = _vars.getIndexValue();
		l = _vars[prop_val].getAssignedValue();
		
		for( j = 0; j < static_cast<int>(_vars.size()); j++ ) 
		{
		  if( j != prop_val ) 
		  { _vars[j].removeFromDomain( l ); }
		}
      }
      else
      {
		int i, status_lower, status_upper;
		//int a, b;
		int l, u;
		
		//a = _vars[prop_val].getMin();
		//.getRangeIndexMin();
		//b = _vars[prop_val].getMax();//.getRangeIndexMax();
		
		if( _prop == WithValueRemoval &&  _vars[prop_val].isAssigned() ) {
		  return;
		}
		
		currentLevel.set(currentLevel.get() + 1 );
		
		if( lastLevel != (currentLevel.get()-1) ) {
		  // not incremental
		  status_lower = CHANGES;
		  status_upper = CHANGES;
		  //typename VarArray::iterator  iter(_vars.begin());
		  for(i=0;i < static_cast<int>(_vars.size()); ++i) {
			iv[i].min = _vars[i].getMin();
			iv[i].max = _vars[i].getMax();
		  }
		}
		else {
		  // incremental
		  status_lower = NO_CHANGES;
		  status_upper = NO_CHANGES;
		  for( i = 0; i < static_cast<int>(_vars.size()); i++ ) {
			l = iv[i].min;
			u = iv[i].max;
			iv[i].min = _vars[i].getMin();
			iv[i].max = _vars[i].getMax();
			if( l != iv[i].min ) status_lower = CHANGES;
			if( u != iv[i].max ) status_upper = CHANGES;
		  }
		}
		
		lastLevel = currentLevel.get();
		
		if( status_lower == NO_CHANGES && status_upper == NO_CHANGES ) {
		  return;
		}
		
		sortit();
		
		status_lower = filterlower();
		if( status_lower != INCONSISTENT ) {
		  status_upper = filterupper();
		}
		
		if( (status_lower == INCONSISTENT) || (status_upper == INCONSISTENT) ) {
		  Controller::fail();
		}
		else
		  if( (status_lower == CHANGES) || (status_upper == CHANGES) ) {
			//    typename VarArray::iterator iter(_vars.begin());
			// i = 0;
			for(i=0;i<static_cast<int>(_vars.size());++i) {
			  _vars[i].setMin(iv[i].min);
			  _vars[i].setMax(iv[i].max);
			  //	      iter->setRange( iv[i].min, iv[i].max );
			  i++;
			}
		  }
		
      }
  }
  
  
  void
	sortmin( interval *v[], int n )
  {
	  int i, current;
	  bool sorted;
	  interval *t;
	  
	  current = n-1;
	  sorted = false;
	  while( !sorted ) {
		sorted = true;
		for( i = 0; i < current; i++ ) {
		  if( v[i]->min > v[i+1]->min ) {
			t = v[i];
			v[i] = v[i+1];
			v[i+1] = t;
			sorted = false;
		  }
		}
		current--;
	  }
  }
  
  void
	sortmax( interval *v[], int n )
  {
	  int i, current;
	  bool sorted;
	  interval *t;
	  
	  current = 0;
	  sorted = false;
	  while( !sorted ) {
		sorted = true;
		for( i = n-1; i > current; i-- ) {
		  if( v[i]->max < v[i-1]->max ) {
			t = v[i];
			v[i] = v[i-1];
			v[i-1] = t;
			sorted = false;
		  }
		}
		current++;
	  }
  }
  
  template<typename VarArray>
	void
	AllDiffConstraint<VarArray>::sortit()
  {
	  int i,j,nb,min,max,last;
	  
	  sortmin(minsorted, n);
	  sortmax(maxsorted, n);
	  
	  min = minsorted[0]->min;
	  max = maxsorted[0]->max + 1;
	  bounds[0] = last = min-2;
	  
	  for (i=j=nb=0;;) { // merge minsorted[] and maxsorted[] into bounds[]
		if (i<n && min<=max) {	// make sure minsorted exhausted first
		  if (min != last)
			bounds[++nb] = last = min;
		  minsorted[i]->minrank = nb;
		  if (++i < n)
			min = minsorted[i]->min;
		} else {
		  if (max != last)
			bounds[++nb] = last = max;
		  maxsorted[j]->maxrank = nb;
		  if (++j == n) break;
		  max = maxsorted[j]->max + 1;
		}
	  }
	  AllDiffConstraint<VarArray>::nb = nb;
	  bounds[nb+1] = bounds[nb] + 2;
  }
  
  
  void
	pathset(int *t, int start, int end, int to)
  {
	  int k, l;
	  for (l=start; (k=l) != end; t[k]=to) {
		l = t[k];
	  }
  }
  
  int
	pathmin(int *t, int i)
  {
	  for (; t[i] < i; i=t[i]) {
		;
	  }
	  return i;
  }
  
  int
	pathmax(int *t, int i)
  {
	  for (; t[i] > i; i=t[i]) {
		;
	  }
	  return i;
  }
  
  
  template<typename VarArray>
	int
	AllDiffConstraint<VarArray>::filterlower()
  {
	  int i,j,w,x,y,z;
	  int changes = 0;
	  
	  for (i=1; i<=nb+1; i++)
		d[i] = bounds[i] - bounds[t[i]=h[i]=i-1];
	  for (i=0; i<n; i++) { // visit intervals in increasing max order
		x = maxsorted[i]->minrank; y = maxsorted[i]->maxrank;
		j = t[z = pathmax(t, x+1)];
		if (--d[z] == 0)
		  t[z = pathmax(t, t[z]=z+1)] = j;
		pathset(t, x+1, z, z); // path compression
		if (d[z] < bounds[z]-bounds[y]) return INCONSISTENT; // no solution
		if (h[x] > x) {
		  maxsorted[i]->min = bounds[w = pathmax(h, h[x])];
		  pathset(h, x, w, w); // path compression
		  changes = 1;
		}
		if (d[z] == bounds[z]-bounds[y]) {
		  pathset(h, h[y], j-1, y); // mark hall interval
		  h[y] = j-1; //("hall interval [%d,%d)\n",bounds[j],bounds[y]);
		}
	  }
	  if( changes )
		return CHANGES;
	  else
		return NO_CHANGES;
  }
  
  
  template<typename VarArray>
	int
	AllDiffConstraint<VarArray>::filterupper()
  {
	  int i,j,w,x,y,z;
	  int changes = 0;
	  
	  for (i=0; i<=nb; i++)
		d[i] = bounds[t[i]=h[i]=i+1] - bounds[i];
	  for (i=n; --i>=0; ) { // visit intervals in decreasing min order
		x = minsorted[i]->maxrank; y = minsorted[i]->minrank;
		j = t[z = pathmin(t, x-1)];
		if (--d[z] == 0)
		  t[z = pathmin(t, t[z]=z-1)] = j;
		pathset(t, x-1, z, z);
		if (d[z] < bounds[y]-bounds[z]) return INCONSISTENT; // no solution
		if (h[x] < x) {
		  minsorted[i]->max = bounds[w = pathmin(h, h[x])] - 1;
		  pathset(h, x, w, w);
		  changes = 1;
		}
		if (d[z] == bounds[y]-bounds[z]) {
		  pathset(h, h[y], j+1, y);
		  h[y] = j+1;
		}
	  }
	  if( changes )
		return CHANGES;
	  else
		return NO_CHANGES;
  }
  
  
  
  
  
  /*IlcConstraint
	IlcNewAllDiff( IlcIntVarArray vars, PropType prop )
  {
	  IlcManager m = vars.getManager();
	  return new (m.getHeap()) IlcNewAllDiffI( m, vars, prop );
  }*/
  
  /*
   *  End of user defined propagator for enforcing bounds consistency
   *=================================================================*/
  