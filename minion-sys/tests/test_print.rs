use minion_sys::ast::{Constant, Constraint, Model, Var, VarDomain};
use minion_sys::print::write_minion_file;

fn printed(model: &Model) -> String {
    let mut output = Vec::new();
    write_minion_file(&mut output, model).unwrap();
    String::from_utf8(output).unwrap()
}

#[test]
fn variable_declarations_and_search_order() {
    let mut model = Model::new();
    for (name, domain) in [
        ("d", VarDomain::Discrete(-2, 3)),
        ("b", VarDomain::Bound(-1, 1)),
        ("s", VarDomain::SparseBound(vec![-5, 0, 7])),
        ("a", VarDomain::Bool),
    ] {
        model.named_variables.add_var(name.into(), domain).unwrap();
    }
    model
        .named_variables
        .add_aux_var("aux".into(), VarDomain::Bool)
        .unwrap();
    let output = printed(&model);
    assert!(
        output.contains(
            "DISCRETE d {-2..3}\nBOUND b {-1..1}\nSPARSEBOUND s { -5, 0, 7 }\nBOOL a\nBOOL aux\n"
        ),
        "{output}"
    );
    assert!(
        output.contains("VARORDER STATIC [d,b,s,a]\nPRINT [[d,b,s,a,aux]]\n"),
        "{output}"
    );
}

#[test]
fn inline_tuples_and_nested_constraint_lists() {
    let x = Var::NameRef("x".into());
    let table = Constraint::Table(
        vec![x],
        vec![vec![Constant::Integer(-1)], vec![Constant::Bool(true)]],
    );
    assert_eq!(table.to_string(), "table([x],{<-1>,<1>})");
    assert_eq!(
        Constraint::WatchedAnd(vec![table, Constraint::True]).to_string(),
        "watched-and({table([x],{<-1>,<1>}),true()})"
    );
    assert_eq!(Constraint::False.to_string(), "false()");
}

#[test]
fn named_tuple_tables_are_written() {
    let mut model = Model::new();
    model
        .add_tuple_table(
            "t".into(),
            vec![vec![Constant::Integer(-1), Constant::Integer(2)]],
        )
        .unwrap();
    assert!(printed(&model).contains("**TUPLELIST**\nt 1 2\n-1 2\n"));
}

#[test]
fn nested_short_tuple_tables_are_written() {
    let mut model = Model::new();
    let x = Var::NameRef("x".into());
    model
        .named_variables
        .add_var("x".into(), VarDomain::Bool)
        .unwrap();
    model.constraints.push(Constraint::WatchedOr(vec![
        Constraint::ShortStr2(vec![x.clone()], vec![vec![(0, Constant::Bool(true))]]),
        Constraint::Reify(
            Box::new(Constraint::HaggisGac(vec![x.clone()], vec![vec![]])),
            x,
        ),
    ]));
    let output = printed(&model);
    assert!(
        output.contains(
            "**SHORTTUPLELIST**\n_minion_sys_short_0 1\n[(0,1)]\n_minion_sys_short_1 1\n[]\n"
        ),
        "{output}"
    );
    assert!(output.contains("watched-or({shortstr2([x],_minion_sys_short_0),reify(haggisgac([x],_minion_sys_short_1),x)})"), "{output}");
}
