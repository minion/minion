//! Types used for representing Minion models in Rust.

use std::{collections::HashMap, fmt::Display};

use crate::print::{print_const_array, print_constraint_array, print_var_array};

pub type VarName = String;
pub type Tuple = Vec<Constant>;
pub type TwoVars = (Var, Var);

/// A Minion model.
#[non_exhaustive]
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Model {
    pub named_variables: SymbolTable,
    pub constraints: Vec<Constraint>,
}

impl Model {
    /// Creates an empty Minion model.
    pub fn new() -> Model {
        Model {
            named_variables: SymbolTable::new(),
            constraints: Vec::new(),
        }
    }
}

impl Default for Model {
    fn default() -> Self {
        Self::new()
    }
}

/// All supported Minion constraints.
#[non_exhaustive]
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Constraint {
    Difference(TwoVars, Var),
    Div(TwoVars, Var),
    DivUndefZero(TwoVars, Var),
    Modulo(TwoVars, Var),
    ModuloUndefZero(TwoVars, Var),
    Pow(TwoVars, Var),
    Product(TwoVars, Var),
    WeightedSumGeq(Vec<Constant>, Vec<Var>, Var),
    WeightedSumLeq(Vec<Constant>, Vec<Var>, Var),
    CheckAssign(Box<Constraint>),
    CheckGsa(Box<Constraint>),
    ForwardChecking(Box<Constraint>),
    Reify(Box<Constraint>, Var),
    ReifyImply(Box<Constraint>, Var),
    ReifyImplyQuick(Box<Constraint>, Var),
    WatchedAnd(Vec<Constraint>),
    WatchedOr(Vec<Constraint>),
    GacAllDiff(Vec<Var>),
    AllDiff(Vec<Var>),
    AllDiffMatrix(Vec<Var>, Constant),
    WatchSumGeq(Vec<Var>, Constant),
    WatchSumLeq(Vec<Var>, Constant),
    OccurrenceGeq(Vec<Var>, Constant, Constant),
    OccurrenceLeq(Vec<Var>, Constant, Constant),
    Occurrence(Vec<Var>, Constant, Var),
    LitSumGeq(Vec<Var>, Vec<Constant>, Constant),
    Gcc(Vec<Var>, Vec<Constant>, Vec<Var>),
    GccWeak(Vec<Var>, Vec<Constant>, Vec<Var>),
    LexLeqRv(Vec<Var>, Vec<Var>),
    LexLeq(Vec<Var>, Vec<Var>),
    LexLess(Vec<Var>, Vec<Var>),
    LexLeqQuick(Vec<Var>, Vec<Var>),
    LexLessQuick(Vec<Var>, Vec<Var>),
    WatchVecNeq(Vec<Var>, Vec<Var>),
    WatchVecExistsLess(Vec<Var>, Vec<Var>),
    Hamming(Vec<Var>, Vec<Var>, Constant),
    NotHamming(Vec<Var>, Vec<Var>, Constant),
    FrameUpdate(Vec<Var>, Vec<Var>, Vec<Var>, Vec<Var>, Constant),
    //HaggisGac(Vec<Var>,Vec<
    //HaggisGacStable
    //ShortStr2
    //ShortcTupleStr2
    NegativeTable(Vec<Var>, Vec<Tuple>),
    Table(Vec<Var>, Vec<Tuple>),
    GacSchema(Vec<Var>, Vec<Tuple>),
    LightTable(Vec<Var>, Vec<Tuple>),
    Mddc(Vec<Var>, Vec<Tuple>),
    NegativeMddc(Vec<Var>, Vec<Tuple>),
    Str2Plus(Vec<Var>, Var),
    Max(Vec<Var>, Var),
    Min(Vec<Var>, Var),
    NvalueGeq(Vec<Var>, Var),
    NvalueLeq(Vec<Var>, Var),
    SumLeq(Vec<Var>, Var),
    SumGeq(Vec<Var>, Var),
    Element(Vec<Var>, Var, Var),
    ElementOne(Vec<Var>, Var, Var),
    ElementUndefZero(Vec<Var>, Var, Var),
    WatchElement(Vec<Var>, Var, Var),
    WatchElementOne(Vec<Var>, Var, Var),
    WatchElementOneUndefZero(Vec<Var>, Var, Var),
    WatchElementUndefZero(Vec<Var>, Var, Var),
    WLiteral(Var, Constant),
    WNotLiteral(Var, Constant),
    WInIntervalSet(Var, Vec<Constant>),
    WInRange(Var, Vec<Constant>),
    WInset(Var, Vec<Constant>),
    WNotInRange(Var, Vec<Constant>),
    WNotInset(Var, Vec<Constant>),
    Abs(Var, Var),
    DisEq(Var, Var),
    Eq(Var, Var),
    MinusEq(Var, Var),
    GacEq(Var, Var),
    WatchLess(Var, Var),
    WatchNeq(Var, Var),
    Ineq(Var, Var, Constant),
    False,
    True,
}

#[allow(clippy::todo, unused_variables)]
impl Display for Constraint {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Constraint::Difference(_, var) => write!(f, "difference({var}"),
            Constraint::Div(_, var) => write!(f, "div({var}"),
            Constraint::DivUndefZero(_, var) => write!(f, "div_undefzero({var})"),
            Constraint::Modulo(_, var) => write!(f, "modulo({var})"),
            Constraint::ModuloUndefZero(_, var) => write!(f, "mod_undefzero({var})"),
            Constraint::Pow(_, var) => write!(f, "pow({var})"),
            Constraint::Product(_, var) => write!(f, "product({var})"),
            Constraint::WeightedSumGeq(constants, vars, var) => {
                write!(
                    f,
                    "weightedsumgeq({},{},{var})",
                    print_const_array(constants),
                    print_var_array(vars)
                )
            }
            Constraint::WeightedSumLeq(constants, vars, var) => {
                write!(
                    f,
                    "weightedsumleq({},{},{var})",
                    print_const_array(constants),
                    print_var_array(vars)
                )
            }
            Constraint::CheckAssign(constraint) => {
                todo!("don't know how to print checkassign constriant...")
            }
            Constraint::CheckGsa(constraint) => {
                todo!("don't know how to print checkgsa constraint...")
            }
            Constraint::ForwardChecking(constraint) => {
                todo!("don't know how to print forwardchecking constraint...")
            }
            Constraint::Reify(constraint, var) => write!(f, "reify({constraint},{var})"),
            Constraint::ReifyImply(constraint, var) => write!(f, "reifyimply({constraint},{var})"),
            Constraint::ReifyImplyQuick(constraint, var) => {
                write!(f, "reifyimply-quick({constraint},{var})")
            }
            Constraint::WatchedAnd(constraints) => {
                write!(f, "watched-and({})", print_constraint_array(constraints))
            }
            Constraint::WatchedOr(constraints) => {
                write!(f, "watched-or({})", print_constraint_array(constraints))
            }
            Constraint::GacAllDiff(vars) => write!(f, "gacalldiff({})", print_var_array(vars)),
            Constraint::AllDiff(vars) => write!(f, "alldiff({})", print_var_array(vars)),
            Constraint::AllDiffMatrix(vars, constant) => {
                write!(f, "alldiffmatrix({},{constant})", print_var_array(vars))
            }
            Constraint::WatchSumGeq(vars, constant) => {
                write!(f, "watchsumgeq({},{constant})", print_var_array(vars))
            }
            Constraint::WatchSumLeq(vars, constant) => {
                write!(f, "watchsumleq({},{constant})", print_var_array(vars))
            }
            Constraint::OccurrenceGeq(vars, constant, constant1) => write!(
                f,
                "occurrencegeq({},{constant},{constant1})",
                print_var_array(vars)
            ),
            Constraint::OccurrenceLeq(vars, constant, constant1) => write!(
                f,
                "occurrenceleq({},{constant},{constant1})",
                print_var_array(vars)
            ),
            Constraint::Occurrence(vars, constant, var) => {
                write!(f, "occurrence({},{constant},{var})", print_var_array(vars))
            }
            Constraint::LitSumGeq(vars, constants, constant) => write!(
                f,
                "litsumgeq({},{},{constant})",
                print_var_array(vars),
                print_const_array(constants)
            ),
            Constraint::Gcc(vars, constants, vars1) => write!(
                f,
                "gcc({},{},{})",
                print_var_array(vars),
                print_const_array(constants),
                print_var_array(vars1)
            ),
            Constraint::GccWeak(vars, constants, vars1) => write!(
                f,
                "gccweak({},{},{})",
                print_var_array(vars),
                print_const_array(constants),
                print_var_array(vars1)
            ),
            Constraint::LexLeqRv(vars, vars1) => write!(
                f,
                "lexleq[rv]({},{})",
                print_var_array(vars),
                print_var_array(vars1)
            ),
            Constraint::LexLeq(vars, vars1) => write!(
                f,
                "lexleq({},{})",
                print_var_array(vars),
                print_var_array(vars1)
            ),
            Constraint::LexLeqQuick(vars, vars1) => write!(
                f,
                "lexleq[quick]({},{})",
                print_var_array(vars),
                print_var_array(vars1)
            ),
            Constraint::LexLess(vars, vars1) => write!(
                f,
                "lexless({},{})",
                print_var_array(vars),
                print_var_array(vars1)
            ),
            Constraint::LexLessQuick(vars, vars1) => write!(
                f,
                "lexless[quick]({},{})",
                print_var_array(vars),
                print_var_array(vars1)
            ),
            Constraint::WatchVecNeq(vars, vars1) => write!(
                f,
                "watchvecneq({},{})",
                print_var_array(vars),
                print_var_array(vars1)
            ),
            Constraint::WatchVecExistsLess(vars, vars1) => {
                todo!("don't know how to print watchvecexistsless...")
            }
            Constraint::Hamming(vars, vars1, constant) => write!(
                f,
                "hamming({},{},{constant})",
                print_var_array(vars),
                print_var_array(vars1)
            ),
            Constraint::NotHamming(vars, vars1, constant) => {
                todo!("don't know how to print nothamming...")
            }
            Constraint::FrameUpdate(vars, vars1, vars2, vars3, constant) => {
                todo!("don't know how to print frame update...")
            }
            Constraint::NegativeTable(_, _)
            | Constraint::Table(_, _)
            | Constraint::GacSchema(_, _)
            | Constraint::LightTable(_, _)
            | Constraint::Mddc(_, _)
            | Constraint::Str2Plus(_, _)
            | Constraint::NegativeMddc(_, _) => {
                todo!("tuples not properly implemented yet, so can't print them...")
            }
            Constraint::Max(vars, var) => write!(f, "max({},{var})", print_var_array(vars)),
            Constraint::Min(vars, var) => write!(f, "min({},{var})", print_var_array(vars)),
            Constraint::NvalueGeq(vars, var) => {
                write!(f, "nvaluegeq({},{var})", print_var_array(vars))
            }
            Constraint::NvalueLeq(vars, var) => {
                write!(f, "nvalueleq({},{var})", print_var_array(vars))
            }
            Constraint::SumLeq(vars, var) => write!(f, "sumleq({},{var})", print_var_array(vars)),
            Constraint::SumGeq(vars, var) => write!(f, "sumgeq({},{var})", print_var_array(vars)),
            Constraint::Element(vars, var, var1) => {
                write!(f, "element({},{var},{var1})", print_var_array(vars))
            }
            Constraint::ElementOne(vars, var, var1) => {
                write!(f, "element_one({},{var},{var1})", print_var_array(vars))
            }
            Constraint::ElementUndefZero(vars, var, var1) => write!(
                f,
                "element_undefzero({},{var},{var1})",
                print_var_array(vars)
            ),
            Constraint::WatchElement(vars, var, var1) => {
                write!(f, "watchelement({},{var},{var1})", print_var_array(vars))
            }
            Constraint::WatchElementUndefZero(vars, var, var1) => write!(
                f,
                "watchelement_undefzero({},{var},{var1})",
                print_var_array(vars)
            ),
            Constraint::WatchElementOne(vars, var, var1) => write!(
                f,
                "watchelement_one({},{var},{var1})",
                print_var_array(vars)
            ),
            Constraint::WatchElementOneUndefZero(vars, var, var1) => write!(
                f,
                "watchelement_one_undefzero({},{var},{var1})",
                print_var_array(vars)
            ),
            Constraint::WLiteral(var, constant) => write!(f, "w-literal({var},{constant})"),
            Constraint::WNotLiteral(var, constant) => write!(f, "w-notliteral({var},{constant})"),
            Constraint::WInIntervalSet(var, constants) => {
                write!(f, "w-inintervalset({var},{})", print_const_array(constants))
            }
            Constraint::WInRange(var, constants) => {
                write!(f, "w-inrange({var},{})", print_const_array(constants))
            }
            Constraint::WNotInRange(var, constants) => {
                write!(f, "w-notinrange({var},{})", print_const_array(constants))
            }
            Constraint::WInset(var, constants) => {
                write!(f, "w-inset({var},{})", print_const_array(constants))
            }
            Constraint::WNotInset(var, constants) => {
                write!(f, "w-notinset({var},{})", print_const_array(constants))
            }
            Constraint::Abs(var, var1) => write!(f, "abs({var},{var1})"),
            Constraint::DisEq(var, var1) => write!(f, "diseq({var},{var1})"),
            Constraint::Eq(var, var1) => write!(f, "eq({var},{var1})"),
            Constraint::MinusEq(var, var1) => write!(f, "minuseq({var},{var1})"),
            Constraint::GacEq(var, var1) => todo!("don't know how to print gaceq..."),
            Constraint::WatchLess(var, var1) => write!(f, "watchless({var},{var1})"),
            Constraint::WatchNeq(var, var1) => write!(f, "watchneq({var},{var1})"),
            Constraint::Ineq(var, var1, constant) => write!(f, "ineq({var},{var1},{constant})"),
            Constraint::False => write!(f, "false"),
            Constraint::True => write!(f, "true"),
        }
    }
}

/// Representation of a Minion Variable.
///
/// A variable can either be a named variable, or an anomynous "constant as a variable".
///
/// The latter is not stored in the symbol table, or counted in Minions internal list of all
/// variables, but is used to allow the use of a constant in the place of a variable in a
/// constraint.
#[derive(Debug, Clone, Eq, PartialEq)]
pub enum Var {
    NameRef(VarName),
    ConstantAsVar(i32),
}

impl Display for Var {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Var::NameRef(n) => write!(f, "{n}"),
            Var::ConstantAsVar(c) => write!(f, "{c}"),
        }
    }
}
/// Representation of a Minion constant.
#[non_exhaustive]
#[derive(Debug, Eq, PartialEq, Clone, Copy)]
pub enum Constant {
    Bool(bool),
    Integer(i32),
}

impl Display for Constant {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Constant::Bool(true) => write!(f, "1"),
            Constant::Bool(false) => write!(f, "0"),
            Constant::Integer(i) => write!(f, "{i}"),
        }
    }
}

/// Representation of variable domains.
#[derive(Debug, Copy, Clone, Eq, PartialEq)]
#[non_exhaustive]
pub enum VarDomain {
    Bound(i32, i32),
    Discrete(i32, i32),
    // FIXME: should be a list of i32!
    // we don't use this anyways, so commenting out for now...
    // SparseBound(i32,i32),
    Bool,
}

#[derive(Debug, Clone, Eq, PartialEq)]
#[non_exhaustive]
/// Stores all named variables in a Minion model alongside their domains.
///
/// Named variables referenced in [constraints](Constraint) must be in the symbol table for the
/// model to be valid. In the future, this will raise some sort of type error.
pub struct SymbolTable {
    table: HashMap<VarName, VarDomain>,

    // order of all variables
    var_order: Vec<VarName>,

    // search order
    search_var_order: Vec<VarName>,
}

impl SymbolTable {
    fn new() -> SymbolTable {
        SymbolTable {
            table: HashMap::new(),
            var_order: Vec::new(),
            search_var_order: Vec::new(),
        }
    }

    /// Creates a new search variable and adds it to the symbol table.
    ///
    /// # Returns
    ///
    /// If a variable already exists with the given name, `None` is returned.
    pub fn add_var(&mut self, name: VarName, vartype: VarDomain) -> Option<()> {
        if self.table.contains_key(&name) {
            return None;
        }

        self.table.insert(name.clone(), vartype);
        self.var_order.push(name.clone());
        self.search_var_order.push(name);

        Some(())
    }

    /// Creates a new auxiliary variable and adds it to the symbol table.
    ///
    /// This variable will excluded from Minions search and printing order.
    ///
    /// # Returns
    ///
    /// If a variable already exists with the given name, `None` is returned.
    pub fn add_aux_var(&mut self, name: VarName, vartype: VarDomain) -> Option<()> {
        if self.table.contains_key(&name) {
            return None;
        }

        self.table.insert(name.clone(), vartype);
        self.var_order.push(name);

        Some(())
    }

    /// Gets the domain of a named variable.
    ///
    /// # Returns
    ///
    /// `None` if no variable is known by that name.
    pub fn get_vartype(&self, name: VarName) -> Option<VarDomain> {
        self.table.get(&name).cloned()
    }

    /// Gets the canonical ordering of all variables.
    pub fn get_variable_order(&self) -> Vec<VarName> {
        self.var_order.clone()
    }

    /// Gets the canonical ordering of search variables (i.e excluding aux vars).
    pub fn get_search_variable_order(&self) -> Vec<VarName> {
        self.search_var_order.clone()
    }

    pub fn contains(&self, name: VarName) -> bool {
        self.table.contains_key(&name)
    }
}
