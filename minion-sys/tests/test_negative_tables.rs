// There are three letter x, y, and z, which are each integers between 0 and 1 inclusive, where its binary and accepts every binary triple except all zeros and all ones

use minion_sys::ast::{Constant, Constraint, Model, Var, VarDomain};
use minion_sys::error::MinionError;

#[test]
#[allow(clippy::panic_in_result_fn)]
fn test_negative_table_constraint() -> Result<(), MinionError> {
    let mut model = Model::new();

    // Declares variables (x, y, z), explicitly setting the integers to be between 0 and 1
    model
        .named_variables
        .add_var(String::from("x"), VarDomain::Discrete(0, 1));
    model
        .named_variables
        .add_var(String::from("y"), VarDomain::Discrete(0, 1));
    model
        .named_variables
        .add_var(String::from("z"), VarDomain::Discrete(0, 1));

    // Defines the forbidden table data (a list of tuples)
    let forbidden_table_data = vec![
        vec![
            Constant::Integer(1),
            Constant::Integer(1),
            Constant::Integer(1),
        ],
        vec![
            Constant::Integer(0),
            Constant::Integer(0),
            Constant::Integer(0),
        ],
    ];
    // Builds the Table constraint
    let vars = vec![
        Var::NameRef(String::from("x")),
        Var::NameRef(String::from("y")),
        Var::NameRef(String::from("z")),
    ];

    model
        .constraints
        .push(Constraint::NegativeTable(vars, forbidden_table_data));

    // Runs the solver via the Minion interface
    let mut sols_counter = 0u32;
    let solver_ctx = minion_sys::run_minion(
        model,
        Box::new(|_| {
            sols_counter += 1;
            true
        }),
    )?;

    // Asserts that we found exactly 6 solutions
    assert_eq!(sols_counter, 6);
    assert_ne!(solver_ctx.get_from_table("Nodes".into()), None);
    Ok(())
}
