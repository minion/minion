// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

/*
 * Functions for using minion as a library.
 */

#include "libwrapper.h"
#include "command_search.h"
#include "info_dumps.h"
#include "inputfile_parse/CSPSpec.h"
#include "minion.h"
#include "parallel/parallel.h"
#include "search/SearchManager.h"
#include "solver.h"
#include "system/minlib/exceptions.hpp"
#include "triggering/trigger_list.h"
#include "tuple_container.h"
#include <iomanip>
#include <limits>
#include <memory>
#include <cstdlib>
#include <functional>
#include <pthread.h>

#ifdef LIBMINION

// from minion.cpp
void doStandardSearch(CSPInstance& instance, SearchMethod args);
void finaliseModel(CSPInstance& instance);

extern thread_local Globals* globals;

// A worker thread runs the whole solver -- model copy, setup, propagation
// and search -- exactly as the main thread does when running sequentially.
// std::thread gives a new thread the platform default stack, which on macOS
// is 512 KB against the main thread's 8 MB, so work that succeeds
// sequentially runs a worker off the end of its stack and kills the whole
// process with SIGBUS. Create workers with an explicit stack instead, so a
// worker gets what the main thread gets on every platform.
namespace {
constexpr size_t workerStackBytes = 8u * 1024 * 1024;

class WorkerThread {
public:
  explicit WorkerThread(std::function<void()> body) {
    auto* payload = new std::function<void()>(std::move(body));
    pthread_attr_t attr;
    if(pthread_attr_init(&attr) != 0) {
      delete payload;
      throw parse_exception("cannot create worker thread attributes");
    }
    if(pthread_attr_setstacksize(&attr, workerStackBytes) != 0) {
      pthread_attr_destroy(&attr);
      delete payload;
      throw parse_exception("cannot set worker thread stack size");
    }
    int err = pthread_create(&tid, &attr, &WorkerThread::trampoline, payload);
    pthread_attr_destroy(&attr);
    if(err != 0) {
      delete payload;
      throw parse_exception("cannot create worker thread");
    }
  }

  WorkerThread(const WorkerThread&) = delete;
  WorkerThread& operator=(const WorkerThread&) = delete;
  WorkerThread(WorkerThread&& o) : tid(o.tid), joined(o.joined) {
    o.joined = true;
  }

  void join() {
    D_ASSERT(!joined);
    pthread_join(tid, NULL);
    joined = true;
  }

  ~WorkerThread() {
    D_ASSERT(joined);
  }

private:
  static void* trampoline(void* p) {
    std::unique_ptr<std::function<void()>> body(
        static_cast<std::function<void()>*>(p));
    (*body)();
    return NULL;
  }

  pthread_t tid;
  bool joined = false;
};
} // namespace

static thread_local std::string ffi_error_message;

static void set_error(const std::string& msg) {
  ffi_error_message = msg;
}

const char* minion_error_message() {
  return ffi_error_message.c_str();
}

// Abort if called while search is active (globals != nullptr).
// Pre-search-only functions should call this to prevent misuse from callbacks.
static void assertNotInSearch(const char* funcName) {
  if(globals != nullptr) {
    fprintf(stderr, "FATAL: %s cannot be called during search. "
                    "Use the *Midsearch variant instead.\n", funcName);
    abort();
  }
}

// RAII guard: sets globals on construction, clears on destruction.
// If globals is already set to ctx (e.g. callback re-entry), this is a no-op.
// Asserts if globals is set to a *different* context (re-entrant call with wrong ctx).
struct ContextGuard {
  bool isOuterEntry;
  ContextGuard(MinionContext* ctx) {
    if(globals == nullptr) {
      globals = ctx;
      isOuterEntry = true;
    } else if(globals == ctx) {
      // Re-entrant call from callback with same context — allowed
      isOuterEntry = false;
    } else {
      // globals is set to a different context — this is a bug
      throw std::runtime_error("Re-entrant minion API call with different context");
    }
  }
  ~ContextGuard() {
    if(isOuterEntry) {
      globals = nullptr;
    }
  }
};

static void resetContextState(MinionContext* ctx)
{
  // Clear the process/thread-local null-trigger list so no stale trigger refs
  // from a previous run survive into the next one.
  clearNullTriggerList();

  // Delete sub-objects to reset state for a fresh run, but keep the context alive
  delete ctx->bools_m;     ctx->bools_m = NULL;
  delete ctx->state_m;     ctx->state_m = NULL;
  delete ctx->queues_m;    ctx->queues_m = NULL;
  delete ctx->options_m;   ctx->options_m = NULL;
  delete ctx->varContainer_m; ctx->varContainer_m = NULL;
  delete ctx->searchMem_m; ctx->searchMem_m = NULL;
  delete ctx->tableOut_m;  ctx->tableOut_m = NULL;
  ctx->callback = NULL;
  ctx->callbackUserdata = NULL;
}

MinionContext* minion_newContext()
{
  MinionContext* ctx = new MinionContext();

  // Settle this context's output destination once, here, and never
  // touch it again. Doing it per run meant every entry and exit
  // rewrote out_m, so concurrent runs on one context fought over it and
  // a finishing run handed the terminal back to a run that was supposed
  // to stay quiet.
  //
  // out_sink_m stays unopened unless LIBMINION_LOG asks for a file; an
  // unopened ofstream discards what is written to it without buffering.
  if(std::getenv("LIBMINION_LOG")) {
    time_t rawtime;
    time(&rawtime);
    stringstream filenameStream;
    filenameStream << "minion";
    filenameStream << put_time(gmtime(&rawtime), "%Y-%m-%d-%H:%M:%S");
    filenameStream << ".log";
    ctx->out_sink_m.open(filenameStream.str(), ios_base::app);
  }
  ctx->out_m = &ctx->out_sink_m;

  return ctx;
}

void minion_freeContext(MinionContext* ctx)
{
  delete ctx;
}

void minion_activateContext(MinionContext* ctx)
{
  assert(globals == nullptr && "Cannot activate context: another context is already active on this thread");
  globals = ctx;
}

void minion_deactivateContext()
{
  globals = nullptr;
}

// Internal helper: body of runMinion, parameterised so the parallel-threads
// controller can call it once per worker without re-installing the
// process-global alarm/signal handlers (which would clobber each other —
// see system/trigger_timer.cpp:69 where activateTrigger overwrites the
// static trig pointer and re-arms alarm()).
//
// installAlarms: when true (single-threaded use), call Parallel::setupAlarm
// and Parallel::endParallelMinion as before. When false (worker thread under
// a thread controller), the controller is responsible for installing the
// shared alarm/ctrl-C atomics; workers just observe via getParallelData().
static MinionResult runMinionImpl(MinionContext* ctx, SearchOptions& options,
                                  SearchMethod& args,
                                  ProbSpec::CSPInstance& instance,
                                  bool (*callback)(MinionContext* ctx, void* userdata),
                                  void* userdata, bool installAlarms)
{
  ContextGuard guard(ctx);
  MinionResult returnCode = MinionResult::MINION_OK;

  /*
   * Adapted from minion_main.
   * Whereas minion_main takes in command line arguments, we take in Minion
   * objects.
   */

  resetContextState(ctx);

  // If the caller supplied a -solsout filename in the SearchOptions, the
  // worker context's own solsoutfile needs to be opened too — under the
  // thread-mode flags each worker has its own globals->solsoutfile, and
  // the parent's open file isn't accessible from here. Each worker
  // opens its own ofstream onto the same path in append+line-flushed
  // mode; the broadened Parallel::lockSolsout serialises writes, and
  // POSIX guarantees small (<PIPE_BUF) writes don't interleave.
  if(!options.solsoutFilename.empty()) {
    globals->solsoutfile.open(options.solsoutFilename, ios::app);
  }

  // Output destination is fixed when the context is created
  // (minion_newContext) and deliberately not touched here: a per-run
  // setup/restore is what made concurrent runs fight over it.
  time_t rawtime;
  time(&rawtime);

  // Pass error codes across FFI boundaries, not exceptions.
  try {

    // No parallel minion in library usage for now
    //getParallelData();

    getState().getOldTimer().startClock();

    globals->callback = callback;
    globals->callbackUserdata = userdata;
    globals->options_m = new SearchOptions(options);
    globals->options_m->findAllSolutions();

    getOptions().printLine("# " + std::string(MinionVersion));
    getOptions().printLine("# Git version: \"" + tostring(GIT_VER) + "\"");

    GET_GLOBAL(global_random_gen).seed(args.randomSeed);
    if(!getOptions().silent) {
      getOutput() << "#  Run at: UTC " << asctime(gmtime(&rawtime)) << endl;
      getOutput() << "# Input filename: " << getOptions().instance_name << endl;
      getOptions().printLine("Using seed: " + tostring(args.randomSeed));
    }

    if(installAlarms) {
      Parallel::setupAlarm(getOptions().timeoutActive, getOptions().time_limit,
                           getOptions().time_limit_is_CPUTime);
    }

    // finaliseModel mutates the CSPInstance (sets default searchOrder /
    // symOrder, calls setupValueOrder). Worker threads under
    // runMinionParallel share the same CSPInstance, so the controller
    // calls finaliseModel ONCE up front and workers skip it. installAlarms
    // serves as the discriminator: it's true on the standalone runMinion
    // path (where the instance must be finalised here) and false on the
    // worker path (where the controller already did it).
    if(installAlarms) {
      finaliseModel(instance);
    }

    // Output graphs, stats, or redump (will not return in these cases)
    infoDumps(instance);

    // Copy args into tableout
    getTableOut().set("RandomSeed", tostring(args.randomSeed));
    getTableOut().set("Preprocess", tostring(args.preprocess));

    getTableOut().set("MinionVersion", MinionVersion);
    getTableOut().set("GitVersion", tostring(GIT_VER));
    getTableOut().set("TimeOut", 0); // will be set to 1 if a timeout occurs.
    getState().getOldTimer().maybePrintTimestepStore(getOutput(), "Parsing Time: ", "ParsingTime",
                                                     getTableOut(), !getOptions().silent);

    SetupCSPOrdering(instance, args);
    BuildCSP(instance);

    // TODO (nd60): how to replace this??
    if(getOptions().commandlistIn != "") {
      doCommandSearch(instance, args);
    } else {
      doStandardSearch(instance, args);
    }

  }

  catch(const parse_exception& e) {
    getOutput() << "Invalid instance: " << e.what() << endl;
    set_error(e.what());
    returnCode = MinionResult::MINION_INVALID_INSTANCE;
  } catch(const std::bad_alloc&) {
    set_error("out of memory");
    returnCode = MinionResult::MINION_MEMORY_ERROR;
  } catch(const std::exception& e) {
    set_error(e.what());
    returnCode = MinionResult::MINION_UNKNOWN_ERROR;
  } catch(...) {
    set_error("unknown exception");
    returnCode = MinionResult::MINION_UNKNOWN_ERROR;
  }

  // Detect timeout: doStandardSearch sets TableOut "TimeOut" to 1
  if(returnCode == MinionResult::MINION_OK && ctx->tableOut_m) {
    try {
      if(getTableOut().get("TimeOut") == "1") {
        set_error("solver timed out");
        returnCode = MinionResult::MINION_TIMEOUT;
      }
    } catch(...) {}
  }

  if(installAlarms) {
    Parallel::endParallelMinion();
  }

  // Don't reset context here - caller may still query results via
  // printMatrix_getValue or TableOut_get

  return returnCode;
}

MinionResult runMinion(MinionContext* ctx, SearchOptions& options, SearchMethod& args,
                       ProbSpec::CSPInstance& instance,
                       bool (*callback)(MinionContext* ctx, void* userdata),
                       void* userdata)
{
  return runMinionImpl(ctx, options, args, instance, callback, userdata,
                       /*installAlarms=*/true);
}

/*********************************************************************/
/*                  Thread-based portfolio search                    */
/*********************************************************************/

#include "system/trigger_timer.h"
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>

namespace {

// Shared coordinator state for one runMinionParallel invocation. Owned by
// the parent thread; lives on the stack of runMinionParallel for the entire
// duration of the parallel run; pointer captured by each worker.
struct ParallelController {
  Parallel::ParallelData* sharedParData;  // aliased into every worker's parData_m
  std::atomic<long long> totalSolutionsFound;
  // Aggregated node count across all workers, contributed before each
  // worker frees its context. Surfaced via parent's TableOut so
  // -jsontableout reports cross-worker work, matching the work-steal
  // path's contract.
  std::atomic<long long> totalNodesExplored{0};
  std::atomic<bool> stopRequested;
  std::mutex callbackMutex;
  long long sollimit;
  bool (*userCallback)(MinionContext*, void*);
  void* userUserdata;
  // Cross-worker optimisation bound channel. Workers push their
  // post-bump optVals[0] here on each solution (CAS-max) and read it
  // back during opt_handler to tighten further than their own local
  // bound. LLONG_MIN sentinel = no bound seen yet. Only used when
  // OptimiseVars.size() == 1 (single-objective). See solver.h's
  // SearchOptions::parallelBoundChannel for direction details.
  std::atomic<long long> sharedOptBound{std::numeric_limits<long long>::min()};
};

// Wrapper callback: invoked on each solution event from any worker. Locks
// the controller mutex so the user callback never re-enters itself, applies
// the shared sollimit, and signals the cross-thread alarm/ctrl-C atomics so
// other workers stop on their next standardTime_ctrlc_checks poll without
// emitting a "Time out" message.
static bool parallelWrapperCallback(MinionContext* ctx, void* ud)
{
  ParallelController* shared = static_cast<ParallelController*>(ud);
  std::lock_guard<std::mutex> lock(shared->callbackMutex);

  // If another thread already requested stop, this thread has produced one
  // more solution but should also exit now.
  if(shared->stopRequested.load(std::memory_order_acquire))
    return false;

  long long count = shared->totalSolutionsFound.fetch_add(1) + 1;
  bool keepGoing = true;
  if(shared->userCallback) {
    keepGoing = shared->userCallback(ctx, shared->userUserdata);
  }

  if(!keepGoing || (shared->sollimit > 0 && count >= shared->sollimit)) {
    shared->stopRequested.store(true, std::memory_order_release);
    if(shared->sharedParData) {
      // Setting BOTH alarm and ctrlC takes the clean-stop branch in
      // common_search.h's standardTime_ctrlc_checks (throws EndOfSearch
      // without printing "Time out").
      shared->sharedParData->alarmTrigger.store(true);
      shared->sharedParData->ctrlCPressed.store(true);
    }
    return false;
  }
  return true;
}

}  // anonymous namespace

MinionResult runMinionParallel(MinionThreadConfig config, SearchOptions& options,
                               SearchMethod& args, ProbSpec::CSPInstance& instance,
                               bool (*callback)(MinionContext* ctx, void* userdata),
                               void* userdata)
{
  if(config.numThreads < 1) {
    set_error("runMinionParallel: numThreads must be >= 1");
    return MinionResult::MINION_INVALID_ARGUMENT;
  }

  // The parent thread may already have a context active (e.g. the CLI main
  // thread). Save and clear so the worker threads' newly-spawned threads
  // start from a clean thread_local pointer; restore before returning. The
  // parent context's solver is not used during the parallel run.
  Globals* savedGlobals = globals;
  globals = nullptr;

  const int N = config.numThreads;

  ParallelController shared;
  shared.sharedParData = Parallel::setupThreadParallelData();
  shared.totalSolutionsFound.store(0);
  shared.stopRequested.store(false);
  shared.sollimit = options.sollimit;
  shared.userCallback = callback;
  shared.userUserdata = userdata;

  // Install the process-wide alarm/SIGXCPU trigger ONCE, pointing at the
  // shared atomic. activateTrigger overwrites a static `trig` pointer; if we
  // let each worker call setupAlarm, the second worker would clobber the
  // first.
  if(options.timeoutActive) {
    activateTrigger(&shared.sharedParData->alarmTrigger, options.timeoutActive,
                    options.time_limit, options.time_limit_is_CPUTime);
  }

  // Each worker creates AND destroys its own context inside its own thread.
  // This is essential: the context's constraints / triggers pin into a
  // thread_local DynamicTriggerList (globals.cpp), so freeing a context on a
  // different thread than the one that built it dereferences invalid
  // thread-local storage.
  std::vector<MinionResult> results(N, MinionResult::MINION_OK);
  // Per-worker copy of State::lastOptimumValues, taken before each
  // worker frees its context. Aggregated by the parent after all
  // workers join — we don't need a cross-worker atomic during search
  // because the in-search shared bound channel (sharedOptBound) is
  // already pruning, and each worker's own lastOptimumValues
  // monotonically improves under its local lock.
  std::vector<std::vector<DomainInt>> perWorkerOptValues(N);
  std::vector<WorkerThread> workers;
  workers.reserve(N);

  // Pre-finalise the shared instance ONCE (it mutates default searchOrder /
  // symOrder / setupValueOrder; doing it concurrently per worker would
  // race). Workers' runMinionImpl skips finaliseModel because we passed
  // installAlarms=false.
  finaliseModel(instance);

  for(int i = 0; i < N; ++i) {
    workers.emplace_back([&, i]() {
      try {
      MinionContext* ctx = minion_newContext();
      // Alias the worker's parData_m to the shared struct so
      // getParallelData() (which lazy-allocates if null) returns the shared
      // one and Parallel::isAlarmActivated() / isCtrlCPressed() read the
      // shared atomic.
      ctx->parData_m = shared.sharedParData;

      // Deep-copy the CSPInstance per worker. BuildCSP holds the
      // ConstraintBlobs by mutable reference and the constraint-builder
      // sometimes mutates them in place (e.g., sorting/normalising args).
      // Sharing the instance across workers therefore races. Copy is cheap
      // for typical models and keeps the workers fully independent.
      ProbSpec::CSPInstance perThreadInstance = instance;

      // Per-thread option / args copies so we can diversify the seed
      // without mutating the caller's structures and without aliasing
      // across threads.
      SearchOptions perThreadOptions = options;
      SearchMethod perThreadArgs = args;
      perThreadArgs.randomSeed = (int)(args.randomSeed ^ (config.baseSeed + (unsigned int)i));
      // Set sollimit to "find all" inside each worker; the controller's
      // wrapper callback enforces the shared sollimit across threads.
      perThreadOptions.sollimit = -1;
      // Per-thread RNG seed already differs (baseSeed XOR threadIdx). We do
      // NOT auto-enable randomiseValvarorder: it can make problems with a
      // good natural ordering dramatically slower (e.g. graceful k5p2 goes
      // from <1s to >30s under random ordering). Users wanting a more
      // diverse portfolio should pass -randomiseorder explicitly.
      // Prevent infinite recursion: the worker is itself a parallel run, so
      // its inner doStandardSearch must take the sequential path.
      perThreadOptions.numParallelThreads = 0;
      // Workers share the controller's parData; they must not also be flagged
      // as fork-mode workers (which would re-init signal handlers etc).
      perThreadOptions.parallel = false;
      // Bound channel: each worker reads/writes the controller's
      // shared atomic to coordinate optimisation pruning. See
      // SearchOptions::parallelBoundChannel for the full contract.
      perThreadOptions.parallelBoundChannel = &shared.sharedOptBound;
      // For N>1, silence per-worker stats prints so the parent's summary is
      // the single source of truth. With N=1, leave silent untouched so
      // behaviour matches sequential runMinion exactly (incl. "Sol:" lines
      // for fixture-style tests).
      if(N > 1) {
        perThreadOptions.silent = true;
      }

      MinionResult r = runMinionImpl(ctx, perThreadOptions, perThreadArgs,
                                     perThreadInstance, parallelWrapperCallback,
                                     &shared, /*installAlarms=*/false);
      results[i] = r;

      // Contribute this worker's node count to the parent aggregate so
      // -jsontableout reports total cross-thread work.
      if(ctx->state_m) {
        shared.totalNodesExplored.fetch_add(
            (long long)ctx->state_m->getNodeCount(),
            std::memory_order_relaxed);
        // Snapshot best optimum found by this worker (empty if it
        // never saw a non-dominated solution). Done before freeContext
        // while ctx->state_m is still alive.
        if(ctx->state_m->isOptimisationProblem()) {
          perWorkerOptValues[i] = ctx->state_m->getLastOptimumValues();
        }
      }

      // Don't let ~Globals try to free the shared parData_m (the destructor
      // currently doesn't, but make the intent explicit and future-proof).
      ctx->parData_m = nullptr;
      minion_freeContext(ctx);
      } catch(const std::exception& e) {
        // Swallow exceptions inside the worker thread lambda — letting them
        // propagate out of std::thread is undefined / aborts. Mark the
        // worker as having returned an error and let aggregation pick it up.
        results[i] = MinionResult::MINION_UNKNOWN_ERROR;
      } catch(...) {
        results[i] = MinionResult::MINION_UNKNOWN_ERROR;
      }
    });
  }

  for(auto& t : workers)
    t.join();

  // Reset alarm so it doesn't carry into a subsequent runMinion call on this
  // process.
  if(options.timeoutActive) {
    activateTrigger(&shared.sharedParData->alarmTrigger, false, 0, false);
  }

  // Aggregate results: if any worker timed out, report timeout; if any
  // worker returned a non-OK error, surface it; otherwise OK.
  MinionResult finalResult = MinionResult::MINION_OK;
  for(int i = 0; i < N; ++i) {
    if(results[i] == MinionResult::MINION_TIMEOUT) {
      // A timeout in one worker counts as a portfolio timeout only if the
      // alarm actually fired (i.e., not the "stopRequested" stop that also
      // sets alarm flags in the shared data).
      if(!shared.stopRequested.load() && options.timeoutActive) {
        finalResult = MinionResult::MINION_TIMEOUT;
      }
    } else if(results[i] != MinionResult::MINION_OK) {
      finalResult = results[i];
      break;
    }
  }

  Parallel::releaseThreadParallelData(shared.sharedParData);

  // Restore the parent thread's context.
  globals = savedGlobals;

  // For N>1, the workers were silenced and the parent prints a single
  // summary line so test scrapers see the aggregated count. For N=1 the
  // single worker already prints "Solutions Found:" itself; printing
  // again would duplicate.
  if(N > 1 && !options.silent) {
    getOutput() << "Solutions Found: " << shared.totalSolutionsFound.load() << endl;
  }

  // Stash aggregated counters on the parent's TableOut so callers
  // reading -jsontableout (and the CLI flush path in minion.cpp) see
  // total cross-worker stats. Mirrors the work-steal aggregator.
  if(savedGlobals != nullptr) {
    long long total_sols = shared.totalSolutionsFound.load();
    long long total_nodes = shared.totalNodesExplored.load();
    getTableOut().set("Nodes", tostring(total_nodes));
    getTableOut().set("Satisfiable", (total_sols == 0 ? 0 : 1));
    getTableOut().set("SolutionsFound", total_sols);
  }

  // Aggregate cross-worker best objective onto the parent's TableOut so
  // -jsontableout in CLI mode and TableOut_get in FFI mode see the
  // portfolio's combined optimum (not whatever a single worker found).
  // Single-objective only — multi-objective lex skipped silently.
  if(savedGlobals != nullptr && instance.is_optimisation_problem &&
     instance.optimiseVariables.size() == 1) {
    bool haveValue = false;
    DomainInt best = 0;
    for(int i = 0; i < N; ++i) {
      const auto& vals = perWorkerOptValues[i];
      if(vals.size() != 1) continue;
      if(!haveValue) {
        best = vals[0];
        haveValue = true;
      } else if(instance.optimiseMinimising) {
        if(vals[0] < best) best = vals[0];
      } else {
        if(vals[0] > best) best = vals[0];
      }
    }
    if(haveValue) {
      getTableOut().set("OptimumValue", tostring(best));
      getTableOut().set("OptimumDirection",
                        instance.optimiseMinimising ? std::string("min")
                                                    : std::string("max"));
    }
  }

  if(finalResult == MinionResult::MINION_TIMEOUT)
    set_error("solver timed out");
  return finalResult;
}

/*********************************************************************/
/*                  Work-stealing parallel search                    */
/*********************************************************************/

#include "parallel/work_steal.h"

namespace {

// Diversify per-worker heuristics for the work-stealing portfolio mode.
// Mutates `args` (var/val ordering) and `opts` (randomiseValvarorder) in
// place. Worker 0 is left untouched so the user's chosen heuristic always
// runs on at least one worker; workers 1..N-1 cycle through a fixed
// palette of (var-order, val-order, randomise) triples designed to give
// genuinely different traversal shapes.
//
// Path replay in StandardSearchManager::replayPath is heuristic-
// independent: it walks recorded BranchSteps applying assignments via
// worldPush/assign/prop, never consulting the receiver's varOrder. So a
// donor on heuristic A and a receiver on heuristic B can cooperate on
// the same shared tree — donor's prefix is replayed verbatim, receiver
// then continues with its own heuristic on the rest of the subtree.
static void applyPortfolioStrategy(int workerIdx, SearchMethod& args,
                                   SearchOptions& opts) {
  if(workerIdx <= 0) return;  // worker 0 keeps user's choice
  // Palette: ordered roughly by likelihood of being a useful diversifier.
  struct Strategy {
    VarOrderEnum order;
    ValOrderEnum valorder;
    bool randomiseShuffle;
  };
  static const Strategy palette[] = {
      {ORDER_SDF,         VALORDER_ASCEND,  false},
      {ORDER_DOMOVERWDEG, VALORDER_ASCEND,  false},
      {ORDER_WDEG,        VALORDER_ASCEND,  false},
      {ORDER_STATIC,      VALORDER_ASCEND,  false},
      {ORDER_SRF,         VALORDER_DESCEND, false},
      {ORDER_CONFLICT,    VALORDER_ASCEND,  false},
      {ORDER_LDF,         VALORDER_ASCEND,  false},
      {ORDER_SDF,         VALORDER_ASCEND,  true },
      {ORDER_DOMOVERWDEG, VALORDER_DESCEND, true },
      {ORDER_STATIC,      VALORDER_DESCEND, false},
      {ORDER_WDEG,        VALORDER_ASCEND,  true },
      {ORDER_SRF,         VALORDER_ASCEND,  true },
  };
  const int paletteSize = (int)(sizeof(palette) / sizeof(palette[0]));
  const Strategy& s = palette[(workerIdx - 1) % paletteSize];
  args.order = s.order;
  args.valorder = ValOrder(s.valorder);
  if(s.randomiseShuffle) opts.randomiseValvarorder = true;
}

// Wrapper callback used by every work-steal worker. Mirrors
// parallelWrapperCallback (mutex-protected, applies sollimit, signals via
// shared parData) but routes solution counting through the
// WorkStealController's own atomics so accounting stays clean even when
// the same controller is reused across runs.
static bool workStealWrapperCallback(MinionContext* ctx, void* ud)
{
  WorkSteal::WorkStealController* shared =
      static_cast<WorkSteal::WorkStealController*>(ud);
  // Probe how long we waited to acquire the per-solution lock. On
  // enumeration-heavy SAT workloads this is the dominant contention
  // point — every solution from every worker queues here.
  auto t0 = std::chrono::steady_clock::now();
  std::lock_guard<std::mutex> lock(shared->callbackMutex);
  {
    auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
                       std::chrono::steady_clock::now() - t0)
                       .count();
    shared->callbackLockWaitNanos.fetch_add(elapsed, std::memory_order_relaxed);
  }

  if(shared->stopRequested.load(std::memory_order_acquire))
    return false;

  long long count = shared->totalSolutionsFound.fetch_add(1) + 1;
  bool keepGoing = true;
  if(shared->userCallback) {
    keepGoing = shared->userCallback((void*)ctx, shared->userUserdata);
  }

  if(!keepGoing || (shared->sollimit > 0 && count >= shared->sollimit)) {
    shared->stopRequested.store(true, std::memory_order_release);
    // Wake any worker blocked on the work queue so they observe the
    // stop and exit.
    shared->queue_cv.notify_all();
    // Also poke the cross-thread alarm flags so any worker mid-search
    // (not at a callback) terminates via standardTime_ctrlc_checks.
    if(globals && globals->parData_m) {
      globals->parData_m->alarmTrigger.store(true);
      globals->parData_m->ctrlCPressed.store(true);
    }
    return false;
  }
  return true;
}

}  // anonymous namespace

MinionResult runMinionWorkSteal(MinionThreadConfig config, SearchOptions& options,
                                SearchMethod& args, ProbSpec::CSPInstance& instance,
                                bool (*callback)(MinionContext* ctx, void* userdata),
                                void* userdata, MinionWorkStealStats* outStats)
{
  if(outStats != nullptr) {
    outStats->donations = 0;
    outStats->itemsTaken = 0;
    outStats->replayFailures = 0;
    outStats->totalNodes = 0;
    outStats->queueLockWaitNanos = 0;
    outStats->idleWaitNanos = 0;
    outStats->callbackLockWaitNanos = 0;
  }
  if(config.numThreads < 1) {
    set_error("runMinionWorkSteal: numThreads must be >= 1");
    return MinionResult::MINION_INVALID_ARGUMENT;
  }

  Globals* savedGlobals = globals;
  globals = nullptr;

  const int N = config.numThreads;

  WorkSteal::WorkStealController ctrl;
  ctrl.numWorkers = N;
  ctrl.sollimit = options.sollimit;
  ctrl.userCallback = reinterpret_cast<bool(*)(void*, void*)>(callback);
  ctrl.userUserdata = userdata;
  // Pre-mark worker 0 busy BEFORE spawning any threads. Otherwise a
  // fast-starting worker N>0 can reach popOrFinish, observe busy==0
  // and idle empty, and declare global termination before worker 0
  // ever calls bootstrapFirstWorker.
  ctrl.busyWorkers.store(1, std::memory_order_relaxed);

  Parallel::ParallelData* sharedParData = Parallel::setupThreadParallelData();

  if(options.timeoutActive) {
    activateTrigger(&sharedParData->alarmTrigger, options.timeoutActive,
                    options.time_limit, options.time_limit_is_CPUTime);
  }

  // Pre-finalise the shared instance once (same reasoning as
  // runMinionParallel — finaliseModel mutates instance state).
  finaliseModel(instance);

  std::vector<MinionResult> results(N, MinionResult::MINION_OK);
  // See runMinionParallel for rationale: per-worker snapshot taken
  // before each worker frees its context, then aggregated by the
  // parent after join.
  std::vector<std::vector<DomainInt>> perWorkerOptValues(N);
  std::vector<WorkerThread> workers;
  workers.reserve(N);

  for(int i = 0; i < N; ++i) {
    workers.emplace_back([&, i]() {
      try {
        MinionContext* ctx = minion_newContext();
        ctx->parData_m = sharedParData;

        ProbSpec::CSPInstance perThreadInstance = instance;
        SearchOptions perThreadOptions = options;
        SearchMethod perThreadArgs = args;
        perThreadArgs.randomSeed =
            (int)(args.randomSeed ^ (config.baseSeed + (unsigned int)i));
        // Portfolio mode: diversify worker heuristics. Worker 0 keeps
        // the user's chosen ordering; later workers cycle through a
        // palette of var/val orderings + optional randomisation.
        if(options.parallelWorkStealPortfolio && N > 1) {
          applyPortfolioStrategy(i, perThreadArgs, perThreadOptions);
        }
        // Workers find all (the controller wrapper enforces sollimit
        // across threads). Disable other parallel modes to avoid
        // recursion into runMinionParallel/runMinionWorkSteal — the
        // worker's doStandardSearch must take the sequential path while
        // SolveCSP dispatches into the work-steal worker loop.
        perThreadOptions.sollimit = -1;
        perThreadOptions.numParallelThreads = 0;
        perThreadOptions.numWorkStealThreads = 0;
        perThreadOptions.parallel = false;
        // Inner workers must NOT recurse into portfolio setup themselves.
        perThreadOptions.parallelWorkStealPortfolio = false;
        // Optimisation bound channel — see SearchOptions::
        // parallelBoundChannel for the contract.
        perThreadOptions.parallelBoundChannel = &ctrl.sharedOptBound;
        // Hand the controller pointer + this worker's index into the
        // search loop via SearchOptions so SolveCSP dispatches into the
        // work-steal worker loop.
        perThreadOptions.workStealController = &ctrl;
        perThreadOptions.workStealWorkerIdx = i;
        // Silence per-worker chatter under N>1 (parent prints summary).
        if(N > 1) {
          perThreadOptions.silent = true;
        }

        MinionResult r = runMinionImpl(ctx, perThreadOptions, perThreadArgs,
                                       perThreadInstance, workStealWrapperCallback,
                                       &ctrl, /*installAlarms=*/false);
        results[i] = r;

        // Contribute this worker's node count to the parent's
        // aggregate so the parent's TableOut / CLI summary reflects
        // total cross-thread work (matching what the tester reads
        // out of -jsontableout).
        if(ctx->state_m) {
          ctrl.totalNodesExplored.fetch_add(
              (long long)ctx->state_m->getNodeCount(),
              std::memory_order_relaxed);
          if(ctx->state_m->isOptimisationProblem()) {
            perWorkerOptValues[i] = ctx->state_m->getLastOptimumValues();
          }
        }

        ctx->parData_m = nullptr;
        minion_freeContext(ctx);
      } catch(const std::exception&) {
        results[i] = MinionResult::MINION_UNKNOWN_ERROR;
      } catch(...) {
        results[i] = MinionResult::MINION_UNKNOWN_ERROR;
      }
    });
  }

  for(auto& t : workers)
    t.join();

  if(options.timeoutActive) {
    activateTrigger(&sharedParData->alarmTrigger, false, 0, false);
  }

  MinionResult finalResult = MinionResult::MINION_OK;
  for(int i = 0; i < N; ++i) {
    if(results[i] == MinionResult::MINION_TIMEOUT) {
      if(!ctrl.stopRequested.load() && options.timeoutActive) {
        finalResult = MinionResult::MINION_TIMEOUT;
      }
    } else if(results[i] != MinionResult::MINION_OK) {
      finalResult = results[i];
      break;
    }
  }

  Parallel::releaseThreadParallelData(sharedParData);

  globals = savedGlobals;

  // Stash aggregated counters on the parent's TableOut so callers that
  // read -jsontableout (e.g. the random tester) see total cross-worker
  // stats. globals points at the parent's context now, so getTableOut()
  // returns the parent's. tableout writing is the CLI caller's
  // responsibility (minion_main calls getTableOut().print_line() after
  // we return).
  if(savedGlobals != nullptr) {
    long long total_sols = ctrl.totalSolutionsFound.load();
    long long total_nodes = ctrl.totalNodesExplored.load();
    getTableOut().set("Nodes", tostring(total_nodes));
    getTableOut().set("Satisfiable", (total_sols == 0 ? 0 : 1));
    getTableOut().set("SolutionsFound", total_sols);
    // Diagnostics: how much donation/stealing actually happened. The
    // tester reads these to confirm work-stealing is exercised, not
    // just spawned-then-idle.
    getTableOut().set("WorkStealDonations", tostring(ctrl.donationsMade.load()));
    getTableOut().set("WorkStealItemsTaken", tostring(ctrl.workItemsTaken.load()));
    getTableOut().set("WorkStealReplayFailures", tostring(ctrl.replayFailures.load()));
    // Contention diagnostics. Cumulative wall time (ns) summed across
    // all workers for each blocking primitive — see work_steal.h for
    // interpretation. Headline ratio: idleWaitNanos / (numWorkers *
    // wallTimeNanos) is "fraction of available CPU spent idle waiting
    // on donations" — if high, donation supply isn't keeping up.
    getTableOut().set("WorkStealQueueLockWaitNs",
                      tostring(ctrl.queueLockWaitNanos.load()));
    getTableOut().set("WorkStealIdleWaitNs",
                      tostring(ctrl.idleWaitNanos.load()));
    getTableOut().set("WorkStealCallbackLockWaitNs",
                      tostring(ctrl.callbackLockWaitNanos.load()));

    // Cross-worker best objective. Same single-objective restriction
    // and direction handling as runMinionParallel above; see there
    // for rationale.
    if(instance.is_optimisation_problem &&
       instance.optimiseVariables.size() == 1) {
      bool haveValue = false;
      DomainInt best = 0;
      for(int i = 0; i < N; ++i) {
        const auto& vals = perWorkerOptValues[i];
        if(vals.size() != 1) continue;
        if(!haveValue) {
          best = vals[0];
          haveValue = true;
        } else if(instance.optimiseMinimising) {
          if(vals[0] < best) best = vals[0];
        } else {
          if(vals[0] > best) best = vals[0];
        }
      }
      if(haveValue) {
        getTableOut().set("OptimumValue", tostring(best));
        getTableOut().set("OptimumDirection",
                          instance.optimiseMinimising ? std::string("min")
                                                      : std::string("max"));
      }
    }
  }

  if(N > 1 && !options.silent) {
    getOutput() << "Solutions Found: " << ctrl.totalSolutionsFound.load() << endl;
    getOutput() << "WorkSteal donations: " << ctrl.donationsMade.load()
         << ", taken: " << ctrl.workItemsTaken.load()
         << ", replay-failures: " << ctrl.replayFailures.load() << endl;
    // Contention numbers: total wait time across all workers for each
    // blocking primitive. Print in ms for human readability.
    long long qNs = ctrl.queueLockWaitNanos.load();
    long long iNs = ctrl.idleWaitNanos.load();
    long long cNs = ctrl.callbackLockWaitNanos.load();
    getOutput() << "WorkSteal contention (cumulative across "
         << N << " workers, ms): queue-lock="
         << (qNs / 1000000) << ", idle-wait="
         << (iNs / 1000000) << ", callback-lock="
         << (cNs / 1000000) << endl;
  }

  if(outStats != nullptr) {
    outStats->donations = ctrl.donationsMade.load();
    outStats->itemsTaken = ctrl.workItemsTaken.load();
    outStats->replayFailures = ctrl.replayFailures.load();
    outStats->totalNodes = ctrl.totalNodesExplored.load();
    outStats->queueLockWaitNanos = ctrl.queueLockWaitNanos.load();
    outStats->idleWaitNanos = ctrl.idleWaitNanos.load();
    outStats->callbackLockWaitNanos = ctrl.callbackLockWaitNanos.load();
  }

  if(finalResult == MinionResult::MINION_TIMEOUT)
    set_error("solver timed out");
  return finalResult;
}

/*********************************************************************/
/*                    Instance building functions                    */
/*********************************************************************/

void newVar(CSPInstance& instance, string name, VariableType type, vector<int> bounds)
{
  vector<DomainInt> dbounds(bounds.begin(), bounds.end());
  Var v = instance.vars.getNewVar(type, dbounds);
  instance.vars.addSymbol(name, v);
  instance.allVars_list.push_back(makeVec(v));
}

Var constantAsVar(int constant)
{
  return Var(VAR_CONSTANT, (DomainInt)constant);
}

// Export of inline function get_constraint as bindings dont like inlines!
ConstraintDef* lib_getConstraint(ConstraintType t)
{
  return get_constraint(t);
}

/************************************************************/
/*                    Internal Functions                    */
/************************************************************/

void finaliseModel(CSPInstance& instance)
{
  /* Add final touches to model and fill in missing defaults.
   *
   * largely copied from MinionThreeInputReader::finalise, but without
   * gadget stuff.
   */

  // Fill in any missing defaults
  if(instance.searchOrder.empty()) {
    instance.searchOrder.push_back(instance.vars.getAllVars());
  }

  vector<Var> allVars = instance.vars.getAllVars();
  set<Var> unusedVars(allVars.begin(), allVars.end());
  for(SysInt i = 0; i < (SysInt)instance.searchOrder.size(); ++i) {
    const vector<Var>& vars_ref = instance.searchOrder[i].varOrder;
    for(vector<Var>::const_iterator it = vars_ref.begin(); it != vars_ref.end(); ++it) {
      unusedVars.erase(*it);
    }
  }

  for(SysInt i = 0; i < (SysInt)instance.searchOrder.size(); ++i)
    instance.searchOrder[i].setupValueOrder();

  if(instance.symOrder.empty())
    instance.symOrder = instance.vars.getAllVars();

  if(instance.symOrder.size() != instance.vars.getAllVars().size()) {
    // MAYBE_PARSER_INFO("Extending symmetry order with auxillery variables");
    vector<Var> allVars = instance.vars.getAllVars();
    for(typename vector<Var>::iterator i = allVars.begin(); i != allVars.end(); ++i) {
      if(find(instance.symOrder.begin(), instance.symOrder.end(), *i) == instance.symOrder.end())
        instance.symOrder.push_back(*i);
    }
  }

  //
  if(instance.symOrder.size() !=
     set<Var>(instance.symOrder.begin(), instance.symOrder.end()).size())
    throw parse_exception("SYMORDER cannot contain any variable more than once");

  if(instance.symOrder.size() != instance.vars.getAllVars().size())
    throw parse_exception("SYMORDER must contain every variable");
}

/*********************************************************************/
/*                    REXPORTING INLINE FUNCTIONS                    */
/*********************************************************************/

/***** Variable *****/

VarResult minion_getVarByName(CSPInstance& instance, char* name)
{
  try {
    Var v = instance.vars.getSymbol(string(name));
    return {MinionResult::MINION_OK, v};
  } catch(const parse_exception& e) {
    set_error(e.what());
    return {MinionResult::MINION_PARSE_ERROR, Var()};
  } catch(const std::exception& e) {
    set_error(e.what());
    return {MinionResult::MINION_UNKNOWN_ERROR, Var()};
  }
}

MinionResult minion_newVar(CSPInstance& instance, char* name, VariableType type, int bound1, int bound2)
{
  assertNotInSearch("minion_newVar");
  try {
    newVar(instance, string(name), type, std::vector<int>({bound1, bound2}));
    return MinionResult::MINION_OK;
  } catch(const parse_exception& e) {
    set_error(e.what());
    return MinionResult::MINION_PARSE_ERROR;
  } catch(const std::exception& e) {
    set_error(e.what());
    return MinionResult::MINION_UNKNOWN_ERROR;
  }
}

MinionResult minion_newSparseBoundVar(CSPInstance& instance, char* name, std::vector<int>* domain)
{
  assertNotInSearch("minion_newSparseBoundVar");
  try {
    vector<DomainInt> dbounds(domain->begin(), domain->end());
    Var v = instance.vars.getNewVar(VAR_SPARSEBOUND, dbounds);
    instance.vars.addSymbol(string(name), v);
    instance.allVars_list.push_back(makeVec(v));
    return MinionResult::MINION_OK;
  } catch(const parse_exception& e) {
    set_error(e.what());
    return MinionResult::MINION_PARSE_ERROR;
  } catch(const std::exception& e) {
    set_error(e.what());
    return MinionResult::MINION_UNKNOWN_ERROR;
  }
}

/***** Tuple *****/
TupleList* tupleList_new(vector<vector<int>>& tupleList)
{
  vector<vector<DomainInt>> dtuples(tupleList.size());
  for(size_t i = 0; i < tupleList.size(); ++i)
    dtuples[i].assign(tupleList[i].begin(), tupleList[i].end());
  return new TupleList(dtuples);
}

void tupleList_free(TupleList* tupleList)
{
  delete tupleList;
}

ShortTupleList* shortTupleList_new(vector<vector<int>>& flat_short_tuples)
{
  vector<vector<pair<SysInt, DomainInt>>> dshort(flat_short_tuples.size());
  for(size_t i = 0; i < flat_short_tuples.size(); ++i) {
    const vector<int>& flat = flat_short_tuples[i];
    if(flat.size() % 2 != 0)
      throw std::runtime_error("shortTupleList_new: each short tuple must be flat-encoded "
                               "as alternating (idx, val) ints — got an odd-length entry");
    dshort[i].reserve(flat.size() / 2);
    for(size_t k = 0; k < flat.size(); k += 2) {
      dshort[i].emplace_back(static_cast<SysInt>(flat[k]),
                             DomainInt(flat[k + 1]));
    }
  }
  return new ShortTupleList(dshort);
}

void shortTupleList_free(ShortTupleList* shortTupleList)
{
  delete shortTupleList;
}

/***** Instance *****/

CSPInstance* instance_new()
{
  return new CSPInstance();
}

void instance_free(CSPInstance* instance)
{
  assertNotInSearch("instance_free");
  delete instance;
}

void instance_addSearchOrder(CSPInstance& instance, SearchOrder& searchOrder)
{
  assertNotInSearch("instance_addSearchOrder");
  instance.searchOrder.push_back(searchOrder);
}

void instance_addConstraint(CSPInstance& instance, ConstraintBlob& constraint)
{
  assertNotInSearch("instance_addConstraint");
  instance.constraints.push_back(constraint);
  // FFI-built blobs bypass the parser, so apply the parser's collapse of
  // degenerate (empty-table / zero-variable) table constraints here.
  normaliseTableConstraintBlobTree(instance.constraints.back());
}

void instance_setOptimise(CSPInstance& instance, bool minimising, Var& var)
{
  assertNotInSearch("instance_setOptimise");
  std::vector<Var> vars{var};
  instance.set_optimise(minimising, vars);
}

MinionResult minion_addConstraintMidsearch(MinionContext* ctx, CSPInstance& instance, ConstraintBlob& constraint)
{
  try {
    ContextGuard guard(ctx);
    // Refuse to install a new constraint if the solver is already in a
    // failed state — addConstraint's setup/fullPropagate would be
    // skipped (and that path is now an INTERNAL_ERROR), so callers must
    // wait for the solver to backtrack out of the failed state before
    // injecting more constraints.
    if(getState().isFailed()) {
      set_error("minion_addConstraintMidsearch: solver state is already failed; back out via the search loop before adding more constraints");
      return MinionResult::MINION_INVALID_ARGUMENT;
    }

    // Keep a stable copy of the blob alive for the lifetime of `instance`.
    // Some built constraints may retain references to blob-owned argument storage.
    instance.constraints.push_back(constraint);
    normaliseTableConstraintBlobTree(instance.constraints.back());

    // A propagator with backtrackable internal state (haggisgac,
    // haggisgac-stable, gacschema) registers itself with the
    // GenericBacktracker in its constructor. Built mid-search it has
    // missed every worldPush so far, yet worldPop still calls pop() on
    // it once per open level -- so it unwinds off the bottom of its own
    // stack (a heap-buffer-overflow read, caught by ASan). Give each
    // object that registered during construction one mark per open
    // level. Must be before addConstraintMidsearch: fullPropagate
    // pushes records belonging to the current level, which have to sit
    // on top of these markers.
    const int btBefore = getState().getGenericBacktracker().size();
    AbstractConstraint* c = build_constraint(instance.constraints.back());
    getState().getGenericBacktracker().markFrom(btBefore,
                                                (int)Controller::getWorldDepth());
    // addConstraintMidsearch returns false when the new constraint wipes
    // out a domain during its initial fullPropagate. That's a normal
    // search outcome: getState().isFailed() is already set, so when
    // control returns to the search loop it backtracks. The caller
    // shouldn't have to treat this as an API error.
    (void)getState().addConstraintMidsearch(c);
    return MinionResult::MINION_OK;
  } catch(const parse_exception& e) {
    set_error(e.what());
    return MinionResult::MINION_PARSE_ERROR;
  } catch(const std::exception& e) {
    set_error(e.what());
    return MinionResult::MINION_UNKNOWN_ERROR;
  }
}

MinionResult minion_newVarMidsearch(MinionContext* ctx, CSPInstance& instance,
                                    char* name, VariableType type,
                                    int bound1, int bound2)
{
  try {
    ContextGuard guard(ctx);

    // Refuse if the solver is in a failed state — adding a variable
    // mid-search assumes the live SearchManager is in a valid state.
    if(getState().isFailed()) {
      set_error("minion_newVarMidsearch: solver state is already failed; back out via the search loop before adding more variables");
      return MinionResult::MINION_INVALID_ARGUMENT;
    }

    // 1. Add to the spec (symbol table, allVars_list, etc.)
    newVar(instance, string(name), type, std::vector<int>({bound1, bound2}));

    // 2. Also register in the live runtime container so mid-search
    //    constraints can reference this variable without segfaulting.
    // 3. Append the new variable into the live search's aux block so it
    //    actually gets branched on. Without this the solver would report
    //    solutions with the new var unassigned.
    Bounds bounds(bound1, bound2);
    AnyVarRef ref;
    switch(type) {
    case VAR_BOUND: {
      UnsignedSysInt idx = getVars().boundVarContainer.varCount();
      getVars().boundVarContainer.addVariables(bounds, 1);
      ref = AnyVarRef(getVars().boundVarContainer.getVarNum(idx));
      break;
    }
    case VAR_DISCRETE: {
      UnsignedSysInt idx = getVars().bigRangeVarContainer.varCount();
      getVars().bigRangeVarContainer.addVariables({{bound1, bound2}});
      ref = AnyVarRef(getVars().bigRangeVarContainer.getVarNum(idx));
      break;
    }
    case VAR_SPARSEBOUND: {
      UnsignedSysInt idx = getVars().sparseBoundVarContainer.varCount();
      getVars().sparseBoundVarContainer.addVariables({bound1, bound2}, 1);
      ref = AnyVarRef(getVars().sparseBoundVarContainer.getVarNum(idx));
      break;
    }
    case VAR_BOOL: {
      UnsignedSysInt idx = getVars().boolVarContainer.varCount();
      getVars().boolVarContainer.addVariables(1);
      ref = AnyVarRef(getVars().boolVarContainer.getVarNum(idx));
      break;
    }
    default:
      set_error("minion_newVarMidsearch: unsupported variable type");
      return MinionResult::MINION_INVALID_ARGUMENT;
    }

    Controller::SearchManager* sm = getState().getSearchManager();
    if(sm == nullptr) {
      set_error("minion_newVarMidsearch: no live SearchManager (not in search?)");
      return MinionResult::MINION_INVALID_INSTANCE;
    }
    sm->appendAuxVar(ref, ValOrder(VALORDER_ASCEND));

    return MinionResult::MINION_OK;
  } catch(const parse_exception& e) {
    set_error(e.what());
    return MinionResult::MINION_PARSE_ERROR;
  } catch(const std::exception& e) {
    set_error(e.what());
    return MinionResult::MINION_UNKNOWN_ERROR;
  }
}

MinionResult minion_newSparseBoundVarMidsearch(MinionContext* ctx, CSPInstance& instance,
                                               char* name, std::vector<int>* domain)
{
  try {
    ContextGuard guard(ctx);

    if(getState().isFailed()) {
      set_error("minion_newSparseBoundVarMidsearch: solver state is already failed");
      return MinionResult::MINION_INVALID_ARGUMENT;
    }

    vector<DomainInt> dbounds(domain->begin(), domain->end());

    // Add to the spec (symbol table, allVars_list, etc.)
    Var v = instance.vars.getNewVar(VAR_SPARSEBOUND, dbounds);
    instance.vars.addSymbol(string(name), v);
    instance.allVars_list.push_back(makeVec(v));

    // Register in the live variable container.
    UnsignedSysInt idx = getVars().sparseBoundVarContainer.varCount();
    getVars().sparseBoundVarContainer.addVariables(dbounds, 1);
    AnyVarRef ref = AnyVarRef(getVars().sparseBoundVarContainer.getVarNum(idx));

    Controller::SearchManager* sm = getState().getSearchManager();
    if(sm == nullptr) {
      set_error("minion_newSparseBoundVarMidsearch: no live SearchManager (not in search?)");
      return MinionResult::MINION_INVALID_INSTANCE;
    }
    sm->appendAuxVar(ref, ValOrder(VALORDER_ASCEND));

    return MinionResult::MINION_OK;
  } catch(const parse_exception& e) {
    set_error(e.what());
    return MinionResult::MINION_PARSE_ERROR;
  } catch(const std::exception& e) {
    set_error(e.what());
    return MinionResult::MINION_UNKNOWN_ERROR;
  }
}

void instance_addTupleTableSymbol(CSPInstance& instance, char* name, TupleList* tuplelist)
{
  assertNotInSearch("instance_addTupleTableSymbol");
  instance.addTableSymbol(name, std::shared_ptr<TupleList>(tuplelist));
}

TupleList* instance_getTupleTableSymbol(CSPInstance& instance, char* name)
{
  return instance.getTableSymbol(name).get();
}

void instance_addShortTupleTableSymbol(CSPInstance& instance, char* name,
                                       ShortTupleList* shorttuplelist)
{
  assertNotInSearch("instance_addShortTupleTableSymbol");
  instance.addShortTableSymbol(name, std::shared_ptr<ShortTupleList>(shorttuplelist));
}

ShortTupleList* instance_getShortTupleTableSymbol(CSPInstance& instance, char* name)
{
  return instance.getShortTableSymbol(name).get();
}

void printMatrix_addVar(CSPInstance& instance, Var var)
{
  assertNotInSearch("printMatrix_addVar");
  instance.print_matrix.push_back({var});
}

int printMatrix_getValue(MinionContext* ctx, int idx)
{
  ContextGuard guard(ctx);
  return checked_cast<int>(globals->state_m->getPrintMatrix()[idx][0].assignedValue());
}

int printMatrix_getValueByName(MinionContext* ctx, CSPInstance& instance, const char* varname)
{
  ContextGuard guard(ctx);
  Var target = instance.vars.getSymbol(string(varname));
  for(SysInt i = 0; i < (SysInt)instance.print_matrix.size(); ++i) {
    if(instance.print_matrix[i].size() == 1 && instance.print_matrix[i][0] == target) {
      return checked_cast<int>(globals->state_m->getPrintMatrix()[i][0].assignedValue());
    }
  }
  throw parse_exception("Variable '" + string(varname) + "' not found in print matrix");
}

int minion_getVarValue(MinionContext* ctx, CSPInstance& instance, const char* varname)
{
  ContextGuard guard(ctx);
  AnyVarRef ref = getAnyVarRefFromString(instance, string(varname));
  return checked_cast<int>(ref.assignedValue());
}

/***** SearchOptions *****/

SearchOptions* searchOptions_new()
{
  return new SearchOptions();
}

void searchOptions_free(SearchOptions* searchOptions)
{
  delete searchOptions;
}

/***** SearchMethod *****/

SearchMethod* searchMethod_new()
{
  return new SearchMethod();
}

void searchMethod_free(SearchMethod* searchMethod)
{
  delete searchMethod;
}

/***** SearchOrder *****/

SearchOrder* searchOrder_new(std::vector<Var>& vars, VarOrderEnum orderEnum, bool findOneSol)
{
  return new SearchOrder(vars, orderEnum, findOneSol);
}

void searchOrder_free(SearchOrder* searchOrder)
{
  delete searchOrder;
}

void searchOrder_setValOrder(SearchOrder& searchOrder, ValOrderEnum valOrder)
{
  // Replace any existing per-variable value ordering with a uniform
  // vector sized to match varOrder. BuildCSP's setupValueOrder is a
  // no-op once valOrder is the right length, so this overwrite is
  // stable whether called before or after vars are added.
  searchOrder.valOrder.assign(searchOrder.varOrder.size(), ValOrder(valOrder));
}

/***** ConstraintBlob *****/

ConstraintBlob* constraint_new(ConstraintType constraint_type)
{
  return new ConstraintBlob(lib_getConstraint(constraint_type));
}

void constraint_free(ConstraintBlob* constraint)
{
  delete constraint;
}

// mirrors MinionThreeInputReader::readGeneralConstraint, but over FFI.
// look there for the why/how

void constraint_addList(ConstraintBlob& constraint, std::vector<Var>& vars)
{
  constraint.vars.push_back(vars);
}

void constraint_addVar(ConstraintBlob& constraint, Var& var)
{
  constraint.vars.push_back(makeVec(var));
}

void constraint_addTwoVars(ConstraintBlob& constraint, Var& var1, Var& var2)
{
  vector<Var> vars(2);
  vars[0] = std::move(var1);
  vars[1] = std::move(var2);
  constraint.vars.push_back(std::move(vars));
}

void constraint_addConstant(ConstraintBlob& constraint, int constant)
{
  constraint.constants.push_back(makeVec((DomainInt)constant));
}

void constraint_addConstantList(ConstraintBlob& constraint, std::vector<int>& constants)
{
  constraint.constants.push_back(vector<DomainInt>(constants.begin(), constants.end()));
}

void constraint_addConstraint(ConstraintBlob& constraint, ConstraintBlob& internal_constraint)
{
  constraint.internal_constraints.push_back(internal_constraint);
}

void constraint_addConstraintList(ConstraintBlob& constraint,
                                  vector<ConstraintBlob>& internal_constraints)
{
  constraint.internal_constraints = std::move(internal_constraints);
}

void constraint_setTuples(ConstraintBlob& constraint, TupleList* tupleList)
{
  constraint.tuples = std::shared_ptr<TupleList>(tupleList);
}

void constraint_setTuplesByName(ConstraintBlob& constraint, CSPInstance& instance, const char* name)
{
  // Share the existing shared_ptr<TupleList> from the instance's
  // symbol table rather than wrapping a raw pointer — otherwise two
  // independent shared_ptrs would end up managing the same TupleList
  // and double-free on destruction.
  constraint.tuples = instance.getTableSymbol(std::string(name));
}

void constraint_setShortTuples(ConstraintBlob& constraint, ShortTupleList* shortTupleList)
{
  constraint.shortTuples = std::shared_ptr<ShortTupleList>(shortTupleList);
}

void constraint_setShortTuplesByName(ConstraintBlob& constraint, CSPInstance& instance,
                                     const char* name)
{
  constraint.shortTuples = instance.getShortTableSymbol(std::string(name));
}

/***** Vector Rexports *****/

std::vector<Var>* vec_var_new()
{
  return new std::vector<Var>();
}

void vec_var_push_back(std::vector<Var>* vec, Var var)
{
  vec->push_back(var);
}

void vec_var_free(std::vector<Var>* vec)
{
  delete vec;
}

std::vector<int>* vec_int_new()
{
  return new std::vector<int>();
}

void vec_int_push_back(std::vector<int>* vec, int n)
{
  vec->push_back(n);
}

void vec_int_free(std::vector<int>* vec)
{
  delete vec;
}

std::vector<ConstraintBlob>* vec_constraints_new()
{
  return new std::vector<ConstraintBlob>();
}

void vec_constraints_push_back(std::vector<ConstraintBlob>* vec, ConstraintBlob& constraint)
{
  // TODO: how to memory manage this?
  // move?
  vec->push_back(std::move(constraint));
}

void vec_constraints_free(std::vector<ConstraintBlob>* vec)
{
  delete vec;
}

std::vector<std::vector<int>>* vec_vec_int_new()
{
  return new std::vector<std::vector<int>>();
}
void vec_vec_int_push_back(std::vector<std::vector<int>>* vec,
                           std::vector<int> new_elem)
{
  vec->push_back(new_elem);
}

void vec_vec_int_push_back_ptr(std::vector<std::vector<int>>* vec,
                               std::vector<int>* new_elem)
{
  vec->push_back(*new_elem);
}

void vec_vec_int_free(std::vector<std::vector<int>>* vec)
{
  delete vec;
}

char* TableOut_get(MinionContext* ctx, char* key) {
  ContextGuard guard(ctx);
  try {
    /*
     * .data() doesn't copy, it just returns a ptr to the internal
     * representation of the string . As we are interfacing with C and using 
     * char*, I will just malloc strcpy here (even though it might not be 
     * idiomatic C++?)
     *
     * It needs this many temporary variables due to memory shenanigans!
     */

    string val_str = getTableOut().get(key);
    const char* val = val_str.data();

    char* heaped_val = (char*) std::malloc(strlen(val) +1);
    strcpy(heaped_val,val);
    return heaped_val;

  } catch(const std::out_of_range&) {
    return NULL;
  }
}

#endif

// vim: cc=80 tw=80
