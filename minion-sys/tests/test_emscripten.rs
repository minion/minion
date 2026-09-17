//! The same sequential exception/recovery test also runs natively.
use minion_sys::ast::{Constraint, Model, Var, VarDomain};
use minion_sys::{
    error::{MinionError, RuntimeError},
    run_minion,
};

fn model() -> Model {
    let mut m = Model::new();
    m.named_variables
        .add_var("x".into(), VarDomain::Discrete(1, 3));
    m
}

#[test]
fn early_stop_error_and_context_destruction() {
    for _ in 0..5 {
        let mut count = 0;
        let ctx = run_minion(
            model(),
            Box::new(|_| {
                count += 1;
                false
            }),
        )
        .unwrap();
        assert_eq!(count, 1);
        #[cfg(target_os = "emscripten")]
        {
            for name in ["TotalTime", "TotalSystemTime", "MaxRSSkB"] {
                assert_eq!(
                    ctx.get_from_table(name.into()).as_deref(),
                    Some("unavailable")
                );
            }
            let wall: f64 = ctx
                .get_from_table("TotalWallTime".into())
                .unwrap()
                .parse()
                .unwrap();
            assert!(wall.is_finite() && wall >= 0.0);
        }
        drop(ctx);
        // This reaches the C++ getSymbol exception handler, not Rust validation.
        let mut invalid = model();
        invalid.constraints.push(Constraint::Eq(
            Var::NameRef("missing".into()),
            Var::ConstantAsVar(1),
        ));
        assert!(matches!(
            run_minion(invalid, Box::new(|_| true)),
            Err(MinionError::RuntimeError(RuntimeError::ParseError(_)))
        ));
        let mut count = 0;
        drop(
            run_minion(
                model(),
                Box::new(|_| {
                    count += 1;
                    true
                }),
            )
            .unwrap(),
        );
        assert_eq!(count, 3);
    }
}

#[cfg(target_os = "emscripten")]
#[test]
fn unsupported_modes_are_recoverable() {
    use minion_sys::{
        RunOptions, TimeLimit, run_minion_parallel, run_minion_with_options, run_minion_work_steal,
    };
    let invalid = |r| {
        assert!(matches!(
            r,
            Err(MinionError::RuntimeError(RuntimeError::InvalidArgument(_)))
        ))
    };
    invalid(run_minion_parallel(1, model(), Box::new(|_| true)));
    invalid(run_minion_work_steal(1, model(), Box::new(|_| true)).map(|_| ()));
    for is_cpu_time in [false, true] {
        invalid(
            run_minion_with_options(
                model(),
                RunOptions {
                    time_limit: Some(TimeLimit {
                        seconds: 1,
                        is_cpu_time,
                    }),
                    ..Default::default()
                },
                Box::new(|_| true),
            )
            .map(|_| ()),
        );
    }
    drop(run_minion(model(), Box::new(|_| true)).unwrap());
}

#[cfg(target_os = "emscripten")]
#[allow(warnings)]
mod raw {
    include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
}

#[cfg(target_os = "emscripten")]
#[test]
fn raw_process_and_thread_options_are_rejected() {
    unsafe {
        for mode in 0..5 {
            let ctx = raw::minion_newContext();
            let options = raw::searchOptions_new();
            let method = raw::searchMethod_new();
            let instance = raw::instance_new();
            match mode {
                0 => (*options).parallel = true,
                1 => (*options).parallelPreprocessCores = 2,
                2 => (*options).numParallelThreads = 1,
                3 => (*options).numWorkStealThreads = 1,
                _ => (*options).parallelWorkStealPortfolio = true,
            }
            assert_eq!(
                raw::runMinion(ctx, options, method, instance, None, std::ptr::null_mut()),
                raw::MinionResult_MINION_INVALID_ARGUMENT
            );
            raw::instance_free(instance);
            raw::searchMethod_free(method);
            raw::searchOptions_free(options);
            raw::minion_freeContext(ctx);
        }
    }
}
