//! Types used for representing Minion models in Rust.

use std::{collections::HashMap, fmt::Display};

use crate::print::{print_const_array, print_constraint_array, print_tuple_array, print_var_array};

/// The name of a variable in a Minion model.
pub type VarName = String;
/// A tuple of constants, used in extensional (table) constraints.
pub type Tuple = Vec<Constant>;
/// A pair of variables, used by three-operand arithmetic constraints.
pub type TwoVars = (Var, Var);

/// A Minion model.
#[non_exhaustive]
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Model {
    pub named_variables: SymbolTable,
    pub constraints: Vec<Constraint>,
    /// Named tuple tables. Needed by tuple-based constraints that
    /// reference a table by name rather than carry tuples inline
    /// (most notably `CT_STR` / `Str2Plus`). Register a table with
    /// [`Model::add_tuple_table`], then reference it from a
    /// constraint via `Var::NameRef(table_name)`.
    ///
    /// Storage order is preserved so the order in which tables are
    /// installed into the `CSPInstance` matches insertion order.
    pub tuple_tables: Vec<(String, Vec<Tuple>)>,
}

impl Model {
    /// Creates an empty Minion model.
    pub fn new() -> Model {
        Model {
            named_variables: SymbolTable::new(),
            constraints: Vec::new(),
            tuple_tables: Vec::new(),
        }
    }

    /// Registers a named tuple table on the model. The table is
    /// copied into Minion's `CSPInstance` at solve time so constraints
    /// like `Str2Plus(vars, Var::NameRef(name))` can look it up.
    ///
    /// Returns `None` if a table with that name is already registered.
    pub fn add_tuple_table(&mut self, name: String, tuples: Vec<Tuple>) -> Option<()> {
        if self.tuple_tables.iter().any(|(n, _)| n == &name) {
            return None;
        }
        self.tuple_tables.push((name, tuples));
        Some(())
    }
}

impl Default for Model {
    fn default() -> Self {
        Self::new()
    }
}

/// All supported Minion constraints.
///
/// Each variant corresponds to a Minion input-language constraint (see the
/// [Minion constraint reference](https://minion-solver.readthedocs.io/en/latest/usage/constraints.html)).
/// Variants are named to match their Minion input names as closely as Rust's
/// naming conventions permit.
///
/// # Argument conventions
///
/// - `Vec<Var>` is a list of variables.
/// - `Var` alone is a single variable (or `Var::ConstantAsVar` for a constant in
///   variable position).
/// - `Vec<Constant>` is a list of integer constants (e.g. weights, values).
/// - `Constant` alone is a single integer constant.
/// - `(Var, Var)` in the `TwoVars` position means two variables.
/// - `Vec<Tuple>` is a list of tuples (a `Tuple` is a `Vec<Constant>`).
/// - `Box<Constraint>` means the variant wraps another constraint (reification,
///   nested boolean operators, etc.).
#[non_exhaustive]
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Constraint {
    // --- Arithmetic: three-operand (x, y, z) ---
    /// `difference(x, y, z)` — `z = |x - y|`. Bounds consistency.
    Difference(TwoVars, Var),
    /// `div(x, y, z)` — `z = floor(x / y)`. False when `y = 0`.
    Div(TwoVars, Var),
    /// `div_undefzero(x, y, z)` — like `Div`, but true (not false) when `y = 0`.
    DivUndefZero(TwoVars, Var),
    /// `modulo(x, y, z)` — `z = x % y`. False when `y = 0`.
    Modulo(TwoVars, Var),
    /// `modulo_undefzero(x, y, z)` — like `Modulo`, but true when `y = 0`.
    ModuloUndefZero(TwoVars, Var),
    /// `pow(x, y, z)` — `z = x^y`. False when `y < 0` (with exceptions for ±1).
    Pow(TwoVars, Var),
    /// `product(x, y, z)` — `z = x * y`.
    Product(TwoVars, Var),

    // --- Arithmetic: weighted sums ---
    /// `weightedsumgeq(weights, vars, total)` — dot product ≥ total.
    WeightedSumGeq(Vec<Constant>, Vec<Var>, Var),
    /// `weightedsumleq(weights, vars, total)` — dot product ≤ total.
    WeightedSumLeq(Vec<Constant>, Vec<Var>, Var),

    // --- Nested / meta constraints ---
    /// `check[assign](c)` — internal: checks `c` after each assignment.
    CheckAssign(Box<Constraint>),
    /// `check[gsa](c)` — internal: checks `c` via GSA (generalised-scope-all).
    CheckGsa(Box<Constraint>),
    /// `forwardchecking(c)` — internal: run `c` in forward-checking mode.
    ForwardChecking(Box<Constraint>),
    /// `reify(c, r)` — `r = 1` iff `c` is satisfied. `r` must be 0/1.
    Reify(Box<Constraint>, Var),
    /// `reifyimply(c, r)` — if `r = 1` then `c` must hold.
    ReifyImply(Box<Constraint>, Var),
    /// `reifyimply-quick(c, r)` — like `ReifyImply` but only checks `c` when `r` is assigned.
    ReifyImplyQuick(Box<Constraint>, Var),
    /// `watched-and({c1, ..., cn})` — all `ci` must be true.
    WatchedAnd(Vec<Constraint>),
    /// `watched-or({c1, ..., cn})` — at least one `ci` must be true.
    WatchedOr(Vec<Constraint>),

    // --- All-different / cardinality / counting ---
    /// `gacalldiff(vars)` — all variables in `vars` take distinct values. GAC.
    GacAllDiff(Vec<Var>),
    /// `alldiff(vars)` — like `GacAllDiff` but weaker (clique of ≠ constraints).
    AllDiff(Vec<Var>),
    /// `alldiffmatrix(matrix, dim)` — Latin-square condition on a `dim×dim` matrix.
    AllDiffMatrix(Vec<Var>, Constant),

    // --- SAT-style sums (Booleans only) ---
    /// `watchsumgeq(vars, c)` — sum of 0/1 `vars` ≥ `c`. Fast for small `c`.
    WatchSumGeq(Vec<Var>, Constant),
    /// `watchsumleq(vars, c)` — sum of 0/1 `vars` ≤ `c`. Fast for `c` close to len.
    WatchSumLeq(Vec<Var>, Constant),

    // --- Occurrence ---
    /// `occurrencegeq(vars, val, count)` — `val` occurs ≥ `count` times. Constants only.
    OccurrenceGeq(Vec<Var>, Constant, Constant),
    /// `occurrenceleq(vars, val, count)` — `val` occurs ≤ `count` times. Constants only.
    OccurrenceLeq(Vec<Var>, Constant, Constant),
    /// `occurrence(vars, val, count)` — `val` occurs exactly `count` times.
    Occurrence(Vec<Var>, Constant, Var),

    // --- Literal sum ---
    /// `litsumgeq(vars, literals, c)` — at least `c` positions where `vars[i] == literals[i]`.
    LitSumGeq(Vec<Var>, Vec<Constant>, Constant),

    // --- Global cardinality ---
    /// `gcc(vars, values, caps)` — each value in `values` appears exactly `caps[i]` times. Strong propagation.
    Gcc(Vec<Var>, Vec<Constant>, Vec<Var>),
    /// `gccweak(vars, values, caps)` — like `Gcc` but weaker, faster propagation on `caps`.
    GccWeak(Vec<Var>, Vec<Constant>, Vec<Var>),

    // --- Lexicographic ordering ---
    /// `lexleq[rv](a, b)` — `a ≤ b` lexicographically. GAC, handles repeated variables.
    LexLeqRv(Vec<Var>, Vec<Var>),
    /// `lexleq(a, b)` — `a ≤ b` lexicographically. GAC, assumes no repeated variables.
    LexLeq(Vec<Var>, Vec<Var>),
    /// `lexless(a, b)` — `a < b` lexicographically. GAC, assumes no repeated variables.
    LexLess(Vec<Var>, Vec<Var>),
    /// `lexleq[quick](a, b)` — `a ≤ b` lexicographically. Fast but weaker propagation.
    LexLeqQuick(Vec<Var>, Vec<Var>),
    /// `lexless[quick](a, b)` — `a < b` lexicographically. Fast but weaker propagation.
    LexLessQuick(Vec<Var>, Vec<Var>),

    // --- Vector comparison ---
    /// `watchvecneq(a, b)` — vectors `a` and `b` differ in at least one position.
    WatchVecNeq(Vec<Var>, Vec<Var>),
    /// `watchvecexists_less(a, b)` — there exists `i` such that `a[i] < b[i]`.
    WatchVecExistsLess(Vec<Var>, Vec<Var>),
    /// `hamming(a, b, c)` — Hamming distance between `a` and `b` ≥ `c`.
    Hamming(Vec<Var>, Vec<Var>, Constant),
    /// `not-hamming(a, b, c)` — Hamming distance between `a` and `b` < `c`.
    NotHamming(Vec<Var>, Vec<Var>, Constant),

    // --- Internal ---
    /// `frameupdate(...)` — internal frame-update constraint.
    FrameUpdate(Vec<Var>, Vec<Var>, Vec<Var>, Vec<Var>, Constant),

    // --- Table (extensional) constraints ---
    /// `negativetable(vars, tuples)` — disallows the given tuples. GAC.
    NegativeTable(Vec<Var>, Vec<Tuple>),
    /// `table(vars, tuples)` — allows only the given tuples. GAC.
    Table(Vec<Var>, Vec<Tuple>),
    /// `gacschema(vars, tuples)` — like `Table` with an alternative GAC algorithm.
    GacSchema(Vec<Var>, Vec<Tuple>),
    /// `lighttable(vars, tuples)` — stateless variant of `Table`, faster for small constraints.
    LightTable(Vec<Var>, Vec<Tuple>),
    /// `mddc(vars, tuples)` — MDDC propagator (multi-valued decision diagram). GAC.
    Mddc(Vec<Var>, Vec<Tuple>),
    /// `negativemddc(vars, tuples)` — negative MDDC. GAC on disallowed tuples.
    NegativeMddc(Vec<Var>, Vec<Tuple>),
    /// `str2plus(vars, table_ref)` — STR2+ algorithm. The second argument is a
    /// `Var::NameRef` referencing a named tuple table registered with
    /// [`Model::add_tuple_table`].
    Str2Plus(Vec<Var>, Var),

    // --- Min/max / nvalue ---
    /// `max(vars, x)` — `x` equals the maximum value in `vars`.
    Max(Vec<Var>, Var),
    /// `min(vars, x)` — `x` equals the minimum value in `vars`.
    Min(Vec<Var>, Var),
    /// `nvaluegeq(vars, x)` — at least `x` distinct values appear in `vars`.
    NvalueGeq(Vec<Var>, Var),
    /// `nvalueleq(vars, x)` — at most `x` distinct values appear in `vars`.
    NvalueLeq(Vec<Var>, Var),

    // --- Sums ---
    /// `sumleq(vars, x)` — sum of `vars` ≤ `x`.
    SumLeq(Vec<Var>, Var),
    /// `sumgeq(vars, x)` — sum of `vars` ≥ `x`.
    SumGeq(Vec<Var>, Var),

    // --- Element (array access) ---
    /// `element(vec, i, e)` — `vec[i] = e`. 0-indexed. Not confluent.
    Element(Vec<Var>, Var, Var),
    /// `element_one(vec, i, e)` — like `Element`, 1-indexed.
    ElementOne(Vec<Var>, Var, Var),
    /// `element_undefzero(vec, i, e)` — like `Element`, but true with `e=0` when `i` is out of bounds.
    ElementUndefZero(Vec<Var>, Var, Var),
    /// `watchelement(vec, i, e)` — like `Element` but watched and GAC.
    WatchElement(Vec<Var>, Var, Var),
    /// `watchelement_one(vec, i, e)` — like `WatchElement`, 1-indexed.
    WatchElementOne(Vec<Var>, Var, Var),
    /// `watchelement_one_undefzero(vec, i, e)` — like `WatchElementOne` with undefzero semantics.
    WatchElementOneUndefZero(Vec<Var>, Var, Var),
    /// `watchelement_undefzero(vec, i, e)` — like `WatchElement` with undefzero semantics.
    WatchElementUndefZero(Vec<Var>, Var, Var),

    // --- Unary constraints ---
    /// `w-literal(x, a)` — `x = a`.
    WLiteral(Var, Constant),
    /// `w-notliteral(x, a)` — `x ≠ a`.
    WNotLiteral(Var, Constant),
    /// `w-inintervalset(x, [a1,a2, b1,b2, ...])` — `x` is in one of the intervals.
    WInIntervalSet(Var, Vec<Constant>),
    /// `w-inrange(x, [a, b])` — `a ≤ x ≤ b`.
    WInRange(Var, Vec<Constant>),
    /// `w-inset(x, vals)` — `x` is in the set `vals`.
    WInset(Var, Vec<Constant>),
    /// `w-notinrange(x, [a, b])` — `x < a` or `x > b`.
    WNotInRange(Var, Vec<Constant>),
    /// `w-notinset(x, vals)` — `x` is not in the set `vals`.
    WNotInset(Var, Vec<Constant>),

    // --- Binary arithmetic / comparison ---
    /// `abs(x, y)` — `x = |y|`.
    Abs(Var, Var),
    /// `diseq(x, y)` — `x ≠ y`. Arc consistency.
    DisEq(Var, Var),
    /// `eq(x, y)` — `x = y`. Bounds consistency.
    Eq(Var, Var),
    /// `minuseq(x, y)` — `x = -y`. Bounds consistency.
    MinusEq(Var, Var),
    /// `gaceq(x, y)` — `x = y`. GAC.
    GacEq(Var, Var),
    /// `watchless(x, y)` — `x < y`. Watched.
    WatchLess(Var, Var),
    /// `watchneq(x, y)` — `x ≠ y`. Watched (may be faster when one var is assigned early).
    WatchNeq(Var, Var),
    /// `ineq(x, y, k)` — `x ≤ y + k`. `k` must be a constant.
    Ineq(Var, Var, Constant),

    // --- Constant ---
    /// `false` — always false. Makes a model unsatisfiable.
    False,
    /// `true` — always true.
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
            Constraint::ModuloUndefZero(_, var) => write!(f, "modulo_undefzero({var})"),
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
            Constraint::CheckAssign(constraint) => write!(f, "check[assign]({constraint})"),
            Constraint::CheckGsa(constraint) => write!(f, "check[gsa]({constraint})"),
            Constraint::ForwardChecking(constraint) => {
                write!(f, "forwardchecking({constraint})")
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
            Constraint::WatchVecExistsLess(vars, vars1) => write!(
                f,
                "watchvecexists_less({},{})",
                print_var_array(vars),
                print_var_array(vars1)
            ),
            Constraint::Hamming(vars, vars1, constant) => write!(
                f,
                "hamming({},{},{constant})",
                print_var_array(vars),
                print_var_array(vars1)
            ),
            Constraint::NotHamming(vars, vars1, constant) => write!(
                f,
                "not-hamming({},{},{constant})",
                print_var_array(vars),
                print_var_array(vars1)
            ),
            Constraint::FrameUpdate(vars, vars1, vars2, vars3, constant) => write!(
                f,
                "frameupdate({},{},{},{},{constant})",
                print_var_array(vars),
                print_var_array(vars1),
                print_var_array(vars2),
                print_var_array(vars3)
            ),
            Constraint::Table(vars, tuples) => {
                write!(f, "table({},{})", print_var_array(vars), print_tuple_array(tuples))
            }
            Constraint::NegativeTable(vars, tuples) => {
                write!(
                    f,
                    "negativetable({},{})",
                    print_var_array(vars),
                    print_tuple_array(tuples)
                )
            }
            Constraint::GacSchema(vars, tuples) => {
                write!(
                    f,
                    "gacschema({},{})",
                    print_var_array(vars),
                    print_tuple_array(tuples)
                )
            }
            Constraint::LightTable(vars, tuples) => {
                write!(
                    f,
                    "lighttable({},{})",
                    print_var_array(vars),
                    print_tuple_array(tuples)
                )
            }
            Constraint::Mddc(vars, tuples) => {
                write!(
                    f,
                    "mddc({},{})",
                    print_var_array(vars),
                    print_tuple_array(tuples)
                )
            }
            Constraint::NegativeMddc(vars, tuples) => {
                write!(
                    f,
                    "negativemddc({},{})",
                    print_var_array(vars),
                    print_tuple_array(tuples)
                )
            }
            Constraint::Str2Plus(vars, table_var) => {
                write!(f, "str2plus({},{table_var})", print_var_array(vars))
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
            Constraint::GacEq(var, var1) => write!(f, "gaceq({var},{var1})"),
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
#[derive(Debug, Clone, Eq, PartialEq)]
#[non_exhaustive]
pub enum VarDomain {
    /// A bound variable with domain `[lower, upper]`. O(1) memory. The solver
    /// only tracks bound changes during search.
    Bound(i32, i32),
    /// A discrete variable with domain `[lower, upper]`. O(domain size) memory.
    /// Supports arbitrary subset removal. Prefer for domains up to ~1000 values.
    Discrete(i32, i32),
    /// Sparse bound variable with an explicit non-contiguous set of domain values.
    /// Unlike Bound/Discrete which take [lower, upper] ranges, this carries the
    /// full domain, e.g. `SparseBound(vec![-5, -2, 0, 3, 7])`.
    SparseBound(Vec<i32>),
    /// A Boolean variable with domain `{0, 1}`.
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

    /// Returns `true` if a variable with the given name exists in the symbol table.
    pub fn contains(&self, name: VarName) -> bool {
        self.table.contains_key(&name)
    }
}
