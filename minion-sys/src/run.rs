#![allow(unreachable_patterns)]
#![allow(unsafe_op_in_unsafe_fn)]

use std::{
    collections::HashMap,
    ffi::{CStr, CString, c_char, c_void},
};

use anyhow::anyhow;

use crate::{
    ast::Tuple,
    ffi::{self},
};
use crate::{
    ast::{Constant, Constraint, Model, Var, VarDomain, VarName},
    error::{MinionError, check_minion_result},
    scoped_ptr::Scoped,
};

/// The callback type used by [`run_minion`].
///
/// Called by Minion whenever a solution is found. The input is
/// a `HashMap` of all named variables along with their value.
///
/// Return `true` to continue searching, `false` to stop.
///
/// Since this is a boxed closure, it can capture state from its environment,
/// eliminating the need for global or thread-local state in callers.
///
/// # Examples
///
/// ```
///   use minion_sys::ast::*;
///   use minion_sys::run_minion;
///   use std::collections::HashMap;
///
///   let mut all_solutions: Vec<HashMap<VarName,Constant>> = vec![];
///
///   let callback = Box::new(|solutions: HashMap<VarName,Constant>| -> bool {
///       all_solutions.push(solutions);
///       true
///   });
///
///   // Build and run the model.
///   let mut model = Model::new();
///
///   // ... omitted for brevity ...
/// # model
/// #     .named_variables
/// #     .add_var("x".to_owned(), VarDomain::Bound(1, 3));
/// # model
/// #     .named_variables
/// #     .add_var("y".to_owned(), VarDomain::Bound(2, 4));
/// # model
/// #     .named_variables
/// #     .add_var("z".to_owned(), VarDomain::Bound(1, 5));
/// #
/// # let leq = Constraint::SumLeq(
/// #     vec![
/// #         Var::NameRef("x".to_owned()),
/// #         Var::NameRef("y".to_owned()),
/// #         Var::NameRef("z".to_owned()),
/// #     ],
/// #     Var::ConstantAsVar(4),
/// # );
/// #
/// # let geq = Constraint::SumGeq(
/// #     vec![
/// #         Var::NameRef("x".to_owned()),
/// #         Var::NameRef("y".to_owned()),
/// #         Var::NameRef("z".to_owned()),
/// #     ],
/// #     Var::ConstantAsVar(4),
/// # );
/// #
/// # let ineq = Constraint::Ineq(
/// #     Var::NameRef("x".to_owned()),
/// #     Var::NameRef("y".to_owned()),
/// #     Constant::Integer(-1),
/// # );
/// #
/// # model.constraints.push(leq);
/// # model.constraints.push(geq);
/// # model.constraints.push(ineq);
///
///   let _solver_ctx = run_minion(model, callback).expect("Error occurred");
///
///   let solution_set_1 = &all_solutions[0];
///   let x1 = solution_set_1.get("x").unwrap();
///   let y1 = solution_set_1.get("y").unwrap();
///   let z1 = solution_set_1.get("z").unwrap();
/// #
/// # assert_eq!(all_solutions.len(),1);
/// # assert_eq!(*x1,Constant::Integer(1));
/// # assert_eq!(*y1,Constant::Integer(2));
/// # assert_eq!(*z1,Constant::Integer(1));
/// ```
pub type Callback<'a> = Box<dyn FnMut(HashMap<VarName, Constant>) -> bool + 'a>;

/// Richer callback that also receives a [`MidSearchContext`] handle so
/// the caller can add variables or constraints from inside a solution
/// callback. See [`run_minion_midsearch`].
pub type MidSearchCallback<'a> =
    Box<dyn FnMut(&mut MidSearchContext<'_>, HashMap<VarName, Constant>) -> bool + 'a>;

/// Handle for mutating the current search from inside a solution callback.
///
/// Callers receive a `&mut MidSearchContext` for the duration of a single
/// callback invocation and can use it to add fresh variables or new
/// constraints via the underlying `minion_newVarMidsearch` and
/// `minion_addConstraintMidsearch` FFI calls. Any variables added this
/// way are tracked so subsequent callbacks' solution maps include them.
///
/// The handle is not `Send` or `Sync` — it is only valid for the current
/// callback invocation on the current thread.
pub struct MidSearchContext<'a> {
    ctx: *mut ffi::MinionContext,
    instance: *mut ffi::ProbSpec_CSPInstance,
    midsearch_vars: &'a mut Vec<VarName>,
    _not_send: std::marker::PhantomData<*mut ()>,
}

impl MidSearchContext<'_> {
    /// Add a fresh variable to the running search.
    ///
    /// The variable is branched on as an aux variable at the end of the
    /// search order, so subsequent solutions will enumerate its values.
    /// It is tracked so its value appears in the solution map of every
    /// later callback.
    pub fn add_var(&mut self, name: &str, domain: VarDomain) -> Result<(), MinionError> {
        let c_name = CString::new(name)
            .map_err(|_| anyhow!("Variable name {:?} contains a null character.", name))?;
        unsafe {
            match domain {
                VarDomain::Bool => {
                    check_minion_result(ffi::minion_newVarMidsearch(
                        self.ctx,
                        self.instance,
                        c_name.as_ptr() as *mut c_char,
                        ffi::VariableType_VAR_BOOL,
                        0,
                        1,
                    ))?;
                }
                VarDomain::Bound(a, b) => {
                    check_minion_result(ffi::minion_newVarMidsearch(
                        self.ctx,
                        self.instance,
                        c_name.as_ptr() as *mut c_char,
                        ffi::VariableType_VAR_BOUND,
                        a,
                        b,
                    ))?;
                }
                VarDomain::Discrete(a, b) => {
                    check_minion_result(ffi::minion_newVarMidsearch(
                        self.ctx,
                        self.instance,
                        c_name.as_ptr() as *mut c_char,
                        ffi::VariableType_VAR_DISCRETE,
                        a,
                        b,
                    ))?;
                }
                VarDomain::SparseBound(ref vals) => {
                    let raw = Scoped::new(ffi::vec_int_new(), |x| ffi::vec_int_free(x as _));
                    for v in vals {
                        ffi::vec_int_push_back(raw.ptr, *v);
                    }
                    check_minion_result(ffi::minion_newSparseBoundVarMidsearch(
                        self.ctx,
                        self.instance,
                        c_name.as_ptr() as *mut c_char,
                        raw.ptr,
                    ))?;
                }
                x => return Err(MinionError::NotImplemented(format!("{x:?}"))),
            }
        }
        self.midsearch_vars.push(name.to_owned());
        Ok(())
    }

    /// Add a new constraint to the running search.
    ///
    /// The constraint is propagated immediately; an immediate
    /// propagation wipeout is reported as a [`MinionError`].
    pub fn add_constraint(&mut self, constraint: Constraint) -> Result<(), MinionError> {
        unsafe {
            let ct = get_constraint_type(&constraint)?;
            let raw = Scoped::new(ffi::constraint_new(ct), |x| ffi::constraint_free(x as _));
            constraint_add_args(self.instance, raw.ptr, &constraint)?;
            check_minion_result(ffi::minion_addConstraintMidsearch(
                self.ctx,
                self.instance,
                raw.ptr,
            ))?;
        }
        Ok(())
    }
}

/// State passed through the C callback's `void* userdata` pointer.
///
/// This replaces the old thread-local approach — all callback state is now
/// passed explicitly through the FFI userdata mechanism.
struct CallbackState<'a> {
    callback: MidSearchCallback<'a>,
    instance: *mut ffi::ProbSpec_CSPInstance,
    /// Variables that exist in the print matrix (set up before `runMinion`).
    /// Read via `printMatrix_getValue` by index.
    print_vars: Vec<VarName>,
    /// Variables added mid-search via [`MidSearchContext::add_var`]. These
    /// are not in the print matrix, so we read them via
    /// `minion_getVarValue` by name.
    midsearch_vars: Vec<VarName>,
}

/// Opaque handle to a Minion solver context.
///
/// Holds solver state (including run statistics) after a solve completes.
/// Query results (e.g. via [`SolverContext::get_from_table`]) before dropping.
pub struct SolverContext {
    ctx: *mut ffi::MinionContext,
}

// Safety: MinionContext is an independent solver instance. It is safe to send
// between threads as long as it is not used concurrently (which we ensure by
// only accessing it through &mut self or after the solve completes).
unsafe impl Send for SolverContext {}

impl SolverContext {
    /// Gets a value from Minion's TableOut (where it stores run statistics).
    pub fn get_from_table(&self, key: String) -> Option<String> {
        unsafe {
            #[allow(clippy::expect_used)]
            let c_string = CString::new(key).expect("");
            let key_ptr = c_string.into_raw();
            let val_ptr: *mut c_char = ffi::TableOut_get(self.ctx, key_ptr);

            drop(CString::from_raw(key_ptr));

            if val_ptr.is_null() {
                None
            } else {
                #[allow(clippy::unwrap_used)]
                let res = CStr::from_ptr(val_ptr).to_str().unwrap().to_owned();
                libc::free(val_ptr as _);
                Some(res)
            }
        }
    }
}

impl Drop for SolverContext {
    fn drop(&mut self) {
        unsafe {
            ffi::minion_freeContext(self.ctx);
        }
    }
}

/// The C callback passed to `runMinion`. Receives the active context and our
/// `CallbackState` via the userdata pointer.
unsafe extern "C" fn run_callback(ctx: *mut ffi::MinionContext, userdata: *mut c_void) -> bool {
    // Safety: userdata is a pointer to our CallbackState, set by run_minion
    // and valid for the duration of the runMinion call.
    let state = unsafe { &mut *(userdata as *mut CallbackState<'_>) };

    // Build solutions HashMap by reading variable values from Minion.
    let mut solutions: HashMap<VarName, Constant> = HashMap::new();

    // Print-matrix variables: read by index — established before runMinion.
    for (i, var) in state.print_vars.iter().enumerate() {
        let v: i32 = unsafe { ffi::printMatrix_getValue(ctx, i as _) };
        solutions.insert(var.clone(), Constant::Integer(v));
    }

    // Mid-search variables: not in the print matrix, read by name.
    for var in state.midsearch_vars.iter() {
        #[allow(clippy::unwrap_used)]
        let c_name = CString::new(var.clone()).unwrap();
        let v: i32 = unsafe { ffi::minion_getVarValue(ctx, state.instance, c_name.as_ptr()) };
        solutions.insert(var.clone(), Constant::Integer(v));
    }

    let mut midctx = MidSearchContext {
        ctx,
        instance: state.instance,
        midsearch_vars: &mut state.midsearch_vars,
        _not_send: std::marker::PhantomData,
    };
    (state.callback)(&mut midctx, solutions)
}

/// Propagation strength, matching minion's `PropagationType`.
///
/// Applied either once at preprocess time (via
/// [`RunOptions::preprocess`]) or at every search node (via
/// [`RunOptions::prop_node`]). Strengthens from `None` (no
/// propagation — only valid for preprocess, not prop_node) through
/// `GAC` (generalised arc consistency, the default per-node level)
/// up to `SSAC` (singleton-SAC).
#[derive(Default, Clone, Copy, Debug, PartialEq, Eq)]
pub enum PropLevel {
    #[default]
    None,
    GAC,
    SACBounds,
    SAC,
    SSACBounds,
    SSAC,
}

impl PropLevel {
    fn to_ffi(self) -> ffi::PropagationType {
        match self {
            PropLevel::None => ffi::PropagationType_PropLevel_None,
            PropLevel::GAC => ffi::PropagationType_PropLevel_GAC,
            PropLevel::SACBounds => ffi::PropagationType_PropLevel_SACBounds,
            PropLevel::SAC => ffi::PropagationType_PropLevel_SAC,
            PropLevel::SSACBounds => ffi::PropagationType_PropLevel_SSACBounds,
            PropLevel::SSAC => ffi::PropagationType_PropLevel_SSAC,
        }
    }
}

/// A propagation level together with minion's `_limit` modifier.
///
/// `limit=true` maps to the `*_limit` spellings in exec mode
/// (`SAC_limit`, `SSACBounds_limit`, etc.), which cap the amount of
/// work the propagator will do before giving up and falling back to
/// the next-weaker level.
#[derive(Default, Clone, Copy, Debug, PartialEq, Eq)]
pub struct Propagation {
    pub level: PropLevel,
    pub limit: bool,
}

impl Propagation {
    fn to_ffi(self) -> ffi::PropagationLevel {
        ffi::PropagationLevel {
            type_: self.level.to_ffi(),
            limit: self.limit,
        }
    }

    pub fn is_default_preprocess(self) -> bool {
        self.level == PropLevel::None && !self.limit
    }

    pub fn is_default_prop_node(self) -> bool {
        self.level == PropLevel::GAC && !self.limit
    }
}

/// Variable-ordering heuristic (maps to Minion's `VarOrderEnum`).
///
/// `Static` follows the variable declaration order and is state-free;
/// the rest (SDF, SRF, LDF, WDeg, DOMOverWDeg, Conflict) depend on the
/// current domain sizes / weight counters at each decision, so their
/// solution order is sensitive to mid-search mutations. `Original`
/// is a (non-state-dependent) alias minion uses internally.
#[derive(Default, Clone, Copy, Debug, PartialEq, Eq)]
pub enum VarOrder {
    #[default]
    Static,
    SDF,
    SRF,
    LDF,
    Original,
    WDeg,
    DOMOverWDeg,
    Conflict,
}

/// Value-ordering heuristic (maps to Minion's `ValOrderEnum`).
///
/// `Random` consumes the solver RNG, so two runs with the same seed
/// reach the same leaves only if they made the same number of prior
/// random draws — i.e. mid-search injection *will* diverge the
/// solution order, even under `VarOrder::Static`.
#[derive(Default, Clone, Copy, Debug, PartialEq, Eq)]
pub enum ValOrder {
    #[default]
    Ascend,
    Descend,
    Random,
}

/// Knobs for a single solve. Extend as needed — using a struct keeps the
/// public call surface stable when we add more options.
#[derive(Clone, Copy, Debug)]
pub struct RunOptions {
    /// Seed for Minion's random heuristics. `None` keeps Minion's own
    /// default (currently `std::random_device{}()`, i.e. non-deterministic).
    ///
    /// Pass `Some(s)` to make a solve reproducible — tests that want to
    /// diff two runs (e.g. "same prefix before the injection point")
    /// must set the same seed for both runs.
    pub seed: Option<u32>,

    /// Variable-ordering heuristic. Defaults to [`VarOrder::Static`].
    pub var_order: VarOrder,

    /// Value-ordering heuristic. Defaults to [`ValOrder::Ascend`].
    pub val_order: ValOrder,

    /// One-shot propagation level applied before search begins.
    /// Default is `None + no limit` — no preprocessing beyond each
    /// constraint's own `fullPropagate`.
    pub preprocess: Propagation,

    /// Propagation level applied at every search node.
    /// Default is `GAC + no limit` — each constraint runs its own
    /// propagator. Stronger levels (SAC, SSAC) add global
    /// propagation on top at every node; weaker levels (None) are
    /// not meaningful here (a solver has to propagate *something*).
    pub prop_node: Propagation,
}

impl Default for RunOptions {
    fn default() -> Self {
        Self {
            seed: None,
            var_order: VarOrder::default(),
            val_order: ValOrder::default(),
            preprocess: Propagation {
                level: PropLevel::None,
                limit: false,
            },
            prop_node: Propagation {
                level: PropLevel::GAC,
                limit: false,
            },
        }
    }
}

impl VarOrder {
    fn to_ffi(self) -> ffi::VarOrderEnum {
        match self {
            VarOrder::Static => ffi::VarOrderEnum_ORDER_STATIC,
            VarOrder::SDF => ffi::VarOrderEnum_ORDER_SDF,
            VarOrder::SRF => ffi::VarOrderEnum_ORDER_SRF,
            VarOrder::LDF => ffi::VarOrderEnum_ORDER_LDF,
            VarOrder::Original => ffi::VarOrderEnum_ORDER_ORIGINAL,
            VarOrder::WDeg => ffi::VarOrderEnum_ORDER_WDEG,
            VarOrder::DOMOverWDeg => ffi::VarOrderEnum_ORDER_DOMOVERWDEG,
            VarOrder::Conflict => ffi::VarOrderEnum_ORDER_CONFLICT,
        }
    }
}

impl ValOrder {
    fn to_ffi(self) -> ffi::ValOrderEnum {
        match self {
            ValOrder::Ascend => ffi::ValOrderEnum_VALORDER_ASCEND,
            ValOrder::Descend => ffi::ValOrderEnum_VALORDER_DESCEND,
            ValOrder::Random => ffi::ValOrderEnum_VALORDER_RANDOM,
        }
    }
}

/// Run Minion on the given [Model].
///
/// The given [callback](Callback) is ran whenever a new solution set is found.
///
/// Returns a [`SolverContext`] on success, which can be used to query run
/// statistics via [`SolverContext::get_from_table`].
///
/// For callbacks that need to add variables or constraints during search,
/// use [`run_minion_midsearch`].
pub fn run_minion(model: Model, callback: Callback<'_>) -> Result<SolverContext, MinionError> {
    run_minion_with_options(model, RunOptions::default(), callback)
}

/// Like [`run_minion`] but lets the caller pin solver-side knobs
/// (e.g. the random seed) via [`RunOptions`].
pub fn run_minion_with_options(
    model: Model,
    options: RunOptions,
    mut callback: Callback<'_>,
) -> Result<SolverContext, MinionError> {
    run_minion_midsearch_with_options(model, options, Box::new(move |_ctx, sol| callback(sol)))
}

/// Run Minion on the given [Model] with a callback that can mutate the
/// running search via a [`MidSearchContext`] handle.
pub fn run_minion_midsearch(
    model: Model,
    callback: MidSearchCallback<'_>,
) -> Result<SolverContext, MinionError> {
    run_minion_midsearch_with_options(model, RunOptions::default(), callback)
}

/// Callback type for parallel portfolio search.
///
/// The closure must be `Send + Sync` because it may be invoked from any of
/// the worker threads. The C-side controller serialises invocations with a
/// mutex (so two workers never call this concurrently) but Rust still needs
/// the bound for soundness when we cross the FFI boundary from multiple
/// threads.
pub type ParallelCallback<'a> = Box<dyn FnMut(HashMap<VarName, Constant>) -> bool + Send + 'a>;

/// State stored behind the parallel callback's userdata pointer. The mutex
/// gives Rust ownership semantics on the FnMut even though the C-side
/// already serialises calls — we need the &mut for the closure invocation.
struct ParallelCallbackState<'a> {
    callback: std::sync::Mutex<ParallelCallback<'a>>,
    // Variables that are in the print matrix of every worker context. The
    // print-matrix order is identical across workers because each worker
    // builds its CSPInstance from the same shared description.
    print_vars: Vec<VarName>,
}

unsafe extern "C" fn parallel_callback_thunk(
    ctx: *mut ffi::MinionContext,
    userdata: *mut c_void,
) -> bool {
    let state = unsafe { &*(userdata as *mut ParallelCallbackState<'_>) };

    let mut solutions: HashMap<VarName, Constant> = HashMap::new();
    for (i, var) in state.print_vars.iter().enumerate() {
        let v: i32 = unsafe { ffi::printMatrix_getValue(ctx, i as _) };
        solutions.insert(var.clone(), Constant::Integer(v));
    }

    // The C-side controller has already locked its callback mutex; this
    // Rust mutex is therefore uncontended in practice. We still take it
    // because Rust's borrow checker doesn't see the C-side serialisation.
    #[allow(clippy::unwrap_used)]
    let mut cb = state.callback.lock().unwrap();
    cb(solutions)
}

/// Run Minion as a portfolio search across `num_threads` worker threads.
///
/// Each thread builds its own solver from a shared `Model`, with a derived
/// random seed; the first thread to find `sollimit` solutions (or to prove
/// unsat) signals the others to stop. The callback is invoked at most once
/// per solution found by any worker, serialised internally so it never
/// re-enters itself.
///
/// `num_threads` must be >= 1. With `num_threads == 1` behaviour is
/// equivalent to a sequential `run_minion`.
///
/// Mid-search mutation is NOT supported in this mode (each worker has its
/// own context; mutating one wouldn't propagate). Use the sequential
/// [`run_minion_midsearch`] path if you need that.
///
/// # Concurrency
///
/// `run_minion_parallel` itself spawns OS threads internally. It is safe
/// to call from a single thread of the host process per call, but it is
/// **not** safe to invoke from multiple host threads concurrently — the
/// underlying Minion alarm/ctrl-C handlers are per-process and racing
/// threaded runs can corrupt them. (The fork-based `-parallel` flag has
/// the same constraint at the process level.) For sequential use across
/// many models, just call this function repeatedly.
pub fn run_minion_parallel(
    num_threads: usize,
    model: Model,
    callback: ParallelCallback<'_>,
) -> Result<(), MinionError> {
    run_minion_parallel_with_options(num_threads, model, RunOptions::default(), callback)
}

/// Run Minion with thread-based work-stealing: N workers cooperatively
/// split the search tree. Worker 0 starts at the root; idle workers wait
/// on a shared queue. Busy workers, on each search node, donate one
/// stealable left-branch (encoded as a path-from-root) to the queue when
/// other workers are idle. Idle workers fast-forward via worldPush +
/// propagate replay and continue search from there.
///
/// Unlike [`run_minion_parallel`] (pure portfolio), this *divides* the
/// search tree across workers and so speeds up UNSAT proving roughly with
/// 1/N. Wdeg counters drift between workers — intended as a diversity
/// source.
///
/// Mid-search mutation is not supported. See [`run_minion_parallel`] for
/// the concurrency caveat (don't call this from multiple host threads
/// concurrently — the underlying alarm/ctrl-C handler state is
/// process-global).
pub fn run_minion_work_steal(
    num_threads: usize,
    model: Model,
    callback: ParallelCallback<'_>,
) -> Result<WorkStealStats, MinionError> {
    run_minion_work_steal_with_options(num_threads, model, RunOptions::default(), callback)
}

/// Aggregate work-stealing diagnostics, populated by
/// [`run_minion_work_steal`] and [`run_minion_work_steal_with_options`].
///
/// `donations` counts donate() calls that fired (a busy worker handed off
/// a sub-tree). `items_taken` counts work items popped and replayed by
/// idle workers. `replay_failures` counts replays that found
/// infeasibility before beginning sub-tree search. `total_nodes` is the
/// sum of per-worker node counts.
///
/// `donations == 0` after a run means the donation/replay path was never
/// exercised — typically because the search finished before idle workers
/// reached their wait. Useful for confirming a test instance is large
/// enough to stress the work-stealing protocol.
#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub struct WorkStealStats {
    pub donations: i64,
    pub items_taken: i64,
    pub replay_failures: i64,
    pub total_nodes: i64,
}

/// Like [`run_minion_work_steal`] but with [`RunOptions`].
#[allow(clippy::unwrap_used)]
pub fn run_minion_work_steal_with_options(
    num_threads: usize,
    model: Model,
    options: RunOptions,
    callback: ParallelCallback<'_>,
) -> Result<WorkStealStats, MinionError> {
    if num_threads == 0 {
        return Err(MinionError::Other(anyhow!(
            "num_threads must be at least 1"
        )));
    }
    if num_threads > i32::MAX as usize {
        return Err(MinionError::Other(anyhow!("num_threads is too large")));
    }

    unsafe {
        let search_opts = ffi::searchOptions_new();
        let search_method = ffi::searchMethod_new();
        let search_instance = ffi::instance_new();

        (*search_opts).silent = true;
        (*search_opts).print_solution = false;
        (*search_opts).sollimit = -1;

        if let Some(seed) = options.seed {
            (*search_method).randomSeed = seed;
        }
        (*search_method).preprocess = options.preprocess.to_ffi();
        (*search_method).propMethod = options.prop_node.to_ffi();

        let mut print_vars: Vec<VarName> = vec![];
        let convert_result =
            convert_model_to_raw(search_instance, &model, &options, &mut print_vars);
        if let Err(e) = convert_result {
            ffi::searchMethod_free(search_method);
            ffi::searchOptions_free(search_opts);
            ffi::instance_free(search_instance);
            return Err(e);
        }

        let state = ParallelCallbackState {
            callback: std::sync::Mutex::new(callback),
            print_vars,
        };
        let userdata = &state as *const ParallelCallbackState<'_> as *mut c_void;

        let cfg = ffi::MinionThreadConfig {
            numThreads: num_threads as i32,
            baseSeed: options.seed.unwrap_or(0),
        };

        let mut raw_stats = ffi::MinionWorkStealStats {
            donations: 0,
            itemsTaken: 0,
            replayFailures: 0,
            totalNodes: 0,
        };

        let res = ffi::runMinionWorkSteal(
            cfg,
            search_opts,
            search_method,
            search_instance,
            Some(parallel_callback_thunk),
            userdata,
            &mut raw_stats,
        );

        ffi::searchMethod_free(search_method);
        ffi::searchOptions_free(search_opts);
        ffi::instance_free(search_instance);

        check_minion_result(res)?;
        Ok(WorkStealStats {
            donations: raw_stats.donations,
            items_taken: raw_stats.itemsTaken,
            replay_failures: raw_stats.replayFailures,
            total_nodes: raw_stats.totalNodes,
        })
    }
}

/// Like [`run_minion_parallel`] but with [`RunOptions`] (random seed, var/
/// val orderings, propagation levels). The seed (if set) is the controller
/// base seed; per-thread seeds are derived as `base XOR thread_index`.
#[allow(clippy::unwrap_used)]
pub fn run_minion_parallel_with_options(
    num_threads: usize,
    model: Model,
    options: RunOptions,
    callback: ParallelCallback<'_>,
) -> Result<(), MinionError> {
    if num_threads == 0 {
        return Err(MinionError::Other(anyhow!(
            "num_threads must be at least 1"
        )));
    }
    if num_threads > i32::MAX as usize {
        return Err(MinionError::Other(anyhow!("num_threads is too large")));
    }

    unsafe {
        let search_opts = ffi::searchOptions_new();
        let search_method = ffi::searchMethod_new();
        let search_instance = ffi::instance_new();

        // Quiet by default: workers don't print to cout under runMinionParallel
        // (they observe getOptions().silent = true via the per-thread copy).
        // The user receives results via the callback.
        (*search_opts).silent = true;
        (*search_opts).print_solution = false;
        // Library convention: enumerate all solutions and let the caller's
        // callback decide when to stop by returning false. The C-side
        // controller enforces this `sollimit = -1` shared value.
        (*search_opts).sollimit = -1;

        if let Some(seed) = options.seed {
            (*search_method).randomSeed = seed;
        }
        (*search_method).preprocess = options.preprocess.to_ffi();
        (*search_method).propMethod = options.prop_node.to_ffi();

        let mut print_vars: Vec<VarName> = vec![];
        let convert_result =
            convert_model_to_raw(search_instance, &model, &options, &mut print_vars);
        if let Err(e) = convert_result {
            ffi::searchMethod_free(search_method);
            ffi::searchOptions_free(search_opts);
            ffi::instance_free(search_instance);
            return Err(e);
        }

        let state = ParallelCallbackState {
            callback: std::sync::Mutex::new(callback),
            print_vars,
        };
        let userdata = &state as *const ParallelCallbackState<'_> as *mut c_void;

        let cfg = ffi::MinionThreadConfig {
            numThreads: num_threads as i32,
            baseSeed: options.seed.unwrap_or(0),
        };

        let res = ffi::runMinionParallel(
            cfg,
            search_opts,
            search_method,
            search_instance,
            Some(parallel_callback_thunk),
            userdata,
        );

        ffi::searchMethod_free(search_method);
        ffi::searchOptions_free(search_opts);
        ffi::instance_free(search_instance);

        check_minion_result(res)?;
        Ok(())
    }
}

/// Like [`run_minion_midsearch`] but with [`RunOptions`].
#[allow(clippy::unwrap_used)]
pub fn run_minion_midsearch_with_options(
    model: Model,
    options: RunOptions,
    callback: MidSearchCallback<'_>,
) -> Result<SolverContext, MinionError> {
    unsafe {
        let ctx = ffi::minion_newContext();
        let search_opts = ffi::searchOptions_new();
        let search_method = ffi::searchMethod_new();
        let search_instance = ffi::instance_new();

        // Use Minion as a quiet library by default. Low-level FFI callers that
        // want native solver output can opt out by configuring SearchOptions
        // themselves instead of going through this wrapper.
        (*search_opts).silent = true;
        (*search_opts).print_solution = false;

        if let Some(seed) = options.seed {
            (*search_method).randomSeed = seed;
        }
        (*search_method).preprocess = options.preprocess.to_ffi();
        (*search_method).propMethod = options.prop_node.to_ffi();

        let mut state = CallbackState {
            callback,
            instance: search_instance,
            print_vars: vec![],
            midsearch_vars: vec![],
        };

        convert_model_to_raw(search_instance, &model, &options, &mut state.print_vars)?;

        let userdata = &mut state as *mut CallbackState<'_> as *mut c_void;
        let res = ffi::runMinion(
            ctx,
            search_opts,
            search_method,
            search_instance,
            Some(run_callback),
            userdata,
        );

        ffi::searchMethod_free(search_method);
        ffi::searchOptions_free(search_opts);
        ffi::instance_free(search_instance);

        match check_minion_result(res) {
            Ok(()) => Ok(SolverContext { ctx }),
            Err(e) => {
                ffi::minion_freeContext(ctx);
                Err(MinionError::from(e))
            }
        }
    }
}

unsafe fn convert_model_to_raw(
    instance: *mut ffi::ProbSpec_CSPInstance,
    model: &Model,
    options: &RunOptions,
    print_vars: &mut Vec<VarName>,
) -> Result<(), MinionError> {
    /*******************************/
    /*        Add variables        */
    /*******************************/

    /*
     * Add variables to:
     * 1. symbol table
     * 2. print matrix
     * 3. search vars
     *
     * These are all done in the order saved in the SymbolTable.
     */

    let search_vars = Scoped::new(ffi::vec_var_new(), |x| ffi::vec_var_free(x as _));

    // initialise all variables, and add all variables to the print order
    for var_name in model.named_variables.get_variable_order() {
        let c_str = CString::new(var_name.clone()).map_err(|_| {
            anyhow!(
                "Variable name {:?} contains a null character.",
                var_name.clone()
            )
        })?;

        let vartype = model
            .named_variables
            .get_vartype(var_name.clone())
            .ok_or(anyhow!("Could not get var type for {:?}", var_name.clone()))?;

        match vartype {
            VarDomain::Bound(a, b) => {
                check_minion_result(ffi::minion_newVar(
                    instance,
                    c_str.as_ptr() as *mut c_char,
                    ffi::VariableType_VAR_BOUND,
                    a,
                    b,
                ))?;
            }
            VarDomain::Discrete(a, b) => {
                check_minion_result(ffi::minion_newVar(
                    instance,
                    c_str.as_ptr() as *mut c_char,
                    ffi::VariableType_VAR_DISCRETE,
                    a,
                    b,
                ))?;
            }
            VarDomain::Bool => {
                check_minion_result(ffi::minion_newVar(
                    instance,
                    c_str.as_ptr() as *mut c_char,
                    ffi::VariableType_VAR_BOOL,
                    0,
                    1,
                ))?;
            }
            VarDomain::SparseBound(ref vals) => {
                let raw = Scoped::new(ffi::vec_int_new(), |x| ffi::vec_int_free(x as _));
                for v in vals {
                    ffi::vec_int_push_back(raw.ptr, *v);
                }
                check_minion_result(ffi::minion_newSparseBoundVar(
                    instance,
                    c_str.as_ptr() as *mut c_char,
                    raw.ptr,
                ))?;
            }
            x => return Err(MinionError::NotImplemented(format!("{x:?}"))),
        };

        let var_result = ffi::minion_getVarByName(instance, c_str.as_ptr() as _);
        check_minion_result(var_result.result)?;
        let var = var_result.var;

        ffi::printMatrix_addVar(instance, var);

        // Remember the order for the callback function.
        print_vars.push(var_name.clone());
    }

    // only add search variables to search order
    for search_var_name in model.named_variables.get_search_variable_order() {
        let c_str = CString::new(search_var_name.clone()).map_err(|_| {
            anyhow!(
                "Variable name {:?} contains a null character.",
                search_var_name.clone()
            )
        })?;
        let var_result = ffi::minion_getVarByName(instance, c_str.as_ptr() as _);
        check_minion_result(var_result.result)?;
        ffi::vec_var_push_back(search_vars.ptr, var_result.var);
    }

    let search_order = Scoped::new(
        ffi::searchOrder_new(search_vars.ptr, options.var_order.to_ffi(), false),
        |x| ffi::searchOrder_free(x as _),
    );
    // Minion's text parser defaults every variable's value-ordering to
    // ASCEND via `SearchOrder::setupValueOrder` during BuildCSP. To set
    // DESCEND or RANDOM from the library path we have to fill the
    // per-variable valOrder vector ourselves before BuildCSP runs.
    ffi::searchOrder_setValOrder(search_order.ptr, options.val_order.to_ffi());

    ffi::instance_addSearchOrder(instance, search_order.ptr);

    /*********************************/
    /*        Add tuple tables       */
    /*********************************/
    //
    // Must happen BEFORE add-constraint: constraints like
    // `Str2Plus(vars, Var::NameRef(table_name))` look the table up
    // by name on the instance.

    for (name, tuples) in &model.tuple_tables {
        let c_name = CString::new(name.clone()).map_err(|_| {
            anyhow!(
                "Tuple-table name {:?} contains a null character.",
                name.clone()
            )
        })?;

        // Build the raw Vec<Vec<DomainInt>> to hand to tupleList_new.
        // tupleList_new takes ownership semantically (it constructs a
        // TupleList from the contents); the vec_vec_int_new/free
        // pair only cleans up the intermediate carrier.
        let raw_tuples = Scoped::new(ffi::vec_vec_int_new(), |x| ffi::vec_vec_int_free(x as _));
        for tuple in tuples {
            let raw_tuple = Scoped::new(ffi::vec_int_new(), |x| ffi::vec_int_free(x as _));
            for constant in tuple {
                let val = match constant {
                    Constant::Integer(n) => *n,
                    Constant::Bool(true) => 1,
                    Constant::Bool(false) => 0,
                    #[allow(unreachable_patterns)]
                    x => return Err(MinionError::NotImplemented(format!("{x:?}"))),
                };
                ffi::vec_int_push_back(raw_tuple.ptr, val);
            }
            ffi::vec_vec_int_push_back_ptr(raw_tuples.ptr, raw_tuple.ptr);
        }

        // `instance_addTupleTableSymbol` copies the name and takes
        // ownership of the TupleList via `shared_ptr<TupleList>`,
        // so we do NOT wrap the result of `tupleList_new` in Scoped.
        let raw_tuple_list = ffi::tupleList_new(raw_tuples.ptr);
        ffi::instance_addTupleTableSymbol(instance, c_name.as_ptr() as *mut c_char, raw_tuple_list);
    }

    /*********************************/
    /*        Add constraints        */
    /*********************************/

    for constraint in &model.constraints {
        // 1. get constraint type and create C++ constraint object
        // 2. run through arguments and add them to the constraint
        // 3. add constraint to instance

        let constraint_type = get_constraint_type(constraint)?;
        let raw_constraint = Scoped::new(ffi::constraint_new(constraint_type), |x| {
            ffi::constraint_free(x as _)
        });

        constraint_add_args(instance, raw_constraint.ptr, constraint)?;
        ffi::instance_addConstraint(instance, raw_constraint.ptr);
    }

    Ok(())
}

unsafe fn get_constraint_type(constraint: &Constraint) -> Result<u32, MinionError> {
    match constraint {
        Constraint::SumGeq(_, _) => Ok(ffi::ConstraintType_CT_GEQSUM),
        Constraint::SumLeq(_, _) => Ok(ffi::ConstraintType_CT_LEQSUM),
        Constraint::Ineq(_, _, _) => Ok(ffi::ConstraintType_CT_INEQ),
        Constraint::Eq(_, _) => Ok(ffi::ConstraintType_CT_EQ),
        Constraint::Difference(_, _) => Ok(ffi::ConstraintType_CT_DIFFERENCE),
        Constraint::Div(_, _) => Ok(ffi::ConstraintType_CT_DIV),
        Constraint::DivUndefZero(_, _) => Ok(ffi::ConstraintType_CT_DIV_UNDEFZERO),
        Constraint::Modulo(_, _) => Ok(ffi::ConstraintType_CT_MODULO),
        Constraint::ModuloUndefZero(_, _) => Ok(ffi::ConstraintType_CT_MODULO_UNDEFZERO),
        Constraint::Pow(_, _) => Ok(ffi::ConstraintType_CT_POW),
        Constraint::Product(_, _) => Ok(ffi::ConstraintType_CT_PRODUCT2),
        Constraint::WeightedSumGeq(_, _, _) => Ok(ffi::ConstraintType_CT_WEIGHTGEQSUM),
        Constraint::WeightedSumLeq(_, _, _) => Ok(ffi::ConstraintType_CT_WEIGHTLEQSUM),
        Constraint::CheckAssign(_) => Ok(ffi::ConstraintType_CT_CHECK_ASSIGN),
        Constraint::CheckGsa(_) => Ok(ffi::ConstraintType_CT_CHECK_GSA),
        Constraint::ForwardChecking(_) => Ok(ffi::ConstraintType_CT_FORWARD_CHECKING),
        Constraint::Reify(_, _) => Ok(ffi::ConstraintType_CT_REIFY),
        Constraint::ReifyImply(_, _) => Ok(ffi::ConstraintType_CT_REIFYIMPLY),
        Constraint::ReifyImplyQuick(_, _) => Ok(ffi::ConstraintType_CT_REIFYIMPLY_QUICK),
        Constraint::WatchedAnd(_) => Ok(ffi::ConstraintType_CT_WATCHED_NEW_AND),
        Constraint::WatchedOr(_) => Ok(ffi::ConstraintType_CT_WATCHED_NEW_OR),
        Constraint::GacAllDiff(_) => Ok(ffi::ConstraintType_CT_GACALLDIFF),
        Constraint::AllDiff(_) => Ok(ffi::ConstraintType_CT_ALLDIFF),
        Constraint::AllDiffMatrix(_, _) => Ok(ffi::ConstraintType_CT_ALLDIFFMATRIX),
        Constraint::WatchSumGeq(_, _) => Ok(ffi::ConstraintType_CT_WATCHED_GEQSUM),
        Constraint::WatchSumLeq(_, _) => Ok(ffi::ConstraintType_CT_WATCHED_LEQSUM),
        Constraint::OccurrenceGeq(_, _, _) => Ok(ffi::ConstraintType_CT_GEQ_OCCURRENCE),
        Constraint::OccurrenceLeq(_, _, _) => Ok(ffi::ConstraintType_CT_LEQ_OCCURRENCE),
        Constraint::Occurrence(_, _, _) => Ok(ffi::ConstraintType_CT_OCCURRENCE),
        Constraint::LitSumGeq(_, _, _) => Ok(ffi::ConstraintType_CT_WATCHED_LITSUM),
        Constraint::Gcc(_, _, _) => Ok(ffi::ConstraintType_CT_GCC),
        Constraint::GccWeak(_, _, _) => Ok(ffi::ConstraintType_CT_GCCWEAK),
        Constraint::LexLeqRv(_, _) => Ok(ffi::ConstraintType_CT_GACLEXLEQ),
        Constraint::LexLeq(_, _) => Ok(ffi::ConstraintType_CT_LEXLEQ),
        Constraint::LexLess(_, _) => Ok(ffi::ConstraintType_CT_LEXLESS),
        Constraint::LexLeqQuick(_, _) => Ok(ffi::ConstraintType_CT_QUICK_LEXLEQ),
        Constraint::LexLessQuick(_, _) => Ok(ffi::ConstraintType_CT_QUICK_LEXLESS),
        Constraint::WatchVecNeq(_, _) => Ok(ffi::ConstraintType_CT_WATCHED_VECNEQ),
        Constraint::WatchVecExistsLess(_, _) => Ok(ffi::ConstraintType_CT_WATCHED_VEC_OR_LESS),
        Constraint::Hamming(_, _, _) => Ok(ffi::ConstraintType_CT_WATCHED_HAMMING),
        Constraint::NotHamming(_, _, _) => Ok(ffi::ConstraintType_CT_WATCHED_NOT_HAMMING),
        Constraint::FrameUpdate(_, _, _, _, _) => Ok(ffi::ConstraintType_CT_FRAMEUPDATE),
        Constraint::NegativeTable(_, _) => Ok(ffi::ConstraintType_CT_WATCHED_NEGATIVE_TABLE),
        Constraint::Table(_, _) => Ok(ffi::ConstraintType_CT_WATCHED_TABLE),
        Constraint::GacSchema(_, _) => Ok(ffi::ConstraintType_CT_GACSCHEMA),
        Constraint::LightTable(_, _) => Ok(ffi::ConstraintType_CT_LIGHTTABLE),
        Constraint::Mddc(_, _) => Ok(ffi::ConstraintType_CT_MDDC),
        Constraint::NegativeMddc(_, _) => Ok(ffi::ConstraintType_CT_NEGATIVEMDDC),
        Constraint::Str2Plus(_, _) => Ok(ffi::ConstraintType_CT_STR),
        Constraint::Max(_, _) => Ok(ffi::ConstraintType_CT_MAX),
        Constraint::Min(_, _) => Ok(ffi::ConstraintType_CT_MIN),
        Constraint::NvalueGeq(_, _) => Ok(ffi::ConstraintType_CT_GEQNVALUE),
        Constraint::NvalueLeq(_, _) => Ok(ffi::ConstraintType_CT_LEQNVALUE),
        Constraint::Element(_, _, _) => Ok(ffi::ConstraintType_CT_ELEMENT),
        Constraint::ElementOne(_, _, _) => Ok(ffi::ConstraintType_CT_ELEMENT_ONE),
        Constraint::ElementUndefZero(_, _, _) => Ok(ffi::ConstraintType_CT_ELEMENT_UNDEFZERO),
        Constraint::WatchElement(_, _, _) => Ok(ffi::ConstraintType_CT_WATCHED_ELEMENT),
        Constraint::WatchElementOne(_, _, _) => Ok(ffi::ConstraintType_CT_WATCHED_ELEMENT_ONE),
        Constraint::WatchElementOneUndefZero(_, _, _) => {
            Ok(ffi::ConstraintType_CT_WATCHED_ELEMENT_ONE_UNDEFZERO)
        }
        Constraint::WatchElementUndefZero(_, _, _) => {
            Ok(ffi::ConstraintType_CT_WATCHED_ELEMENT_UNDEFZERO)
        }
        Constraint::WLiteral(_, _) => Ok(ffi::ConstraintType_CT_WATCHED_LIT),
        Constraint::WNotLiteral(_, _) => Ok(ffi::ConstraintType_CT_WATCHED_NOTLIT),
        Constraint::WInIntervalSet(_, _) => Ok(ffi::ConstraintType_CT_WATCHED_ININTERVALSET),
        Constraint::WInRange(_, _) => Ok(ffi::ConstraintType_CT_WATCHED_INRANGE),
        Constraint::WInset(_, _) => Ok(ffi::ConstraintType_CT_WATCHED_INSET),
        Constraint::WNotInRange(_, _) => Ok(ffi::ConstraintType_CT_WATCHED_NOT_INRANGE),
        Constraint::WNotInset(_, _) => Ok(ffi::ConstraintType_CT_WATCHED_NOT_INSET),
        Constraint::Abs(_, _) => Ok(ffi::ConstraintType_CT_ABS),
        Constraint::DisEq(_, _) => Ok(ffi::ConstraintType_CT_DISEQ),
        Constraint::MinusEq(_, _) => Ok(ffi::ConstraintType_CT_MINUSEQ),
        Constraint::GacEq(_, _) => Ok(ffi::ConstraintType_CT_GACEQ),
        Constraint::WatchLess(_, _) => Ok(ffi::ConstraintType_CT_WATCHED_LESS),
        Constraint::WatchNeq(_, _) => Ok(ffi::ConstraintType_CT_WATCHED_NEQ),
        Constraint::True => Ok(ffi::ConstraintType_CT_TRUE),
        Constraint::False => Ok(ffi::ConstraintType_CT_FALSE),

        #[allow(unreachable_patterns)]
        x => Err(MinionError::NotImplemented(format!(
            "Constraint not implemented {x:?}",
        ))),
    }
}

unsafe fn constraint_add_args(
    i: *mut ffi::ProbSpec_CSPInstance,
    r_constr: *mut ffi::ProbSpec_ConstraintBlob,
    constr: &Constraint,
) -> Result<(), MinionError> {
    match constr {
        Constraint::SumGeq(lhs_vars, rhs_var) => {
            read_list(i, r_constr, lhs_vars)?;
            read_var(i, r_constr, rhs_var)?;
            Ok(())
        }
        Constraint::SumLeq(lhs_vars, rhs_var) => {
            read_list(i, r_constr, lhs_vars)?;
            read_var(i, r_constr, rhs_var)?;
            Ok(())
        }
        Constraint::Ineq(var1, var2, c) => {
            read_var(i, r_constr, var1)?;
            read_var(i, r_constr, var2)?;
            read_constant(r_constr, c)?;
            Ok(())
        }
        Constraint::Eq(var1, var2) => {
            read_var(i, r_constr, var1)?;
            read_var(i, r_constr, var2)?;
            Ok(())
        }
        Constraint::Difference((a, b), c) => {
            read_2_vars(i, r_constr, a, b)?;
            read_var(i, r_constr, c)?;
            Ok(())
        }
        Constraint::Div((a, b), c) => {
            read_2_vars(i, r_constr, a, b)?;
            read_var(i, r_constr, c)?;
            Ok(())
        }
        Constraint::DivUndefZero((a, b), c) => {
            read_2_vars(i, r_constr, a, b)?;
            read_var(i, r_constr, c)?;
            Ok(())
        }
        Constraint::Modulo((a, b), c) => {
            read_2_vars(i, r_constr, a, b)?;
            read_var(i, r_constr, c)?;
            Ok(())
        }
        Constraint::ModuloUndefZero((a, b), c) => {
            read_2_vars(i, r_constr, a, b)?;
            read_var(i, r_constr, c)?;
            Ok(())
        }
        Constraint::Pow((a, b), c) => {
            read_2_vars(i, r_constr, a, b)?;
            read_var(i, r_constr, c)?;
            Ok(())
        }
        Constraint::Product((a, b), c) => {
            read_2_vars(i, r_constr, a, b)?;
            read_var(i, r_constr, c)?;
            Ok(())
        }
        Constraint::WeightedSumGeq(a, b, c) => {
            read_constant_list(r_constr, a)?;
            read_list(i, r_constr, b)?;
            read_var(i, r_constr, c)?;
            Ok(())
        }
        Constraint::WeightedSumLeq(a, b, c) => {
            read_constant_list(r_constr, a)?;
            read_list(i, r_constr, b)?;
            read_var(i, r_constr, c)?;
            Ok(())
        }
        Constraint::CheckAssign(a) => {
            read_constraint(i, r_constr, (**a).clone())?;
            Ok(())
        }
        Constraint::CheckGsa(a) => {
            read_constraint(i, r_constr, (**a).clone())?;
            Ok(())
        }
        Constraint::ForwardChecking(a) => {
            read_constraint(i, r_constr, (**a).clone())?;
            Ok(())
        }
        Constraint::Reify(a, b) => {
            read_constraint(i, r_constr, (**a).clone())?;
            read_var(i, r_constr, b)?;
            Ok(())
        }
        Constraint::ReifyImply(a, b) => {
            read_constraint(i, r_constr, (**a).clone())?;
            read_var(i, r_constr, b)?;
            Ok(())
        }
        Constraint::ReifyImplyQuick(a, b) => {
            read_constraint(i, r_constr, (**a).clone())?;
            read_var(i, r_constr, b)?;
            Ok(())
        }
        Constraint::WatchedAnd(a) => {
            read_constraint_list(i, r_constr, a)?;
            Ok(())
        }
        Constraint::WatchedOr(a) => {
            read_constraint_list(i, r_constr, a)?;
            Ok(())
        }
        Constraint::GacAllDiff(a) => {
            read_list(i, r_constr, a)?;
            Ok(())
        }
        Constraint::AllDiff(a) => {
            read_list(i, r_constr, a)?;
            Ok(())
        }
        Constraint::AllDiffMatrix(a, b) => {
            read_list(i, r_constr, a)?;
            read_constant(r_constr, b)?;
            Ok(())
        }
        Constraint::WatchSumGeq(a, b) => {
            read_list(i, r_constr, a)?;
            read_constant(r_constr, b)?;
            Ok(())
        }
        Constraint::WatchSumLeq(a, b) => {
            read_list(i, r_constr, a)?;
            read_constant(r_constr, b)?;
            Ok(())
        }
        Constraint::OccurrenceGeq(a, b, c) => {
            read_list(i, r_constr, a)?;
            read_constant(r_constr, b)?;
            read_constant(r_constr, c)?;
            Ok(())
        }
        Constraint::OccurrenceLeq(a, b, c) => {
            read_list(i, r_constr, a)?;
            read_constant(r_constr, b)?;
            read_constant(r_constr, c)?;
            Ok(())
        }
        Constraint::Occurrence(a, b, c) => {
            read_list(i, r_constr, a)?;
            read_constant(r_constr, b)?;
            read_var(i, r_constr, c)?;
            Ok(())
        }
        Constraint::LexLess(a, b)
        | Constraint::LexLeq(a, b)
        | Constraint::LexLeqRv(a, b)
        | Constraint::LexLeqQuick(a, b)
        | Constraint::LexLessQuick(a, b)
        | Constraint::WatchVecNeq(a, b)
        | Constraint::WatchVecExistsLess(a, b) => {
            read_list(i, r_constr, a)?;
            read_list(i, r_constr, b)?;
            Ok(())
        }
        Constraint::LitSumGeq(a, b, c) => {
            read_list(i, r_constr, a)?;
            read_constant_list(r_constr, b)?;
            read_constant(r_constr, c)?;
            Ok(())
        }
        Constraint::Gcc(a, b, c) | Constraint::GccWeak(a, b, c) => {
            read_list(i, r_constr, a)?;
            read_constant_list(r_constr, b)?;
            read_list(i, r_constr, c)?;
            Ok(())
        }
        Constraint::Hamming(a, b, c) | Constraint::NotHamming(a, b, c) => {
            read_list(i, r_constr, a)?;
            read_list(i, r_constr, b)?;
            read_constant(r_constr, c)?;
            Ok(())
        }
        Constraint::FrameUpdate(a, b, c, d, e) => {
            read_list(i, r_constr, a)?;
            read_list(i, r_constr, b)?;
            read_list(i, r_constr, c)?;
            read_list(i, r_constr, d)?;
            read_constant(r_constr, e)?;
            Ok(())
        }
        Constraint::NegativeTable(vars, tuple_list)
        | Constraint::Table(vars, tuple_list)
        | Constraint::GacSchema(vars, tuple_list)
        | Constraint::LightTable(vars, tuple_list)
        | Constraint::Mddc(vars, tuple_list)
        | Constraint::NegativeMddc(vars, tuple_list) => {
            read_list(i, r_constr, vars)?;
            read_tuple_list(r_constr, tuple_list)?;
            Ok(())
        }
        Constraint::Str2Plus(vars, table_var) => {
            // CT_STR references a tuple table by NAME, not a variable.
            // Instead of looking it up in the symbol table as a var
            // (which fails), resolve it against the instance's
            // tuple-table symbol table via `constraint_setTuplesByName`
            // — that reuses the existing shared_ptr so refcounts stay
            // correct.
            read_list(i, r_constr, vars)?;
            let name = match table_var {
                Var::NameRef(n) => n.clone(),
                other => {
                    return Err(MinionError::NotImplemented(format!(
                        "Str2Plus second argument must be Var::NameRef(tuple_table_name), got {other:?}"
                    )));
                }
            };
            let c_name = CString::new(name.clone())
                .map_err(|_| anyhow!("Tuple-table name {name:?} contains a null character."))?;
            ffi::constraint_setTuplesByName(r_constr, i, c_name.as_ptr());
            Ok(())
        }
        Constraint::Max(a, b)
        | Constraint::Min(a, b)
        | Constraint::NvalueGeq(a, b)
        | Constraint::NvalueLeq(a, b) => {
            read_list(i, r_constr, a)?;
            read_var(i, r_constr, b)?;
            Ok(())
        }
        Constraint::Element(vec, j, e)
        | Constraint::ElementOne(vec, j, e)
        | Constraint::ElementUndefZero(vec, j, e)
        | Constraint::WatchElement(vec, j, e)
        | Constraint::WatchElementOne(vec, j, e)
        | Constraint::WatchElementOneUndefZero(vec, j, e)
        | Constraint::WatchElementUndefZero(vec, j, e) => {
            read_list(i, r_constr, vec)?;
            read_var(i, r_constr, j)?;
            read_var(i, r_constr, e)?;
            Ok(())
        }
        Constraint::WLiteral(a, b) | Constraint::WNotLiteral(a, b) => {
            read_var(i, r_constr, a)?;
            read_constant(r_constr, b)?;
            Ok(())
        }
        Constraint::WInIntervalSet(var, consts)
        | Constraint::WInRange(var, consts)
        | Constraint::WNotInRange(var, consts)
        | Constraint::WInset(var, consts)
        | Constraint::WNotInset(var, consts) => {
            read_var(i, r_constr, var)?;
            read_constant_list(r_constr, consts)?;
            Ok(())
        }
        Constraint::Abs(a, b)
        | Constraint::DisEq(a, b)
        | Constraint::MinusEq(a, b)
        | Constraint::GacEq(a, b)
        | Constraint::WatchLess(a, b)
        | Constraint::WatchNeq(a, b) => {
            read_var(i, r_constr, a)?;
            read_var(i, r_constr, b)?;
            Ok(())
        }
        Constraint::AllDiffMatrix(vars, c) => {
            read_list(i, r_constr, vars)?;
            read_constant(r_constr, c)?;
            Ok(())
        }

        Constraint::True => Ok(()),
        Constraint::False => Ok(()),
        #[allow(unreachable_patterns)]
        x => Err(MinionError::NotImplemented(format!("{x:?}"))),
    }
}

// DO NOT call manually - this assumes that all needed vars are already in the symbol table.
// TODO not happy with this just assuming the name is in the symbol table
/// Resolve an AST Var to a raw FFI Var.
unsafe fn resolve_var(
    instance: *mut ffi::ProbSpec_CSPInstance,
    var: &Var,
) -> Result<ffi::ProbSpec_Var, MinionError> {
    match var {
        Var::NameRef(name) => {
            let c_str = CString::new(name.clone()).map_err(|_| {
                anyhow!(
                    "Variable name {:?} contains a null character.",
                    name.clone()
                )
            })?;
            let var_result = ffi::minion_getVarByName(instance, c_str.as_ptr() as _);
            check_minion_result(var_result.result)?;
            Ok(var_result.var)
        }
        Var::ConstantAsVar(n) => Ok(ffi::constantAsVar(*n)),
    }
}

unsafe fn read_list(
    instance: *mut ffi::ProbSpec_CSPInstance,
    raw_constraint: *mut ffi::ProbSpec_ConstraintBlob,
    vars: &Vec<Var>,
) -> Result<(), MinionError> {
    let raw_vars = Scoped::new(ffi::vec_var_new(), |x| ffi::vec_var_free(x as _));
    for var in vars {
        let raw_var = resolve_var(instance, var)?;
        ffi::vec_var_push_back(raw_vars.ptr, raw_var);
    }

    ffi::constraint_addList(raw_constraint, raw_vars.ptr);

    Ok(())
}

unsafe fn read_var(
    instance: *mut ffi::ProbSpec_CSPInstance,
    raw_constraint: *mut ffi::ProbSpec_ConstraintBlob,
    var: &Var,
) -> Result<(), MinionError> {
    let raw_vars = Scoped::new(ffi::vec_var_new(), |x| ffi::vec_var_free(x as _));
    let raw_var = resolve_var(instance, var)?;
    ffi::vec_var_push_back(raw_vars.ptr, raw_var);
    ffi::constraint_addList(raw_constraint, raw_vars.ptr);

    Ok(())
}

unsafe fn read_2_vars(
    instance: *mut ffi::ProbSpec_CSPInstance,
    raw_constraint: *mut ffi::ProbSpec_ConstraintBlob,
    var1: &Var,
    var2: &Var,
) -> Result<(), MinionError> {
    let mut raw_var = resolve_var(instance, var1)?;
    let mut raw_var2 = resolve_var(instance, var2)?;
    // todo: does this move or copy? I am confus!
    // TODO need to mkae the semantics of move vs copy / ownership clear in libminion!!
    // This shouldve leaked everywhere by now but i think libminion copies stuff??
    ffi::constraint_addTwoVars(raw_constraint, &mut raw_var, &mut raw_var2);
    Ok(())
}

unsafe fn read_constant(
    raw_constraint: *mut ffi::ProbSpec_ConstraintBlob,
    constant: &Constant,
) -> Result<(), MinionError> {
    let val: i32 = match constant {
        Constant::Integer(n) => Ok(*n),
        Constant::Bool(true) => Ok(1),
        Constant::Bool(false) => Ok(0),
        x => Err(MinionError::NotImplemented(format!("{x:?}"))),
    }?;

    ffi::constraint_addConstant(raw_constraint, val);

    Ok(())
}

unsafe fn read_constant_list(
    raw_constraint: *mut ffi::ProbSpec_ConstraintBlob,
    constants: &[Constant],
) -> Result<(), MinionError> {
    let raw_consts = Scoped::new(ffi::vec_int_new(), |x| ffi::vec_int_free(x as _));

    for constant in constants.iter() {
        let val = match constant {
            Constant::Integer(n) => Ok(*n),
            Constant::Bool(true) => Ok(1),
            Constant::Bool(false) => Ok(0),
            #[allow(unreachable_patterns)] // TODO: can there be other types?
            x => Err(MinionError::NotImplemented(format!("{x:?}"))),
        }?;

        ffi::vec_int_push_back(raw_consts.ptr, val);
    }

    ffi::constraint_addConstantList(raw_constraint, raw_consts.ptr);
    Ok(())
}

//TODO: check if the inner constraint is listed in the model or not?
//Does this matter?
// TODO: type-check inner constraints vars and tuples and so on?
unsafe fn read_constraint(
    instance: *mut ffi::ProbSpec_CSPInstance,
    raw_constraint: *mut ffi::ProbSpec_ConstraintBlob,
    inner_constraint: Constraint,
) -> Result<(), MinionError> {
    let constraint_type = get_constraint_type(&inner_constraint)?;
    let raw_inner_constraint = Scoped::new(ffi::constraint_new(constraint_type), |x| {
        ffi::constraint_free(x as _)
    });

    constraint_add_args(instance, raw_inner_constraint.ptr, &inner_constraint)?;

    ffi::constraint_addConstraint(raw_constraint, raw_inner_constraint.ptr);
    Ok(())
}

unsafe fn read_constraint_list(
    instance: *mut ffi::ProbSpec_CSPInstance,
    raw_constraint: *mut ffi::ProbSpec_ConstraintBlob,
    inner_constraints: &[Constraint],
) -> Result<(), MinionError> {
    let raw_inners = Scoped::new(ffi::vec_constraints_new(), |x| {
        ffi::vec_constraints_free(x as _)
    });
    for inner_constraint in inner_constraints.iter() {
        let constraint_type = get_constraint_type(inner_constraint)?;
        let raw_inner_constraint = Scoped::new(ffi::constraint_new(constraint_type), |x| {
            ffi::constraint_free(x as _)
        });

        constraint_add_args(instance, raw_inner_constraint.ptr, inner_constraint)?;
        ffi::vec_constraints_push_back(raw_inners.ptr, raw_inner_constraint.ptr);
    }

    ffi::constraint_addConstraintList(raw_constraint, raw_inners.ptr);
    Ok(())
}

unsafe fn read_tuple_list(
    raw_constraint: *mut ffi::ProbSpec_ConstraintBlob,
    tuples: &Vec<Tuple>,
) -> Result<(), MinionError> {
    // a tuple list is just a vec<vec<int>>, where each inner vec is a tuple
    let raw_tuples = Scoped::new(ffi::vec_vec_int_new(), |x| ffi::vec_vec_int_free(x as _));
    for tuple in tuples {
        let raw_tuple = Scoped::new(ffi::vec_int_new(), |x| ffi::vec_int_free(x as _));
        for constant in tuple.iter() {
            let val = match constant {
                Constant::Integer(n) => Ok(*n),
                Constant::Bool(true) => Ok(1),
                Constant::Bool(false) => Ok(0),
                #[allow(unreachable_patterns)] // TODO: can there be other types?
                x => Err(MinionError::NotImplemented(format!("{:?}", x))),
            }?;

            ffi::vec_int_push_back(raw_tuple.ptr, val);
        }

        ffi::vec_vec_int_push_back_ptr(raw_tuples.ptr, raw_tuple.ptr);
    }

    // `constraint_setTuples` transfers ownership of `TupleList` into Minion via shared_ptr.
    // Do not wrap this pointer in `Scoped` or it will be freed too early.
    let raw_tuple_list = ffi::tupleList_new(raw_tuples.ptr);
    ffi::constraint_setTuples(raw_constraint, raw_tuple_list);

    Ok(())
}
