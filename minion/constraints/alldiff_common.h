// The implementation of the alldiff GAC algorithm,
// shared between alldiffgacslow and dynamicalldiff.


// whether to backtrack the matching.
// This one is a slowdown.
// #define BTMATCHING

// This one seems to break even.
// Whether to use a TMS to track whether spare values are present in the scc
// #define SPAREVALUESOPT

// Reverse the var list. Leave this switched on.
#define REVERSELIST

// Check domain size -- if it is greater than numvars, then no need to wake the constraint.
//#define CHECKDOMSIZE

// Do not process SCCs independently. Do the whole lot each time.
//#define NOSCC

// Warning: if this is not defined, then watchedalldiff probably won't do anything.
//#define USEWATCHES

// Optimize the case where a value was assigned. Only works in the absence of NOSCC.
#define ASSIGNOPT 

// Use the special queue
#define SPECIALQUEUE

// store matching from one run to the next.
#define INCREMENTALMATCH

// Use BFS instead of HK
#define BFSMATCHING

// Use staging a la Schulte and Stuckey
#define STAGED

#include <stdlib.h>
#include <iostream>
#include <vector>
#include <deque>
#include <algorithm>
#include <utility>

using namespace std;

#include "alldiff_gcc_shared.h"

template<typename VarArray>
#ifndef DYNAMICALLDIFF
struct GacAlldiff : public AbstractConstraint    // name changed fortrunk.
#else
struct DynamicAlldiff : public DynamicConstraint
#endif
{
  virtual string constraint_name()
  { 
  #ifndef DYNAMICALLDIFF
      return "GacAlldiff"; 
  #else
      return "DynamicAlldiff";
  #endif
  }
  
  typedef typename VarArray::value_type VarRef;  // what for?
  DomainInt dom_min, dom_max;
  VarArray var_array;
  
  int numvars, numvals;
  
  vector<int> SCCs;    // Variable numbers
  ReversibleMonotonicSet SCCSplit;
  // If !SCCSplit.isMember(anIndex) then anIndex is the last index in an SCC.
  
  ReversibleMonotonicSet sparevaluespresent;
  
  D_DATA(MoveablePointer SCCSplit2);
  
  vector<int> varToSCCIndex;  // Mirror of the SCCs array.
  
  #ifndef DYNAMICALLDIFF
  GacAlldiff(StateObj* _stateObj, const VarArray& _var_array) : AbstractConstraint(_stateObj),
  #else
  DynamicAlldiff(StateObj* _stateObj, const VarArray& _var_array) : DynamicConstraint(_stateObj),
  #endif
  
    #ifndef REVERSELIST
    var_array(_var_array), 
    #else
    var_array(_var_array.rbegin(), _var_array.rend()),
    #endif
    SCCSplit(_stateObj, _var_array.size()), 
    sparevaluespresent(_stateObj, _var_array.size()), constraint_locked(false)
  {
      dom_min=var_array[0].getInitialMin();
      dom_max=var_array[0].getInitialMax();
      SCCs.resize(var_array.size());
      varToSCCIndex.resize(var_array.size());
      for(int i=0; i<var_array.size(); ++i)
      {
          if(var_array[i].getInitialMin()<dom_min)
              dom_min=var_array[i].getInitialMin();
          if(var_array[i].getInitialMax()>dom_max)
              dom_max=var_array[i].getInitialMax();
          
          SCCs[i]=i;
          varToSCCIndex[i]=i;
          //D_ASSERT(SCCSplit.isMember(i));  // Can't do this until the thing is locked.
      }
      numvars=var_array.size();  // number of variables in the constraint
      numvals=dom_max-dom_min+1;
      
      to_process.reserve(var_array.size());
      
      // Set up data structures
      initialize_hopcroft();
      initialize_tarjan();
      
      // The matching in both directions.
      
      #ifndef BTMATCHING
      varvalmatching.resize(numvars); // maps var to actual value
      valvarmatching.resize(numvals); // maps val-dom_min to var.
      #else
      varvalmatching=getMemory(stateObj).backTrack().requestArray<int>(numvars);
      valvarmatching=getMemory(stateObj).backTrack().requestArray<int>(numvals);
      #endif
      
      sccs_to_process.reserve(numvars);
      
      // Initialize matching to satisfy the invariant
      // that the values are all different in varvalmatching.
      // watches DS is used in alldiffgacslow and in debugging.
      #if !defined(DYNAMICALLDIFF) || !defined(NO_DEBUG)
      if(usewatches()) watches.resize(numvars);
      #endif
      
      for(int i=0; i<numvars && i<numvals; i++)
      {
          varvalmatching[i]=i+dom_min;
          valvarmatching[i]=i;
          
          #if !defined(DYNAMICALLDIFF) || !defined(NO_DEBUG)
          if(usewatches()) watches[i].reserve(numvals, stateObj);
          #endif
      }
      
      D_DATA(SCCSplit2=getMemory(stateObj).backTrack().request_bytes((sizeof(char) * numvars)));
      D_DATA(for(int i=0; i<numvars; i++) ((char *)SCCSplit2.get_ptr())[i]=1);
      
      
  }
  
  // only used in dynamic version.
  #ifdef DYNAMICALLDIFF
  int dynamic_trigger_count()
  {
	// First an array of watches for the matching, then a 2d array of mixed triggers
    // indexed by [var][count] where count is increased from 0 as the triggers are used.
	return numvars+numvars*numvals;
  }
  
  inline DynamicTrigger * get_dt(int var, int counter)
  {
      // index the square array of dynamic triggers from dt
      D_ASSERT(bt_triggers_start == dynamic_trigger_start()+numvars);
      D_ASSERT(counter<numvals);
      D_ASSERT(var<numvars);
      D_ASSERT((bt_triggers_start+(var*numvals)+counter) >= dynamic_trigger_start());
      D_ASSERT((bt_triggers_start+(var*numvals)+counter) < dynamic_trigger_start() + dynamic_trigger_count());
      
      return bt_triggers_start+(var*numvals)+counter;
  }
  #endif
  
  // Should only be used in non-dynamic version.
  #ifndef DYNAMICALLDIFF
  virtual triggerCollection setup_internal()
  {
    D_INFO(2, DI_SUMCON, "Setting up Constraint");
    triggerCollection t;
    int array_size = var_array.size();
    for(int i = 0; i < array_size; ++i)
	  t.push_back(make_trigger(var_array[i], Trigger(this, i), DomainChanged));
    return t;
  }
  #endif
  
  #ifndef DYNAMICALLDIFF
  virtual AbstractConstraint* reverse_constraint()
  { return new CheckAssignConstraint<VarArray, GacAlldiff>(stateObj, var_array, *this); }
  #endif
  
  smallset to_process;  // set of vars to process.
  
  #if !defined(DYNAMICALLDIFF) || !defined(NO_DEBUG)
  vector<smallset_list_bt> watches;
  #endif
  
  #ifdef USEWATCHES
  inline bool usewatches()
  {
      return true;
  }
  #else
  inline bool usewatches()
  {
      return false;
  }
  #endif
  
  bool constraint_locked;
  
  #ifndef DYNAMICALLDIFF
  
  PROPAGATE_FUNCTION(int prop_var, DomainDelta)
  {
    D_ASSERT(prop_var>=0 && prop_var<var_array.size());
    
    // return if all the watches are still in place.
    #ifndef BTMATCHING 
    if(usewatches() && !to_process.in(prop_var) 
        && var_array[prop_var].inDomain(varvalmatching[prop_var]))     // This still has to be here, because we still wake up if the matchingis broken.
    #else
    if(usewatches() && !to_process.in(prop_var))
    #endif
    {
        smallset_list_bt& watch = watches[prop_var];
        short * list = ((short *) watch.list.get_ptr());
        int count=list[watch.maxsize];
        bool valout=false;
        
        for(int i=0; i<count; i++)
        {
            D_DATA(cout << "Checking var "<< prop_var << " val " << list[i]+dom_min << endl );
            if(!var_array[prop_var].inDomain(list[i]+dom_min))
            {
                valout=true;
                break;
            }
        }
        if(!valout)
        {
            // none of the watches were disturbed.
            #ifndef NO_DEBUG
            cout << "None of the watches were disturbed. Saved a call with watches." <<endl;
            
            #endif
            return;
        }
    }
    #ifdef CHECKDOMSIZE
    // If the domain size is >= numvars, then return.
    //if(!constraint_locked)
    {
        int count=0;
        for(int i=var_array[prop_var].getMin(); i<=var_array[prop_var].getMax(); i++)
        {
            if(var_array[prop_var].inDomain(i))
            {
                count++;
            }
        }
        if(count>=numvars)
            return;
    }
    // could improve domain counting by also applying it when
    // the constraint is already queued, DONE since it might avoid calls to Tarjan's.
    // i.e. some vars may not need to be queued in to_process.
    #endif
    
    #ifdef STAGED
    if(var_array[prop_var].isAssigned())
    {
        int assignedval=var_array[prop_var].getAssignedValue();
        for(int i=0; i<numvars; i++)
        {
            if(i!=prop_var)
            {
                var_array[i].removeFromDomain(assignedval);
            }
        }
    }
    #endif
    
    if(!to_process.in(prop_var))
    {
        to_process.insert(prop_var);
    }
    
    if(!constraint_locked)
    {
        #ifdef SPECIALQUEUE
        constraint_locked = true;
        getQueue(stateObj).pushSpecialTrigger(this);
        #else
        #ifdef NOSCC
        do_prop_noscc();
        #else
        do_prop();
        #endif
        #endif
    }
  }
  
  #else
  
  DYNAMIC_PROPAGATE_FUNCTION(DynamicTrigger* trig)
  {
      // get variable number from the trigger
    int prop_var = trig->trigger_info();
    #ifndef NO_DEBUG
    // check that some value has been disturbed; otherwise the watches are malfunctioning.
    if(var_array[prop_var].inDomain(varvalmatching[prop_var]))
    {
        smallset_list_bt& watch = watches[prop_var];
        short * list = ((short *) watch.list.get_ptr());
        int count=list[watch.maxsize];
        bool valout=false;
        
        for(int i=0; i<count; i++)
        {
            D_DATA(cout << "Checking var "<< prop_var << " val " << list[i]+dom_min << endl );
            if(!var_array[prop_var].inDomain(list[i]+dom_min))
            {
                valout=true;
                break;
            }
        }
        if(!valout)
        {
            // none of the watches were disturbed.
            cout << "None of the watches in the DS were disturbed. BT triggers must not match with watches DS." <<endl;
            cout << "Variable " << prop_var <<", val in matching: " << varvalmatching[prop_var] << endl;
            D_ASSERT(false);
        }
    }
    #endif
    
    #ifdef CHECKDOMSIZE
    // If the domain size is >= numvars, then return.
    // WHY IS THIS HERE WHEN checkdomsize and dynamic triggers don't work together??
    int count=0;
    for(int i=var_array[prop_var].getMin(); i<=var_array[prop_var].getMax(); i++)
    {
        if(var_array[prop_var].inDomain(i))
        {
            count++;
        }
    }
    if(count>=numvars)
        return;
    #endif
    
    #ifdef STAGED
    if(var_array[prop_var].isAssigned())
    {
        int assignedval=var_array[prop_var].getAssignedValue();
        for(int i=0; i<numvars; i++)
        {
            if(i!=prop_var)
            {
                var_array[i].removeFromDomain(assignedval);
            }
        }
    }
    #endif
    
    if(!to_process.in(prop_var))
    {
        to_process.insert(prop_var);
    }
    
    if(!constraint_locked)
    {
        #ifdef SPECIALQUEUE
        constraint_locked = true;
        getQueue(stateObj).pushSpecialTrigger(this);
        #else
        #ifdef NOSCC
        do_prop_noscc();
        #else
        do_prop();
        #endif
        #endif
    }
  }
  
  #endif
  
  virtual void special_unlock() { constraint_locked = false; to_process.clear(); }
  virtual void special_check()
  {
    constraint_locked = false;  // should be above the if.
    
    if(getState(stateObj).isFailed())
    {
        to_process.clear();
        return;
    }
    
    #ifndef NOSCC
    do_prop();
    #else
    do_prop_noscc();
    #endif
    to_process.clear();
  }
  
  
  void do_prop()
  {
    #ifndef DYNAMICALLDIFF
    PROP_INFO_ADDONE(AlldiffGacSlow);
    #else
    PROP_INFO_ADDONE(WatchedAlldiff);
    #endif
    
    #ifdef DYNAMICALLDIFF
    bt_triggers_start=dynamic_trigger_start()+numvars;
    #endif
    
    #ifndef NO_DEBUG
    cout << "Entering do_prop."<<endl;
    cout << "Varvalmatching:" <<varvalmatching<<endl;
    cout << "SCCs:" << SCCs <<endl;
    cout << "varToSCCIndex: "<< varToSCCIndex<<endl;
    cout << "Domains:" <<endl;
    for(int i=0; i<numvars; i++)
    {
        cout << "var:" << i << " vals:" ;
        for(int j=dom_min; j<=dom_max; j++)
        {
            if(var_array[i].inDomain(j))
            {
                cout << j <<", ";
            }
        }
        cout << endl;
    }
    // Check the matching is valid.
    for(int i=0; i<numvars; i++)
    {
        if(!var_array[i].inDomain(varvalmatching[i]))
        {
            cout << "val in matching removed: " << i << ", " << varvalmatching[i] <<endl;
        }
        for(int j=i+1; j<numvars; j++)
        {
            D_ASSERT(varvalmatching[i]!=varvalmatching[j]);
            D_ASSERT(SCCs[i]!=SCCs[j]);
        }
        D_ASSERT(SCCSplit.isMember(i) == (((char*)SCCSplit2.get_ptr())[i]==1));
        cout << (int)SCCSplit.isMember(i) << ", " << (int) (((char*)SCCSplit2.get_ptr())[i]==1) << endl;
        // The matches correspond.
        #ifndef BFSMATCHING
        D_ASSERT(valvarmatching[varvalmatching[i]-dom_min]==i);
        #endif
    }
    
    #ifdef DYNAMICALLDIFF
    // Can't yet check all the watches are in the right place, but can check
    // there are the right number of them on each var.
    for(int i=0; i<var_array.size(); i++)
    {
        int count=0;
        DynamicTrigger* trig=get_dt(i, 0);
        while(trig->queue!=NULL && count<numvals)
        {
            count++;
            if(count<numvals) trig=get_dt(i,count);
        }
        D_ASSERT(count==watches[i].size());
    }
    #endif
    
    // Check that if an element of the matching is removed, then the var is 
    // in to_process.
    for(int i=0; i<var_array.size(); i++)
    {
        if(!var_array[i].inDomain(varvalmatching[i]))
        {
            D_ASSERT(to_process.in(i));
        }
    }
    #endif
    // end of debug.
    
    #ifndef INCREMENTALMATCH
    // clear the matching.
    for(int i=0; i<numvars && i<numvals; i++)
    {
      varvalmatching[i]=i+dom_min;
      valvarmatching[i]=i;
    }
    #endif
    
    sccs_to_process.clear();
    {
    vector<int>& toiterate = to_process.getlist();
    D_DATA(cout << "About to loop for to_process variables."<<endl);
    
    for(int i=0; i<toiterate.size(); ++i)
    {
        int tempvar=toiterate[i];
        
        
        int sccindex_start=varToSCCIndex[tempvar];
        int sccindex_end=varToSCCIndex[tempvar];
        
        while(sccindex_start>0 && SCCSplit.isMember(sccindex_start-1))
        {
            sccindex_start--;   // seek the first item in the SCC.
        }
        
        while(SCCSplit.isMember(sccindex_end) && sccindex_end<(numvars-1))
        {
            sccindex_end++;   // seek the last item in the SCC
        }
        
        if(!var_array[tempvar].inDomain(varvalmatching[tempvar]))
        {
            // Find the start of the SCC in SCCs
            var_indices.clear();
            
            // Add a greedy repair here.
            
            // Actually should queue the SCCs which need to be hopcrofted,
            // and make greedy repairs to var matchings as we process them here.
            
            D_DATA(cout << "Varvalmatching:" <<varvalmatching<<endl);
            
            D_DATA(cout << "start:" << sccindex_start << " end:"<< sccindex_end<<endl);
            
            if(!matching_wrapper(sccindex_start, sccindex_end))
                return;
            
            #ifdef DYNAMICALLDIFF
            // sync the watches to the matching, 
            // since the matching has just changed.
            #ifndef BTMATCHING
            DynamicTrigger * trig = dynamic_trigger_start();
            for(int j=0; j<numvars; j++)
            {
                var_array[j].addWatchTrigger(trig + j, DomainRemoval, varvalmatching[j]);
                D_DATA(cout << "Adding watch for var " << j << " val " << varvalmatching[j] << endl);
            }
            #endif
            #endif
            
            
            D_DATA(cout << "Fixed varvalmatching:" << varvalmatching <<endl);
            
            // now both varvalmatching and valvarmatching contain 
            // a complete matching for the SCC.
            // Also, valinlocalmatching contains the domain values
            // used in this SCC.
        }
        
        #ifdef ASSIGNOPT
        if(var_array[tempvar].isAssigned())  // Optimize the case where it is assigned.
        #else
        if(false)
        #endif
        {
            // Split tempvar off from the rest of the SCC
            if(SCCSplit.isMember(sccindex_start))
            {
                // Swap it to the front.
                //cout <<"Before swap:" <<SCCs<<endl;
                sccs_to_process.remove(sccindex_start);
                
                int swapvar=SCCs[sccindex_start];
                SCCs[sccindex_start]=SCCs[varToSCCIndex[tempvar]];
                SCCs[varToSCCIndex[tempvar]]=swapvar;
                
                varToSCCIndex[swapvar]=varToSCCIndex[tempvar];
                varToSCCIndex[tempvar]=sccindex_start;
                
                //cout <<"After swap:" <<SCCs <<endl;
                //cout <<"varToSCCIndex:" << varToSCCIndex <<endl;
                
                // Split the SCCs
                SCCSplit.remove(sccindex_start);
                D_DATA(((char*)SCCSplit2.get_ptr())[sccindex_start]=0);
                
                sccindex_start++;
                int tempval=var_array[tempvar].getAssignedValue();
                
                // Now remove the value from the reduced SCC
                for(int i=sccindex_start; i<=sccindex_end; i++)
                {
                    D_DATA(cout << "Removing var: "<< SCCs[i] << " val:" << tempval <<endl);
                    var_array[SCCs[i]].removeFromDomain(tempval);
                }
                
                if(sccindex_start<sccindex_end)
                {
                    D_ASSERT(!sccs_to_process.in(sccindex_start));
                    sccs_to_process.insert(sccindex_start);
                }
            }
            // Else: The SCC is unit anyway. Should be no work to do, and do not insert into sccs_to_process.
            
        }
        else
        {
            // Not assigned, just some vals removed, so 
            if(!sccs_to_process.in(sccindex_start) && sccindex_start<sccindex_end)
            {
                sccs_to_process.insert(sccindex_start);
            }
        }
        
    } // end of loop.
    }
    
    
    #ifndef NO_DEBUG
    // Check the matching is valid.
    for(int i=0; i<numvars; i++)
    {
        D_ASSERT(var_array[i].inDomain(varvalmatching[i]));
        for(int j=i+1; j<numvars; j++)
        {
            D_ASSERT(varvalmatching[i]!=varvalmatching[j]);
            D_ASSERT(SCCs[i]!=SCCs[j]);
        }
        D_ASSERT(SCCSplit.isMember(i) == (((char*)SCCSplit2.get_ptr())[i]==1));
        D_ASSERT(SCCs[varToSCCIndex[i]]==i);
    }
    #endif
    
    // Call Tarjan's for each disturbed SCC.
    {
    vector<int> & toiterate=sccs_to_process.getlist();
    for(int i=0; i<toiterate.size(); i++)
    {
        int j=toiterate[i];
        
        // remake var_indices for this SCC.
        var_indices.clear();
        for(int k=j; k<numvars; k++)
        {
            #ifdef CHECKDOMSIZE
            if(!var_array[SCCs[k]].inDomain(varvalmatching[SCCs[k]]))
            {
                int l=j; while(SCCSplit.isMember(l) && l<(numvars-1)) l++;
                if(!matching_wrapper(j, l))
                    return;
            }
            #endif
            
            var_indices.push_back(SCCs[k]);
            if(!SCCSplit.isMember(k)) break;
        }
        
        //cout << "Running tarjan's on component "<< var_indices <<endl;
        tarjan_recursive(j);
    }
    }
    
    
    //  print components out
    
    /*
    cout<<"SCCs:"<<endl;
    for(int i=0; i<SCCs.size(); i++)
    {
        cout<< SCCs[i] <<endl;
    }
    cout<<"SCCSplit:"<<endl;
    for(int i=0; i<SCCs.size(); i++)
    {
        cout << ((SCCSplit.isMember(i))?"Nosplit":"Split") <<endl;
    }*/
    
    return;
  }
  
  // Simpler version which does not maintain SCCs and simply calls hopcroft and
  // visit for the whole set of variables.
  void do_prop_noscc()
  {
    #ifndef DYNAMICALLDIFF
    PROP_INFO_ADDONE(AlldiffGacSlow);
    #else
    PROP_INFO_ADDONE(WatchedAlldiff);
    #endif
    
    #ifdef DYNAMICALLDIFF
    bt_triggers_start=dynamic_trigger_start()+numvars;
    #endif
    
    #ifndef INCREMENTALMATCH
    // clear the matching.
    for(int i=0; i<numvars && i<numvals; i++)
    {
      varvalmatching[i]=i+dom_min;
      valvarmatching[i]=i;
    }
    #endif
    
    #ifndef NO_DEBUG
    cout << "Entering do_prop."<<endl;
    cout << "Varvalmatching:" <<varvalmatching<<endl;
    cout << "SCCs:" << SCCs <<endl;
    cout << "varToSCCIndex: "<< varToSCCIndex<<endl;
    cout << "Domains:" <<endl;
    for(int i=0; i<numvars; i++)
    {
        cout << "var:" << i << " vals:" ;
        for(int j=dom_min; j<=dom_max; j++)
        {
            if(var_array[i].inDomain(j))
            {
                cout << j <<", ";
            }
        }
        cout << endl;
    }
    // Check the matching is valid.
    for(int i=0; i<numvars; i++)
    {
        if(!var_array[i].inDomain(varvalmatching[i]))
        {
            cout << "val in matching removed: " << i << ", " << varvalmatching[i] <<endl;
        }
        for(int j=i+1; j<numvars; j++)
        {
            D_ASSERT(varvalmatching[i]!=varvalmatching[j]);
            D_ASSERT(SCCs[i]!=SCCs[j]);
        }
        D_ASSERT(SCCSplit.isMember(i) == (((char*)SCCSplit2.get_ptr())[i]==1));
        cout << (int)SCCSplit.isMember(i) << ", " << (int) (((char*)SCCSplit2.get_ptr())[i]==1) << endl;
        // The matches correspond.
        D_ASSERT(valvarmatching[varvalmatching[i]-dom_min]==i);
    }
    
    #ifdef DYNAMICALLDIFF
    // Can't yet check all the watches are in the right place, but can check
    // there are the right number of them on each var.
    for(int i=0; i<var_array.size(); i++)
    {
        int count=0;
        DynamicTrigger* trig=get_dt(i, 0);
        while(trig->queue!=NULL && count<numvals)
        {
            count++;
            if(count<numvals) trig=get_dt(i,count);
        }
        D_ASSERT(count==watches[i].size());
    }
    #endif
    
    // Check that if an element of the matching is removed, then the var is 
    // in to_process.
    for(int i=0; i<var_array.size(); i++)
    {
        if(!var_array[i].inDomain(varvalmatching[i]))
        {
            D_ASSERT(to_process.in(i));
        }
    }
    #endif
    
    // Call hopcroft for the whole matching.
    if(!matching_wrapper(0, numvars-1))
        return;
    
    #ifdef DYNAMICALLDIFF
    // sync the watches to the matching, 
    // since the matching has just changed.
    #ifndef BTMATCHING
    DynamicTrigger * trig = dynamic_trigger_start();
    for(int j=0; j<numvars; j++)
    {
        var_array[j].addWatchTrigger(trig + j, DomainRemoval, varvalmatching[j]);
    }
    #endif
    #endif
    
    
    D_DATA(cout << "Fixed varvalmatching:" << varvalmatching <<endl);
    
    // now both varvalmatching and valvarmatching contain 
    // a complete matching for the SCC.
    // Also, valinlocalmatching contains the domain values
    // used in this SCC.
    
    #ifndef NO_DEBUG
    // Check the matching is valid.
    for(int i=0; i<numvars; i++)
    {
        D_ASSERT(var_array[i].inDomain(varvalmatching[i]));
        for(int j=i+1; j<numvars; j++)
        {
            D_ASSERT(varvalmatching[i]!=varvalmatching[j]);
            D_ASSERT(SCCs[i]!=SCCs[j]);
        }
        D_ASSERT(SCCSplit.isMember(i) == (((char*)SCCSplit2.get_ptr())[i]==1));
        D_ASSERT(SCCs[varToSCCIndex[i]]==i);
    }
    #endif
    
    
    // Call Tarjan's for all vars
    
    var_indices.clear();
    for(int i=0; i<numvars; i++) var_indices.push_back(i);
    
    tarjan_recursive(0);
    
    return;
  }
  
  inline bool greedymatch(int tempvar, int sccindex_start, int sccindex_end)
  {
    D_ASSERT(!var_array[tempvar].inDomain(varvalmatching[tempvar]));
    
    valinlocalmatching.clear();
    for(int j=sccindex_start; j<=sccindex_end; j++)
    {
        valinlocalmatching.insert(varvalmatching[SCCs[j]]-dom_min);
    }
    
    for(int val=var_array[tempvar].getMin(); val<=var_array[tempvar].getMax(); val++)
    {
        if(var_array[tempvar].inDomain(val) && !valinlocalmatching.in(val-dom_min))
        {
            varvalmatching[tempvar]=val;
            valvarmatching[val-dom_min]=tempvar;
            return true;
        }
    }
    return false;
  }
  
  inline bool greedymatch2(int , int sccindex_start, int sccindex_end)
  {
      // process all the broken matchings.
    
    valinlocalmatching.clear();
    for(int j=sccindex_start; j<=sccindex_end; j++)
    {
        valinlocalmatching.insert(varvalmatching[SCCs[j]]-dom_min);
    }
    
    for(int j=sccindex_start; j<=sccindex_end; j++)
    {
        int tempvar=SCCs[j];
        
        if(var_array[tempvar].inDomain(varvalmatching[tempvar]))
        {
            bool found=false;
            for(int val=var_array[tempvar].getMin(); val<=var_array[tempvar].getMax(); val++)
            {
                if(var_array[tempvar].inDomain(val) && !valinlocalmatching.in(val-dom_min))
                {
                    varvalmatching[tempvar]=val;
                    valvarmatching[val-dom_min]=tempvar;
                    valinlocalmatching.insert(val-dom_min);
                    found=true;
                    break;
                }
            }
            if(!found)
            {
                return false;
            }
        }
    }
    
    return true;
  }
  
  virtual BOOL full_check_unsat()
  { 
    int v_size = var_array.size();
    for(int i = 0; i < v_size; ++i)
	{
	  if(var_array[i].isAssigned())
	  {
	  
	    for(int j = i + 1; j < v_size; ++j)
		{
		  if(var_array[j].isAssigned())
		  {
		    if(var_array[i].getAssignedValue() == var_array[j].getAssignedValue())
		      return true;
		  }
		}
		
	  }
	}
	
	return false;
  }
  
  virtual BOOL check_unsat(int i, DomainDelta)
  {
    int v_size = var_array.size();
	if(!var_array[i].isAssigned()) return false;
    
	DomainInt assign_val = var_array[i].getAssignedValue();
    for(int loop = 0; loop < v_size; ++loop)
	{
	  if(loop != i)
	  {
	    if(var_array[loop].isAssigned() && 
		   var_array[loop].getAssignedValue() == assign_val)
		return true;
	  }
	}
	return false;
  }
  
  virtual void full_propagate()
  { 
      #if defined(USEWATCHES) && defined(CHECKDOMSIZE)
      cout << "Watches and Quimper&Walsh's criterion do not safely co-exist." <<endl;
      FAIL_EXIT();
      #endif
      
      #if defined(DYNAMICALLDIFF) && !defined(USEWATCHES)
      cout << "watchedalldiff does not work if USEWATCHES is not defined." << endl;
      FAIL_EXIT();
      #endif
      
      // Is this guaranteed to be called before do_prop is ever called??
      // I hope so, because the following test has to be done first.
      if(numvars>numvals)
      {
          getState(stateObj).setFailed(true);
          return;
      }
      
      // process all variables.
      to_process.clear();    // It seems like this is called twice at the top of the tree, so the clear is necessary.
      
      for(int i=0; i<numvars; i++)
      {
          to_process.insert(i);
      }
      
      #ifdef DYNAMICALLDIFF
        // Clear all the BT triggers. This is so that the tests at
        // the start of do_prop will work.
        bt_triggers_start=dynamic_trigger_start()+numvars;
        for(int i=0; i<var_indices.size(); i++)
        {
            int var=var_indices[i];
            for(int j=triggercount[var]; j<numvals; j++)
            {
                if( get_dt(var, j)->queue == NULL)
                {
                    break;
                }
                get_dt(var, j)->sleepDynamicTriggerBT(stateObj);
            }
        }
        
      // set the variable numbers in the watches.
      DynamicTrigger * trig=dynamic_trigger_start();
      for(int i=0; i<numvars; i++)
      {
          (trig+i)->trigger_info() = i;
      }
      for(int i=0; i<numvars; i++)
      {
          for(int j=0; j<numvals; j++)
          {
              (trig+numvars+(i*numvals)+j)->trigger_info() = i;
          }
      }
      
      #ifndef BTMATCHING
      // set the watches here so that they start synced to the varvalmatching.
        trig = dynamic_trigger_start();
        for(int j=0; j<numvars; j++)
        {
            var_array[j].addWatchTrigger(trig + j, DomainRemoval, varvalmatching[j]);
            D_DATA(cout << "Adding watch for var " << j << " val " << varvalmatching[j] << endl);
        }
      #endif
      #endif
      
      #ifndef NOSCC
      do_prop();
      #else
      do_prop_noscc();
      #endif
  }
	
	virtual BOOL check_assignment(DomainInt* v, int array_size)
	{
	  D_ASSERT(array_size == var_array.size());
	  for(int i=0;i<array_size;i++)
		for( int j=i+1;j<array_size;j++)
		  if(v[i]==v[j]) return false;
	  return true;
	}
	
	virtual vector<AnyVarRef> get_vars()
	{
	  vector<AnyVarRef> vars;
	  vars.reserve(var_array.size());
	  for(unsigned i = 0; i < var_array.size(); ++i)
	    vars.push_back(var_array[i]);
	  return vars;
	}
    
    
  virtual void get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  {  
      bool matchok=true;
      for(int i=0; i<numvars; i++)
      {
          if(!var_array[i].inDomain(varvalmatching[i]))
          {
              matchok=false;
              break;
          }
      }
      
      if(!matchok)
      {
          if(numvals<numvars) return; // there can't be a matching.
          
          matchok=bfsmatching(0, numvars-1);
      }
      
      if(!matchok)
      {
          return;
      }
      else
      {
          for(int i=0; i<numvars; i++)
          {
              assignment.push_back(make_pair(i, varvalmatching[i]));
          }
          return;
      }
  }
    
    // ------------------------------- Targan's algorithm ------------------------------------
    // based on the following pseudocode from wikipedia.
        /*
        Input: Graph G = (V, E), Start node v0

        max_dfs := 0  // Counter for dfs
        U := V        // Collection of unvisited nodes
        S := {}       // An initially empty stack
        tarjan(v0)    // Call the function with the start node
        
        procedure tarjan(v)
        v.dfs := max_dfs;          // Set the depth index
        v.lowlink := max_dfs;      // v.lowlink <= v.dfs
        max_dfs := max_dfs + 1;    // Increment the counter
        S.push(v);                 // Place v on the stack
        U := U \ {v};              // Separate v from U
        forall (v, v') in E do     // Consider the neighboring nodes
          if (v' in U)
            tarjan(v');            // recursive call
            v.lowlink := min(v.lowlink, v'.lowlink);
          // Ask whether v' is on the stack 
          // by a clever constant time method
          // (for example, setting a flag on the node when it is pushed or popped) 
          elseif (v' in S)
            v.lowlink := min(v.lowlink, v'.dfs);
          end if
        end for
        if (v.lowlink = v.dfs)     // the root of a strongly connected component
          print "SZK:";
          repeat
            v' := S.pop;
            print v';
          until (v' = v);
        end if
        */
    
    vector<int> tstack;
    smallset_nolist in_tstack;
    smallset_nolist visited;
    vector<int> dfsnum;
    vector<int> lowlink;
    
    //vector<int> iterationstack;
    vector<int> curnodestack;
    
    #ifndef BTMATCHING
    vector<int> varvalmatching; // For each var, give the matching value.
    // valvarmatching is from val-dom_min to var.
    vector<int> valvarmatching;   // need to set size somewhere.
    // -1 means unmatched.
    #else
    MoveableArray<int> varvalmatching;
    MoveableArray<int> valvarmatching;
    #endif
    
    // Filled in before calling tarjan's.
    bool scc_split;
    
    int sccindex;
    
    int max_dfs;
    
    vector<int> spare_values;
    bool include_sink;
    vector<int> var_indices;  // Should be a pointer so it can be changed.
    
    smallset sccs_to_process;   // Indices to the first var in the SCC to process.
    
    int varcount;
    
    #ifdef DYNAMICALLDIFF
    vector<int> triggercount; // number of triggers on the variable
    DynamicTrigger * bt_triggers_start;   // points at dynamic_trigger_start()+numvars
    #endif
    
    // An integer represents a vertex, where 0 .. numvars-1 represent the vars,
    // numvars .. numvars+numvals-1 represents the values (val-dom_min+numvars),
    // numvars+numvals is the sink,
    // numvars+numvals+1 is the 
    
    void initialize_tarjan()
    {
        int numnodes=numvars+numvals+1;  // One sink node.
        tstack.reserve(numnodes);
        in_tstack.reserve(numnodes);
        visited.reserve(numnodes);
        max_dfs=1;
        scc_split=false;
        dfsnum.resize(numnodes);
        lowlink.resize(numnodes);
        
        //iterationstack.resize(numnodes);
        curnodestack.reserve(numnodes);
        #ifdef DYNAMICALLDIFF
        triggercount.resize(numvars);
        #endif
    }
    
    void tarjan_recursive(int sccindex_start)
    {
        
        valinlocalmatching.clear();
        
        int localmax=var_array[var_indices[0]].getMax();
        int localmin=var_array[var_indices[0]].getMin();
        valinlocalmatching.insert(varvalmatching[var_indices[0]]-dom_min);
        
        for(int i=1; i<var_indices.size(); i++)
        {
            int tempvar=var_indices[i];
            int tempmax=var_array[tempvar].getMax();
            int tempmin=var_array[tempvar].getMin();
            if(tempmax>localmax) localmax=tempmax;
            if(tempmin<localmin) localmin=tempmin;
            valinlocalmatching.insert(varvalmatching[var_indices[i]]-dom_min);
        }
        
        if(usewatches())
        {
            for(int i=0; i<var_indices.size(); i++)
            {
                #if !defined(DYNAMICALLDIFF) || !defined(NO_DEBUG)
                watches[var_indices[i]].clear();
                D_DATA(cout << "Adding DT for var " << var_indices[i] << " val " << varvalmatching[var_indices[i]] << endl);
                // watch the value from the matching.
                watches[var_indices[i]].insert(varvalmatching[var_indices[i]]-dom_min);
                #endif
                
                #ifdef DYNAMICALLDIFF
                // Clear all the triggers on this variable. At end now.
                int var=var_indices[i];
                triggercount[var]=1;
                
                var_array[var].addDynamicTriggerBT(get_dt(var, 0), 
                    DomainRemoval, varvalmatching[var]);
                D_DATA(cout << "Adding DT for var " << var_indices[i] << " val " << varvalmatching[var_indices[i]] << endl);
                #endif
            }
        }
        
        // spare_values
        // This should be computed somehow on demand because it might not be used.
        // Actually it should be used exactly once.
        #ifdef SPAREVALUESOPT
        if(sparevaluespresent.isMember(sccindex_start))
        {
        #endif
            spare_values.clear();
            for(int val=localmin; val<=localmax; ++val)
            {
                if(!valinlocalmatching.in(val-dom_min))
                {
                    for(int j=0; j<var_indices.size(); j++)
                    {
                        if(var_array[var_indices[j]].inDomain(val))
                        {
                            spare_values.push_back(val-dom_min+numvars);
                            break;
                        }
                    }
                }
            }
            //cout << "With spare values "<< spare_values <<endl;
            
            include_sink= (spare_values.size()>0);  // This should be in the TMS.
        #ifdef SPAREVALUESOPT    
            if(!include_sink)
            {
                // Set some bits in sparevaluespresent.
                for(int scci=sccindex_start; ; scci++)
                {
                    D_ASSERT(sparevaluespresent.isMember(scci));
                    sparevaluespresent.remove(scci);
                    if(!SCCSplit.isMember(scci))  // means this is the last element
                    {
                        break;
                    }
                }
            }
        }
        else
        {
            include_sink=false;
        }
        #endif
        // Just generate the spare_values if no empty sets have been seen above
        // in this branch.
        
        tstack.clear();
        in_tstack.clear();
        
        visited.clear();
        max_dfs=1;
        
        scc_split=false;
        sccindex=sccindex_start;
        
        for(int i=0; i<var_indices.size(); ++i)
        {
            int curnode=var_indices[i];
            if(!visited.in(curnode))
            {
                D_DATA(cout << "(Re)starting tarjan's algorithm, value:"<< curnode <<endl);
                varcount=0;
                visit(curnode, sccindex_start);
                D_DATA(cout << "Returned from tarjan's algorithm." << endl);
            }
        }
        
        // Clear any extra watches.
        #ifdef DYNAMICALLDIFF
        if(usewatches())
        {
            for(int i=0; i<var_indices.size(); i++)
            {
                int var=var_indices[i];
                for(int j=triggercount[var]; j<numvals; j++)
                {
                    if( get_dt(var, j)->queue == NULL)
                    {
                        break;
                    }
                    get_dt(var, j)->sleepDynamicTriggerBT(stateObj);
                }
            }
        }
        #endif
    }
    
    void visit(int curnode, int sccindex_start)
    {
        tstack.push_back(curnode);
        in_tstack.insert(curnode);
        dfsnum[curnode]=max_dfs;
        lowlink[curnode]=max_dfs;
        max_dfs++;
        visited.insert(curnode);
        //cout << "Visiting node: " <<curnode<<endl;
        
        if(curnode==numvars+numvals)
        {
            //cout << "Visiting sink node." <<endl;
            D_ASSERT(include_sink);
            // It's the sink so it links to all spare values.
            
            for(int i=0; i<spare_values.size(); ++i)
            {
                int newnode=spare_values[i];
                //cout << "About to visit spare value: " << newnode-numvars+dom_min <<endl;
                if(!visited.in(newnode))
                {
                    visit(newnode, sccindex_start);
                    if(lowlink[newnode]<lowlink[curnode])
                    {
                        lowlink[curnode]=lowlink[newnode];
                    }
                }
                else
                {
                    // Already visited newnode
                    if(in_tstack.in(newnode) && dfsnum[newnode]<lowlink[curnode])
                    {
                        lowlink[curnode]=dfsnum[newnode];
                    }
                }
            }
        }
        else if(curnode<numvars)  // This case should never occur with merge nodes.
        {
            D_ASSERT(find(var_indices.begin(), var_indices.end(), curnode)!=var_indices.end());
            varcount++;
            //cout << "Visiting node variable: "<< curnode<<endl;
            int newnode=varvalmatching[curnode]-dom_min+numvars;
            D_ASSERT(var_array[curnode].inDomain(newnode+dom_min-numvars));
            
            if(!visited.in(newnode))
            {
                visit(newnode, sccindex_start);
                if(lowlink[newnode]<lowlink[curnode])
                {
                    lowlink[curnode]=lowlink[newnode];
                }
            }
            else
            {
                // Already visited newnode
                if(in_tstack.in(newnode) && dfsnum[newnode]<lowlink[curnode])
                {
                    lowlink[curnode]=dfsnum[newnode];  // Why dfsnum not lowlink?
                }
            }
        }
        else
        {
            // curnode is a value
            // This is the only case where watches are set.
            //cout << "Visiting node val: "<< curnode+dom_min-numvars <<endl;
            
            D_ASSERT(curnode>=numvars && curnode<(numvars+numvals));
            #ifndef NO_DEBUG
            bool found=false;
            for(int i=0; i<var_indices.size(); i++)
            {
                if(var_array[var_indices[i]].inDomain(curnode+dom_min-numvars))
                {
                    found=true;
                }
            }
            D_ASSERT(found);
            #endif
            
            int lowlinkvar=-1;
            for(int i=0; i<var_indices.size(); i++)
            {
                int newnode=var_indices[i];
                if(varvalmatching[newnode]!=curnode-numvars+dom_min)   // if the value is not in the matching.
                {
                    if(var_array[newnode].inDomain(curnode+dom_min-numvars))
                    {
                        //newnode=varvalmatching[newnode]-dom_min+numvars;  // Changed here for merge nodes
                        if(!visited.in(newnode))
                        {
                            // set a watch
                            if(usewatches())
                            {
                                D_DATA(cout << "Adding DT for var " << newnode << " val " << curnode-numvars+dom_min << endl);
                                
                                #if !defined(DYNAMICALLDIFF) || !defined(NO_DEBUG)
                                watches[newnode].insert(curnode-numvars);
                                #endif
                                
                                #ifdef DYNAMICALLDIFF
                                var_array[newnode].addDynamicTriggerBT(get_dt(newnode, triggercount[newnode]), 
                                    DomainRemoval, curnode-numvars+dom_min);
                                triggercount[newnode]++;
                                #endif
                            }
                            visit(newnode, sccindex_start);
                            if(lowlink[newnode]<lowlink[curnode])
                            {
                                lowlink[curnode]=lowlink[newnode];
                                lowlinkvar=-1;   // Would be placing a watch where there already is one.
                            }
                        }
                        else
                        {
                            // Already visited newnode
                            if(in_tstack.in(newnode) && dfsnum[newnode]<lowlink[curnode])
                            {
                                lowlink[curnode]=dfsnum[newnode];
                                lowlinkvar=newnode;
                            }
                        }
                    }
                }
            }
            
            
            
            // Why the find? Can we not use some DS which is already lying around?
            // And why does it include the whole matching in the find??
            if(include_sink && valinlocalmatching.in(curnode-numvars))
            //    find(varvalmatching.begin(), varvalmatching.end(), curnode+dom_min-numvars)!=varvalmatching.end())
            {
                int newnode=numvars+numvals;
                if(!visited.in(newnode))
                {
                    visit(newnode, sccindex_start);
                    if(lowlink[newnode]<lowlink[curnode])
                    {
                        lowlink[curnode]=lowlink[newnode];
                        lowlinkvar=-1;
                    }
                }
                else
                {
                    // Already visited newnode
                    if(in_tstack.in(newnode) && dfsnum[newnode]<lowlink[curnode])
                    {
                        lowlink[curnode]=dfsnum[newnode];
                        lowlinkvar=-1;
                    }
                }
            }
            
            // Where did the low link value come from? insert that edge into watches.
            if(usewatches() && lowlinkvar!=-1)
            {
                D_DATA(cout << "Adding DT for var " << lowlinkvar << " val " << curnode-numvars+dom_min << endl);
                
                #if !defined(DYNAMICALLDIFF) || !defined(NO_DEBUG)
                watches[lowlinkvar].insert(curnode-numvars);
                #endif
                #ifdef DYNAMICALLDIFF
                var_array[lowlinkvar].addDynamicTriggerBT(get_dt(lowlinkvar, triggercount[lowlinkvar]), 
                    DomainRemoval, curnode-numvars+dom_min);
                triggercount[lowlinkvar]++;
                #endif
            }
        }
        
        //cout << "On way back up, curnode:" << curnode<< ", lowlink:"<<lowlink[curnode]<< ", dfsnum:"<<dfsnum[curnode]<<endl;
        if(lowlink[curnode]==dfsnum[curnode])
        {
            // Did the SCC split?
            // Perhaps we traversed all vars but didn't unroll the recursion right to the top.
            // Then lowlink[curnode]!=1. Or perhaps we didn't traverse all the variables.
            // I think these two cases cover everything.
            if(lowlink[curnode]!=1 || varcount<var_indices.size())
            {
                scc_split=true;  // The SCC has split and there is some work to do later.
            }
            
            // Doing something with the components should not be necessary unless the scc has split.
            // The first SCC found is deep in the tree, so the flag will be set to its final value
            // the first time we are here.
            // so it is OK to assume that scc_split has been
            // set correctly before we do the following.
            if(scc_split)
            {
                // For each variable, write it to the scc array.
                // If its the last one, flip the bit.
                
                varinlocalmatching.clear();  // Borrow this datastructure for a minute.
                
                //vector<int> tempset;  // pretend this is SCCs for the time being.
                //tempset.reserve(numvars);
                D_DATA(cout <<"Writing new SCC:"<<endl );
                bool containsvars=false;
                for(vector<int>::iterator tstackit=(--tstack.end());  ; --tstackit)
                {
                    int copynode=(*tstackit);
                    //cout << "SCC element: "<< copynode<<endl;
                    if(copynode<numvars)
                    {
                        containsvars=true;
                        SCCs[sccindex]=copynode;
                        varToSCCIndex[copynode]=sccindex;
                        sccindex++;
                        //tempset.push_back(copynode);  // need to write into sccs instead.
                        varinlocalmatching.insert(copynode);
                        D_DATA(cout << "Stored SCC element "<< copynode<<endl);
                    }
                    
                    if(copynode==curnode)
                    {
                        // Beware it might be an SCC containing just one value.
                        
                        if(containsvars)
                        {
                            D_DATA(cout << "Inserting split point." <<endl);
                            SCCSplit.remove(sccindex-1);
                            D_DATA(((char*)SCCSplit2.get_ptr())[sccindex-1]=0);
                        }
                        // The one written last was the last one in the SCC.
                        break;
                    }
                    
                    // Should be no split points in the middle of writing an SCC.
                    //D_ASSERT(copynode==curnode || copynode>=numvars || SCCSplit.isMember(sccindex-1));
                }
                // Just print more stuff here.
                
                
                // For each value, iterate through the current
                // SCC and remove it from any other variable other
                // than the one in this SCC.
                //cout << "Starting loop"<<endl;
                //if(containsvars) // why is this OK? because of bug above, in case where numnode is a variable.
                {
                    while(true)
                    {
                        int copynode=(*(--tstack.end()));
                        
                        tstack.pop_back();
                        in_tstack.remove(copynode);
                        
                        if(copynode>=numvars && copynode!=(numvars+numvals))
                        {
                            // It's a value. Iterate through old SCC and remove it from
                            // any variables not in tempset.
                            //cout << "Trashing value "<< copynode+dom_min-numvars << endl;
                            for(int i=0; i<var_indices.size(); i++)
                            {
                                int curvar=var_indices[i];
                                if(!varinlocalmatching.in(curvar))
                                {
                                    // var not in tempset so might have to do some test against matching.
                                    // Why doing this test? something wrong with the assigned variable optimization?
                                    if(varvalmatching[curvar]!=copynode+dom_min-numvars)
                                    {
                                        D_DATA(cout << "Removing var: "<< curvar << " val:" << copynode+dom_min-numvars <<endl);
                    
                                        var_array[curvar].removeFromDomain(copynode+dom_min-numvars);
                                    }
                                }
                            }
                        }
                        
                        if(copynode==curnode)
                        {
                            break;
                        }
                    }
                }
            }
        }
    }
    
  // -------------------------Hopcroft-Karp algorithm -----------------------------
  // Can be applied to a subset of var_array as required.
  
  // Each domain value has a label which is numvars+
  
  
  // These two are for the valvar version of hopcroft.
  smallset_nolist varinlocalmatching;    // indicates whether a var is recorded in localmatching.
  smallset valinlocalmatching;
  
  //smallset varinlocalmatching;    // indicates whether a var is recorded in localmatching.
  //smallset_nolist valinlocalmatching;
  
  
  // Uprevious (pred) gives (for each CSP value) the value-dom_min
  // it was matched to in the previous layer. If it was unmatched,
  // -1 is used.
  vector<int> uprevious;  // -2 means unset, -1 labelled unmatched. 
  
  vector<vector<int> > vprevious;  // map val-dom_min to vector of vars.
  smallset_nolist invprevious;     // is there a mapping in vprevious for val? Allows fast unset.
  
  smallset layer;
  smallset unmatched;   // contains vals-dom_min.
  
  vector<vector<int> > newlayer;
  smallset innewlayer;
  
  void initialize_hopcroft()
  {
      // Initialize all datastructures to do with hopcroft-karp
      // Surely could reduce the number of arrays etc used for hopcroft-karp??
      int numvals=dom_max-dom_min+1;
      
      varinlocalmatching.reserve(numvars);
      valinlocalmatching.reserve(numvals);
      uprevious.resize(numvars, -2);
      
      vprevious.resize(numvals);
      for(int i=0; i<numvals; ++i)
      {
          vprevious[i].reserve(numvars);
      }
      invprevious.reserve(numvals);
      
      layer.reserve(numvars);
      unmatched.reserve(numvals);
      
      newlayer.resize(numvals);
      for(int i=0; i<numvals; ++i)
      {
          newlayer[i].reserve(numvars);
      }
      innewlayer.reserve(numvals);
      
      // for BFS algorithm
      prev.resize(numvars+numvals, -1);
  }
  
  
    
// ------------------------- Hopcroft which takes start and end indices.
    
    inline bool matching_wrapper(int sccstart, int sccend)
    {
        #ifdef BFSMATCHING
        return bfs_wrapper(sccstart,sccend);
        #else
        return hopcroft_wrapper(sccstart,sccend);
        #endif
    }

    inline bool hopcroft_wrapper(int sccstart, int sccend)
    {
        // Call hopcroft for the whole matching.
        if(!hopcroft(sccstart, sccend))
        {
            // The constraint is unsatisfiable (no matching).
            D_DATA(cout << "About to fail. Changed varvalmatching: "<< varvalmatching <<endl);
            
            // temporary mess to see if hopcroft is failing prematurely.
            /*vector<int> t=varvalmatching;
            vector<int> t2=valvarmatching;
            if(bfs_wrapper(sccstart, sccend))
            {
                cout << "BFS fixed matching, but hopcroft did not!" <<endl;
                cout << "hopcroft varvalmatching:"<< t<<endl;
                cout << "hopcroft valvarmatching:"<< t2<<endl;
                cout << "BFS varvalmatching:" << varvalmatching <<endl;
                
                {vector<int>& toiterate=valinlocalmatching.getlist();
                    for(int j=0; j<toiterate.size(); j++)
                    {
                        int tempval=toiterate[j];
                        t[t2[tempval]]=tempval+dom_min;
                    }
                }
                cout << "copied back hopcroft varvalmatching:" << t <<endl;
                
                BOOL bfsflag=true, hopflag=true;
                for(int scci=sccstart; scci<=sccend; scci++)
                {
                    if(!var_array[SCCs[scci]].inDomain(t[SCCs[scci]]))
                        hopflag=false;
                    if(!var_array[SCCs[scci]].inDomain(t[SCCs[scci]]))
                        bfsflag=false;
                }
                cout << "hopcroft matching valid: "<< hopflag <<endl;
                cout << "bfs matching valid: "<< bfsflag <<endl;
            }*/
            
            for(int j=0; j<numvars; j++)
            {
                // Restore valvarmatching because it might be messed up by Hopcroft.
                valvarmatching[varvalmatching[j]-dom_min]=j;
            }
            
            //D_DATA(cout << varinlocalmatching.getlist()<<endl);
            getState(stateObj).setFailed(true);
            //varvalmatching=tempmatching;  // copy back.
            return false;
        }
        
        // Here, copy from valvarmatching to varvalmatching.
        // Using valinlocalmatching left over from hopcroft.
        // This must not be done when failing, because it might mess
        // up varvalmatching for the next invocation.
        {vector<int>& toiterate=valinlocalmatching.getlist();
            for(int j=0; j<toiterate.size(); j++)
            {
                int tempval=toiterate[j];
                varvalmatching[valvarmatching[tempval]]=tempval+dom_min;
            }
        }
        return true;
    }
    
    inline bool hopcroft(int sccstart, int sccend)
    {
        // Domain value convention:
        // Within hopcroft and recurse,
        // a domain value is represented as val-dom_min always.
        
        // Variables are always represented as their index in
        // var_array. sccstart and sccend indicates which variables
        // we are allowed to use here.
        
        int localnumvars=sccend-sccstart+1;
        
        // Construct the valinlocalmatching for this SCC, checking each val
        // to see it's in the relevant domain.
        valinlocalmatching.clear();
        
        for(int i=sccstart; i<=sccend; i++)
        {
            int tempvar=SCCs[i];
            if(var_array[tempvar].inDomain(varvalmatching[tempvar]))
            {
                valinlocalmatching.insert(varvalmatching[tempvar]-dom_min);
                // Check the two matching arrays correspond.
                //D_ASSERT(valvarmatching[varvalmatching[tempvar]-dom_min]==tempvar);
            }
        }
        
        /*# initialize greedy matching (redundant, but faster than full search)
        matching = {}
        for u in graph:
            for v in graph[u]:
                if v not in matching:
                    matching[v] = u
                    break
        */
        
        if(valinlocalmatching.size()==localnumvars)
        {
            return true;
        }
        
        // uprevious == pred
        // vprevious == preds
        
        // need sets u and v
        // u is easy, v is union of domains[0..numvar-1]
        
        while(true)
        {
            /*
            preds = {}
            unmatched = []
            pred = dict([(u,unmatched) for u in graph])
            for v in matching:
                del pred[matching[v]]
            layer = list(pred)
            */
            // Set up layer and uprevious.
            invprevious.clear();
            unmatched.clear();
            
            layer.clear();
            
            // Reconstruct varinlocalmatching here.
            // WHY end up with duplicates in valvarmatching here???????
            // Because it's left over from a bad state when do_prop was last invoked, and failed.
            varinlocalmatching.clear();
            {
                vector<int>& toiterate=valinlocalmatching.getlist();
                for(int i=0; i<toiterate.size(); ++i)
                {
                    if(!varinlocalmatching.in(valvarmatching[toiterate[i]]))   // This should not be conditional --BUG
                        varinlocalmatching.insert(valvarmatching[toiterate[i]]);
                }
            }
            
            for(int i=sccstart; i<=sccend; ++i)
            {
                int tempvar=SCCs[i];
                if(varinlocalmatching.in(tempvar))  // The only use of varinlocalmatching.
                {
                    uprevious[tempvar]=-2;   // Out of uprevious
                }
                else
                {
                    layer.insert(tempvar);
                    uprevious[tempvar]=-1;  // In layer, and set to unmatched in uprevious.
                }
            }
            
            /*cout<< "Uprevious:" <<endl;
            for(int i=0; i<localnumvars; ++i)
            {
                cout<< "for variable "<<var_indices[i]<<" value "<< uprevious[i]<<endl;
            }*/
            
            // we have now calculated layer
            /*
            while layer and not unmatched:
			    newLayer = {}
            */
            
            while(layer.size()!=0 && unmatched.size()==0)
            {
                innewlayer.clear();
                
                /*
                for u in layer:
                    for v in graph[u]:
                        if v not in preds:
                            newLayer.setdefault(v,[]).append(u)
                */
                {
                vector<int>& toiterate=layer.getlist();
                for(int i=0; i<toiterate.size(); ++i)
                {
                    //cout<<"Layer item: "<<(*setit)<<endl;
                    int tempvar=toiterate[i];
                    for(DomainInt realval=var_array[tempvar].getMin(); realval<=var_array[tempvar].getMax(); realval++)
                    {
                        if(var_array[tempvar].inDomain(realval))
                        {
                            int tempval=realval-dom_min;
                            
                            if(!invprevious.in(tempval))  // if tempval not found in vprevious
                            {
                                if(!innewlayer.in(tempval))
                                {
                                    innewlayer.insert(tempval);
                                    newlayer[tempval].clear();
                                }
                                newlayer[tempval].push_back(tempvar);
                            }
                        }
                    }
                }
                }
                /*
                layer = []
                for v in newLayer:
                    preds[v] = newLayer[v]
                    if v in matching:
                        layer.append(matching[v])
                        pred[matching[v]] = v
                    else:
                        unmatched.append(v)
                */
                
                layer.clear();
                
                /*cout<<"Local matching state:"<<endl;
                {
                vector<int>& toiterate = valinlocalmatching.getlist();
                for(int i=0; i<toiterate.size(); ++i)
                {
                    int temp=toiterate[i];
                    D_ASSERT(varinlocalmatching.in(localmatching[temp]));
                    cout << "mapping "<< localmatching[temp] << " to value " << temp <<endl; 
                }
                }*/
                
                {
                vector<int>& toiterate = innewlayer.getlist();
                for(int i=0; i<toiterate.size(); ++i)
                {
                    int tempval = toiterate[i]; // for v in newlayer.
                    //cout << "Looping for value "<< tempval <<endl;
                    
                    D_ASSERT(innewlayer.in(tempval));
                    // insert mapping in vprevious
                    invprevious.insert(tempval);
                    
                    vprevious[tempval]=newlayer[tempval];  // This should be a copy???
                    /*vprevious[tempval].resize(newlayer[tempval].size());
                    for(int x=0; x<newlayer[tempval].size(); x++)
                    {
                        vprevious[tempval][x]=newlayer[tempval][x];
                    }*/
                    
                    if(valinlocalmatching.in(tempval))
                    {
                        int match=valvarmatching[tempval];
                        //cout << "Matched to variable:" << match << endl;
                        layer.insert(match);
                        uprevious[match]=tempval;
                    }
                    else
                    {
                        //cout<<"inserting value into unmatched:"<<tempval<<endl;
                        unmatched.insert(tempval);
                    }
                }
                }
                
                //cout << "At end of layering loop." << endl;
            }
            //cout << "Out of layering loop."<<endl;
            // did we finish layering without finding any alternating paths?
            // we do not need to calculate unlayered here.
            /*
            # did we finish layering without finding any alternating paths?
            if not unmatched:
                unlayered = {}
                for u in graph:
                    for v in graph[u]:
                        if v not in preds:
                            unlayered[v] = None
                return (matching,list(pred),list(unlayered))
            */
            //cout << "Unmatched size:" << unmatched.size() << endl;
            if(unmatched.size()==0)
            {
                //cout << "Size of matching:" << valinlocalmatching.size() << endl;
                
                if(valinlocalmatching.size()==localnumvars)
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
            
            /*
            for v in unmatched: recurse(v)
            */
            {
            vector<int>& toiterate=unmatched.getlist();
            for(int i=0; i<toiterate.size(); ++i)
            {
                int tempval=toiterate[i];
                //cout<<"unmatched value:"<<tempval<<endl;
                recurse(tempval);
                //cout <<"Returned from recursion."<<endl;
            }
            }
        }
        return false;
    }
    
    
    bool recurse(int val)
    {
        // Again values are val-dom_min in this function.
        // Clearly this should be turned into a loop.
        //cout << "Entering recurse with value " <<val <<endl;
        if(invprevious.in(val))
        {
            vector<int>& listvars=vprevious[val];  //L
            
            // Remove the value from vprevious.
            invprevious.remove(val);
            
            for(int i=0; i<listvars.size(); ++i)  //for u in L
            {
                int tempvar=listvars[i];
                int pu=uprevious[tempvar];
                if(pu!=-2)   // if u in pred:
                {
                    uprevious[tempvar]=-2;
                    //cout<<"Variable: "<<tempvar<<endl;
                    if(pu==-1 || recurse(pu))
                    {
                        //cout << "Setting "<< tempvar << " to " << val <<endl;
                        
                        if(!valinlocalmatching.in(val))  // If we are not replacing a mapping
                        {
                            valinlocalmatching.insert(val);
                        }
                        
                        valvarmatching[val]=tempvar;
                        //varvalmatching[tempvar]=val+dom_min;  // This will be 
                        return true;
                    }
                }
            }
        }
        return false;
    }
    
    // BFS alternative to hopcroft. --------------------------------------------
    
    inline bool bfs_wrapper(int sccstart, int sccend)
    {
        // Call hopcroft for the whole matching.
        if(!bfsmatching(sccstart, sccend))
        {
            // The constraint is unsatisfiable (no matching).
            D_DATA(cout << "About to fail. Changed varvalmatching: "<< varvalmatching <<endl);
            
            //D_DATA(cout << varinlocalmatching.getlist()<<endl);
            getState(stateObj).setFailed(true);
            //varvalmatching=tempmatching;  // copy back.
            return false;
        }
        
        return true;
    }
    
    deque<int> fifo;
    vector<int> prev;
    vector<int> matchbac;
    // use push_back to push, front() and pop_front() to pop.
    
    //Also use invprevious to record which values are matched.
    // (recording val-dom_min)
    
    inline bool bfsmatching(int sccstart, int sccend)
    {
        // construct the set of matched values.
        invprevious.clear();
        for(int sccindex=sccstart; sccindex<=sccend; sccindex++)
        {
            int var=SCCs[sccindex];
            if(var_array[var].inDomain(varvalmatching[var]))
            {
                invprevious.insert(varvalmatching[var]-dom_min);
            }
        }
        
        // back up the matching to cover failure
        matchbac=varvalmatching;
        
        // iterate through the SCC looking for broken matches
        for(int sccindex=sccstart; sccindex<=sccend; sccindex++)
        {
            int startvar=SCCs[sccindex];
            if(!var_array[startvar].inDomain(varvalmatching[startvar]))
            {
                D_DATA(cout << "Searching for augmenting path for var: " << startvar <<endl );
                // Matching edge lost; BFS search for augmenting path to fix it.
                fifo.clear();  // this should be constant time but probably is not.
                fifo.push_back(startvar);
                visited.clear();
                visited.insert(startvar);
                bool finished=false;
                while(!fifo.empty() && !finished)
                {
                    // pop a vertex and expand it.
                    int curnode=fifo.front();
                    fifo.pop_front();
                    D_DATA(cout << "Popped vertex " << (curnode<numvars? "(var)":"(val)") << (curnode<numvars? curnode : curnode+dom_min-numvars ) <<endl);
                    if(curnode<numvars)
                    { // it's a variable
                        // put all corresponding values in the fifo. 
                        // Need to check if we are completing an even alternating path.
                        for(int val=var_array[curnode].getMin(); val<=var_array[curnode].getMax(); val++)
                        {
                            if(var_array[curnode].inDomain(val) 
                                && val!=varvalmatching[curnode])
                            {
                                if(!invprevious.in(val-dom_min))
                                {
                                    // This vertex completes an even alternating path. 
                                    // Unwind and apply the path here
                                    D_DATA(cout << "Found augmenting path:" <<endl);
                                    int unwindvar=curnode;
                                    int unwindval=val;
                                    while(true)
                                    {
                                        //invprevious.remove(varvalmatching[unwindvar]-dom_min);
                                        D_ASSERT(var_array[unwindvar].inDomain(unwindval));
                                        D_ASSERT(varvalmatching[unwindvar]!=unwindval);
                                        
                                        varvalmatching[unwindvar]=unwindval;
                                        D_DATA(cout << "Setting var "<< unwindvar << " to "<< unwindval <<endl);
                                        
                                        //invprevious.insert(unwindval-dom_min);
                                        
                                        if(unwindvar==startvar)
                                        {
                                            break;
                                        }
                                        
                                        unwindval=prev[unwindvar];
                                        unwindvar=prev[unwindval-dom_min+numvars];
                                    }
                                    
                                    #ifndef NO_DEBUG
                                    cout << "varvalmatching:";
                                    for(int sccindex=sccstart; sccindex<=sccend; sccindex++)
                                    {
                                        if(var_array[SCCs[sccindex]].inDomain(varvalmatching[SCCs[sccindex]]))
                                            cout << SCCs[sccindex] << "->" << varvalmatching[SCCs[sccindex]] << ", ";
                                    }
                                    cout << endl;
                                    #endif
                                    
                                    invprevious.clear();
                                    for(int sccindex=sccstart; sccindex<=sccend; sccindex++)
                                    {
                                        int var=SCCs[sccindex];
                                        if(var_array[var].inDomain(varvalmatching[var]))
                                        {
                                            invprevious.insert(varvalmatching[var]-dom_min);
                                        }
                                    }
                                    
                                    finished=true;
                                    break;  // get out of for loop
                                }
                                else
                                {
                                    if(!visited.in(val-dom_min+numvars))
                                    {
                                        visited.insert(val-dom_min+numvars);
                                        prev[val-dom_min+numvars]=curnode;
                                        fifo.push_back(val-dom_min+numvars);
                                    }
                                }
                            }
                        } // end for
                    }
                    else
                    { // popped a value from the stack.
                        D_ASSERT(curnode>=numvars && curnode < numvars+numvals);
                        int stackval=curnode+dom_min-numvars;
                        int vartoqueue=-1;
                        for(int scci=sccstart; scci<=sccend; scci++)
                        {
                            if(varvalmatching[SCCs[scci]]==stackval && var_array[SCCs[scci]].inDomain(stackval))
                            {
                                vartoqueue=SCCs[scci];
                                break;
                            }
                        }
                        D_ASSERT(vartoqueue>=0);  // if this assertion fails, then invprevious must be wrong.
                        if(!visited.in(vartoqueue)) // I think it's impossible for this test to be false.
                        {
                            visited.insert(vartoqueue);
                            prev[vartoqueue]=stackval;
                            fifo.push_back(vartoqueue);
                        }
                    }
                }
                if(!finished)
                {   // no augmenting path found
                    D_DATA(cout << "No augmenting path found."<<endl);
                    // restore the matching to its state before the algo was called.
                    varvalmatching=matchbac;
                    return false;
                }
                
            }
        }
        return true;
    }
    
};  // end of AlldiffGacSlow
