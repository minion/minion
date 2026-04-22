//! based on this minion test file:
//! https://github.com/minion/minion/blob/main/test_instances/test_watchedor_reifyimply_1.minion
//!
//! ```text
//! #TEST SOLCOUNT 7
//! # Recursive test
//! MINION 3
//!
//! **VARIABLES**
//! BOOL a
//! BOOL b
//! BOOL c
//!
//! **CONSTRAINTS**
//!
//! reifyimply(watched-or({w-inset(a,[1]),w-inset(b,[0])}), c)
//!
//! **EOF**
//! ```

use minion_sys::ast::{Constant, Constraint, Model, Var, VarDomain};
use minion_sys::error::MinionError;
#[test]
#[allow(clippy::panic_in_result_fn)]
fn test_watchedor_reifyimply_1() -> Result<(), MinionError> {
    let mut model = Model::new();
    model
        .named_variables
        .add_var(String::from("a"), VarDomain::Bool);
    model
        .named_variables
        .add_var(String::from("b"), VarDomain::Bool);
    model
        .named_variables
        .add_var(String::from("c"), VarDomain::Bool);

    model.constraints.push(Constraint::ReifyImply(
        Box::new(Constraint::WatchedOr(vec![
            Constraint::WInset(Var::NameRef(String::from("a")), vec![Constant::Bool(true)]),
            Constraint::WInset(Var::NameRef(String::from("b")), vec![Constant::Bool(false)]),
        ])),
        Var::NameRef(String::from("c")),
    ));

    let mut sols_counter = 0i32;
    let solver_ctx = minion_sys::run_minion(
        model,
        Box::new(|_| {
            sols_counter += 1;
            true
        }),
    )?;

    assert_eq!(sols_counter, 7);
    assert_ne!(solver_ctx.get_from_table("Nodes".into()), None);
    Ok(())
}
