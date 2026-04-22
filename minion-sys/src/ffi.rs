#![allow(warnings)]

use std::ffi::CString;
use std::sync::atomic::{AtomicI32, Ordering};
include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

#[cfg(test)]
mod tests {
    use std::ffi::{CString, c_char, c_void};
    use std::process::{Command, ExitStatus};

    use super::*;

    // solutions
    static X_VAL: AtomicI32 = AtomicI32::new(0);
    static Y_VAL: AtomicI32 = AtomicI32::new(0);
    static Z_VAL: AtomicI32 = AtomicI32::new(0);
    const MIDSEARCH_SCENARIO_ENV: &str = "MINION_MIDSEARCH_SCENARIO";
    const MIDSEARCH_CHILD_TEST: &str = "ffi::tests::midsearch_child_runner";
    const ISSUE_SCENARIO_ENV: &str = "MINION_ISSUE_MIDSEARCH_REIFY_NEWVAR";
    const ISSUE_CHILD_TEST: &str = "ffi::tests::midsearch_reify_newvar_issue_child_runner";

    #[derive(Clone, Copy, Debug)]
    enum MidsearchScenario {
        AddVarOnly,
        AddVarThenAddEqConstraint,
    }

    impl MidsearchScenario {
        fn as_env_value(self) -> &'static str {
            match self {
                MidsearchScenario::AddVarOnly => "add_var_only",
                MidsearchScenario::AddVarThenAddEqConstraint => "add_var_then_add_eq_constraint",
            }
        }

        fn from_env_value(value: &str) -> Option<Self> {
            match value {
                "add_var_only" => Some(MidsearchScenario::AddVarOnly),
                "add_var_then_add_eq_constraint" => {
                    Some(MidsearchScenario::AddVarThenAddEqConstraint)
                }
                _ => None,
            }
        }
    }

    #[derive(Debug)]
    struct MidsearchState {
        scenario: MidsearchScenario,
        instance: *mut ProbSpec_CSPInstance,
        callback_count: u32,
        added_var_ok: bool,
        looked_up_new_var_ok: bool,
        add_constraint_result: Option<MinionResult>,
    }

    #[derive(Debug)]
    struct MidsearchOutcome {
        run_code: MinionResult,
        callback_count: u32,
        added_var_ok: bool,
        looked_up_new_var_ok: bool,
        add_constraint_result: Option<MinionResult>,
    }

    #[derive(Debug)]
    struct IssueState {
        instance: *mut ProbSpec_CSPInstance,
        callback_count: u32,
        add_var_results: Vec<MinionResult>,
        add_constraint_results: Vec<MinionResult>,
    }

    #[derive(Debug)]
    struct IssueOutcome {
        run_code: MinionResult,
        callback_count: u32,
        add_var_results: Vec<MinionResult>,
        add_constraint_results: Vec<MinionResult>,
    }

    #[derive(Default, Debug)]
    struct SpawnStats {
        ok: usize,
        nonzero: usize,
        signal_11: usize,
        signal_6: usize,
        other_signal: usize,
    }

    #[cfg(unix)]
    fn status_signal(status: &ExitStatus) -> Option<i32> {
        use std::os::unix::process::ExitStatusExt;
        status.signal()
    }

    #[cfg(not(unix))]
    fn status_signal(_: &ExitStatus) -> Option<i32> {
        None
    }

    fn classify_status(status: &ExitStatus, stats: &mut SpawnStats) {
        if status.success() {
            stats.ok += 1;
            return;
        }

        stats.nonzero += 1;

        match status_signal(status) {
            Some(11) => stats.signal_11 += 1,
            Some(6) => stats.signal_6 += 1,
            Some(_) => stats.other_signal += 1,
            None => {}
        }
    }

    unsafe fn build_two_var_instance(instance: *mut ProbSpec_CSPInstance) {
        let x_name = b"x\0";
        let y_name = b"y\0";

        assert_eq!(
            minion_newVar(
                instance,
                x_name.as_ptr() as *mut c_char,
                VariableType_VAR_BOUND,
                0,
                1
            ),
            MinionResult_MINION_OK
        );
        assert_eq!(
            minion_newVar(
                instance,
                y_name.as_ptr() as *mut c_char,
                VariableType_VAR_BOUND,
                0,
                1
            ),
            MinionResult_MINION_OK
        );

        let x = minion_getVarByName(instance, x_name.as_ptr() as *mut c_char);
        let y = minion_getVarByName(instance, y_name.as_ptr() as *mut c_char);
        assert_eq!(x.result, MinionResult_MINION_OK);
        assert_eq!(y.result, MinionResult_MINION_OK);

        printMatrix_addVar(instance, x.var);
        printMatrix_addVar(instance, y.var);

        let search_vars = vec_var_new();
        vec_var_push_back(search_vars, x.var);
        vec_var_push_back(search_vars, y.var);
        let search_order = searchOrder_new(search_vars, VarOrderEnum_ORDER_STATIC, false);
        instance_addSearchOrder(instance, search_order);
    }

    unsafe fn build_issue_instance(instance: *mut ProbSpec_CSPInstance) {
        let x_name = b"x\0";
        let y_name = b"y\0";

        assert_eq!(
            minion_newVar(
                instance,
                x_name.as_ptr() as *mut c_char,
                VariableType_VAR_BOUND,
                0,
                3
            ),
            MinionResult_MINION_OK
        );
        assert_eq!(
            minion_newVar(
                instance,
                y_name.as_ptr() as *mut c_char,
                VariableType_VAR_BOUND,
                0,
                3
            ),
            MinionResult_MINION_OK
        );

        let x = minion_getVarByName(instance, x_name.as_ptr() as *mut c_char);
        let y = minion_getVarByName(instance, y_name.as_ptr() as *mut c_char);
        assert_eq!(x.result, MinionResult_MINION_OK);
        assert_eq!(y.result, MinionResult_MINION_OK);

        printMatrix_addVar(instance, x.var);
        printMatrix_addVar(instance, y.var);

        let search_vars = vec_var_new();
        vec_var_push_back(search_vars, x.var);
        vec_var_push_back(search_vars, y.var);
        let search_order = searchOrder_new(search_vars, VarOrderEnum_ORDER_STATIC, false);
        instance_addSearchOrder(instance, search_order);

        // sumgeq([x, y], 2)
        let geq = constraint_new(ConstraintType_CT_GEQSUM);
        let lhs = vec_var_new();
        vec_var_push_back(lhs, x.var);
        vec_var_push_back(lhs, y.var);
        let rhs = vec_var_new();
        vec_var_push_back(rhs, constantAsVar(2));
        constraint_addList(geq, lhs);
        constraint_addList(geq, rhs);
        instance_addConstraint(instance, geq);
    }

    unsafe fn add_issue_dominance_like_reify(
        ctx: *mut MinionContext,
        instance: *mut ProbSpec_CSPInstance,
        y_value: i32,
        aux_name: &str,
    ) -> (MinionResult, MinionResult) {
        let aux_name_c = CString::new(aux_name).expect("bad aux name");
        let add_var = minion_newVarMidsearch(
            ctx,
            instance,
            aux_name_c.as_ptr() as *mut c_char,
            VariableType_VAR_BOOL,
            0,
            1,
        );
        if add_var != MinionResult_MINION_OK {
            return (add_var, add_var);
        }

        let x_res = minion_getVarByName(instance, b"x\0".as_ptr() as *mut c_char);
        let y_res = minion_getVarByName(instance, b"y\0".as_ptr() as *mut c_char);
        let aux_res = minion_getVarByName(instance, aux_name_c.as_ptr() as *mut c_char);
        if x_res.result != MinionResult_MINION_OK
            || y_res.result != MinionResult_MINION_OK
            || aux_res.result != MinionResult_MINION_OK
        {
            return (add_var, MinionResult_MINION_INVALID_INSTANCE);
        }

        let mut x = x_res.var;
        let mut y = y_res.var;
        let mut aux = aux_res.var;

        // w-literal(aux, 0)
        let wlit = constraint_new(ConstraintType_CT_WATCHED_LIT);
        constraint_addVar(wlit, &mut aux);
        constraint_addConstant(wlit, 0);
        let add_wlit = minion_addConstraintMidsearch(ctx, instance, wlit);
        if add_wlit != MinionResult_MINION_OK {
            return (add_var, add_wlit);
        }

        // ineq(0, x, 0)
        let ineq_x = constraint_new(ConstraintType_CT_INEQ);
        let mut c0 = constantAsVar(0);
        constraint_addVar(ineq_x, &mut c0);
        constraint_addVar(ineq_x, &mut x);
        constraint_addConstant(ineq_x, 0);

        // ineq(y_value, y, 0)
        let ineq_y = constraint_new(ConstraintType_CT_INEQ);
        let mut cy = constantAsVar(y_value);
        constraint_addVar(ineq_y, &mut cy);
        constraint_addVar(ineq_y, &mut y);
        constraint_addConstant(ineq_y, 0);

        let and_inner = constraint_new(ConstraintType_CT_WATCHED_NEW_AND);
        constraint_addConstraint(and_inner, ineq_x);
        constraint_addConstraint(and_inner, ineq_y);

        // sumgeq([-1, x], 0)
        let sum_x = constraint_new(ConstraintType_CT_GEQSUM);
        let sx_lhs = vec_var_new();
        vec_var_push_back(sx_lhs, constantAsVar(-1));
        vec_var_push_back(sx_lhs, x);
        let sx_rhs = vec_var_new();
        vec_var_push_back(sx_rhs, constantAsVar(0));
        constraint_addList(sum_x, sx_lhs);
        constraint_addList(sum_x, sx_rhs);

        // sumgeq([-1, y], y_value)
        let sum_y = constraint_new(ConstraintType_CT_GEQSUM);
        let sy_lhs = vec_var_new();
        vec_var_push_back(sy_lhs, constantAsVar(-1));
        vec_var_push_back(sy_lhs, y);
        let sy_rhs = vec_var_new();
        vec_var_push_back(sy_rhs, constantAsVar(y_value));
        constraint_addList(sum_y, sy_lhs);
        constraint_addList(sum_y, sy_rhs);

        let or_inner = constraint_new(ConstraintType_CT_WATCHED_NEW_OR);
        constraint_addConstraint(or_inner, sum_x);
        constraint_addConstraint(or_inner, sum_y);

        let and_outer = constraint_new(ConstraintType_CT_WATCHED_NEW_AND);
        constraint_addConstraint(and_outer, and_inner);
        constraint_addConstraint(and_outer, or_inner);

        // reify(and_outer, aux)
        let reify = constraint_new(ConstraintType_CT_REIFY);
        constraint_addConstraint(reify, and_outer);
        constraint_addVar(reify, &mut aux);

        let add_reify = minion_addConstraintMidsearch(ctx, instance, reify);
        (add_var, add_reify)
    }

    unsafe extern "C" fn midsearch_mutation_callback(
        ctx: *mut MinionContext,
        userdata: *mut c_void,
    ) -> bool {
        let state = &mut *(userdata as *mut MidsearchState);
        state.callback_count += 1;

        if state.callback_count > 1 {
            return false;
        }

        let dyn_name = b"dyn_mid\0";
        state.added_var_ok = minion_newVarMidsearch(
            ctx,
            state.instance,
            dyn_name.as_ptr() as *mut c_char,
            VariableType_VAR_BOUND,
            0,
            1,
        ) == MinionResult_MINION_OK;

        let dyn_var = minion_getVarByName(state.instance, dyn_name.as_ptr() as *mut c_char);
        state.looked_up_new_var_ok = dyn_var.result == MinionResult_MINION_OK;

        if matches!(state.scenario, MidsearchScenario::AddVarThenAddEqConstraint)
            && state.added_var_ok
            && state.looked_up_new_var_ok
        {
            let x_name = b"x\0";
            let x_var = minion_getVarByName(state.instance, x_name.as_ptr() as *mut c_char);
            if x_var.result == MinionResult_MINION_OK {
                let eq = constraint_new(ConstraintType_CT_EQ);
                let mut dyn_v = dyn_var.var;
                let mut x_v = x_var.var;
                constraint_addVar(eq, &mut dyn_v);
                constraint_addVar(eq, &mut x_v);
                state.add_constraint_result =
                    Some(minion_addConstraintMidsearch(ctx, state.instance, eq));
            } else {
                state.add_constraint_result = Some(x_var.result);
            }
        }

        false
    }

    unsafe extern "C" fn issue_midsearch_callback(
        ctx: *mut MinionContext,
        userdata: *mut c_void,
    ) -> bool {
        let state = &mut *(userdata as *mut IssueState);
        state.callback_count += 1;

        if state.callback_count == 1 {
            let (add_var, add_reify) =
                add_issue_dominance_like_reify(ctx, state.instance, 2, "dyn_aux_0");
            state.add_var_results.push(add_var);
            state.add_constraint_results.push(add_reify);
            return true;
        }

        if state.callback_count == 2 {
            let add_unused = minion_newVarMidsearch(
                ctx,
                state.instance,
                b"dyn_aux_1_unused\0".as_ptr() as *mut c_char,
                VariableType_VAR_BOOL,
                0,
                1,
            );
            state.add_var_results.push(add_unused);

            let (add_var, add_reify) =
                add_issue_dominance_like_reify(ctx, state.instance, 3, "dyn_aux_2");
            state.add_var_results.push(add_var);
            state.add_constraint_results.push(add_reify);
            return true;
        }

        false
    }

    unsafe fn run_midsearch_scenario_once(scenario: MidsearchScenario) -> MidsearchOutcome {
        let ctx = minion_newContext();
        let options = searchOptions_new();
        let args = searchMethod_new();
        let instance = instance_new();

        (*options).silent = true;
        (*options).print_solution = false;

        build_two_var_instance(instance);

        let mut state = MidsearchState {
            scenario,
            instance,
            callback_count: 0,
            added_var_ok: false,
            looked_up_new_var_ok: false,
            add_constraint_result: None,
        };

        let run_code = runMinion(
            ctx,
            options,
            args,
            instance,
            Some(midsearch_mutation_callback),
            (&mut state as *mut MidsearchState).cast::<c_void>(),
        );

        minion_freeContext(ctx);

        MidsearchOutcome {
            run_code,
            callback_count: state.callback_count,
            added_var_ok: state.added_var_ok,
            looked_up_new_var_ok: state.looked_up_new_var_ok,
            add_constraint_result: state.add_constraint_result,
        }
    }

    unsafe fn run_issue_scenario_once() -> IssueOutcome {
        let ctx = minion_newContext();
        let options = searchOptions_new();
        let args = searchMethod_new();
        let instance = instance_new();

        (*options).silent = true;
        (*options).print_solution = false;

        build_issue_instance(instance);

        let mut state = IssueState {
            instance,
            callback_count: 0,
            add_var_results: vec![],
            add_constraint_results: vec![],
        };

        let run_code = runMinion(
            ctx,
            options,
            args,
            instance,
            Some(issue_midsearch_callback),
            (&mut state as *mut IssueState).cast::<c_void>(),
        );

        minion_freeContext(ctx);

        IssueOutcome {
            run_code,
            callback_count: state.callback_count,
            add_var_results: state.add_var_results,
            add_constraint_results: state.add_constraint_results,
        }
    }

    fn run_midsearch_child(scenario: MidsearchScenario) -> ExitStatus {
        let current_test_binary =
            std::env::current_exe().expect("could not find current test binary");

        Command::new(current_test_binary)
            .arg("--exact")
            .arg(MIDSEARCH_CHILD_TEST)
            .arg("--nocapture")
            .env(MIDSEARCH_SCENARIO_ENV, scenario.as_env_value())
            .status()
            .expect("could not execute child test")
    }

    fn run_issue_child() -> ExitStatus {
        let current_test_binary =
            std::env::current_exe().expect("could not find current test binary");

        Command::new(current_test_binary)
            .arg("--exact")
            .arg(ISSUE_CHILD_TEST)
            .arg("--nocapture")
            .env(ISSUE_SCENARIO_ENV, "1")
            .status()
            .expect("could not execute issue child test")
    }

    pub extern "C" fn hello_from_rust(ctx: *mut MinionContext, _userdata: *mut c_void) -> bool {
        unsafe {
            X_VAL.store(printMatrix_getValue(ctx, 0) as _, Ordering::Relaxed);
            Y_VAL.store(printMatrix_getValue(ctx, 1) as _, Ordering::Relaxed);
            Z_VAL.store(printMatrix_getValue(ctx, 2) as _, Ordering::Relaxed);
            return true;
        }
    }

    #[test]
    fn xyz_raw() {
        // A simple constraints model, manually written using FFI functions.
        // Testing to see if it does not segfault.
        // Results can be manually inspected in the outputted minion logs.
        unsafe {
            // See https://rust-lang.github.io/rust-bindgen/cpp.html
            let ctx = minion_newContext();
            let options = searchOptions_new();
            let args = searchMethod_new();
            let instance = instance_new();

            let x_str = CString::new("x").expect("bad x");
            let y_str = CString::new("y").expect("bad y");
            let z_str = CString::new("z").expect("bad z");

            assert_eq!(
                minion_newVar(instance, x_str.as_ptr() as _, VariableType_VAR_BOUND, 1, 3),
                MinionResult_MINION_OK
            );
            assert_eq!(
                minion_newVar(instance, y_str.as_ptr() as _, VariableType_VAR_BOUND, 2, 4),
                MinionResult_MINION_OK
            );
            assert_eq!(
                minion_newVar(instance, z_str.as_ptr() as _, VariableType_VAR_BOUND, 1, 5),
                MinionResult_MINION_OK
            );

            let x_res = minion_getVarByName(instance, x_str.as_ptr() as _);
            assert_eq!(x_res.result, MinionResult_MINION_OK);
            let x = x_res.var;
            let y_res = minion_getVarByName(instance, y_str.as_ptr() as _);
            assert_eq!(y_res.result, MinionResult_MINION_OK);
            let y = y_res.var;
            let z_res = minion_getVarByName(instance, z_str.as_ptr() as _);
            assert_eq!(z_res.result, MinionResult_MINION_OK);
            let z = z_res.var;

            // PRINT
            printMatrix_addVar(instance, x);
            printMatrix_addVar(instance, y);
            printMatrix_addVar(instance, z);

            // VARORDER
            let search_vars = vec_var_new();
            vec_var_push_back(search_vars as _, x);
            vec_var_push_back(search_vars as _, y);
            vec_var_push_back(search_vars as _, z);
            let search_order = searchOrder_new(search_vars as _, VarOrderEnum_ORDER_STATIC, false);
            instance_addSearchOrder(instance, search_order);

            // CONSTRAINTS
            let leq = constraint_new(ConstraintType_CT_LEQSUM);
            let geq = constraint_new(ConstraintType_CT_GEQSUM);
            let ineq = constraint_new(ConstraintType_CT_INEQ);

            let rhs_vars = vec_var_new();
            vec_var_push_back(rhs_vars, constantAsVar(4));

            // leq / geq : [var] [var]
            constraint_addList(leq, search_vars as _);
            constraint_addList(leq, rhs_vars as _);

            constraint_addList(geq, search_vars as _);
            constraint_addList(geq, rhs_vars as _);

            // ineq: [var] [var] [const]
            let x_vec = vec_var_new();
            vec_var_push_back(x_vec, x);

            let y_vec = vec_var_new();
            vec_var_push_back(y_vec, y);

            let const_vec = vec_int_new();
            vec_int_push_back(const_vec, -1);

            constraint_addList(ineq, x_vec as _);
            constraint_addList(ineq, y_vec as _);
            constraint_addConstantList(ineq, const_vec as _);

            instance_addConstraint(instance, leq);
            instance_addConstraint(instance, geq);
            instance_addConstraint(instance, ineq);

            let res = runMinion(
                ctx,
                options,
                args,
                instance,
                Some(hello_from_rust),
                std::ptr::null_mut(),
            );

            // does it get this far?
            assert_eq!(res, 0);

            // test if solutions are correct
            assert_eq!(X_VAL.load(Ordering::Relaxed), 1);
            assert_eq!(Y_VAL.load(Ordering::Relaxed), 2);
            assert_eq!(Z_VAL.load(Ordering::Relaxed), 1);

            minion_freeContext(ctx);
        }
    }

    #[test]
    fn midsearch_child_runner() {
        let Ok(scenario_raw) = std::env::var(MIDSEARCH_SCENARIO_ENV) else {
            return;
        };
        let scenario = MidsearchScenario::from_env_value(&scenario_raw)
            .expect("invalid value for MINION_MIDSEARCH_SCENARIO");

        let outcome = unsafe { run_midsearch_scenario_once(scenario) };

        assert_eq!(
            outcome.run_code, MinionResult_MINION_OK,
            "runMinion failed in child with outcome={outcome:#?}"
        );
        assert!(
            outcome.callback_count >= 1,
            "callback was never called; outcome={outcome:#?}"
        );
        assert!(
            outcome.added_var_ok,
            "minion_newVarMidsearch failed in callback; outcome={outcome:#?}"
        );
        assert!(
            outcome.looked_up_new_var_ok,
            "minion_getVarByName for new variable failed; outcome={outcome:#?}"
        );

        if matches!(scenario, MidsearchScenario::AddVarThenAddEqConstraint) {
            assert_eq!(
                outcome.add_constraint_result,
                Some(MinionResult_MINION_OK),
                "mid-search eq constraint using fresh variable failed; outcome={outcome:#?}"
            );
        }
    }

    #[test]
    #[ignore = "diagnostic stress test; runs child processes to detect crashes while mutating model mid-search"]
    fn midsearch_variable_addition_stress() {
        let scenarios = [
            MidsearchScenario::AddVarOnly,
            MidsearchScenario::AddVarThenAddEqConstraint,
        ];
        let iterations = 20usize;
        let mut any_failures = false;

        for scenario in scenarios {
            let mut stats = SpawnStats::default();

            for _ in 0..iterations {
                let status = run_midsearch_child(scenario);
                classify_status(&status, &mut stats);
            }

            eprintln!(
                "midsearch scenario={} iterations={} stats={stats:?}",
                scenario.as_env_value(),
                iterations
            );

            if stats.nonzero > 0 {
                any_failures = true;
            }
        }

        assert!(
            !any_failures,
            "at least one subprocess crashed/failed in mid-search variable-addition stress run"
        );
    }

    #[test]
    fn midsearch_reify_newvar_issue_child_runner() {
        let outcome = unsafe { run_issue_scenario_once() };

        assert_eq!(
            outcome.run_code, MinionResult_MINION_OK,
            "runMinion failed unexpectedly in issue child with outcome={outcome:#?}"
        );
        assert!(
            outcome.callback_count >= 1,
            "issue callback was never called; outcome={outcome:#?}"
        );
    }

    #[test]
    #[ignore = "diagnostic reproducer for Minion segfault with mid-search reify on freshly-added bool vars"]
    fn midsearch_reify_newvar_issue_reproducer() {
        let status = run_issue_child();

        #[cfg(unix)]
        assert_eq!(
            status_signal(&status),
            Some(11),
            "expected SIGSEGV from issue reproducer child, got status={status:?}"
        );

        #[cfg(not(unix))]
        assert!(
            !status.success(),
            "expected child failure on issue reproducer, got status={status:?}"
        );
    }
}
