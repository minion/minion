// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef PREPROCESS_H
#define PREPROCESS_H

#include "solver.h"

#include "parallel/parallel.h"
#include "parallel/preprocess_parallel.h"

#if !defined(_WIN32)
#include <unistd.h>
#endif

template <typename Var, typename Vars, typename Prop>
bool inline check_fail(Var& var, DomainInt val, Vars& vars, Prop prop) {
  Controller::worldPush();
  var.assign(val);
  prop(vars);

  bool checkFailed = getState().isFailed();

  Controller::worldPop();

  return checkFailed;
}

// Copied from dump_state.cpp
template <typename T>
inline string getNameFromVar(const T& v) {
  return getState().getInstance()->vars.getName(v.getBaseVar());
}

template <typename Var, typename Vars, typename Prop>
bool inline check_fail_range(Var& var, DomainInt lowval, DomainInt highval, Vars& vars, Prop prop) {
  Controller::worldPush();

  var.setMin(lowval);
  var.setMax(highval);
  prop(vars);

  bool checkFailed = getState().isFailed();
  Controller::worldPop();

  return checkFailed;
}

inline bool checkSACTimeout() {
  if(Parallel::isAlarmActivated()) {
    if(Parallel::isCtrlCPressed()) {
      getState().setFailed();
      return true;
    } else {
      getOptions().printLine("Time out in preprocessing.");
      getTableOut().set("TimeOut", 1);
      return true;
    }
  }
  return false;
}

template <typename Var, typename Prop>
bool pruneDomainTop(Var& var, vector<Var>& vararray, Prop prop, bool limit) {
  bool pruned = false;
  bool everfailed = false;
  DomainInt gallop = 1;
  while(true) {
    if(checkSACTimeout())
      throw EndOfSearch();
    if(var.min() == var.max()) {
      return pruned;
    }
    DomainInt maxval = var.max();
    DomainInt step = maxval - gallop;
    bool check = check_fail_range(var, step + 1, maxval, vararray, prop);
    if(check) {
      pruned = true;
      var.setMax(step);
      prop(vararray);
      if(getState().isFailed())
        return pruned;
      if(everfailed && limit) {
        gallop /= 2;
      } else {
        gallop *= 2;
      }
      DomainInt maxstep = var.max() - var.min();
      if(maxstep == 0)
        return pruned;
      gallop = min(gallop, maxstep);
    } else {
      everfailed = true;
      gallop /= 2;
    }
    if(gallop == 0) {
      return pruned;
    }
  }
}

template <typename Var, typename Prop>
bool pruneDomain_bottom(Var& var, vector<Var>& vararray, Prop prop, bool limit) {
  bool pruned = false;
  bool everfailed = false;
  DomainInt gallop = 1;
  while(true) {
    if(checkSACTimeout())
      throw EndOfSearch();
    if(var.min() == var.max()) {
      return pruned;
    }
    DomainInt minval = var.min();
    DomainInt step = minval + gallop;
    bool check = check_fail_range(var, minval, step - 1, vararray, prop);
    if(check) {
      pruned = true;
      var.setMin(step);
      prop(vararray);
      if(getState().isFailed())
        return pruned;
      if(everfailed && limit) {
        gallop /= 2;
      } else {
        gallop *= 2;
      }
      DomainInt maxstep = var.max() - var.min();
      if(maxstep == 0)
        return pruned;
      gallop = min(gallop, maxstep);
    } else {
      everfailed = true;
      gallop /= 2;
    }
    if(gallop == 0) {
      return pruned;
    }
  }
}

#include "constraints/constraint_collect_events.h"

// Bounds-pruning pass over a slice of vararray. Iterates only over indices in `owned`
// and tries to tighten min/max via galloping search. Returns true if any prune happened.
// Throws EndOfSearch on timeout. Returns early (with partial result) if state becomes failed.
template <typename Var, typename Prop>
bool runBoundsLoopSlice(vector<Var>& vararray, Prop prop, const vector<SysInt>& owned, bool limit) {
  bool reduced = false;
  for(size_t k = 0; k < owned.size(); ++k) {
    SysInt i = owned[k];
    Var& var = vararray[i];
    if(!var.isAssigned()) {
      if(pruneDomain_bottom(var, vararray, prop, limit))
        reduced = true;
      if(getState().isFailed())
        return reduced;
      if(pruneDomainTop(var, vararray, prop, limit))
        reduced = true;
      if(getState().isFailed())
        return reduced;
      if(checkSACTimeout())
        throw EndOfSearch();
    }
  }
  return reduced;
}

// Interior-value pruning pass over a slice of vararray. For each owned variable, tests every
// interior value via check_fail and removes failed ones. Returns true if any prune happened.
template <typename Var, typename Prop>
bool runValueLoopSlice(vector<Var>& vararray, Prop prop, const vector<SysInt>& owned) {
  bool reduced = false;
  for(size_t k = 0; k < owned.size(); ++k) {
    SysInt i = owned[k];
    Var& var = vararray[i];
    if(!var.isBound()) {
      for(DomainInt val = var.min() + 1; val <= var.max() - 1; ++val) {
        if(checkSACTimeout())
          throw EndOfSearch();
        if(var.inDomain(val) && check_fail(var, val, vararray, prop)) {
          reduced = true;
          var.removeFromDomain(val);
          prop(vararray);
          if(getState().isFailed())
            return reduced;
        }
      }
    }
  }
  return reduced;
}

// Parallel SAC fixpoint runner. Forks N worker processes per round; each
// worker runs the standard sequential SAC algorithm on its slice of variables
// (using the slice helpers above) and emits the final per-owned-variable
// domain into a shared-memory slot. The parent merges all slots into its own
// state, re-propagates, and runs another round if anything tightened.
//
// Only meaningful when getOptions().parallelPreprocessCores > 0 and the
// platform supports fork; the dispatcher in propagateSAC_internal already
// checks both. Sound: each worker's prunings are SAC-consistent in its COW
// state, which is reachable from the parent's state, so applying them in the
// parent is correct. The final fixpoint equals the sequential SAC fixpoint
// (which is unique); only the number of rounds and wall-clock time differ.
#if !defined(_WIN32)
template <typename Var, typename Prop>
void runParallelSACFixpoint(vector<Var>& vararray, Prop prop, bool onlyCheckBounds, bool limit) {
  using namespace ParallelSAC;

  int N = getOptions().parallelPreprocessCores;
  if(N <= 0)
    N = 1;

  // Compute literal counts for partitioning. Cast through long long because
  // DomainInt can be 64-bit under -domains64.
  std::vector<long long> litCounts;
  litCounts.reserve(vararray.size());
  for(SysInt i = 0; i < (SysInt)vararray.size(); ++i) {
    long long c = checked_cast<long long>(vararray[i].max() - vararray[i].min() + 1);
    if(c < 1)
      c = 1;
    litCounts.push_back(c);
  }

  std::vector<std::vector<SysInt>> partition = partitionByLiteralCount(litCounts, N);

  // Capacity per worker = total owned literal count + small per-var headroom.
  // Each owned var emits at most 1 setMin + 1 setMax + (lit-2) removeVal.
  std::vector<uint32_t> caps;
  caps.reserve(N);
  for(int w = 0; w < N; ++w) {
    unsigned long long cap = 0;
    for(size_t k = 0; k < partition[w].size(); ++k) {
      SysInt idx = partition[w][k];
      cap += (unsigned long long)litCounts[idx] + 4ULL;
    }
    if(cap < 16ULL)
      cap = 16ULL;
    if(cap > (unsigned long long)0xFFFFFFFFULL) {
      D_FATAL_ERROR("Parallel preprocess: per-worker capacity exceeds 2^32 entries");
    }
    caps.push_back((uint32_t)cap);
  }

  bool anythingReduced = true;
  int rounds = 0;
  int upperlimit = std::min(5, (int)log2(vararray.size()));

  while(anythingReduced) {
    if(limit) {
      rounds++;
      if(rounds > upperlimit)
        return;
    }
    anythingReduced = false;

    // Allocate one slot per worker for this round.
    std::vector<WorkerSlotHeader*> slots;
    slots.reserve(N);
    for(int w = 0; w < N; ++w)
      slots.push_back(allocateSlot(caps[w]));

    Parallel::RoundHandle round = Parallel::beginRound();

    for(int w = 0; w < N; ++w) {
      int forked = Parallel::forkForRound(round);
      if(forked == 0) {
        // Child: run sequential SAC on this worker's owned slice.
        WorkerSlotHeader* mySlot = slots[w];
        const std::vector<SysInt>& owned = partition[w];
        try {
          bool sliceReduced = true;
          while(sliceReduced) {
            // Inner bounds fixpoint on owned vars.
            while(sliceReduced) {
              sliceReduced = runBoundsLoopSlice(vararray, prop, owned, limit);
              if(getState().isFailed())
                break;
            }
            if(getState().isFailed())
              break;
            if(!onlyCheckBounds)
              sliceReduced = runValueLoopSlice(vararray, prop, owned);
            else
              sliceReduced = false;
            if(getState().isFailed())
              break;
          }

          if(getState().isFailed())
            mySlot->failed = 1;

          // Emit final domain of each owned variable as setMin + setMax +
          // removeVal entries. Parent applies these onto its own state.
          for(size_t k = 0; k < owned.size(); ++k) {
            SysInt idx = owned[k];
            Var& var = vararray[idx];
            DomainInt vmin = var.min();
            DomainInt vmax = var.max();
            slotAppend(mySlot, (uint32_t)idx, KIND_SET_MIN, checked_cast<long long>(vmin));
            slotAppend(mySlot, (uint32_t)idx, KIND_SET_MAX, checked_cast<long long>(vmax));
            if(!var.isBound() && vmin < vmax) {
              for(DomainInt v = vmin + 1; v < vmax; ++v) {
                if(!var.inDomain(v))
                  slotAppend(mySlot, (uint32_t)idx, KIND_REMOVE_VAL,
                             checked_cast<long long>(v));
              }
            }
          }
          mySlot->status.store(STATUS_OK);
        } catch(EndOfSearch&) {
          // Record whatever was pruned up to the timeout point.
          if(getState().isFailed())
            mySlot->failed = 1;
          for(size_t k = 0; k < owned.size(); ++k) {
            SysInt idx = owned[k];
            Var& var = vararray[idx];
            DomainInt vmin = var.min();
            DomainInt vmax = var.max();
            slotAppend(mySlot, (uint32_t)idx, KIND_SET_MIN, checked_cast<long long>(vmin));
            slotAppend(mySlot, (uint32_t)idx, KIND_SET_MAX, checked_cast<long long>(vmax));
            if(!var.isBound() && vmin < vmax) {
              for(DomainInt v = vmin + 1; v < vmax; ++v) {
                if(!var.inDomain(v))
                  slotAppend(mySlot, (uint32_t)idx, KIND_REMOVE_VAL,
                             checked_cast<long long>(v));
              }
            }
          }
          mySlot->status.store(STATUS_EOS);
        } catch(...) {
          mySlot->status.store(STATUS_FATAL);
        }
        // _exit avoids running atexit / global destructors which would touch
        // shared mmap regions and the parent's state.
        _exit(0);
      }
      // Parent: continue forking next worker.
    }

    // Parent: wait for all children.
    Parallel::endRound(round);

    // This is one full fork-join round; record it for the random
    // tester's adaptive-sizing sweep so we can tell if the parallel
    // path actually fired more than once (rounds >= 2 means at least
    // one round produced prunings AND another was needed to converge).
    getState().incrementParallelPreprocessRounds();

    // Scan slot statuses and merge prunings.
    bool anyEOS = false;
    bool anyProvedFailure = false;
    long long roundPrunings = 0;
    for(int w = 0; w < N; ++w) {
      uint32_t s = slots[w]->status.load();
      if(s == STATUS_RUNNING || s == STATUS_FATAL) {
        D_FATAL_ERROR("Parallel preprocess worker exited unexpectedly");
      }
      if(s == STATUS_EOS)
        anyEOS = true;
      if(slots[w]->failed)
        anyProvedFailure = true;

      uint32_t count = slots[w]->entry_count;
      PruneEntry* es = slotEntries(slots[w]);
      for(uint32_t k = 0; k < count; ++k) {
        SysInt idx = (SysInt)es[k].var_idx;
        Var& var = vararray[idx];
        DomainInt v(checked_cast<SysInt>(es[k].a));
        switch(es[k].kind) {
        case KIND_SET_MIN:
          if(v > var.min()) {
            var.setMin(v);
            anythingReduced = true;
            ++roundPrunings;
          }
          break;
        case KIND_SET_MAX:
          if(v < var.max()) {
            var.setMax(v);
            anythingReduced = true;
            ++roundPrunings;
          }
          break;
        case KIND_REMOVE_VAL:
          if(var.inDomain(v)) {
            var.removeFromDomain(v);
            anythingReduced = true;
            ++roundPrunings;
          }
          break;
        }
      }
    }
    getState().incrementParallelPreprocessPrunings(roundPrunings);

    for(int w = 0; w < N; ++w)
      releaseSlot(slots[w]);

    // Re-propagate to derive cross-worker consequences and to set isFailed if
    // applicable.
    if(!getState().isFailed()) {
      prop(vararray);
    }

    if(anyProvedFailure && !getState().isFailed()) {
      // A worker derived a contradiction from a state reachable from ours
      // (its COW state was a tightening of ours), so the problem is unsat.
      getState().setFailed();
    }

    if(getState().isFailed())
      return;

    if(anyEOS)
      throw EndOfSearch();
  }
}
#endif

template <typename Var, typename Prop>
void propagateSAC_internal(vector<Var>& vararray, Prop prop, bool onlyCheckBounds, bool limit,
                           bool allowParallel = false) {
  getQueue().propagateQueue();
  if(getState().isFailed())
    return;

#if !defined(_WIN32)
  if(allowParallel && getOptions().parallelPreprocessCores > 0) {
    runParallelSACFixpoint(vararray, prop, onlyCheckBounds, limit);
    if(getState().isFailed())
      return;
  } else
#endif
  {
    bool reduced = true;
    int loops = 0;

    int upperlimit = std::min(5, (int)log2(vararray.size()));

    vector<SysInt> allIdx;
    allIdx.reserve(vararray.size());
    for(SysInt i = 0; i < (SysInt)vararray.size(); ++i)
      allIdx.push_back(i);

    while(reduced) {
      // First loop around bounds as long as possible
      while(reduced) {
        if(limit) {
          loops++;
          if(loops > upperlimit) {
            return;
          }
        }
        reduced = runBoundsLoopSlice(vararray, prop, allIdx, limit);
        if(getState().isFailed())
          return;
      }

      // Then try inside domain
      if(!onlyCheckBounds) {
        reduced = runValueLoopSlice(vararray, prop, allIdx);
        if(getState().isFailed())
          return;
      }
    }
  }
}

struct PropagateGAC {
  PropagationLevel level;

  PropagateGAC(PropagationLevel _level) : level(_level) {}

  template <typename Vars>
  void operator()(Vars&) {
    getQueue().propagateQueue();
  }
};

struct PropagateSAC {
  PropagationLevel level;
  bool allowParallel;

  PropagateSAC(PropagationLevel _level, bool _allowParallel = true)
      : level(_level), allowParallel(_allowParallel) {}

  template <typename Vars>
  void operator()(Vars& vars) {
    propagateSAC_internal(vars, PropagateGAC(level), false, level.limit, allowParallel);
  }
};

struct PropagateSAC_Bounds {
  PropagationLevel level;
  bool allowParallel;

  PropagateSAC_Bounds(PropagationLevel _level, bool _allowParallel = true)
      : level(_level), allowParallel(_allowParallel) {}

  template <typename Vars>
  void operator()(Vars& vars) {
    propagateSAC_internal(vars, PropagateGAC(level), true, level.limit, allowParallel);
  }
};

struct PropagateSSAC {
  PropagationLevel level;

  PropagateSSAC(PropagationLevel _level) : level(_level) {}

  template <typename Vars>
  void operator()(Vars& vars) {
    PropagateSAC sac(level, /*allowParallel=*/false);
    propagateSAC_internal(vars, sac, false, level.limit, /*allowParallel=*/false);
  }
};

struct PropagateSSAC_Bounds {
  PropagationLevel level;

  PropagateSSAC_Bounds(PropagationLevel _level) : level(_level) {}

  template <typename Vars>
  void operator()(Vars& vars) {
    PropagateSAC sac(level, /*allowParallel=*/false);
    propagateSAC_internal(vars, sac, true, level.limit, /*allowParallel=*/false);
  }
};

// Class heirarchy to allow virtual function calls to the above.
struct Propagate {
  virtual ~Propagate() {}
  virtual void prop(vector<AnyVarRef>& vars){};
};

struct PropGAC : Propagate {
  PropGAC(PropagationLevel level) : prop_obj(level) {}

  PropagateGAC prop_obj;
  inline void prop(vector<AnyVarRef>& vars) {
    prop_obj(vars);
  }
};

struct PropSAC : Propagate {
  PropSAC(PropagationLevel level) : prop_obj(level) {}

  PropagateSAC prop_obj;
  inline void prop(vector<AnyVarRef>& vars) {
    prop_obj(vars);
  }
};

struct PropSSAC : Propagate {
  PropSSAC(PropagationLevel level) : prop_obj(level) {}

  PropagateSSAC prop_obj;
  inline void prop(vector<AnyVarRef>& vars) {
    prop_obj(vars);
  }
};

struct PropSAC_Bounds : Propagate {
  PropSAC_Bounds(PropagationLevel level) : prop_obj(level) {}

  PropagateSAC_Bounds prop_obj;
  inline void prop(vector<AnyVarRef>& vars) {
    prop_obj(vars);
  }
};

struct PropSSAC_Bounds : Propagate {
  PropSSAC_Bounds(PropagationLevel level) : prop_obj(level) {}

  PropagateSSAC_Bounds prop_obj;
  inline void prop(vector<AnyVarRef>& vars) {
    prop_obj(vars);
  }
};

void PropogateCSP(PropagationLevel, vector<AnyVarRef>&, bool printInfo = false);

#endif
