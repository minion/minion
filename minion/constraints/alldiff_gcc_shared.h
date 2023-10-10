// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef ALLDIFF_GCC_SHARED_H
#define ALLDIFF_GCC_SHARED_H

#include "../triggering/constraint_abstract.h"
#include <vector>

#define REVERSELIST // Is this really necessary now?

#ifdef P
#undef P
#endif

#define P(x)
//#define P(x) cout << x << endl

struct smallset {
  // a small set of integers (could be templated?) which
  // clears in constant time, set membership in
  // constant time and direct iteration.
  // Add an item is constant time, remove is not.

  UnsignedSysInt cert;

  vector<UnsignedSysInt> membership;

  vector<SysInt> list;

  void reserve(SysInt size) {
    // This must be called before anything is put in the set.
    D_ASSERT(membership.size() == 0);
    membership.resize(size, 0);
    list.reserve(size);
    cert = 1;
  }

  inline bool in(SysInt val) {
    return membership[val] == cert;
  }

  inline void insert(SysInt val) {
    D_ASSERT(membership[val] < cert);
    D_ASSERT(val >= 0);
    list.push_back(val);
    membership[val] = cert;
  }

  inline SysInt size() {
    return list.size();
  }

  inline void remove(SysInt val) {
    if(in(val)) {
      membership[val] = 0;
      list.erase(find(list.begin(), list.end(), val));
    }
  }

  inline vector<SysInt>& getlist() {
    return list;
  }

  inline void clear() {
    if(cert > 2000000000) {
      list.clear();
      cert = 1;
      for(SysInt i = 0; i < (SysInt)membership.size(); i++) {
        membership[i] = 0;
      }
    } else {
      cert++;
      list.clear();
    }
  }
};

struct smallset_nolist {
  // a small set of integers (could be templated?) which
  // clears in constant time, set membership in
  // constant time, no iteration
  // Add and remove item is constant time

  UnsignedSysInt cert;

  vector<UnsignedSysInt> membership;

  void reserve(SysInt size) {
    D_ASSERT(membership.size() == 0);
    // This must be called before anything is put in the set.
    membership.resize(size, 0);
    cert = 1;
  }

  inline bool in(DomainInt val) {
    return membership[checked_cast<SysInt>(val)] == cert;
  }

  inline void insert(DomainInt val) {
    D_ASSERT(membership[checked_cast<SysInt>(val)] < cert);
    D_ASSERT(val >= 0);
    membership[checked_cast<SysInt>(val)] = cert;
  }

  inline void remove(DomainInt val) {
    membership[checked_cast<SysInt>(val)] = 0;
  }

  // Use only in debugging/stats functions
  inline SysInt size() {
    SysInt counter = 0;
    for(SysInt i = 0; i < (SysInt)membership.size(); i++) {
      if(in(i))
        counter++;
    }
    return counter;
  }

  inline void clear() {
    if(cert > 2000000000) {
      cert = 1;
      for(SysInt i = 0; i < (SysInt)membership.size(); i++) {
        membership[i] = 0;
      }
    } else {
      cert++;
    }
  }
};

struct smallset_list_bt {
  // This one can only be cleared then populated.
  // Must not be partially populated, then go to
  // a new node, then populated some more.
  // Membership array does not backtrack, clearly.

  UnsignedSysInt cert;

  vector<UnsignedSysInt> membership;

  void* list;
  SysInt maxsize;

  void reserve(SysInt size) {
    // This must be called before anything is put in the set.
    maxsize = size;
    membership.resize(size);

    for(SysInt i = 0; i < size; i++)
      membership[i] = 0;

    cert = 1;
    list = getMemory().backTrack().request_bytes((size + 1) * sizeof(short));
    ((short*)list)[maxsize] = 0; // The count is stored in the last element of the array.
  }

  inline bool in(SysInt val) {
    D_ASSERT(val < maxsize && val >= 0);

    return membership[val] == cert;
  }

  inline void insert(SysInt val) {
    D_ASSERT(val < maxsize && val >= 0);
    // D_DATA(print());

    D_DATA(sanitycheck());
    if(membership[val] == cert) {
      return;
    }
    membership[val] = cert;
    short* ptr = ((short*)list);
    SysInt count = ptr[maxsize];
    D_ASSERT(count < maxsize);
    ptr[maxsize] = (short)count + 1;
    ptr[count] = (short)val;
    // D_DATA(print());
    D_DATA(sanitycheck());
  }

  inline void clear() {
    D_DATA(cout << "clearing list " << (list) << endl);
    D_ASSERT(cert < 2000000000);

    if(cert > 2000000000) {
      cert = 1;
      for(SysInt i = 0; i < (SysInt)membership.size(); i++) {
        membership[i] = 0;
      }
    } else {
      cert++;
    }

    ((short*)list)[maxsize] = 0;
  }

  SysInt size() {
    return (SysInt)((short*)list)[maxsize];
  }

  void sanitycheck() {
    short* l = (short*)list;
    for(SysInt i = 0; i < l[maxsize]; i++) {
      for(SysInt j = i + 1; j < l[maxsize]; j++) {
        D_ASSERT(l[i] != l[j]);
      }
      D_ASSERT(membership[l[i]] == cert);
    }
  }

  void print() {
    short* l = (short*)list;
    cout << "smallset_list_bt length:" << l[maxsize] << " at location " << (&l[maxsize]) << endl;
    for(SysInt i = 0; i < maxsize; i++) {
      cout << "smallset_list_bt item:" << l[i] << " at location " << (&l[i]) << endl;
    }
    cout << "certificate:" << cert << endl;
    cout << membership << endl;
  }
};

template <typename VarArray, bool UseIncGraph>
struct FlowConstraint : public AbstractConstraint {
protected:
  // Base class for GAC alldiff and GCC
  FlowConstraint(const VarArray& _varArray)
      : numvars(0),
        numvals(0),
        domMin(0),
        domMax(0),
#ifndef REVERSELIST
        varArray(_varArray),
#else
        varArray(_varArray.rbegin(), _varArray.rend()),
#endif
        constraintLocked(false) {
    if(varArray.size() > 0) {
      domMin = checked_cast<SysInt>(varArray[0].initialMin());
      domMax = checked_cast<SysInt>(varArray[0].initialMax());
    }
    for(SysInt i = 0; i < (SysInt)varArray.size(); ++i) {
      if(varArray[i].initialMin() < domMin)
        domMin = checked_cast<SysInt>(varArray[i].initialMin());
      if(varArray[i].initialMax() > domMax)
        domMax = checked_cast<SysInt>(varArray[i].initialMax());
    }
    numvars = varArray.size(); // number of variables in the constraint
    numvals = domMax - domMin + 1;

    // to_process.reserve(varArray.size()); Could this be shared as well??

    if(UseIncGraph) {
      // refactor this to use initial upper and lower bounds.
      adjlist.resize(numvars + numvals);
      adjlistpos.resize(numvars + numvals);
      for(SysInt i = 0; i < numvars; i++) {
        adjlist[i].resize(numvals);
        for(SysInt j = 0; j < numvals; j++)
          adjlist[i][j] = j + domMin;
        adjlistpos[i].resize(numvals);
        for(SysInt j = 0; j < numvals; j++)
          adjlistpos[i][j] = j;
      }
      for(SysInt i = numvars; i < numvars + numvals; i++) {
        adjlist[i].resize(numvars);
        for(SysInt j = 0; j < numvars; j++)
          adjlist[i][j] = j;
        adjlistpos[i].resize(numvars);
        for(SysInt j = 0; j < numvars; j++)
          adjlistpos[i][j] = j;
      }
      adjlistlength = getMemory().backTrack().template requestArray<SysInt>(numvars + numvals);
      for(SysInt i = 0; i < numvars; i++)
        adjlistlength[i] = numvals;
      for(SysInt i = numvars; i < numvars + numvals; i++)
        adjlistlength[i] = numvars;
    }

#ifndef BTMATCHING
    varvalmatching.resize(numvars); // maps var to actual value
    valvarmatching.resize(numvals); // maps val-domMin to var.
#else
    varvalmatching = getMemory().backTrack().template requestArray<SysInt>(numvars);
    valvarmatching = getMemory().backTrack().template requestArray<SysInt>(numvals);
#endif
  }

  SysInt numvars, numvals, domMin, domMax;

  VarArray varArray;

  bool constraintLocked;

#ifndef BTMATCHING
  vector<SysInt> varvalmatching; // For each var, give the matching value.
  // valvarmatching is from val-domMin to var.
  vector<SysInt> valvarmatching; // need to set size somewhere.
// -1 means unmatched.
#else
  SysInt* varvalmatching;
  SysInt* valvarmatching;
#endif

  // ------------------ Incremental adjacency lists --------------------------

  // adjlist[varnum or val-domMin+numvars] is the vector of vals in the
  // domain of the variable, or variables with val in their domain.
  vector<vector<SysInt>> adjlist;
  SysInt* adjlistlength;
  vector<vector<SysInt>> adjlistpos; // position of a variable in adjlist.

  inline void adjlist_remove(SysInt var, SysInt val) {
    // swap item at position varidx to the end, then reduce the length by 1.
    SysInt validx = val - domMin + numvars;
    SysInt varidx = adjlistpos[validx][var];
    D_ASSERT(varidx < adjlistlength[validx]); // var is actually in the list.
    delfromlist(validx, varidx);

    delfromlist(var, adjlistpos[var][val - domMin]);
  }

  inline void delfromlist(SysInt i, SysInt j) {
    // delete item in list i at position j
    SysInt t = adjlist[i][adjlistlength[i] - 1];
    adjlist[i][adjlistlength[i] - 1] = adjlist[i][j];

    if(i < numvars) {
      adjlistpos[i][adjlist[i][j] - domMin] = adjlistlength[i] - 1;
      adjlistpos[i][t - domMin] = j;
    } else {
      adjlistpos[i][adjlist[i][j]] = adjlistlength[i] - 1;
      adjlistpos[i][t] = j;
    }
    adjlist[i][j] = t;
    adjlistlength[i] = adjlistlength[i] - 1;
  }

  void check_adjlists() {
    for(SysInt i = 0; i < numvars; i++) {
      D_ASSERT(varArray[i].min() >= domMin);
      D_ASSERT(varArray[i].max() <= domMax);
      for(SysInt j = domMin; j <= domMax; j++) {
        D_DATA(bool in = adjlistpos[i][j - domMin] < adjlistlength[i]);
        D_DATA(bool in2 =
                   adjlistpos[j - domMin + numvars][i] < adjlistlength[j - domMin + numvars]);
        D_ASSERT(in == in2);
        D_ASSERT(in == varArray[i].inDomain(j));
      }
    }
  }

  // -------------------------Hopcroft-Karp algorithm
  // -----------------------------
  // Can be applied to a subset of varArray as required.

  // Each domain value has a label which is numvars+

  // These two are for the valvar version of hopcroft.
  smallset_nolist varinlocalmatching; // indicates whether a var is recorded in
                                      // localmatching.
  smallset valinlocalmatching;

  // smallset varinlocalmatching;    // indicates whether a var is recorded in
  // localmatching.
  // smallset_nolist valinlocalmatching;

  // Uprevious (pred) gives (for each CSP value) the value-domMin
  // it was matched to in the previous layer. If it was unmatched,
  // -1 is used.
  vector<SysInt> uprevious; // -2 means unset, -1 labelled unmatched.

  vector<vector<SysInt>> vprevious; // map val-domMin to vector of vars.
  smallset_nolist invprevious;      // is there a mapping in vprevious for val?
                                    // Allows fast unset.

  smallset layer;
  smallset unmatched; // contains vals-domMin.

  vector<vector<SysInt>> newlayer;
  smallset innewlayer;

  void initialize_hopcroft() {
    // Initialize all datastructures to do with hopcroft-karp
    // Surely could reduce the number of arrays etc used for hopcroft-karp??
    varinlocalmatching.reserve(numvars);
    valinlocalmatching.reserve(numvals);
    uprevious.resize(numvars, -2);

    vprevious.resize(numvals);
    for(SysInt i = 0; i < numvals; ++i) {
      vprevious[i].reserve(numvars);
    }
    invprevious.reserve(numvals);

    layer.reserve(numvars);
    unmatched.reserve(numvals);

    newlayer.resize(numvals);
    for(SysInt i = 0; i < numvals; ++i) {
      newlayer[i].reserve(numvars);
    }
    innewlayer.reserve(numvals);
  }

  // Hopcroft-Karp which takes start and end indices.

  inline bool hopcroft_wrapper(SysInt sccstart, SysInt sccend, vector<SysInt>& SCCs,
                               bool allowedToFail) {
    // Call hopcroft for the whole matching.
    if(!hopcroft(sccstart, sccend, SCCs)) {
      // The constraint is unsatisfiable (no matching).
      P("About to fail. Changed varvalmatching: " << varvalmatching);

      for(SysInt j = 0; j < numvars; j++) {
        // Restore valvarmatching because it might be messed up by Hopcroft.
        valvarmatching[varvalmatching[j] - domMin] = j;
      }

      if(allowedToFail)
        getState().setFailed(true);
      return false;
    }

    // Here, copy from valvarmatching to varvalmatching.
    // Using valinlocalmatching left over from hopcroft.
    // This must not be done when failing, because it might mess
    // up varvalmatching for the next invocation.
    {
      vector<SysInt>& toiterate = valinlocalmatching.getlist();
      for(SysInt j = 0; j < (SysInt)toiterate.size(); j++) {
        SysInt tempval = toiterate[j];
        varvalmatching[valvarmatching[tempval]] = tempval + domMin;
      }
    }
    return true;
  }

  inline bool hopcroft(SysInt sccstart, SysInt sccend, vector<SysInt>& SCCs) {
    // Domain value convention:
    // Within hopcroft and recurse,
    // a domain value is represented as val-domMin always.

    // Variables are always represented as their index in
    // varArray. sccstart and sccend indicates which variables
    // we are allowed to use here.

    SysInt localnumvars = sccend - sccstart + 1;

    // Construct the valinlocalmatching for this SCC, checking each val
    // to see it's in the relevant domain.
    valinlocalmatching.clear();

    for(SysInt i = sccstart; i <= sccend; i++) {
      SysInt tempvar = SCCs[i];
      if(varArray[tempvar].inDomain(varvalmatching[tempvar])) {
        valinlocalmatching.insert(varvalmatching[tempvar] - domMin);
        // Check the two matching arrays correspond.
        // D_ASSERT(valvarmatching[varvalmatching[tempvar]-domMin]==tempvar);
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

    if(valinlocalmatching.size() == localnumvars) {
      return true;
    }

    // uprevious == pred
    // vprevious == preds

    // need sets u and v
    // u is easy, v is union of domains[0..numvar-1]

    while(true) {
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
      // Because it's left over from a bad state when do_prop was last invoked,
      // and failed.
      varinlocalmatching.clear();
      {
        vector<SysInt>& toiterate = valinlocalmatching.getlist();
        for(SysInt i = 0; i < (SysInt)toiterate.size(); ++i) {
          if(!varinlocalmatching.in(valvarmatching[toiterate[i]])) // This should not be
                                                                   // conditional --BUG
            varinlocalmatching.insert(valvarmatching[toiterate[i]]);
        }
      }

      for(SysInt i = sccstart; i <= sccend; ++i) {
        SysInt tempvar = SCCs[i];
        if(varinlocalmatching.in(tempvar)) // The only use of varinlocalmatching.
        {
          uprevious[tempvar] = -2; // Out of uprevious
        } else {
          layer.insert(tempvar);
          uprevious[tempvar] = -1; // In layer, and set to unmatched in uprevious.
        }
      }

      /*cout<< "Uprevious:" <<endl;
      for(SysInt i=0; i<localnumvars; ++i)
      {
          cout<< "for variable "<<var_indices[i]<<" value "<<
      uprevious[i]<<endl;
      }*/

      // we have now calculated layer
      /*
      while layer and not unmatched:
          newLayer = {}
      */

      while(layer.size() != 0 && unmatched.size() == 0) {
        innewlayer.clear();

        /*
        for u in layer:
            for v in graph[u]:
                if v not in preds:
                    newLayer.setdefault(v,[]).append(u)
        */
        {
          vector<SysInt>& toiterate = layer.getlist();
          for(SysInt i = 0; i < (SysInt)toiterate.size(); ++i) {
            // cout<<"Layer item: "<<(*setit)<<endl;
            SysInt tempvar = toiterate[i];
            for(DomainInt realval = varArray[tempvar].min();
                realval <= varArray[tempvar].max(); realval++) {
              if(varArray[tempvar].inDomain(realval)) {
                SysInt tempval = checked_cast<SysInt>(realval - domMin);

                if(!invprevious.in(tempval)) // if tempval not found in vprevious
                {
                  if(!innewlayer.in(tempval)) {
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
        vector<SysInt>& toiterate = valinlocalmatching.getlist();
        for(SysInt i=0; i<(SysInt)toiterate.size(); ++i)
        {
            SysInt temp=toiterate[i];
            D_ASSERT(varinlocalmatching.in(localmatching[temp]));
            cout << "mapping "<< localmatching[temp] << " to value " << temp
        <<endl;
        }
        }*/

        {
          vector<SysInt>& toiterate = innewlayer.getlist();
          for(SysInt i = 0; i < (SysInt)toiterate.size(); ++i) {
            SysInt tempval = toiterate[i]; // for v in newlayer.
            // cout << "Looping for value "<< tempval <<endl;

            D_ASSERT(innewlayer.in(tempval));
            // insert mapping in vprevious
            invprevious.insert(tempval);

            vprevious[tempval] = newlayer[tempval]; // This should be a copy???
            /*vprevious[tempval].resize(newlayer[tempval].size());
            for(SysInt x=0; x<(SysInt)newlayer[tempval].size(); x++)
            {
                vprevious[tempval][x]=newlayer[tempval][x];
            }*/

            if(valinlocalmatching.in(tempval)) {
              SysInt match = valvarmatching[tempval];
              // cout << "Matched to variable:" << match << endl;
              layer.insert(match);
              uprevious[match] = tempval;
            } else {
              // cout<<"inserting value into unmatched:"<<tempval<<endl;
              unmatched.insert(tempval);
            }
          }
        }

        // cout << "At end of layering loop." << endl;
      }
      // cout << "Out of layering loop."<<endl;
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
      // cout << "Unmatched size:" << unmatched.size() << endl;
      if(unmatched.size() == 0) {
        // cout << "Size of matching:" << valinlocalmatching.size() << endl;

        if(valinlocalmatching.size() == localnumvars) {
          return true;
        } else {
          return false;
        }
      }

      /*
      for v in unmatched: recurse(v)
      */
      {
        vector<SysInt>& toiterate = unmatched.getlist();
        for(SysInt i = 0; i < (SysInt)toiterate.size(); ++i) {
          SysInt tempval = toiterate[i];
          // cout<<"unmatched value:"<<tempval<<endl;
          recurse(tempval);
          // cout <<"Returned from recursion."<<endl;
        }
      }
    }
    return false;
  }

  bool recurse(SysInt val) {
    // Again values are val-domMin in this function.
    // Clearly this should be turned into a loop.
    // cout << "Entering recurse with value " <<val <<endl;
    if(invprevious.in(val)) {
      vector<SysInt>& listvars = vprevious[val]; // L

      // Remove the value from vprevious.
      invprevious.remove(val);

      for(SysInt i = 0; i < (SysInt)listvars.size(); ++i) // for u in L
      {
        SysInt tempvar = listvars[i];
        SysInt pu = uprevious[tempvar];
        if(pu != -2) // if u in pred:
        {
          uprevious[tempvar] = -2;
          // cout<<"Variable: "<<tempvar<<endl;
          if(pu == -1 || recurse(pu)) {
            // cout << "Setting "<< tempvar << " to " << val <<endl;

            if(!valinlocalmatching.in(val)) // If we are not replacing a mapping
            {
              valinlocalmatching.insert(val);
            }

            valvarmatching[val] = tempvar;
            // varvalmatching[tempvar]=val+domMin;  // This will be
            return true;
          }
        }
      }
    }
    return false;
  }

  // ----------------------- new hopcroft-karp implementation ----------------

  // This one uses domMin-1 as a marker for 'free variable' in matching.
  // also has upper as the upper bound for value nodes, indexed by val+domMin
  // usage is the occurrences of each value in the matching.

  // Oh no -- does this work with SCCs??
  // First do it without using SCCs.

  vector<vector<SysInt>> edges; // turn this into a box of boxes??
  smallset_nolist varvalused;
  smallset thislayer;
  deque<SysInt> fifo;
  vector<SysInt> augpath; // alternating path stored here with vars and val-domMin

  void hopcroft2Setup() {
    edges.resize(numvars + numvals + 1);
    for(SysInt i = 0; i < numvars; i++) {
      edges.reserve(numvals);
    }
    for(SysInt i = numvars; i <= numvars + numvals; i++) {
      edges.reserve(numvars);
    }
    varvalused.reserve(numvars + numvals);
    thislayer.reserve(numvars + numvals);
  }

  inline bool hopcroft_wrapper2(vector<SysInt>& vars_in_scc, vector<SysInt>& matching,
                                vector<SysInt>& upper, vector<SysInt>& usage) {
    if(!hopcroft2(vars_in_scc, matching, upper, usage)) {
      getState().setFailed(true);
      return false;
    }
    return true;
  }

  inline bool hopcroft2(vector<SysInt>& vars_in_scc, vector<SysInt>& matching,
                        vector<SysInt>& upper, vector<SysInt>& usage) {
    // The return value is whether the matching is complete over teh variables
    // in the SCC.
    // Clear any values from matching which are no longer in domain.
    // Clear vals if their usage is larger than the upper bound.
    for(SysInt i = 0; i < (SysInt)vars_in_scc.size(); i++) {
      SysInt var = vars_in_scc[i];
      if(matching[var] != domMin - 1) {
        SysInt match = matching[var];
        if(!varArray[var].inDomain(match) || usage[match - domMin] > upper[match - domMin]) {
          usage[match - domMin]--;
          matching[var] = domMin - 1;
        }
      }
    }

    // in here vars are numbered 0.. numvars-1, vals: numvars..numvars+numvals-1

    // a value node with cap>1 will only appear in one layer,
    // but the DFS is allowed to visit it multiple times.
    // The DFS is not allowed to traverse an edge more than once.

    // darn, does the DFS visit nodes it is not supposed to?

    while(true) {
      // Find all free variables in current SCC and insert into edges
      edges[numvars + numvals].clear();
      varvalused.clear();
      fifo.clear();

      SysInt unmatched = 0;
      for(SysInt i = 0; i < (SysInt)vars_in_scc.size(); ++i) {
        SysInt tempvar = vars_in_scc[i];
        if(matching[tempvar] == domMin - 1) {
          edges[numvars + numvals].push_back(tempvar);
          edges[tempvar].clear();
          fifo.push_back(tempvar);
          varvalused.insert(tempvar);
          unmatched++;
        }
      }

      if(unmatched == 0) {
        return true;
      }

      // BFS until we see a free value vertex.

      bool foundFreeValNode = false;
      while(!fifo.empty()) {
        // first process a layer of vars
        while(!fifo.empty() && fifo.front() < numvars) {
          SysInt curnode = fifo.front();
          fifo.pop_front();
          // curnode is a variable.
          // next layer is adjacent values which are not saturated.
          for(SysInt i = 0; i < adjlistlength[curnode]; i++) {
            SysInt realval = adjlist[curnode][i];
            SysInt validx = realval - domMin + numvars;
            if(!varvalused.in(validx)) {
              edges[curnode].push_back(validx);

              if(!thislayer.in(validx)) { // have not seen this value before.
                // add it to the new layer.
                thislayer.insert(validx);

                fifo.push_back(validx);
                edges[validx].clear();
              }
              if(usage[realval - domMin] < upper[realval - domMin]) {
                foundFreeValNode = true;
              }
            }
          }
        }

        // transfer things from thislayer to varvalused.
        vector<SysInt>& temp1 = thislayer.getlist();
        for(SysInt i = 0; i < (SysInt)temp1.size(); i++) {
          varvalused.insert(temp1[i]);
        }
        thislayer.clear();

        if(foundFreeValNode) { // we have seen at least one unsaturated value
                               // vertex and
          // must have expanded all variable vertices in the
          // layer above.
          break;
        }

        while(!fifo.empty() && fifo.front() >= numvars) {
          SysInt curnode = fifo.front();
          fifo.pop_front();
          // curnode is a value
          // next layer is variables, following matching edges.
          for(SysInt i = 0; i < adjlistlength[curnode]; i++) {
            SysInt var = adjlist[curnode][i];
            if(!varvalused.in(var) && matching[var] == curnode + domMin - numvars) {
              edges[curnode].push_back(var);
              if(!thislayer.in(var)) { // have not seen this variable before.
                // add it to the new layer.
                thislayer.insert(var);

                fifo.push_back(var);
                edges[var].clear();
              }
            }
          }
        }

        // transfer things from thislayer to varvalused.
        vector<SysInt>& temp2 = thislayer.getlist();
        for(SysInt i = 0; i < (SysInt)temp2.size(); i++) {
          varvalused.insert(temp2[i]);
        }
        thislayer.clear();

      } // end of BFS loop.

      if(foundFreeValNode) {
        // Find a set of minimal-length augmenting paths using DFS within
        // the edges ds.
        // starting at layer 0.

        for(SysInt i = 0; i < (SysInt)edges[numvars + numvals].size(); i++) {
          augpath.clear();
          augpath.push_back(edges[numvars + numvals][i]);
          dfs_hopcroft2(augpath, upper, usage, matching, edges);
        }
      } else {
        return false;
      }

    } // end of main loop.

    // should not be possible to reach here.
    D_ASSERT(false);
    return false;
  }

  // return value indicates whether an augmenting path was found.
  // DFS can visit a value vertex multiple times up to upper-usage,
  // but can only use an edge once.
  bool dfs_hopcroft2(vector<SysInt>& augpath, vector<SysInt>& upper, vector<SysInt>& usage,
                     vector<SysInt>& matching, vector<vector<SysInt>>& edges) {
    SysInt var = augpath.back();
    vector<SysInt>& outedges = edges[var];

    while(!outedges.empty()) {
      SysInt validx = outedges.back();
      outedges.pop_back();
      D_ASSERT(varArray[var].inDomain(validx - numvars + domMin));

      // does this complete an augmenting path?
      if(usage[validx - numvars] < upper[validx - numvars]) {
        augpath.push_back(validx);
        apply_augmenting_path(augpath, matching, usage);
        return true;
      }

      vector<SysInt>& outedges2 = edges[validx];

      augpath.push_back(validx);
      while(!outedges2.empty()) {
        SysInt var2 = outedges2.back();
        outedges2.pop_back();

        augpath.push_back(var2);
        if(dfs_hopcroft2(augpath, upper, usage, matching, edges)) {
          return true;
        }
        augpath.pop_back(); // remove var2
      }
      augpath.pop_back(); // remove validx
    }
    return false;
  }

  inline void apply_augmenting_path(vector<SysInt>& augpath, vector<SysInt>& matching,
                                    vector<SysInt>& usage) {
    D_ASSERT((augpath.size() & 1) == 0);
    for(SysInt i = 0; i < (SysInt)augpath.size(); i = i + 2) {
      SysInt var = augpath[i];
      SysInt validx = augpath[i + 1];
      if(matching[var] != domMin - 1) {
        usage[matching[var] - domMin]--;
      }
      matching[var] = validx - numvars + domMin;
      D_ASSERT(varArray[var].inDomain(validx - numvars + domMin));
      usage[validx - numvars]++;
    }
    augpath.clear();
  }
};

struct deque_fixedSize {
  // replacement for stl deque. This one is a fixed size circular array.
  // pluggable for deque in gcc_common.h -- no faster.
  vector<SysInt> list;
  SysInt head, tail;

  deque_fixedSize() {
    head = tail = 0;
  }

  void reserve(SysInt size) {
    list.resize(size);
  }

  inline void clear() {
    head = tail = 0;
  }

  inline bool empty() {
    return head == tail;
  }

  inline void push_back(SysInt val) {
    list[tail] = val;
    if(++tail == (SysInt)list.size()) {
      tail = 0;
    }
  }

  inline SysInt front() {
    D_ASSERT(head != tail);
    return list[head];
  }

  inline void pop_front() {
    D_ASSERT(head != tail);
    if(++head == (SysInt)list.size()) {
      head = 0;
    }
  }
};

// This class contains backtracking memory with lists of important values for
// each
// variable.
template <typename VarArray>
struct InternalDynamicTriggers {
  short* watches; // should also template on the type here.

  // watches contains:
  // [0.. numvars-1]  indices to the start of the list for each variable.
  // [numvars] index to the start of the free list.
  // [numvars+1 .. 5*numvars+2*numvals+1] cells, that are pairs of adjacent
  // shorts, <val, nextidx>.
  // -1 for nextidx means end of list.
  SysInt numvars;

  VarArray varArray; // not nice to have this in here..

  InternalDynamicTriggers(SysInt _numvars, SysInt numvals, VarArray _varArray)
      : numvars(_numvars), varArray(_varArray) {
    watches = getMemory().backTrack().template requestArray<short>(numvars + 1 + 4 * numvars +
                                                                   2 * numvals);

    for(SysInt i = 0; i < numvars; i++)
      watches[i] = -1;
    watches[numvars] = numvars + 1;
    for(SysInt i = numvars + 2; i < (numvars + 1 + 4 * numvars + 2 * numvals);
        i = i + 2) { // link up the freelist.
      watches[i] = i + 1;
    }
    watches[numvars + 1 + 4 * numvars + 2 * numvals - 1] = -1;
  }

  inline bool doesItTrigger(SysInt var) { // does the changed variable var
                                          // trigger propagation on the target
                                          // variables.
    SysInt idx = watches[var];            // start of linked list.
    if(idx == -1) {                       // must be the first call, because otherwise all variables
                                          // would have at least two important values.
      return true;
    }
    while(idx != -1) {
      if(!varArray[var].inDomain(watches[idx])) {
        return true;
      }
      idx = watches[idx + 1]; // go to next.
    }
    return false;
  }

  inline void addwatch(SysInt var, SysInt val) {
    // cout << "In addwatch. var:" << var << " val:" << val << endl;
    // printlist(var);

    // chop out the first elelemnt in the free list
    SysInt idx = watches[numvars];
    D_ASSERT(idx != -1);
    watches[numvars] = watches[idx + 1];

    watches[idx] = val;

    // splice into list for var at head.
    watches[idx + 1] = watches[var];
    watches[var] = idx;
    // printlist(var);

    // cout << "Exiting addwatch" <<endl;
  }

  void printlist(SysInt var) {
    cout << "Var: " << var << " values: ";
    SysInt idx = watches[var];
    while(idx != -1) {
      cout << watches[idx] << " ";
      idx = watches[idx + 1];
    }
    cout << endl;
  }

  inline void clearwatches(SysInt var) {
    // go through and find end of list.
    // cout << "In clearwatches for var: "<<var <<endl;
    SysInt idx = watches[var];
    if(idx == -1)
      return;

    // find the end of the list
    while(watches[idx + 1] != -1) {
      idx = watches[idx + 1]; // next
    }
    // splice list into freelist.
    watches[idx + 1] = watches[numvars];
    watches[numvars] = watches[var];
    watches[var] = -1;

    // cout << "Leaving clearwatches" <<endl;
  }
};

#endif
