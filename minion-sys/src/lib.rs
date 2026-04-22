//! This crate provides low level Rust bindings to the [Minion](https://github.com/minion/minion)
//! constraint solver.
//!
//! # Examples
//!
//! Consider the following Minion problem:
//!
//! ```plaintext
//! MINION 3
//! **VARIABLES**
//! DISCRETE x #
//! {1..3}
//! DISCRETE y #
//! {2..4}
//! DISCRETE z #
//! {1..5}
//! **SEARCH**
//! PRINT[[x],[y],[z]]
//! VARORDER STATIC [x, y, z]
//! **CONSTRAINTS**
//! sumleq([x,y,z],4)
//! ineq(x, y, -1)
//! **EOF**
//! ```
//!
//! This can be solved in Rust like so:
//!
//! ```
//! use minion_sys::ast::*;
//! use minion_sys::run_minion;
//! use std::collections::HashMap;
//!
//! // Collect solutions using a closure — no globals needed.
//! let mut all_solutions: Vec<HashMap<VarName,Constant>> = vec![];
//!
//! let callback: Box<dyn FnMut(HashMap<VarName,Constant>) -> bool> =
//!     Box::new(|solutions| {
//!         all_solutions.push(solutions);
//!         true
//!     });
//!
//! // Build and run the model.
//! let mut model = Model::new();
//! model
//!     .named_variables
//!     .add_var("x".to_owned(), VarDomain::Bound(1, 3));
//! model
//!     .named_variables
//!     .add_var("y".to_owned(), VarDomain::Bound(2, 4));
//! model
//!     .named_variables
//!     .add_var("z".to_owned(), VarDomain::Bound(1, 5));
//!
//! let leq = Constraint::SumLeq(
//!     vec![
//!         Var::NameRef("x".to_owned()),
//!         Var::NameRef("y".to_owned()),
//!         Var::NameRef("z".to_owned()),
//!     ],
//!     Var::ConstantAsVar(4),
//! );
//!
//! let geq = Constraint::SumGeq(
//!     vec![
//!         Var::NameRef("x".to_owned()),
//!         Var::NameRef("y".to_owned()),
//!         Var::NameRef("z".to_owned()),
//!     ],
//!     Var::ConstantAsVar(4),
//! );
//!
//! let ineq = Constraint::Ineq(
//!     Var::NameRef("x".to_owned()),
//!     Var::NameRef("y".to_owned()),
//!     Constant::Integer(-1),
//! );
//!
//! model.constraints.push(leq);
//! model.constraints.push(geq);
//! model.constraints.push(ineq);
//!
//! let _solver_ctx = run_minion(model, callback).expect("Error occurred");
//!
//! let solution_set_1 = &all_solutions[0];
//! let x1 = solution_set_1.get("x").unwrap();
//! let y1 = solution_set_1.get("y").unwrap();
//! let z1 = solution_set_1.get("z").unwrap();
//!
//! assert_eq!(all_solutions.len(),1);
//! assert_eq!(*x1,Constant::Integer(1));
//! assert_eq!(*y1,Constant::Integer(2));
//! assert_eq!(*z1,Constant::Integer(1));
//! ```
//!
//! ## `PRINT` and `VARORDER`
//!
//! These bindings have no replacement for Minion's `PRINT` and `VARORDER` statements - any
//! variable given to the model that does not have a constant value is considered a search
//! variable. Solutions are returned through the [callback function](Callback) as a `HashMap`.

pub use run::*;

pub mod error;
mod ffi;

pub mod ast;
mod run;

mod scoped_ptr;

pub mod print;
