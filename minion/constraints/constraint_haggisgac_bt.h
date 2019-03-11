// LIST BASED CODE WONT BE WORKING

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

#include "constraint_checkassign.h"
#include "constraint_haggisgac_tuples.h"
#include <algorithm>

/** @help constraints;haggisgac-stable Description
An extensional constraint that enforces GAC. haggisgac-stable
is a variant of haggisgac which uses less memory in some cases,
and can also be faster (or slower). The input is identical to
haggisgac.
*/

/** @help constraints;haggisgac-stable References
help constraints haggisgac
*/

/** @help constraints;haggisgac Description
An extensional constraint that enforces GAC. This constraint make uses
of 'short tuples', which allow some values to be marked as don't care.
When this allows the set of tuples to be reduced in size, this leads to
performance gains.

The variables used in the constraint have to be BOOL or DISCRETE variables.
Other types are not supported.
*/

/** @help constraints;haggisgac Example

Consider the constraint 'min([x1,x2,x3],x4)'' on Booleans variables
x1,x2,x3,x4.

Represented as a TUPLELIST for a table or gacschema constraint, this would
look like:

**TUPLELIST**
mycon 8 4
0 0 0 0
0 0 1 0
0 1 0 0
0 1 1 0
1 0 0 0
1 0 1 0
1 1 0 0
1 1 1 1

Short tuples give us a way of shrinking this list. Short tuples consist
of pairs (x,y), where x is a varible position, and y is a value for that
variable. For example:

[(0,0),(3,0)]

Represents 'If the variable at index 0 is 0, and the variable at index
3 is 0, then the constraint is true'.

This allows us to represent our constraint as follows:

**SHORTTUPLELIST**
mycon 4
[(0,0),(3,0)]
[(1,0),(3,0)]
[(2,0),(3,0)]
[(0,1),(1,1),(2,1),(3,1)]

Note that some tuples are double-represented here. The first 3 short
tuples all allow the assignment '0 0 0 0'. This is fine. The important
thing for efficency is to try to give a small list of short tuples.


We use this tuple by writing:

haggisgac([x1,x2,x3,x4], mycon)

and now the variables [x1,x2,x3,x4] will satisfy the constraint mycon.
*/

/** @help constraints;haggisgac References
help input shorttuplelist
help constraints table
help constraints negativetable
help constraints shortstr2
*/

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
#define SupportsGACUseZeroVals 1

#ifdef CLASSNAME
#undef CLASSNAME
#endif

#define CLASSNAME HaggisGAC
template <typename VarArray>
struct HaggisGAC : public AbstractConstraint, Backtrackable {

  virtual string constraintName() {
    return "haggisgac";
  }

#include "constraint_haggisgac_common.h"

  CONSTRAINT_ARG_LIST2(vars, data);

  struct Support {
    vector<SupportCell> supportCells; // Size can't be more than r, but can be less.

    SysInt arity; // could use vector.size() but don't want to destruct
                  // SupportCells when arity decreases
                  // or reconstruct existing ones when it increases.

    Support* nextFree; // for when Support is in Free List.

    Support() {
      supportCells.resize(0);
      arity = 0;
      nextFree = 0;
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

// For each variable, a vector of values with 0 supports (or had 0 supports
// when added to the vector).
#if SupportsGACUseZeroVals
  vector<vector<SysInt>> zeroLits;
  vector<char> inZeroLits; // is a literal in zeroVals
#endif

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

  HaggisGAC(const VarArray& _var_array, ShortTupleList* tuples)
      : vars(_var_array), supportFreeList(0), data(tuples) {
    init();

    litsWithLostExplicitSupport.reserve(
        numlits); // max poss size, not necessarily optimal choice here
  }

  ////////////////////////////////////////////////////////////////////////////
  // Backtracking mechanism

  struct BTRecord {
    bool is_removal; // removal or addition was made.
    Support* sup;

    friend std::ostream& operator<<(std::ostream& o, const BTRecord& rec) {
      if(rec.sup == 0)
        return o << "ZeroMarker";
      o << "BTRecord:" << rec.is_removal << ",";
      // o<< rec.sup->literals;
      return o;
    }
  };

  vector<BTRecord> backtrack_stack;

  void mark() {
    struct BTRecord temp = {false, 0};
    backtrack_stack.push_back(temp); // marker.
  }

  void pop() {
    // cout << "BACKTRACKING:" << endl;
    // cout << backtrack_stack <<endl;
    while(backtrack_stack.back().sup != 0) {
      BTRecord temp = backtrack_stack.back();
      backtrack_stack.pop_back();
      if(temp.is_removal) {
        addSupportInternal(temp.sup);
      } else {
        deleteSupportInternal(temp.sup, true);
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
      DomainInt lit = firstLiteralPerVar[var] + valoriginal - vars[var].initialMin();
      supCells[i].literal = checked_cast<SysInt>(lit);
    }
    // now have enough supCells, and sup and literal of each is correct

    addSupportInternal(newsup);
    struct BTRecord temp;
    temp.is_removal = false;
    temp.sup = newsup;
    backtrack_stack.push_back(temp);
    // return newsup;
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

    // printStructures();

    // return sup_internal;
  }

  void deleteSupport(Support* sup) {
    struct BTRecord temp;
    temp.is_removal = true;
    temp.sup = sup;
    backtrack_stack.push_back(temp);

    deleteSupportInternal(sup, false);
  }

  void deleteSupportInternal(Support* sup, bool Backtracking) {
    D_ASSERT(sup != 0);

    vector<SupportCell>& supCells = sup->supportCells;
    SysInt supArity = sup->arity;
    // cout << "Removing support (internal) :" << litlist << endl;

    // oldIndex is where supportsPerVar = numsupports used to be
    // Off by 1 error?

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
          // I believe that each literal can only be marked once here in a call
          // to update_counters.
          // so we should be able to push it onto a list
          //
          // As long as we do not actually call find_new_support.
          // So probably should shove things onto a list and then call find
          // supports later
          // Surely don't need to update lost supports on backtracking in
          // non-backtrack-stable code?
          if(!Backtracking &&
             supportsPerVar[var] == (supports - 1)) { // since supports not decremented yet
            litsWithLostExplicitSupport.push_back(lit);
          }
#if SupportsGACUseZeroVals
          // Still need to add to zerovals even if above test is true
          // because we might find a new implicit support, later lose it, and
          // then it will
          // be essential that it is in zerovals.  Obviously if an explicit
          // support is
          // found then it will later get deleted from zerovals.
          if(!inZeroLits[lit]) {
            inZeroLits[lit] = true;
            zeroLits[var].push_back(lit);
          }
#endif
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
      // This plays a crucial role in moving together the vars which previously
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
    // Similarly to the above, each var can only be added to this list once per
    // call to update_counters
    // Because it can only lose its last implicit support once since we are only
    // deleting supports.
    //

    // I hope we only need to do this when NOT backtracking, at least for non
    // backtrack-stable version
    // When we backtrack we will add supports which did support it so there is
    // no need to find new supports

    //      cout << supportNumPtrs[supports] << " " << oldIndex << endl;

    if(!Backtracking) {
      for(SysInt i = supportNumPtrs[supports]; i < oldIndex; i++) {
        varsWithLostImplicitSupport.push_back(varsPerSupport[i]);
      }
    } else {
      // We are Backtracking
      // Can re-use the support when it is removed by BT.
      // Stick it on the free list
      sup->nextFree = supportFreeList;
      supportFreeList = sup;
    }
    // else can't re-use it because a ptr to it is on the BT stack.
  }

  void findSupportsIncrementalHelper(SysInt var, DomainInt val) {
    literalsScratch.clear();
    bool foundsupport = findNewSupport<false>(var, val);

    if(!foundsupport) {
      vars[var].removeFromDomain(val);
      // note we are not doing this internally,
      // i.e. trying to update counters etc.
      // So counters won't have changed until we are retriggered on the removal
    } else {
      // addSupport(&newsupportbox);
      addSupport();
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

      if(hasNoKnownSupport(var, lit) && vars[var].inDomain(val)) {
        findSupportsIncrementalHelper(var, val);
      }
    }

    for(SysInt i = (SysInt)varsWithLostImplicitSupport.size() - 1; i >= 0; i--) {

      SysInt var = varsWithLostImplicitSupport[i];
      varsWithLostImplicitSupport.pop_back(); // actually probably unnecessary -
                                              // will get resized to 0 later

      if(supportsPerVar[var] == supports) { // otherwise has found implicit support in the meantime
#if !SupportsGACUseZeroVals
        for(DomainInt val = vars[var].min(); val <= vars[var].max(); val++) {
          SysInt lit = firstLiteralPerVar[var] + val - vars[var].initialMin();
#else
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
#endif

#if !SupportsGACUseZeroVals
                        if(vars[var].inDomain(val) && (literalList[lit].supportCellList] == 0)){
#else
          if(vars[var].inDomain(val)) { // tested literalList  above
#endif
                          D_ASSERT(hasNoKnownSupport(var, lit));
                          PROP_INFO_ADDONE(CounterA);
                          findSupportsIncrementalHelper(var, val);
                          // No longer do we remove lit from zerolits in this
                          // case if support is found.
                          // However this is correct as it can be removed lazily
                          // next time the list is traversed
                          // And we might even get lucky and save this small
                          // amount of work.
                        } // } to trick vim bracket matching
        }                 // } to trick vim bracket matching
      }
    }
  }

  virtual void propagateDynInt(SysInt lit, DomainDelta) {

    //  cout << "Propagate called: var= " << var << "val = " << val << endl;
    // printStructures();

    updateCounters(lit);

    findSupportsIncremental();
  }

  virtual void fullPropagate() {
    full_prop_init();

    litsWithLostExplicitSupport.resize(0);
    varsWithLostImplicitSupport.resize(0);

    for(SysInt i = 0; i < (SysInt)vars.size(); i++) {
      varsWithLostImplicitSupport.push_back(i);
    }

    findSupportsIncremental();
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> ret;
    ret.reserve(vars.size());
    for(unsigned i = 0; i < vars.size(); ++i)
      ret.push_back(vars[i]);
    return ret;
  }
}; // end of class

template <typename T>
AbstractConstraint* BuildCT_HAGGISGAC(const T& t1, ConstraintBlob& b) {
  return new HaggisGAC<T>(t1, b.short_tuples);
}

/* JSON
  { "type": "constraint",
    "name": "haggisgac",
    "internal_name": "CT_HAGGISGAC",
    "args": [ "read_list", "read_short_tuples" ]
  }
*/
