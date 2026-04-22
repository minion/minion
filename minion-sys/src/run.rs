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

/// State passed through the C callback's `void* userdata` pointer.
///
/// This replaces the old thread-local approach — all callback state is now
/// passed explicitly through the FFI userdata mechanism.
struct CallbackState<'a> {
    callback: Callback<'a>,
    print_vars: Vec<VarName>,
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

    if state.print_vars.is_empty() {
        return true;
    }

    // Build solutions HashMap by reading variable values from Minion.
    let mut solutions: HashMap<VarName, Constant> = HashMap::new();
    for (i, var) in state.print_vars.iter().enumerate() {
        let solution_int: i32 = unsafe { ffi::printMatrix_getValue(ctx, i as _) };
        let solution: Constant = Constant::Integer(solution_int);
        solutions.insert(var.to_string(), solution);
    }

    (state.callback)(solutions)
}

/// Run Minion on the given [Model].
///
/// The given [callback](Callback) is ran whenever a new solution set is found.
///
/// Returns a [`SolverContext`] on success, which can be used to query run
/// statistics via [`SolverContext::get_from_table`].
#[allow(clippy::unwrap_used)]
pub fn run_minion(model: Model, callback: Callback<'_>) -> Result<SolverContext, MinionError> {
    let mut state = CallbackState {
        callback,
        print_vars: vec![],
    };

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

        convert_model_to_raw(search_instance, &model, &mut state.print_vars)?;

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

        let (vartype_raw, domain_low, domain_high) = match vartype {
            VarDomain::Bound(a, b) => Ok((ffi::VariableType_VAR_BOUND, a, b)),
            VarDomain::Discrete(a, b) => Ok((ffi::VariableType_VAR_DISCRETE, a, b)),
            VarDomain::Bool => Ok((ffi::VariableType_VAR_BOOL, 0, 1)), // TODO: will this work?
            x => Err(MinionError::NotImplemented(format!("{x:?}"))),
        }?;

        check_minion_result(ffi::minion_newVar(
            instance,
            c_str.as_ptr() as *mut c_char,
            vartype_raw,
            domain_low,
            domain_high,
        ))?;

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
        ffi::searchOrder_new(search_vars.ptr, ffi::VarOrderEnum_ORDER_STATIC, false),
        |x| ffi::searchOrder_free(x as _),
    );

    ffi::instance_addSearchOrder(instance, search_order.ptr);

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
        Constraint::LexLessQuick(_, _) => Ok(ffi::ConstraintType_CT_QUICK_LEXLEQ),
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
        Constraint::LexLess(a, b) => {
            read_list(i, r_constr, a)?;
            read_list(i, r_constr, b)?;
            Ok(())
        }
        Constraint::LexLeq(a, b) => {
            read_list(i, r_constr, a)?;
            read_list(i, r_constr, b)?;
            Ok(())
        }
        //Constraint::LitSumGeq(_, _, _) => todo!(),
        //Constraint::Gcc(_, _, _) => todo!(),
        //Constraint::GccWeak(_, _, _) => todo!(),
        //Constraint::LexLeqRv(_, _) => todo!(),
        //Constraint::LexLeqQuick(_, _) => todo!(),
        //Constraint::LexLessQuick(_, _) => todo!(),
        //Constraint::WatchVecNeq(_, _) => todo!(),
        //Constraint::WatchVecExistsLess(_, _) => todo!(),
        //Constraint::Hamming(_, _, _) => todo!(),
        //Constraint::NotHamming(_, _, _) => todo!(),
        //Constraint::FrameUpdate(_, _, _, _, _) => todo!(),
        Constraint::NegativeTable(vars, tuple_list) | Constraint::Table(vars, tuple_list) => {
            read_list(i, r_constr, vars)?;
            read_tuple_list(r_constr, tuple_list)?;
            Ok(())
        }
        //Constraint::GacSchema(_, _) => todo!(),
        //Constraint::LightTable(_, _) => todo!(),
        //Constraint::Mddc(_, _) => todo!(),
        //Constraint::NegativeMddc(_, _) => todo!(),
        //Constraint::Str2Plus(_, _) => todo!(),
        //Constraint::Max(_, _) => todo!(),
        //Constraint::Min(_, _) => todo!(),
        //Constraint::NvalueGeq(_, _) => todo!(),
        //Constraint::NvalueLeq(_, _) => todo!(),
        //Constraint::Element(_, _, _) => todo!(),
        //Constraint::ElementUndefZero(_, _, _) => todo!(),
        //Constraint::WatchElement(_, _, _) => todo!(),
        //Constraint::WatchElementOne(_, _, _) => todo!(),
        Constraint::ElementOne(vec, j, e) => {
            read_list(i, r_constr, vec)?;
            read_var(i, r_constr, j)?;
            read_var(i, r_constr, e)?;
            Ok(())
        }
        //Constraint::WatchElementOneUndefZero(_, _, _) => todo!(),
        //Constraint::WatchElementUndefZero(_, _, _) => todo!(),
        Constraint::WLiteral(a, b) => {
            read_var(i, r_constr, a)?;
            read_constant(r_constr, b)?;
            Ok(())
        }
        //Constraint::WNotLiteral(_, _) => todo!(),
        Constraint::WInIntervalSet(var, consts) => {
            read_var(i, r_constr, var)?;
            read_constant_list(r_constr, consts)?;
            Ok(())
        }
        //Constraint::WInRange(_, _) => todo!(),
        Constraint::WInset(a, b) => {
            read_var(i, r_constr, a)?;
            read_constant_list(r_constr, b)?;
            Ok(())
        }
        //Constraint::WNotInRange(_, _) => todo!(),
        //Constraint::WNotInset(_, _) => todo!(),
        Constraint::Abs(a, b) => {
            read_var(i, r_constr, a)?;
            read_var(i, r_constr, b)?;
            Ok(())
        }
        Constraint::DisEq(a, b) => {
            read_var(i, r_constr, a)?;
            read_var(i, r_constr, b)?;
            Ok(())
        }
        Constraint::MinusEq(a, b) => {
            read_var(i, r_constr, a)?;
            read_var(i, r_constr, b)?;
            Ok(())
        }
        //Constraint::GacEq(_, _) => todo!(),
        //Constraint::WatchLess(_, _) => todo!(),
        // TODO: ensure that this is a bool?
        Constraint::WatchNeq(a, b) => {
            read_var(i, r_constr, a)?;
            read_var(i, r_constr, b)?;
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
