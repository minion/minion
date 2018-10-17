// CHECK full length and short support segments of code
//
//
// LIST BASED CODE WONT BE WORKING

// Started on git branch  supportsgac+bstable+adaptive
//      intended for supportsgac
//      + long supports
//      + better memory
//      + backtrack stability
//      + adaptive use or ignoring of full length supports

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

#include <algorithm>
#include "constraint_checkassign.h"
#include "constraint_haggisgac_tuples.h"

// Default will be List.
// If any special case is defined list will be switched off
// If two options given compile errors are expected to result.

#define UseElementShort 0
#define UseElementLong 0
#define UseLexLeqShort 0
#define UseLexLeqLong 0
#define UseSquarePackingShort 0
#define UseSquarePackingLong 0
#define UseList 1
#define UseNDOneList 0
#define SupportsGacNoCopyList 1

// The algorithm iGAC or short-supports-gac

// Does it place dynamic triggers for the supports.
#define SupportsGACUseDT 1

// Switches on the zeroLits array.
// This flag is a small slowdown on qg-supportsgac-7-9 -findallsols
//
#ifdef CLASSNAME
#undef CLASSNAME
#endif

#define CLASSNAME HaggisGACStable

template <typename VarArray>
struct HaggisGACStable : public AbstractConstraint, Backtrackable {
  virtual string constraint_name() {
    return "haggisgac-stable";
  }

  CONSTRAINT_ARG_LIST2(vars, data);

#include "constraint_haggisgac_common.h"

  struct Support {
    vector<SupportCell> supportCells; // Size can't be more than r, but can be less.

    SysInt arity; // could use vector.size() but don't want to destruct
                  // SupportCells when arity decreases
                  // or reconstruct existing ones when it increases.

    Support* nextFree; // for when Support is in Free List.
    bool active;
    SysInt numLastSupported;

    Support() {
      supportCells.resize(0);
      arity = 0;
      nextFree = 0;
      active = true;
      numLastSupported = 0;
    }
  };

  VarArray vars;

  vector<pair<SysInt, DomainInt>>
      literalsScratch; // used instead of per-Support list, as scratch space

  SysInt numvals;
  SysInt numlits;

  // Counters
  SysInt supports; // 0 to rd.
  vector<SysInt> supportsPerVar;

  vector<SysInt> litsWithLostExplicitSupport;
  vector<SysInt> varsWithLostImplicitSupport;

  // 2d array (indexed by var then val) of sentinels,
  // at the head of list of supports.
  // Needs a sentinel at the start so that dlx-style removals work correctly.
  vector<Literal> literalList;
  vector<SysInt> firstLiteralPerVar;

  vector<Support*> lastSupportPerLit; // could be in Literal type
  vector<Support*> lastSupportPerVar;
  vector<Support*> deletedSupports;

  // For each variable, a vector of values with 0 supports (or had 0 supports
  // when added to the vector).
  vector<vector<SysInt>> zeroLits;
  vector<char> inZeroLits; // is a literal in zeroVals

  // Partition of variables by number of supports.
  vector<SysInt> varsPerSupport; // Permutation of the variables
  vector<SysInt> varsPerSupInv;  // Inverse mapping of the above.

  vector<SysInt> supportNumPtrs; // rd+1 indices into varsPerSupport
                                 // representing the partition

  Support* supportFreeList; // singly-linked list of spare Support objects.

  HaggisGACTuples* tuple_list;

  vector<vector<SysInt>> tuple_list_pos; // current position in tuple_lists (for
                                         // each var and val). Wraps around.

  ////////////////////////////////////////////////////////////////////////////
  // Ctor

  ShortTupleList* data;

  HaggisGACStable(const VarArray& _var_array, ShortTupleList* tuples)
      : vars(_var_array), supportFreeList(0), data(tuples) {
    init();

    // Pointers to the last implicit/explicit support for a var/literal
    //
    lastSupportPerVar.resize(vars.size(), 0); // actually don't think we care what initial val is
    lastSupportPerLit.resize(numlits, 0);
    deletedSupports.reserve(numlits); // max poss size, not necessarily best choice
  }

  ////////////////////////////////////////////////////////////////////////////
  // Backtracking mechanism

  struct BTRecord {
    // bool is_removal;   // removal or addition was made.
    SysInt var;
    SysInt lit;
    Support* sup;

    friend std::ostream& operator<<(std::ostream& o, const BTRecord& rec) {
      if(rec.sup == 0)
        return o << "ZeroMarker";
      o << "BTRecord:";
      o << rec.var << "," << rec.lit;
      // o<< rec.sup->literals;
      return o;
    }
  };

  // Have support stack
  // shove deletions onto them.
  // Shove deleted literals onto bt record
  // go through support stack and destroy any zero supports
  //
  // on backtrack destroy support if it gets to zero and inactive

  vector<BTRecord> backtrack_stack;

  void mark() {
    struct BTRecord temp = {0, 0, 0};
    backtrack_stack.push_back(temp); // marker.
  }

  void pop() {
    // cout << "BACKTRACKING:" << endl;
    // cout << backtrack_stack <<endl;
    while(backtrack_stack.back().sup != 0) {
      BTRecord temp = backtrack_stack.back();
      backtrack_stack.pop_back();
      // BTStable Change
      if(!(temp.sup->active)) {
        if(hasNoKnownSupport(temp.var, temp.lit)) {
          // we need to add support back in
          addSupportInternal(temp.sup);
        } else {
          // could be clever with -- here but let's play safe
          if(temp.sup->numLastSupported == 1) {
            // we can add tempsup to supportFreeList
            // cout << "adding support to Free List " << temp.sup->literals <<
            // endl ;
            addToSupportFreeList(temp.sup);
          } else {
            temp.sup->numLastSupported--;
          }
        }
      }
      // Might possibly have to add lit back onto zeroLits
      //
      // Hard to be certain zeroLits is correct so play safe and test it
      if(!inZeroLits[temp.lit] && literalList[temp.lit].supportCellList == 0) {
        inZeroLits[temp.lit] = true;
        zeroLits[temp.var].push_back(temp.lit);
      }
    }

    backtrack_stack.pop_back(); // Pop the marker.
    // cout << "END OF BACKTRACKING." << endl;
  }

  ////////////////////////////////////////////////////////////////////////////
  // Add and delete support

  // don't need argument?   Just use litlist member?
  //
  // Support* addSupport(box<pair<SysInt, DomainInt> >* litlist)
  void addSupport() {
    Support* newsup = getFreeSupport();
    vector<SupportCell>& supCells = newsup->supportCells;
    SysInt oldsize = supCells.size();
    SysInt newsize = literalsScratch.size();

    newsup->arity = newsize;

    if(newsize > oldsize) {
      supCells.resize(newsize);
      // make sure pointers to support cell are correct
      // need only be done once as will always point to
      // its own support
      for(SysInt i = oldsize; i < newsize; i++) {
        supCells[i].sup = newsup;
      }
    }

    for(SysInt i = 0; i < newsize; i++) {
      SysInt var = literalsScratch[i].first;
      DomainInt valoriginal = literalsScratch[i].second;
      const SysInt lit =
          checked_cast<SysInt>(firstLiteralPerVar[var] + valoriginal - vars[var].getInitialMin());
      supCells[i].literal = lit;
    }
    // now have enough supCells, and sup and literal of each is correct

    addSupportInternal(newsup);
  }

  // these guys can be void
  //
  //

  // Takes a support which has:
  //          arity correct
  //          supCells containing at least arity elements
  //          each supCells[i[ in range has
  //                literal correct
  //                sup correct

  void addSupportInternal(Support* sup_internal) {
    // add a new support given literals but not pointers in place

    // cout << "Adding support (internal) :" << litlist_internal << endl;
    // D_ASSERT(litlist_internal.size()>0);
    //// It should be possible to deal with empty supports, but currently they
    /// wil
    // cause a memory leak.

    vector<SupportCell>& supCells = sup_internal->supportCells;

    SysInt litsize = sup_internal->arity;

    sup_internal->active = true;

    if(litsize < (SysInt)vars.size()) {
      // it's a short support, so update supportsPerVar and supports
      for(SysInt i = 0; i < litsize; i++) {

        SysInt lit = supCells[i].literal;
        SysInt var = literalList[lit].var;

        // Stitch it into the start of literalList.supportCellList

        supCells[i].prev = 0;
        supCells[i].next = literalList[lit].supportCellList;
        if(literalList[lit].supportCellList != 0) {
          literalList[lit].supportCellList->prev = &(supCells[i]);
        } else {
          // Attach trigger if this is the first support containing var,val.
          attach_trigger(var, literalList[lit].val, lit);
        }
        literalList[lit].supportCellList = &(supCells[i]);

        // update counters
        supportsPerVar[var]++;
        // Update partition
        // swap var to the end of its cell.
        partition_swap(var, varsPerSupport[supportNumPtrs[supportsPerVar[var]] - 1]);
        // Move the boundary so var is now in the higher cell.
        supportNumPtrs[supportsPerVar[var]]--;
      }
      supports++;
    } else {
      // it's a full length support so don't update those counters
      for(SysInt i = 0; i < litsize; i++) {

        SysInt lit = supCells[i].literal;
        SysInt var = literalList[lit].var;

        // Stitch it into the start of literalList.supportCellList

        supCells[i].prev = 0;
        supCells[i].next = literalList[lit].supportCellList;
        if(literalList[lit].supportCellList != 0) {
          literalList[lit].supportCellList->prev = &(supCells[i]);
        } else {
          // Attach trigger if this is the first support containing var,val.
          attach_trigger(var, literalList[lit].val, lit);
        }
        literalList[lit].supportCellList = &(supCells[i]);
      }
    }

    // printStructures();

    // return sup_internal;
  }

  void deleteSupport(Support* sup) {
    deleteSupportInternal(sup, false);
  }

  void deleteSupportInternal(Support* sup, bool Backtracking) {
    D_ASSERT(sup != 0);

    sup->active = false;
    sup->numLastSupported = 0;

    vector<SupportCell>& supCells = sup->supportCells;
    SysInt supArity = sup->arity;

    if(supArity < (SysInt)vars.size()) {
      // it's a short support

      SysInt oldIndex = supportNumPtrs[supports];

      for(SysInt i = 0; i < supArity; i++) {

        SupportCell& supCell = supCells[i];
        SysInt lit = supCell.literal;
        SysInt var = literalList[lit].var;

        // D_ASSERT(prev[var]!=0);
        // decrement counters
        supportsPerVar[var]--;

        if(supCell.prev == 0) { // this was the first support in list

          literalList[lit].supportCellList = supCell.next;

          if(supCell.next == 0) {
            // We have lost the last support for lit
            //
            // I believe that each literal can only be marked once here in a
            // call to update_counters.
            // so we should be able to push it onto a list
            //
            // As long as we do not actually call find_new_support.
            // So probably should shove things onto a list and then call find
            // supports later

            if(!Backtracking &&
               supportsPerVar[var] == (supports - 1)) { // since supports not decremented yet
              litsWithLostExplicitSupport.push_back(lit);
              lastSupportPerLit[lit] = sup;
            } else

                // PREVIOUSLY there was no else here.   We had to add to
                // zerolits even if above true
                //
                // But now we don't because if we remove the value then we would
                // remove it from zerolits
                // and put it back on going back throug hthe backtrack stack.
                //
                // Only case where we need to add it is if we DO find a new
                // support which is implicit
                // And that case is covered elsewhere - search for NOTEAAA.
                //
                // However if test above is false then we have to check for
                // zeroLits
                //

                if(!inZeroLits[lit]) {
              inZeroLits[lit] = true;
              zeroLits[var].push_back(lit);
            }
            // Remove trigger since this is the last support containing var,val.
            if(SupportsGACUseDT) {
              detach_trigger(lit);
            }
          } else {
            supCell.next->prev = 0;
          }
        } else {
          supCell.prev->next = supCell.next;
          if(supCell.next != 0) {
            supCell.next->prev = supCell.prev;
          }
        }

        // Update partition
        // swap var to the start of its cell.
        // This plays a crucial role in moving together the vars which
        // previously
        // had 1 less than numsupports and soon will have numsupports.

        partition_swap(var, varsPerSupport[supportNumPtrs[supportsPerVar[var] + 1]]);
        //
        // Move the boundary so var is now in the lower cell.
        supportNumPtrs[supportsPerVar[var] + 1]++;
      }
      supports--;

      // For following code it is essential that partition swaps compress
      // vars together which used to have SupportsPerVar[i] = supports-1 and
      // now have supportsPerVar[i] = supports (because supports has been
      // decremented
      //
      //
      //
      // Similarly to the above, each var can only be added to this list once
      // per call to update_counters
      // Because it can only lose its last implicit support once since we are
      // only deleting supports.
      //

      // I hope we only need to do this when NOT backtracking, at least for non
      // backtrack-stable version
      // When we backtrack we will add supports which did support it so there is
      // no need to find new supports

      //      cout << supportNumPtrs[supports] << " " << oldIndex << endl;

      if(!Backtracking) {
        for(SysInt i = supportNumPtrs[supports]; i < oldIndex; i++) {
          varsWithLostImplicitSupport.push_back(varsPerSupport[i]);
          lastSupportPerVar[varsPerSupport[i]] = sup;
        }
        deletedSupports.push_back(sup);
      } else {
        // We are Backtracking
        // Can re-use the support when it is removed by BT.
        // Stick it on the free list
        addToSupportFreeList(sup);
      }
    } else {
      // it's a full length support

      for(SysInt i = 0; i < supArity; i++) {

        SupportCell& supCell = supCells[i];
        SysInt lit = supCell.literal;
        SysInt var = literalList[lit].var;

        if(supCell.prev == 0) { // this was the first support in list

          literalList[lit].supportCellList = supCell.next;

          if(supCell.next == 0) {
            // We have lost the last support for lit
            //
            // I believe that each literal can only be marked once here in a
            // call to update_counters.
            // so we should be able to push it onto a list
            //
            // As long as we do not actually call find_new_support.
            // So probably should shove things onto a list and then call find
            // supports later

            if(!Backtracking && supportsPerVar[var] == supports) { // supports won't be decremented
              litsWithLostExplicitSupport.push_back(lit);
              lastSupportPerLit[lit] = sup;
            } else

                // PREVIOUSLY there was no else here.   We had to add to
                // zerolits even if above true
                //
                // But now we don't because if we remove the value then we would
                // remove it from zerolits
                // and put it back on going back throug hthe backtrack stack.
                //
                // Only case where we need to add it is if we DO find a new
                // support which is implicit
                // And that case is covered elsewhere - search for NOTEAAA.
                //
                // However if test above is false then we have to check for
                // zeroLits
                //

                if(!inZeroLits[lit]) {
              inZeroLits[lit] = true;
              zeroLits[var].push_back(lit);
            }
            // Remove trigger since this is the last support containing var,val.
            if(SupportsGACUseDT) {
              detach_trigger(lit);
            }
          } else {
            supCell.next->prev = 0;
          }
        } else {
          supCell.prev->next = supCell.next;
          if(supCell.next != 0) {
            supCell.next->prev = supCell.prev;
          }
        }
      }

      // Since this was a full length supports no var has lost its last implicit
      // support

      if(!Backtracking) {
        deletedSupports.push_back(sup);
      } else {
        // We are Backtracking
        // Can re-use the support when it is removed by BT.
        // Stick it on the free list
        addToSupportFreeList(sup);
      }
    }
  }

  bool findSupportsIncrementalHelper(SysInt var, DomainInt val) {
    literalsScratch.clear();
    bool foundsupport = findNewSupport<false>(var, val);

    if(!foundsupport) {
      vars[var].removeFromDomain(val);
      // note we are not doing this internally,
      // i.e. trying to update counters etc.
      // So counters won't have changed until we are retriggered on the removal
    } else {
      addSupport();
    }
    return foundsupport;
  }

  void findSupportsInitial() {
    // called from Full Propagate
    // We do not assign responsibility for removals as this is called at the
    // root.

    for(SysInt i = (SysInt)varsWithLostImplicitSupport.size() - 1; i >= 0; i--) {

      SysInt var = varsWithLostImplicitSupport[i];
      varsWithLostImplicitSupport.pop_back(); // actually probably unnecessary -
                                              // will get resized to 0 later

      if(supportsPerVar[var] == supports) { // otherwise has found implicit support in the meantime
        for(SysInt j = 0; j < (SysInt)zeroLits[var].size(); j++) {
          SysInt lit = zeroLits[var][j];
          if(literalList[lit].supportCellList != 0) {
            // No longer a zero val. remove from vector.
            zeroLits[var][j] = zeroLits[var][zeroLits[var].size() - 1];
            zeroLits[var].pop_back();
            inZeroLits[lit] = false;
            j--;
            continue;
          }
          DomainInt val = literalList[lit].val;

          if(vars[var].inDomain(val)) { // tested supportCellList  above
            findSupportsIncrementalHelper(var, val);
          }
        }
      }
    }
  }

  void findSupportsIncremental() {
    // For list of vars which have lost their last implicit support
    // do this ...
    // but don't need to redo if we have stored that list ahead of time
    //  ... and we can check if it still has support
    //
    // For each variable where the number of supports is equal to the total...

    for(SysInt i = (SysInt)litsWithLostExplicitSupport.size() - 1; i >= 0; i--) {
      SysInt lit = litsWithLostExplicitSupport[i];
      SysInt var = literalList[lit].var;
      DomainInt val = literalList[lit].val;

      litsWithLostExplicitSupport.pop_back(); // actually probably unnecessary -
                                              // will get resized to 0 later

      if(vars[var].inDomain(val)) {
        if(hasNoKnownSupport(var, lit) && !findSupportsIncrementalHelper(var, val)) {
          // removed val so must annotate why
          lastSupportPerLit[lit]->numLastSupported++;
          struct BTRecord backtrackInfo = {var, lit, lastSupportPerLit[lit]};
          backtrack_stack.push_back(backtrackInfo);
        } else { // We now have an alternative support for this literal
          // so we need to do nothing EXCEPT ...
          //  ... add it to zeroLits if necessary
          //
          //  Note this is the only case where we need to add the lit to
          //  zeroLits from NOTEAAA

          if(!inZeroLits[lit] && literalList[lit].supportCellList == 0) {
            inZeroLits[lit] = true;
            zeroLits[var].push_back(lit);
          }
        }
      } else { // Need to cover out of domain lits but has known support so it
               // will have support on BT.
        lastSupportPerLit[lit]->numLastSupported++;
        struct BTRecord backtrackInfo = {var, lit, lastSupportPerLit[lit]};
        backtrack_stack.push_back(backtrackInfo);
      }
    }

    for(SysInt i = (SysInt)varsWithLostImplicitSupport.size() - 1; i >= 0; i--) {

      SysInt var = varsWithLostImplicitSupport[i];
      varsWithLostImplicitSupport.pop_back(); // actually probably unnecessary -
                                              // will get resized to 0 later

      if(supportsPerVar[var] == supports) { // otherwise has found implicit support in the meantime
        for(SysInt j = 0; j < (SysInt)zeroLits[var].size() && supportsPerVar[var] == supports;
            j++) {
          SysInt lit = zeroLits[var][j];
          if(literalList[lit].supportCellList != 0) {
            // No longer a zero val. remove from vector.
            zeroLits[var][j] = zeroLits[var][zeroLits[var].size() - 1];
            zeroLits[var].pop_back();
            inZeroLits[lit] = false;
            j--;
            continue;
          }
          DomainInt val = literalList[lit].val;

          if(vars[var].inDomain(val)) { // tested literalList  above
            if(!findSupportsIncrementalHelper(var, val)) {
              lastSupportPerVar[var]->numLastSupported++;
              struct BTRecord backtrackInfo = {var, lit, lastSupportPerVar[var]};
              backtrack_stack.push_back(backtrackInfo);
              // No longer in domain.  remove from zeroLits vector.
              zeroLits[var][j] = zeroLits[var][zeroLits[var].size() - 1];
              zeroLits[var].pop_back();
              inZeroLits[lit] = false;
              j--;
              continue;
            }
            // Else we have found new implicit or explicit support is found
            //
            // No longer do we remove j from zerovals in this case if explicit
            // support is found.
            // However this is correct as it can be removed lazily next time the
            // list is traversed
            // And we might even get lucky and save this small amount of work.
          } else {
            // This is a nasty case because we are doing backtrack stability
            // var=val has been removed, and it must have been by something
            // outside this
            // constraint.  Therefore when it is restored we need to make sure
            // it has
            // support in this constraint.   Since it has no explicit support,
            // its last
            // support must be this implicit support we are deleting.  So we
            // have to restore
            // it on bracktracking.
            lastSupportPerVar[var]->numLastSupported++;
            struct BTRecord backtrackInfo = {var, lit, lastSupportPerVar[var]};
            backtrack_stack.push_back(backtrackInfo);
            // No longer in domain. remove from vector.
            zeroLits[var][j] = zeroLits[var][zeroLits[var].size() - 1];
            zeroLits[var].pop_back();
            inZeroLits[lit] = false;
            j--;
            continue;
          }
        }
      }
    }
  }

  virtual void propagateDynInt(SysInt lit, DomainDelta) {

    //  cout << "Propagate called: var= " << var << "val = " << val << endl;
    // printStructures();

    deletedSupports.resize(0);

    updateCounters(lit);

    findSupportsIncremental();

    while(deletedSupports.size() > 0) {
      if(deletedSupports.back()->numLastSupported == 0) {
        addToSupportFreeList(deletedSupports.back());
      }
      deletedSupports.pop_back();
    }
  }

  inline void addToSupportFreeList(Support* sup) {
    sup->nextFree = supportFreeList;
    supportFreeList = sup;
  }

  virtual void full_propagate() {
    full_prop_init();

    // reset containers defined in constructor
    lastSupportPerVar.clear();
    lastSupportPerVar.resize(vars.size(), 0); // actually don't think we care what initial val is
    lastSupportPerLit.clear();
    lastSupportPerLit.resize(numlits, 0);
    deletedSupports.clear();

    for(int i = 0; i < dynamic_trigger_count(); ++i)
      detach_trigger(i);

    litsWithLostExplicitSupport.resize(0);
    varsWithLostImplicitSupport.resize(0);

    for(SysInt i = 0; i < (SysInt)vars.size(); i++) {
      varsWithLostImplicitSupport.push_back(i);
    }

    findSupportsInitial();
  }

  virtual vector<AnyVarRef> get_vars() {
    vector<AnyVarRef> ret;
    ret.reserve(vars.size());
    for(unsigned i = 0; i < vars.size(); ++i)
      ret.push_back(vars[i]);
    return ret;
  }
}; // end of class

template <typename T>
AbstractConstraint* BuildCT_HAGGISGAC_STABLE(const T& t1, ConstraintBlob& b) {
  return new HaggisGACStable<T>(t1, b.short_tuples);
}

/* JSON
  { "type": "constraint",
    "name": "haggisgac-stable",
    "internal_name": "CT_HAGGISGAC_STABLE",
    "args": [ "read_list", "read_short_tuples" ]
  }
*/
