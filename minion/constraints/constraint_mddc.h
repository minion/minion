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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
* USA.
*/

/** @help constraints;mddc Description
MDDC (mddc) is an implementation of MDDC(sp) by Cheng and Yap. It enforces GAC
on a
constraint using a multi-valued decision diagram (MDD).

The MDD required for the propagator is constructed from a set of satisfying
tuples. The constraint has the same syntax as 'table' and can function
as a drop-in replacement.

For examples on how to call it, see the help for 'table'. Substitute 'mddc' for
'table'.
*/

/** @help constraints;mddc Notes
This constraint enforces generalized arc consistency.
*/

/** @help constraints;negativemddc Description
Negative MDDC (negativemddc) is an implementation of MDDC(sp) by Cheng and Yap.
It enforces GAC on a constraint using a multi-valued decision diagram (MDD).

The MDD required for the propagator is constructed from a set of unsatisfying
(negative) tuples. The constraint has the same syntax as 'negativetable' and
can function as a drop-in replacement.
*/

/** @help constraints;negativemddc Notes
This constraint enforces generalized arc consistency.
*/

#ifndef CONSTRAINT_MDDC_H
#define CONSTRAINT_MDDC_H

#include <stdlib.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include "constraint_checkassign.h"

using namespace std;

// MDD constraint
// Implemented as close as possible to the paper by Cheng and Yap.
// Uses sparse sets.

#include "arrayset.h"

// Backtracking version of above. Used for gno set.
struct arrayset_bt {
  vector<SysInt> vals;
  vector<SysInt> vals_pos;
  ReversibleInt size;
  DomainInt minval;

  arrayset_bt() : size() {}

  void initialise(DomainInt low, DomainInt high) {
    minval = low;
    vals_pos.resize(checked_cast<SysInt>(high - low + 1));
    vals.resize(checked_cast<SysInt>(high - low + 1));
    for(SysInt i = 0; i < checked_cast<SysInt>(high - low + 1); i++) {
      vals[i] = checked_cast<SysInt>(i + low);
      vals_pos[i] = i;
    }
    size = 0;
  }

  void clear() {
    size = 0;
  }

  bool in(DomainInt val) {
    return vals_pos[checked_cast<SysInt>(val - minval)] < size;
  }

  // This method looks a bit messy, due to stupid C++ optimisers not being
  // clever enough to realise various things don't alias, and this method
  // being called as much as it is.
  void unsafe_insert(DomainInt val) {
    D_ASSERT(!in(val));
    const SysInt minval_cpy = checked_cast<SysInt>(minval);
    const SysInt validx = checked_cast<SysInt>(val - minval_cpy);
    const SysInt size_cpy = checked_cast<SysInt>(size);
    const SysInt swapval = vals[size_cpy];
    const SysInt vpvx = vals_pos[validx];
    vals[vpvx] = swapval;
    vals[size_cpy] = checked_cast<SysInt>(val);

    vals_pos[checked_cast<SysInt>(swapval - minval_cpy)] = vpvx;
    vals_pos[validx] = size_cpy;

    size = size + 1;
  }

  void insert(DomainInt val) {
    if(!in(val)) {
      unsafe_insert(val);
    }
  }

  void unsafe_remove(DomainInt val) {
    // swap to posiition size-1 then reduce size
    D_ASSERT(in(val));
    const SysInt validx = checked_cast<SysInt>(val - minval);
    const SysInt swapval = vals[checked_cast<SysInt>(size - 1)];
    vals[vals_pos[validx]] = swapval;
    vals[checked_cast<SysInt>(size - 1)] = checked_cast<SysInt>(val);

    vals_pos[checked_cast<SysInt>(swapval - minval)] = vals_pos[validx];
    vals_pos[validx] = checked_cast<SysInt>(size - 1);

    size = size - 1;
  }

  void remove(DomainInt val) {
    if(in(val)) {
      unsafe_remove(val);
    }
  }

  void fill() {
    size = vals.size();
  }
};

struct MDDNode {
  MDDNode(MDDNode* _parent, char _type) : parent(_parent), type(_type) {}

  MDDNode* parent;
  vector<std::pair<DomainInt, MDDNode*>> links; // pairs val,next
  SysInt id;                                    // Integer that uniquely defines this node.
  char type;                                    //   -1 is tt, 0 is normal, -2 is ff.
};

template <typename VarArray, bool isNegative = false>
struct MDDC : public AbstractConstraint {
  virtual string constraint_name() {
    return "mddc";
  }

  virtual string full_output_name() {
    return ConOutput::print_con(constraint_name(), vars, tuples);
  }

  VarArray vars;

  bool constraint_locked;

  //  gyes is set of mdd nodes that have been visited already in this call.
  arrayset gyes;
  // gno is the set of removed mdd nodes.
  arrayset_bt gno;

  TupleList* tuples;
  
  // All nodes of the mdd, used for freeing.
  vector<MDDNode*> mddnodes;

  MDDNode* top;

  vector<arrayset> gacvalues; // Opposite of the sets in the Cheng and Yap
                              // paper: these start empty and are fille d

  MDDC(const VarArray& _var_array, TupleList* _tuples)
      :
      
        vars(_var_array),
        constraint_locked(false),
        gno(),
        tuples(_tuples) {
    if(isNegative) {
      init_negative(_tuples);
    } else {
      init(_tuples);
    }

    // set up the two sets of mdd nodes.

    gyes.initialise(0, (SysInt)mddnodes.size() - 1);
    gno.initialise(0, (SysInt)mddnodes.size() - 1);

    // Set up gacvalues.

    gacvalues.resize(vars.size());
    for(SysInt i = 0; i < (SysInt)vars.size(); i++) {
      gacvalues[i].initialise(vars[i].getInitialMin(), vars[i].getInitialMax());
    }
  }

  //
  // This one accepted an mdd written in a fairly difficult format.
  /*void old_init(TupleList* tuples) {
      // convert tuples into mdd nodes
      int tlsize=tuples->size();

      int tuplelen=tuples->tuple_size();

      DomainInt* tupdata=tuples->getPointer();

      CHECK(tuplelen%2 == 0, "Tuples must be even length in MDDC");  // Check it
  is even length.

      mddnodes.resize(tlsize);

      for(int nodeid=0; nodeid<tlsize; nodeid++) {
          vector<DomainInt> tup(tupdata+(tuplelen*nodeid),
  tupdata+(tuplelen*(nodeid+1) ));   // inefficient.

          if(nodeid<tlsize-1) {
              // Not final tt node.
              for(int pair=0; pair<(SysInt)tup.size(); pair=pair+2) {
                  if(tup[pair]==-1 && tup[pair+1]==-1) break;

                  CHECK(tup[pair+1] >0 && tup[pair+1]<tlsize, "Links in MDD must
  be in range 1 up to the number of nodes");

                  mddnodes[nodeid].links.push_back(std::make_pair(tup[pair],
  checked_cast<SysInt>(tup[pair+1])));  // push the domain value
              }

              mddnodes[nodeid].id=nodeid;

          }
          else {
              // Final tt node.
              CHECK(tup[0]==-1 && tup[1]==-1, "Final MDD node must be all -1s,
  cannot link to any other nodes.");
              mddnodes[nodeid].id=-1;
          }
      }
  }*/

  // This one converts a list of tuples (i.e. a standard table constraint) to an
  // mdd.
  void init(TupleList* tuples) {
    // First build a trie by inserting the tuples one by one.

    SysInt tlsize = checked_cast<SysInt>(tuples->size());

    SysInt tuplelen = checked_cast<SysInt>(tuples->tuple_size());

    DomainInt* tupdata = tuples->getPointer();

    // Make the top node.
    top = new MDDNode(NULL, 0);
    mddnodes.push_back(top);

    for(int tupid = 0; tupid < tlsize; tupid++) {
      vector<DomainInt> tup(tupdata + (tuplelen * tupid),
                            tupdata + (tuplelen * (tupid + 1))); // inefficient.

      MDDNode* curnode = top;

      for(int i = 0; i < tuplelen; i++) {
        // Search for value.
        vector<std::pair<DomainInt, MDDNode*>>& links = curnode->links;

        int idx = find_link(links, tup[i]);

        if(idx == -1) {
          // New node needed.
          MDDNode* newnode = new MDDNode(curnode, 0);
          if(i == tuplelen - 1) {
            newnode->type = -1; // At the end of the tuple -- make a tt node.
          }
          mddnodes.push_back(newnode);

          mklink(curnode, newnode, tup[i]);

          // Move to this new node.
          curnode = newnode;
        } else {
          // Follow the link.
          curnode = links[idx].second;
        }
      }
      D_ASSERT(curnode->type == -1); // tt node.
    }

    // Now mdd is a trie with lots of tt nodes as the leaves.
    // Start merging from the leaves upwards.
    // label the nodes with a unique integer.
    for(int i = 0; i < (SysInt)mddnodes.size(); i++) {
      mddnodes[i]->id = i;
    }

    compress_from_leaves();
  }

  // This one converts a negative list of tuples (i.e. a negative table
  // constraint) to an mdd.
  void init_negative(TupleList* tuples) {
    // First build a trie by inserting the tuples one by one.

    SysInt tlsize = checked_cast<SysInt>(tuples->size());

    SysInt tuplelen = checked_cast<SysInt>(tuples->tuple_size());

    DomainInt* tupdata = tuples->getPointer();

    // Make the top node.
    top = new MDDNode(NULL, -1); // Start with just a tt node.
    mddnodes.push_back(top);

    for(int tupid = 0; tupid < tlsize; tupid++) {
      vector<DomainInt> tup(tupdata + (tuplelen * tupid),
                            tupdata + (tuplelen * (tupid + 1))); // inefficient.

      MDDNode* curnode = top;

      for(int i = 0; i < tuplelen; i++) {
        // Search for value.
        vector<std::pair<DomainInt, MDDNode*>>& links = curnode->links;

        int idx = find_link(links, tup[i]);

        if(idx == -1) {
          // New node(s) needed.

          DomainInt tupval = tup[i];

          MDDNode* newnode;

          // D_ASSERT(curnode->type==-1);   // tt node

          curnode->type = 0; // make it an internal node.

          for(DomainInt val = vars[i].getInitialMin(); val <= vars[i].getInitialMax(); val++) {
            if(val == tupval) {
              // Make the next node in the tuple.
              newnode = new MDDNode(curnode, 0);
              if(i == tuplelen - 1) {
                newnode->type = -2; // At the end of the tuple -- make it an ff node.
              }
              mddnodes.push_back(newnode);
              mklink(curnode, newnode, val);
            } else {
              // Make a tt node.
              MDDNode* tempnode = new MDDNode(curnode, -1);
              mddnodes.push_back(tempnode);
              mklink(curnode, tempnode, val);
            }
          }

          // Move to the new node.
          curnode = newnode;
        } else {
          // Follow the link.
          curnode = links[idx].second;
          if(i == tuplelen - 1) {
            // This is the case where a tt leaf node already exists.
            D_ASSERT(curnode->type == -1);
            // Change it to an ff node.
            curnode->type = -2;
          }
        }
      }
      D_ASSERT(curnode->type == -2); // ff node at end of tuple.
    }
    // Now mdd is a trie with lots of tt nodes as the leaves.
    // Start merging from the leaves upwards.
    // label the nodes with a unique integer.
    for(int i = 0; i < (SysInt)mddnodes.size(); i++) {
      mddnodes[i]->id = i;
    }

    compress_from_leaves();
  }

  void mklink(MDDNode* curnode, MDDNode* newnode, DomainInt value) {

    vector<std::pair<DomainInt, MDDNode*>>& links = curnode->links;

    // Make the link
    links.push_back(std::make_pair(value, newnode));

    // Swap it into place.
    for(int i = (SysInt)links.size() - 1; i > 0; i--) {
      if(links[i].first < links[i - 1].first) {
        std::pair<DomainInt, MDDNode*> temp = links[i];
        links[i] = links[i - 1];
        links[i - 1] = temp;
      } else {
        break;
      }
    }
  }

  void compress_from_leaves() {

    for(int layer = vars.size(); layer >= 0; layer--) {
      // Need a list of all nodes in this layer, with the index of their
      // parent.

      vector<MDDNode*> nodelist;

      find_layer(0, layer, top, nodelist);

      // Use stupid algorithm to find duplicates.
      // Only need to look at layers 1..n where n is number of vars.
      // Layer 0 only contains
      for(int i = 1; i < (SysInt)nodelist.size(); i++) {
        for(int j = i + 1; j < (SysInt)nodelist.size(); j++) {
          bool match = true;
          MDDNode* n1 = nodelist[i];
          MDDNode* n2 = nodelist[j];

          if(n1->type != n2->type)
            match = false;
          if(n1->links.size() != n2->links.size())
            match = false;
          for(int k = 0; k < (SysInt)(n1->links.size()) && match; k++) {
            if(n1->links[k] != n2->links[k])
              match = false;
          }

          if(match) {
            // we have a match.
            // Delete nodelist[j].

            MDDNode* todelete = nodelist[j];
            nodelist[j] = nodelist.back();
            nodelist.pop_back();
            j--;

            mddnodes[todelete->id] = mddnodes.back();
            mddnodes.pop_back();
            if(todelete->id != (SysInt)mddnodes.size()) {
              // If we didn't just delete the last element...
              mddnodes[todelete->id]->id = todelete->id; // fix the id of the moved node.
            }

            vector<std::pair<DomainInt, MDDNode*>>& parlinks = todelete->parent->links;
            //  Search parlinks for the pointer to change
            for(int k = 0; k < (SysInt)parlinks.size(); k++) {
              if(parlinks[k].second == todelete) {
                // todelete is being deleted so rewire this pointer to
                // nodelinks[i]
                parlinks[k].second = nodelist[i];
                break;
              }
            }

            delete todelete;
          }
        }
      }
    }

    // ensure nodes are labelled with unique integers.
    for(int i = 0; i < (SysInt)mddnodes.size(); i++) {
      D_ASSERT(mddnodes[i]->id == i);
    }
  }

  void find_layer(int currentlayer, int targetlayer, MDDNode* curnode, vector<MDDNode*>& nodelist) {
    if(currentlayer < targetlayer) {
      for(int i = 0; i < (SysInt)curnode->links.size(); i++) {
        find_layer(currentlayer + 1, targetlayer, curnode->links[i].second, nodelist);
      }
    } else {
      D_ASSERT(currentlayer == targetlayer);

      nodelist.push_back(curnode);
    }
  }

  virtual SysInt dynamic_trigger_count() {
    return vars.size();
  }

  void setup_triggers() {
    for(SysInt i = 0; i < (SysInt)vars.size(); i++) {
      moveTriggerInt(vars[i], i, DomainChanged);
    }
  }

  virtual void full_propagate() {
    setup_triggers();
    // Just propagate.
    do_prop();
  }

  virtual vector<AnyVarRef> get_vars() {
    vector<AnyVarRef> ret;
    ret.reserve(vars.size());
    for(unsigned i = 0; i < vars.size(); ++i)
      ret.push_back(vars[i]);
    return ret;
  }

  virtual bool check_assignment(DomainInt* tup, SysInt v_size) {
    MDDNode* curnode = top;
    for(SysInt i = 0; i < v_size; i++) {

      if(curnode->type == -1) {
        // tt node.
        return true;
      }

      vector<std::pair<DomainInt, MDDNode*>>& links = curnode->links;
      int idx = find_link(links, tup[i]);

      if(idx == -1)
        return false; // implicit ff node.

      curnode = links[idx].second;
    }

    if(curnode->type == -1) {
      return true;
    } else if(curnode->type == -2) {
      return false;
    } else {
      D_ASSERT(false);
      return true;
    }
  }

  // Binary search for a value in a vector
  inline SysInt find_link(vector<std::pair<DomainInt, MDDNode*>> links, DomainInt value) {
    // Binary search to find the index where the first element of the pair
    // equals value.
    SysInt first = 0;
    SysInt last = (SysInt)links.size() - 1;

    while(first <= last) {
      SysInt mid = ((last - first) >> 1) + first;
      if(links[mid].first == value) {
        return mid;
      }
      if(links[mid].first < value) {

        first = mid + 1;
      } else {
        last = mid - 1;
      }
    }

    return -1;
  }

  virtual bool get_satisfying_assignment(box<pair<SysInt, DomainInt>>& assignment) {
    // Run a depth-first search that is similar to the propagator

    bool flag = mddcrecurse_assignment(top, 0, assignment);

    return flag;
  }

  virtual AbstractConstraint* reverse_constraint() {
    return forward_check_negation(this);
  }

  virtual void propagateDynInt(SysInt prop_var, DomainDelta) {
    if(!constraint_locked) {
      constraint_locked = true;
      getQueue().pushSpecialTrigger(this);
    }
  }

  virtual void special_unlock() {
    constraint_locked = false;
  }

  virtual void special_check() {
    constraint_locked = false;
    D_ASSERT(!getState().isFailed());
    do_prop();
  }

  SysInt delta;

  void do_prop() {

    // Clear gyes -- gno persists from the last call.
    gyes.clear();

    // Clear gacvalues
    for(SysInt var = 0; var < (SysInt)vars.size(); var++) {
      gacvalues[var].clear();
    }

    delta = vars.size(); // Horizon for the dfs. All vars from delta onwards are
                         // considered to have full support.

    mddcrecurse(top, 0);

    // Prune the domains.
    for(SysInt var = 0; var < delta; var++) {
      for(DomainInt val = vars[var].getMin(); val <= vars[var].getMax(); val++) {
        if(!gacvalues[var].in(val)) {
          vars[var].removeFromDomain(val);
        }
      }
    }

    // Don't need the other bits of the algorithm because Minion does it for us.
  }

  // DFS of the MDD.
  bool mddcrecurse(MDDNode* curnode, SysInt level) {
    if(curnode->type == -1) {
      //  special value indicating this is node tt.
      if(level < delta) {
        delta = level; // This variable and all >= are now fully supported.
      }
      return true;
    }

    if(curnode->type == -2) {
      // special value for ff.  This is never used.
      return false;
    }

    if(gyes.in(curnode->id)) {
      // if curnode is in the gyes set
      return true;
    }

    if(gno.in(curnode->id)) {
      // if in gno set
      return false;
    }

    bool res = false;

    // Iterate through links from this MDD node.

    vector<std::pair<DomainInt, MDDNode*>>& links = curnode->links;
    SysInt linksize = links.size();
    for(int k = 0; k < linksize; k++) {
      DomainInt val = links[k].first;
      MDDNode* newnode = links[k].second;

      if(vars[level].inDomain(val)) {
        bool returnvalue = mddcrecurse(newnode, level + 1);

        if(returnvalue) {
          res = true;
          gacvalues[level].insert(val);
          if(level + 1 == delta && gacvalues[level].size == vars[level].getDomSize()) {
            delta = level;
            break; // for k loop.
          }
        }
      }
    }

    if(res) {
      // add to gyes
      D_ASSERT(!gyes.in(curnode->id));
      gyes.insert(curnode->id);
    } else {
      // add to gno
      D_ASSERT(!gno.in(curnode->id));
      gno.insert(curnode->id);
    }

    return res;
  }

  // Modified version of the above to do the getSatisfyingAssignment
  // DFS of the MDD. Doesn't use g_yes because there is no need to
  // visit the same node twice.
  // It does use gno as usual.
  // It does NOT use delta.
  bool mddcrecurse_assignment(MDDNode* curnode, SysInt level,
                              box<pair<SysInt, DomainInt>>& assignment) {
    if(curnode->type == -1) {
      //  special value indicating this is node tt.
      return true;
    }

    if(curnode->type == -2) {
      // special value for ff.  This is never used.
      return false;
    }

    if(gno.in(curnode->id)) {
      // if in gno set
      return false;
    }

    // Iterate through links from this MDD node.

    vector<std::pair<DomainInt, MDDNode*>>& links = curnode->links;
    SysInt linksize = links.size();
    for(int k = 0; k < linksize; k++) {
      DomainInt val = links[k].first;
      MDDNode* newnode = links[k].second;

      if(vars[level].inDomain(val)) {
        assignment.push_back(std::make_pair(level, val));
        bool returnvalue = mddcrecurse_assignment(newnode, level + 1, assignment);

        if(returnvalue) {
          // Found an assignment, unwind the recursion.
          return true;
        } else {
          assignment.pop_back();
        }
      }
    }

    // This node cannot be extended to an assignment.
    // add to gno
    D_ASSERT(!gno.in(curnode->id));
    gno.insert(curnode->id);

    return false;
  }

  void print_mdd() {
    for(int i = 0; i < (SysInt)mddnodes.size(); i++) {
      print_mdd_node(mddnodes[i]);
    }
  }
  void print_mdd_node(MDDNode* n) {
    std::cout << "Node id:" << n->id << " type:" << ((int)n->type) << " links: " << n->links
              << endl;
  }
};

template <typename T>
AbstractConstraint* BuildCT_MDDC(const T& t1, ConstraintBlob& b) {
  return new MDDC<T>(t1, b.tuples);
}

/* JSON
  { "type": "constraint",
    "name": "mddc",
    "internal_name": "CT_MDDC",
    "args": [ "read_list", "read_tuples" ]
  }
*/

template <typename T>
AbstractConstraint* BuildCT_NEGATIVEMDDC(const T& t1, ConstraintBlob& b) {
  return new MDDC<T, true>(t1, b.tuples);
}

/* JSON
  { "type": "constraint",
    "name": "negativemddc",
    "internal_name": "CT_NEGATIVEMDDC",
    "args": [ "read_list", "read_tuples" ]
  }
*/

#endif
