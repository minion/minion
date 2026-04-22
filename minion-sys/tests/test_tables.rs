// There are three letter x, y, and z, which are each integers between 1 and 3 inclusive, where x + y = z

use minion_sys::ast::{Constant, Constraint, Model, Var, VarDomain};
use minion_sys::error::MinionError;

#[test]
#[allow(clippy::panic_in_result_fn)]
fn test_table_constraint() -> Result<(), MinionError> {
    let mut model = Model::new();

    // Declares variables (x, y, z), explicitly setting the integers to be between 1 and 3
    model
        .named_variables
        .add_var(String::from("x"), VarDomain::Discrete(1, 3));
    model
        .named_variables
        .add_var(String::from("y"), VarDomain::Discrete(1, 3));
    model
        .named_variables
        .add_var(String::from("z"), VarDomain::Discrete(1, 3));

    // Defines the table data (a list of tuples)
    let table_data = vec![
        vec![
            Constant::Integer(1),
            Constant::Integer(1),
            Constant::Integer(2),
        ],
        vec![
            Constant::Integer(1),
            Constant::Integer(2),
            Constant::Integer(3),
        ],
        vec![
            Constant::Integer(2),
            Constant::Integer(1),
            Constant::Integer(3),
        ],
    ];
    // Builds the Table constraint
    let vars = vec![
        Var::NameRef(String::from("x")),
        Var::NameRef(String::from("y")),
        Var::NameRef(String::from("z")),
    ];

    model.constraints.push(Constraint::Table(vars, table_data));

    // Runs the solver via the Minion interface
    let mut sols_counter = 0u32;
    let solver_ctx = minion_sys::run_minion(
        model,
        Box::new(|_| {
            sols_counter += 1;
            true
        }),
    )?;

    // Asserts that we found exactly 3 solutions (one for each row in the table)
    assert_eq!(sols_counter, 3);
    assert_ne!(solver_ctx.get_from_table("Nodes".into()), None);
    Ok(())
}
