// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#include "minion.h"

#include "preprocess.h"


//  Function to collect AMOs between pairs of boolean or 0/1 variables for Savile Row's
//  AMO detect function. 
void collectAMOs(vector<AnyVarRef>& vars) {
  PropagateGAC propGAC(PropLevel_GAC);
  if(getOptions().gatherAMOs) {
    // Extra pass to collect the mutexes.
    // Two versions -- a sparse one (where the scopes of the PB constraints have been passed in) and a dense one. 
    
    vector<AnyVarRef> adjlists = getAnyVarRefFromVar(getState().getInstance()->mutexDetectList2);
    
    if(adjlists.empty()) {
      // No adjacency lists passed in. Dense version. 
      // Populate listbools.
      std::vector<AnyVarRef> listbools;
      
      for(int i = 0; i < vars.size(); i++) {
        if(vars[i].min() == 0 && vars[i].max() == 1) {
          listbools.push_back(vars[i]);
        }
      }
      
      //  Make a 'collect events' constraint and attach it to listbools.
      CollectEvents<std::vector<AnyVarRef>>* c = new CollectEvents<std::vector<AnyVarRef>>(listbools);
      getState().addConstraint((AbstractConstraint*)c);
      
      std::vector<std::pair<int, DomainInt>>& assignments = c->assignments;
      
      std::vector<int> listallpairs;
      
      getOutput() << "BOOLNAMES ";
      for(int i = 0; i < listbools.size(); i++) {
        getOutput() << getNameFromVar(listbools[i]);
        if(i < listbools.size() - 1) {
          getOutput() << " ";
        }
      }
      getOutput() << std::endl;
      
      for(SysInt i = 0; i < (SysInt)listbools.size(); ++i) {
        AnyVarRef& var = listbools[i];
        
        c->liftTriggersLessEqual(i);
        
        Controller::worldPush();
        
        var.setMax(0);
        propGAC(vars);
        
        for(int j = 0; j < assignments.size(); j++) {
          listallpairs.push_back(-i - 1);
          listallpairs.push_back((assignments[j].second == 1) ? (-assignments[j].first - 1)
                                                              : (assignments[j].first + 1));
        }
        assignments.clear();
        
        getState().setFailed(false);
        
        Controller::worldPop();
        
        Controller::worldPush();
      
        var.setMin(1);
        propGAC(vars);
      
        for(int j = 0; j < assignments.size(); j++) {
          listallpairs.push_back(i + 1);
          listallpairs.push_back((assignments[j].second == 1) ? (-assignments[j].first - 1)
                                                              : (assignments[j].first + 1));
        }
        assignments.clear();
      
        getState().setFailed(false);
      
        Controller::worldPop();
      }
      
      getOutput() << "AMO " << listallpairs.size() / 2 << " ";
      for(int i = 0; i < listallpairs.size(); i++) {
        getOutput() << listallpairs[i];
        if(i < listallpairs.size() - 1) {
          getOutput() << " ";
        }
      }
      getOutput() << std::endl;
    }
    else {
      //  Adjacency lists passed in. Do sparse version. 
      // Populate listbools with inverse
      vector<AnyVarRef> listbools;
      unordered_map<string, int> listbools_inv;
      
      int n=0;
      for(int i = 0; i < vars.size(); i++) {
        if(vars[i].min() == 0 && vars[i].max() == 1) {
          listbools.push_back(vars[i]);
          listbools_inv[getNameFromVar(vars[i])]=n;
          n++;
        }
      }
      
      getOutput() << "BOOLNAMES ";
      for(int i = 0; i < listbools.size(); i++) {
        getOutput() << getNameFromVar(listbools[i]);
        if(i < listbools.size() - 1) {
          getOutput() << " ";
        }
      }
      getOutput() << std::endl;
      
      //  Split the big adjacency list into multiple lists. 
      vector<vector<AnyVarRef>> adj;
      vector<AnyVarRef> f;
      adj.push_back(f);
      
      for(int i = 0; i < adjlists.size(); i=i+2) {
        if(adjlists[i].min()==0 && adjlists[i].max()==0 && adjlists[i+1].min()==0 && adjlists[i+1].max()==0) {
          // Start a new list.
          vector<AnyVarRef> n;
          adj.push_back(n);
        }
        else {
          adj.back().push_back(adjlists[i]);
          adj.back().push_back(adjlists[i+1]);
        }
      }
      
      if(adj.back().empty()) {
        adj.pop_back();
      }
      
      std::vector<int> listallpairs;
      
      for(int i=0; i<adj.size(); i++) {
        vector<AnyVarRef>& adjlist=adj[i];
        
        //  First items are a pair var,val to assign 
        string varname=getNameFromVar(adjlist[0]);
        if(! (listbools_inv.count(varname)==1)) {
          continue;
        }
        int varidx=listbools_inv[varname];
        
        Controller::worldPush();
        AnyVarRef& var=adjlist[0];
        DomainInt val=adjlist[1].min();
        
        var.setMax(val);
        var.setMin(val);
        propGAC(vars);
        
        //  Check the other entries for mutexes. 
        for(int j=2; j<adjlist.size(); j=j+2) {
          // check if the literal k has been pruned. 
          if(adjlist[j+1].min()==0 && adjlist[j].min()>0) {
            string varname2=getNameFromVar(adjlist[j]);
            if(listbools_inv.count(varname2)==1) {
              listallpairs.push_back( (val==1) ? (varidx+1) : (-varidx-1) );
              int varidx2=listbools_inv[varname2];
              listallpairs.push_back( -varidx2-1 );
            }
          }
          else if(adjlist[j+1].min()==1 && adjlist[j].max()<1) {
            string varname2=getNameFromVar(adjlist[j]);
            if(listbools_inv.count(varname2)==1) {
              listallpairs.push_back( (val==1) ? (varidx+1) : (-varidx-1) );
              int varidx2=listbools_inv[varname2];
              listallpairs.push_back( varidx2+1 );
            }
          }
        }
        // revert.
        getState().setFailed(false);
        Controller::worldPop();
      }
      
      //  Output
      getOutput() << "AMO " << listallpairs.size() / 2 << " ";
      for(int i = 0; i < listallpairs.size(); i++) {
        getOutput() << listallpairs[i];
        if(i < listallpairs.size() - 1) {
          getOutput() << " ";
        }
      }
      getOutput() << std::endl;
    }
  }
  
  if(getOptions().gatherAMOsExtra) {
    // Extra pass to collect the mutexes.  Strong version. 
    
    //  List of (var, val, var, val) to test. 
    vector<AnyVarRef> testMutexes = getAnyVarRefFromVar(getState().getInstance()->mutexDetectList);
    
    // Different output format. Just returns 0 or 1 for each set of 4 entries in testMutexes. 
    
    getOutput() << "AMO ";
    
    for(SysInt i = 0; i < (SysInt)testMutexes.size(); i=i+4) {
      AnyVarRef& var1 = testMutexes[i];
      AnyVarRef& var2 = testMutexes[i+2];

      DomainInt val1 = testMutexes[i+1].min();
      DomainInt val2 = testMutexes[i+3].min();

      Controller::worldPush();
      var1.setMax(val1);
      var1.setMin(val1);
      var2.setMax(val2);
      var2.setMin(val2);
      propGAC(vars);

      if(getState().isFailed()) {
        //  1 means mutex
        getOutput() << "1";
      }
      else {
        getOutput() << "0";
      }
      
      getState().setFailed(false);
      Controller::worldPop();
    }
    
    getOutput() << std::endl;
  }
}



/// Apply a high level of consistency to a CSP.
/** This function is not particularly optimised, implementing only the most
 * basic SAC and SSAC algorithms */
void PropogateCSP(PropagationLevel preprocessLevel, vector<AnyVarRef>& vars, bool printInfo) {
  if(preprocessLevel.type == PropLevel_None) {
    return;
  }

  PropagateGAC propGAC(preprocessLevel);
  propGAC(vars);

  if(preprocessLevel.type == PropLevel_GAC) {
    return;
  }

  DomainInt lits = litCount(vars);
  bool boundsCheck = ((preprocessLevel.type == PropLevel_SACBounds) ||
                      (preprocessLevel.type == PropLevel_SSACBounds));

  if(boundsCheck) {
    PropagateSAC_Bounds prop_SACBounds(preprocessLevel);
    prop_SACBounds(vars);
  } else {
    PropagateSAC prop_SAC(preprocessLevel);
    prop_SAC(vars);
  }

  if(printInfo) {
    getOutput() << "SAC" << (boundsCheck ? "Bounds" : "") << " Removed " << (lits - litCount(vars))
         << " literals" << endl;
  }

  if(getState().isFailed()) {
    return;
  }

  if(preprocessLevel.type == PropLevel_SAC || preprocessLevel.type == PropLevel_SACBounds) {
    return;
  }

  lits = litCount(vars);
  if(boundsCheck) {
    PropagateSSAC_Bounds prop_SSACBounds(preprocessLevel);
    prop_SSACBounds(vars);
  } else {
    PropagateSSAC prop_SSAC(preprocessLevel);
    prop_SSAC(vars);
  }
  if(printInfo) {
    getOutput() << "SSAC" << (boundsCheck ? "Bounds" : "") << " Removed " << (lits - litCount(vars))
         << " literals" << endl;
  }
}
