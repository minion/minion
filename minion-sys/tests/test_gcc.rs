use minion_sys::ast::{Constant, Constraint, Model, Var, VarDomain};
use minion_sys::error::MinionError;

#[test]
#[allow(clippy::panic_in_result_fn)]
fn test_gcc_constraint() -> Result<(), MinionError> {
    let mut model = Model::new();

    model
        .named_variables
        .add_var(String::from("x"), VarDomain::Discrete(1, 2));
    model
        .named_variables
        .add_var(String::from("y"), VarDomain::Discrete(1, 2));
    model
        .named_variables
        .add_var(String::from("count_1"), VarDomain::Discrete(1, 1));
    model
        .named_variables
        .add_var(String::from("count_2"), VarDomain::Discrete(1, 1));

    model.constraints.push(Constraint::Gcc(
        vec![
            Var::NameRef(String::from("x")),
            Var::NameRef(String::from("y")),
        ],
        vec![Constant::Integer(1), Constant::Integer(2)],
        vec![
            Var::NameRef(String::from("count_1")),
            Var::NameRef(String::from("count_2")),
        ],
    ));

    let mut sols_counter = 0u32;
    minion_sys::run_minion(
        model,
        Box::new(|_| {
            sols_counter += 1;
            true
        }),
    )?;

    assert_eq!(sols_counter, 2);
    Ok(())
}
