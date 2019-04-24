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

#ifndef SEARCHMANAGER_H
#define SEARCHMANAGER_H

#include "../preprocess.h"
#include "common_search.h"
#include "variable_orderings.h"

namespace Controller {

template <typename T>
void inline maybe_print_search_assignment(T& var, DomainInt val, BOOL equal) {
  if(getOptions().dumptree) {
    cout << "SearchAssign:" << var << (equal ? " = " : " != ") << val << endl;
  }
  if(getOptions().dumptreeobj) {
    getOptions().dumptreeobj->branch(getState().getNodeCount(), getBaseVarName(var), val, equal);
  }
}

// instead of carrying around the pos everywhere, the VariableOrder object has
// pos in it as a reversible<SysInt>.

struct SearchManager {
  inline virtual ~SearchManager() {}

  virtual void search() = 0;
};

struct StandardSearchManager : public SearchManager {

  inline virtual ~StandardSearchManager() {}

  std::function<void(const vector<AnyVarRef>&, const vector<Controller::triple>&)> check_func;
  std::function<void(void)> handle_sol_func, handle_opt_func;

  vector<AnyVarRef> varArray;
  shared_ptr<VariableOrder> varOrder;

  shared_ptr<Propagate> prop; // Propagate is the type of the base class. Method
                              // prop->prop(varArray)

  vector<Controller::triple> branches; // L & R branches so far (isLeftBranch?,var,value)

  StandardSearchManager(
      shared_ptr<VariableOrder> _varOrder, shared_ptr<Propagate> _prop,
      std::function<void(const vector<AnyVarRef>&, const vector<Controller::triple>&)> _check_func,
      std::function<void(void)> _handle_sol_func, std::function<void(void)> _handle_opt_func)
      : check_func(_check_func),
        handle_sol_func(_handle_sol_func),
        handle_opt_func(_handle_opt_func),
        varOrder(_varOrder),
        prop(_prop) {
    varArray = varOrder->getVars();
    branches.reserve(varArray.size());
  }

  void reset() {
    branches.clear();
  }

  // returns false if left branch not possible.
  inline void branch_left(pair<SysInt, DomainInt> picked) {
    D_ASSERT(picked.first != -1);
    D_ASSERT(!varArray[picked.first].isAssigned());

    worldPush();
    varArray[picked.first].assign(picked.second);
    maybe_print_search_assignment(varArray[picked.first], picked.second, true);
    branches.push_back(Controller::triple(true, picked.first, picked.second));
  }

  inline bool branch_right() {
    while(!branches.empty() && !branches.back().isLeft) { // pop off all the
                                                          // RBs
      maybe_print_right_backtrack();
      D_ASSERT(!branches.back().stolen);
      branches.pop_back();
    }

    if(branches.empty())
      return false;

    worldPop();

    SysInt var = branches.back().var;
    DomainInt val = branches.back().val;
    bool isLeft = branches.back().isLeft;
    (void)isLeft;
  
    bool stolen = branches.back().stolen;

    D_ASSERT(isLeft);
    // remove the left branch.
    branches.pop_back();

    D_ASSERT(varArray[var].inDomain(val));

    // special case the upper and lower bounds to make it work for bound
    // variables
    if(varArray[var].min() == val) {
      varArray[var].setMin(val + 1);
    } else if(varArray[var].max() == val) {
      varArray[var].setMax(val - 1);
    } else {
      varArray[var].removeFromDomain(val);
    }
    maybe_print_search_assignment(varArray[var], val, false);
    branches.push_back(Controller::triple(false, var, val));

    // If this branch was stolen, then we want to carry on
    // backtracking
    if(stolen)
      return branch_right();
    else
      return true;
  }

  inline bool in_aux_vars() {
    return varOrder->hasAuxVars() && !branches.empty() &&
           branches.back().var >= varOrder->auxVarStart();
  }

  inline void jump_out_aux_vars() {
    while(in_aux_vars()) {
      if(branches.back().isLeft) {
        worldPop();
        maybe_print_right_backtrack();
      }

      branches.pop_back();
    }
  }

  // Steal work isn't used, but is left here in case anyone ever wants it.
  int steal_work() { // find where to steal the topmost left branch from this search.
    for(UnsignedSysInt newceil = 0; newceil < branches.size() - 1; ++newceil) {
      if(branches[newceil].isLeft && !branches[newceil].stolen) {
        return newceil;
      }
    }
    return -1;
  }

  // Most basic search procedure
  virtual void search() {
    maybe_print_node();
    while(true) {
      D_ASSERT(getQueue().isQueuesEmpty());

      getState().incrementNodeCount();

      check_func(varArray, branches);

      pair<SysInt, DomainInt> varval = varOrder->pickVarVal();

      if(varval.first == -1) {
        // We have found a solution!
        check_sol_is_correct();
        maybe_print_node(true);
        handle_sol_func();
        if(varOrder->hasAuxVars()) { // There are AUX vars at the end of the var ordering.
          // Backtrack out of them.
          jump_out_aux_vars();
        }

        // If we are not finished, then go into the loop below.
        getState().setFailed(true);
      } else {
        maybe_print_node();
        branch_left(varval);
        prop->prop(varArray);
      }

      if(getOptions().parallel && !getState().isFailed() && !in_aux_vars()) {
        if(getOptions().parallelStealHigh) {
          bool doFork = Parallel::shouldDoFork();

          int steal = steal_work();

          int isParent = 0;

          // std::cerr << "Fork?" << getState().isFailed() << "\n";;
          if(doFork) {
            // std::cerr << "Yes, do a fork!\n";

            isParent = Parallel::doFork();

            if(isParent) {
              if(steal != -1) {
                branches[steal].stolen = true;
              }
            } else {
              // Todo, avoid this process creation?
              if(steal == -1) {
                Parallel::endParallelMinion();
                exit(0);
              }
              // lockSolsout();
              // std::cerr << "stealing " << steal << "\n";
              // std::cerr << branches << "\n";
              while(branches.size() > steal + 1) {
                // std::cerr << "right branch: " << branches << "\n";
                int ret = branch_right();
                if(!ret) {
                  D_FATAL_ERROR("Error in parallel forking");
                }
              }
              D_ASSERT(branches.size() == steal + 1);
              prop->prop(varArray);
              // std::cerr << branches << "\n";
              // unlockSolsout();
            }
          }
        } else {
          bool doFork = Parallel::shouldDoFork();
          if(doFork) {
            // std::cerr << "Yes, do a fork!\n";

            int isParent = Parallel::doFork();
            D_CHECK(isParent >= 0);
            if(isParent) {
              // Force to ignore left branch
              getState().setFailed(true);
            } else {
              // Force to stay in left branch
              reset();
            }
          }
        }
      }

      // loop to
      while(getState().isFailed()) {
        if(branches.size() == 0) {
          return;
        }

        maybe_print_backtrack();
        bool flag = branch_right();
        if(!flag) { // No remaining left branches to branch right.
          return;
        }
        // Deal with optimisation variables
        getState().incrementBacktrackCount();
        handle_opt_func();
        prop->prop(varArray);
      }
    }
  }
};
} // namespace Controller

#endif
