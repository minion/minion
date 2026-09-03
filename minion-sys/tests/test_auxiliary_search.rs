use std::cell::RefCell;
use std::rc::Rc;

use minion_sys::ast::{Constant, Constraint, Model, Var, VarDomain};

#[test]
fn auxiliary_variables_are_assigned_before_a_solution_is_reported() {
    let mut model = Model::new();
    model
        .named_variables
        .add_var("x".to_owned(), VarDomain::Bool)
        .unwrap();
    model
        .named_variables
        .add_aux_var("a".to_owned(), VarDomain::Discrete(0, 1))
        .unwrap();
    model
        .named_variables
        .add_aux_var("b".to_owned(), VarDomain::Discrete(0, 1))
        .unwrap();

    model.constraints.push(Constraint::Eq(
        Var::NameRef("x".to_owned()),
        Var::ConstantAsVar(0),
    ));
    let auxiliaries = vec![Var::NameRef("a".to_owned()), Var::NameRef("b".to_owned())];
    model.constraints.push(Constraint::SumLeq(
        auxiliaries.clone(),
        Var::ConstantAsVar(1),
    ));
    model
        .constraints
        .push(Constraint::SumGeq(auxiliaries, Var::ConstantAsVar(1)));

    let solutions = Rc::new(RefCell::new(Vec::new()));
    let captured = Rc::clone(&solutions);
    minion_sys::run_minion(
        model,
        Box::new(move |solution| {
            captured.borrow_mut().push(solution);
            true
        }),
    )
    .unwrap();

    let solutions = solutions.borrow();
    assert_eq!(
        solutions.len(),
        1,
        "auxiliary assignments must not duplicate solutions"
    );
    let solution = &solutions[0];
    let Constant::Integer(a) = solution["a"] else {
        panic!("expected integer auxiliary")
    };
    let Constant::Integer(b) = solution["b"] else {
        panic!("expected integer auxiliary")
    };
    assert_eq!(a + b, 1);
}
