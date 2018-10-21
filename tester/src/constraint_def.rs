extern crate itertools;
extern crate rand;

use num_integer::Integer;

use counter::{get_unique_name, get_unique_value};
use std::collections::HashSet;
use std::fmt;
use std::iter::FromIterator;

use self::rand::Rng;

use std::sync::Arc;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum VarType {
    Constant,
    Bool,
    Bound,
    Discrete,
}

use self::VarType::*;

#[derive(Clone, Copy, Debug)]
pub enum Arg {
    Var(VarType),
    List(VarType),
    TwoVars(VarType),
    //    ConstantList,
    Tuples,
    //    Short_Tuples,
    Constraint,
    //    Constraint_List,
}

use self::Arg::*;

#[derive(Debug, Clone)]
pub struct MinionVariable {
    pub var_type: VarType,
    pub name: String,
    pub domain: Vec<i64>,
}

#[derive(Clone)]
pub struct Tuples {
    pub tupledata: Vec<Vec<i64>>,
    pub name: String,
}

impl Tuples {
    fn new(tupledata: Vec<Vec<i64>>) -> Tuples {
        Tuples {
            tupledata,
            name: get_unique_name("tuples", ""),
        }
    }
}

fn random_in_range(low: i64, high: i64) -> i64 {
    let mut rng = rand::thread_rng();
    rng.gen_range(low, high)
}

pub fn random_sublist(list: &[i64]) -> Vec<i64> {
    let mut vec = vec![];
    while vec.len() == 0 {
        for l in list {
            if rand::random() {
                vec.push(*l)
            }
        }
    }
    vec
}

pub fn random_sublist_of_size(list: &[i64], target: i64) -> Vec<i64> {
    let mut rng = rand::thread_rng();
    let mut vec = list.to_vec();
    rng.shuffle(&mut vec);
    while vec.len() as i64 > target {
        vec.pop();
    }
    vec.sort();
    vec
}

impl MinionVariable {
    fn random(d: VarType) -> Arc<MinionVariable> {
        let domain = match d {
            Constant => vec![random_in_range(-10, 10)],
            Bool => random_sublist(&vec![0, 1]),
            Discrete => {
                let v: Vec<i64> = (-10..10).collect();
                random_sublist_of_size(&v, random_in_range(1, 8))
            }
            Bound => {
                let low = random_in_range(-10, 10);
                let len = random_in_range(1, 8);
                (low..(low + len)).collect()
            }
        };

        let name = match d {
            Constant => domain[0].to_string(),
            _ => "x".to_owned() + &get_unique_value().to_string(),
        };

        Arc::new(MinionVariable {
            var_type: d,
            name: name,
            domain: domain,
        })
    }

    fn get_value(&self) -> i64 {
        if self.domain.len() != 1 {
            panic!("Invalid var!");
        }
        self.domain[0]
    }
}

#[derive(Clone)]
pub struct ConstraintDef {
    pub name: String,
    pub arg: Vec<Arg>,
    pub checker: Arc<Fn(&ConstraintInstance, &[&[i64]]) -> bool + Sync + Send>,
    pub gac: bool,
    pub reifygac: bool,
    pub valid_instance: fn(&ConstraintInstance) -> bool,
}

impl ConstraintDef {
    fn new(
        name: &str,
        arg: Vec<Arg>,
        checker: fn(&[&[i64]]) -> bool,
        gac: bool,
        reifygac: bool,
    ) -> ConstraintDef {
        ConstraintDef {
            name: name.to_string(),
            arg,
            checker: Arc::new(move |_c: &ConstraintInstance, v: &[&[i64]]| -> bool { checker(v) }),
            gac,
            reifygac,
            valid_instance: |_| true,
        }
    }

    fn new_parent(
        name: &str,
        arg: Vec<Arg>,
        checker: fn(&ConstraintInstance, &[&[i64]]) -> bool,
        gac: bool,
        reifygac: bool,
    ) -> ConstraintDef {
        ConstraintDef {
            name: name.to_string(),
            arg,
            checker: Arc::new(checker),
            gac,
            reifygac,
            valid_instance: |_| true,
        }
    }

    fn new_with_validator(
        name: &str,
        arg: Vec<Arg>,
        checker: fn(&[&[i64]]) -> bool,
        gac: bool,
        reifygac: bool,
        valid_instance: fn(&ConstraintInstance) -> bool,
    ) -> ConstraintDef {
        ConstraintDef {
            name: name.to_string(),
            arg,
            checker: Arc::new(move |_c: &ConstraintInstance, v: &[&[i64]]| -> bool { checker(v) }),
            gac,
            reifygac,
            valid_instance,
        }
    }
}

impl fmt::Debug for ConstraintDef {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(
            f,
            "ConstraintDef {{{:?},{:?},{:?},{:?}}}",
            self.name, self.arg, self.gac, self.reifygac
        )
    }
}

#[derive(Clone)]
pub struct ConstraintInstance {
    pub constraint: ConstraintDef,
    varlist: Arc<Vec<Vec<Arc<MinionVariable>>>>,
    pub tuples: Option<Tuples>,
    pub child_constraints: Vec<ConstraintInstance>,
}

impl ConstraintInstance {
    pub fn vars(&self) -> Arc<Vec<Vec<Arc<MinionVariable>>>> {
        let mut varlist = (*self.varlist).clone();
        for child in &self.child_constraints {
            varlist.extend((*child.varlist).clone().into_iter());
        }
        Arc::new(varlist)
    }

    fn check_tuple(&self, tup: &Vec<i64>) -> bool {
        let mut slices: Vec<&[i64]> = vec![];
        let mut place: usize = 0;
        for var in self.vars().iter() {
            let i = var.len();
            slices.push(&tup[place..place + i]);
            place += i;
        }
        return (self.constraint.checker)(self, &slices);
    }

    fn build_tuples(&self) -> Vec<Vec<i64>> {
        use self::itertools::Itertools;

        // Special case 0 variables
        if self.vars().concat().len() == 0 {
            if self.check_tuple(&vec![]) {
                return vec![vec![]];
            } else {
                return vec![];
            }
        }
        let v: Vec<Vec<i64>> = (*self.vars())
            .concat()
            .iter()
            .map(|x| x.domain.clone())
            .collect();
        return v
            .iter()
            .cloned()
            .multi_cartesian_product()
            .filter(|t| self.check_tuple(t))
            .collect();
    }

    pub fn tableise(&self, limit: usize) -> Option<ConstraintInstance> {
        let tuples = self.build_tuples();
        if tuples.len() > limit {
            return None;
        }

        Some(ConstraintInstance {
            constraint: STANDARD_TABLE_CONSTRAINT.clone(),
            varlist: {
                let vars: Vec<Arc<MinionVariable>> =
                    self.vars().iter().cloned().into_iter().flatten().collect();
                Arc::new(vec![vars])
            },
            tuples: Some(Tuples::new(tuples)),
            child_constraints: vec![],
        })
    }
    /*
    pub fn variables(&self) -> usize {
        self.lengths.iter().clone().sum()
    }
    */
}

pub fn build_random_instance(constraint: &ConstraintDef) -> ConstraintInstance {
    build_random_instance_with_children(constraint, &[])
}

pub fn build_random_instance_with_children(
    constraint: &ConstraintDef,
    children: &[&ConstraintDef],
) -> ConstraintInstance {
    loop {
        let mut variables: Vec<Vec<Arc<MinionVariable>>> = vec![];
        let mut constraints: Vec<ConstraintInstance> = vec![];
        let mut constraint_child = 0;

        for i in 0..constraint.arg.len() {
            match constraint.arg[i] {
                Var(d) => {
                    variables.push(vec![MinionVariable::random(d)]);
                }
                List(d) => {
                    let len = rand::random::<usize>() % 5;
                    variables.push((0..len).map(|_x| MinionVariable::random(d)).collect());
                }

                TwoVars(d) => {
                    variables.push(vec![MinionVariable::random(d), MinionVariable::random(d)]);
                }
                Tuples => unimplemented!(),
                Constraint => {
                    // Leave dummy empty vec in variables
                    variables.push(vec![]);
                    constraints.push(build_random_instance_with_children(
                        children[constraint_child],
                        &[],
                    ));
                    constraint_child += 1;
                }
            }
        }
        let c = ConstraintInstance {
            constraint: constraint.clone(),
            varlist: Arc::new(variables),
            tuples: None,
            child_constraints: constraints,
        };
        if (constraint.valid_instance)(&c) {
            return c;
        }
    }
}

fn check_alldiff(v: &[&[i64]]) -> bool {
    assert!(v.len() == 1);
    for i in 0..v[0].len() {
        for j in (i + 1)..v[0].len() {
            if v[0][i] == v[0][j] {
                return false;
            }
        }
    }
    true
}

fn check_element(v: &[&[i64]], offset: i64, undefzero: bool) -> bool {
    let index = v[1][0] - offset;
    if index < 0 || index >= v[0].len() as i64 {
        if undefzero {
            return v[2][0] == 0;
        } else {
            return false;
        }
    }
    return v[0][index as usize] == v[2][0];
}

lazy_static! {
    pub static ref STANDARD_TABLE_CONSTRAINT: ConstraintDef = {
        ConstraintDef {
            name: "str2plus".to_string(),
            arg: vec![List(Discrete), Tuples],
            checker: Arc::new(|_, __| unimplemented!()),
            gac: true,
            reifygac: true,
            valid_instance: |_| true,
        }
    };
}

lazy_static! {
    pub static ref NESTED_CONSTRAINT_LIST: Vec<ConstraintDef> = nested_constraint_list();
}

fn nested_constraint_list() -> Vec<ConstraintDef> {
    vec![
        ConstraintDef::new_parent(
            "reify", // CT_REIFY
            vec![Constraint, Var(Bool)],
            |c, assign| {
                let ref child = c.child_constraints[0];
                let ischildtrue = (child.constraint.checker)(child, &assign[2..]);
                (assign[1][0] == 1) == ischildtrue
            },
            false,
            false,
        ),
        ConstraintDef::new_parent(
            "reifyimply-quick", // CT_REIFYIMPLY_QUICK
            vec![Constraint, Var(Bool)],
            |c, assign| {
                let ref child = c.child_constraints[0];
                let ischildtrue = (child.constraint.checker)(child, &assign[2..]);
                (assign[1][0] == 1) <= ischildtrue
            },
            false,
            false,
        ),
        ConstraintDef::new_parent(
            "reifyimply", // CT_REIFYIMPLY
            vec![Constraint, Var(Bool)],
            |c, assign| {
                let ref child = c.child_constraints[0];
                let ischildtrue = (child.constraint.checker)(child, &assign[2..]);
                (assign[1][0] == 1) <= ischildtrue
            },
            false,
            false,
        ),
        ConstraintDef::new_parent(
            "forwardchecking", // CT_FORWARD_CHECKING
            vec![Constraint],
            |c, assign| {
                (c.child_constraints[0].constraint.checker)(&c.child_constraints[0], &assign[1..])
            },
            false,
            false,
        ),
        ConstraintDef::new_parent(
            "check[gsa]", // CT_CHECK_GSA
            vec![Constraint],
            |c, assign| {
                (c.child_constraints[0].constraint.checker)(&c.child_constraints[0], &assign[1..])
            },
            false,
            false,
        ),
        ConstraintDef::new_parent(
            "check[assign]", // CT_CHECK_ASSIGN
            vec![Constraint],
            |c, assign| {
                (c.child_constraints[0].constraint.checker)(&c.child_constraints[0], &assign[1..])
            },
            false,
            false,
        ),
    ]
}

lazy_static! {
    pub static ref CONSTRAINT_LIST: Vec<ConstraintDef> = constraint_list();
}

fn constraint_list() -> Vec<ConstraintDef> {
    vec![
        ConstraintDef::new(
            "abs", // ABS
            vec![Var(Discrete), Var(Discrete)],
            |v| v[0][0] == v[1][0].abs(),
            false,
            false,
        ),
        ConstraintDef::new(
            "alldiff", // ALLDIFF
            vec![List(Discrete)],
            check_alldiff,
            false,
            false,
        ),
        // TODO: ALLDIFF_MATRIX
        ConstraintDef::new(
            "difference", // CT_DIFFERENCE
            vec![Var(Discrete), Var(Discrete), Var(Discrete)],
            |v| (v[0][0] - v[1][0]).abs() == v[2][0],
            false,
            false,
        ),
        // CT_DISEQ_REIFY handled by DISEQ
        ConstraintDef::new(
            "diseq", // CT_DISEQ
            vec![Var(Discrete), Var(Discrete)],
            |v| (v[0][0] != v[1][0]),
            false,
            false,
        ),
        ConstraintDef::new(
            "div_undefzero", // CT_DIV_UNDEFZERO
            vec![Var(Discrete), Var(Discrete), Var(Discrete)],
            |v| {
                (v[2][0]
                    == (if v[1][0] == 0 {
                        0
                    } else {
                        v[0][0].div_floor(&v[1][0])
                    }))
            },
            false,
            false,
        ),
        ConstraintDef::new(
            "div", // CT_DIV_UNDEFZERO
            vec![Var(Discrete), Var(Discrete), Var(Discrete)],
            |v| {
                (if v[1][0] == 0 {
                    false
                } else {
                    v[2][0] == v[0][0].div_floor(&v[1][0])
                })
            },
            false,
            false,
        ),
        ConstraintDef::new(
            "element_one", // CT_ELEMENT_ONE
            vec![List(Discrete), Var(Discrete), Var(Discrete)],
            |v| check_element(v, 1, false),
            false,
            false,
        ),
        ConstraintDef::new(
            "element_undefzero", // CT_ELEMENT_UNDEFZERO
            vec![List(Discrete), Var(Discrete), Var(Discrete)],
            |v| check_element(v, 0, true),
            false,
            false,
        ),
        ConstraintDef::new(
            "element", // CT_ELEMENT
            vec![List(Discrete), Var(Discrete), Var(Discrete)],
            |v| check_element(v, 0, false),
            false,
            false,
        ),
        // CT_EQ_REIFY handled by CT_EQ
        ConstraintDef::new(
            "eq", // CT_EQ
            vec![Var(Discrete), Var(Discrete)],
            |v| (v[0][0] == v[1][0]),
            false,
            false,
        ),
        ConstraintDef::new(
            "false", // CT_FALSE
            vec![],
            |_| false,
            true,
            true,
        ),
        // TODO: CT_FRAMEUPDATE
        ConstraintDef::new(
            "gacalldiff",
            vec![List(Discrete)],
            check_alldiff,
            true,
            false,
        ),
        ConstraintDef::new(
            "gaceq", // CT_GACEQ
            vec![Var(Discrete), Var(Discrete)],
            |v| (v[0][0] == v[1][0]),
            true,
            true,
        ),
        ConstraintDef::new_with_validator(
            "lexleq[rv]", // CT_GACLEXLEQ
            vec![List(Discrete), List(Discrete)],
            |v| (v[0] <= v[1]),
            true,
            true,
            |v| v.vars()[0].len() == v.vars()[1].len(),
        ),
        // TODO: CT_GACSCHEMA

        // TODO: CT_GCC, CT_GCCWEAK
        ConstraintDef::new(
            "occurrencegeq", // CT_GEQ_OCCURRENCE
            vec![List(Discrete), Var(Constant), Var(Constant)],
            |v| (v[0].iter().filter(|x| **x == v[1][0]).count() as i64 >= v[2][0]),
            false,
            false,
        ),
        ConstraintDef::new(
            "nvaluegeq", // CT_GEQNVALUE
            vec![List(Discrete), Var(Constant)],
            |v| {
                let h: HashSet<i64> = HashSet::from_iter(v[0].iter().cloned());
                h.len() as i64 >= v[1][0]
            },
            false,
            false,
        ),
        ConstraintDef::new(
            "sumgeq", // CT_GEQSUM
            vec![List(Discrete), Var(Discrete)],
            |v| (v[0].iter().sum::<i64>() >= v[1][0]),
            false,
            false,
        ),
        // TODO: CT_HAGGISGAC_STABLE, CT_HAGGISGAC
        ConstraintDef::new(
            "ineq", // CT_INEQ
            vec![Var(Discrete), Var(Discrete), Var(Constant)],
            |v| v[0][0] <= v[1][0] + v[2][0],
            false,
            false,
        ),
        ConstraintDef::new(
            "occurrenceleq", // CT_LEQ_OCCURRENCE
            vec![List(Discrete), Var(Constant), Var(Constant)],
            |v| (v[0].iter().filter(|x| **x == v[1][0]).count() as i64 <= v[2][0]),
            false,
            false,
        ),
        ConstraintDef::new(
            "nvalueleq", // CT_LEQNVALUE
            vec![List(Discrete), Var(Constant)],
            |v| {
                let h: HashSet<i64> = HashSet::from_iter(v[0].iter().cloned());
                h.len() as i64 <= v[1][0]
            },
            false,
            false,
        ),
        ConstraintDef::new(
            "sumleq", // CT_LEQSUM
            vec![List(Discrete), Var(Discrete)],
            |v| (v[0].iter().sum::<i64>() <= v[1][0]),
            false,
            false,
        ),
        ConstraintDef::new_with_validator(
            "lexleq", // CT_LEXLEQ
            vec![List(Discrete), List(Discrete)],
            |v| (v[0] <= v[1]),
            true,
            true,
            |v| v.vars()[0].len() == v.vars()[1].len(),
        ),
        ConstraintDef::new_with_validator(
            "lexless", // CT_LEXLESS
            vec![List(Discrete), List(Discrete)],
            |v| (v[0] < v[1]),
            true,
            true,
            |v| v.vars()[0].len() == v.vars()[1].len(),
        ),
        // TODO: CT_LIGHTTABLE
        ConstraintDef::new(
            "max", // CT_MAX
            vec![List(Discrete), Var(Discrete)],
            |v| (v[0].iter().cloned().max() == Some(v[1][0])),
            false,
            false,
        ),
        // TODO: CT_MDDC
        ConstraintDef::new(
            "min", // CT_MIN
            vec![List(Discrete), Var(Discrete)],
            |v| (v[0].iter().cloned().min() == Some(v[1][0])),
            false,
            false,
        ),
        // MINUSEQ_REIFY handled by CT_MINUSEQ
        ConstraintDef::new(
            "minuseq", // CT_MINUSEQ
            vec![Var(Discrete), Var(Discrete)],
            |v| (v[0][0] == -v[1][0]),
            false,
            false,
        ),
        ConstraintDef::new(
            "modulo_undefzero", // CT_MODULO_UNDEFZERO
            vec![Var(Discrete), Var(Discrete), Var(Discrete)],
            |v| {
                (v[2][0]
                    == (if v[1][0] == 0 {
                        0
                    } else {
                        v[0][0].mod_floor(&v[1][0])
                    }))
            },
            false,
            false,
        ),
        ConstraintDef::new(
            "modulo", // CT_MODULO
            vec![Var(Discrete), Var(Discrete), Var(Discrete)],
            |v| (v[1][0] != 0 && (v[0][0].mod_floor(&v[1][0]) == v[2][0])),
            false,
            false,
        ),
        //TODO: CT_NEGATIVEMDDC
        ConstraintDef::new(
            "occurrence", // CT_OCCURRENCE
            vec![List(Discrete), Var(Constant), Var(Discrete)],
            |v| (v[0].iter().filter(|x| **x == v[1][0]).count() as i64 == v[2][0]),
            false,
            false,
        ),
        ConstraintDef::new(
            "pow", // CT_POW
            vec![Var(Discrete), Var(Discrete), Var(Discrete)],
            |v| {
                if v[0][0] == 0 {
                    if v[1][0] < 0 {
                        return false;
                    }
                    return v[2][0] == (if v[1][0] == 0 { 1 } else { 0 });
                }

                if v[0][0] == 1 {
                    return v[2][0] == 1;
                }

                if v[0][0] == -1 {
                    return v[2][0] == (if v[1][0].is_even() { 1 } else { -1 });
                }

                if v[1][0] < 0 {
                    return false;
                }

                v[0][0].pow(v[1][0] as u32) == v[2][0]
            },
            false,
            false,
        ),
        //TODO: CT_NEGATIVEMDDC
        ConstraintDef::new(
            "product", // CT_PRODUCT
            vec![Var(Discrete), Var(Discrete), Var(Discrete)],
            |v| (v[0][0] * v[1][0] == v[2][0]),
            false,
            false,
        ),
        ConstraintDef::new_with_validator(
            "lexleq[quick]", // CT_QUICK_LEXLEQ
            vec![List(Discrete), List(Discrete)],
            |v| (v[0] <= v[1]),
            false,
            false,
            |v| v.vars()[0].len() == v.vars()[1].len(),
        ),
        ConstraintDef::new_with_validator(
            "lexless[quick]", // CT_QUICK_LEXLESS
            vec![List(Discrete), List(Discrete)],
            |v| (v[0] < v[1]),
            false,
            false,
            |v| v.vars()[0].len() == v.vars()[1].len(),
        ),
        // TODO: CT_SHORTSTR_TUPLE, CT_SHORTSTR, CT_STR
        ConstraintDef::new(
            "true", // CT_TRUEArc
            vec![],
            |_| true,
            true,
            true,
        ),
        ConstraintDef::new(
            "watchelement_one_undefzero", // CT_WATCHED_ELEMENT_ONE_UNDEFZERO
            vec![List(Discrete), Var(Discrete), Var(Discrete)],
            |v| check_element(v, 1, true),
            true,
            false,
        ),
        ConstraintDef::new(
            "watchelement_one", // CT_WATCHED_ELEMENT_ONE
            vec![List(Discrete), Var(Discrete), Var(Discrete)],
            |v| check_element(v, 1, false),
            true,
            false,
        ),
        ConstraintDef::new(
            "watchelement_undefzero", // CT_WATCHED_ELEMENT_UNDEFZERO
            vec![List(Discrete), Var(Discrete), Var(Discrete)],
            |v| check_element(v, 0, true),
            true,
            false,
        ),
        ConstraintDef::new(
            "watchelement", // CT_WATCHED_ELEMENT
            vec![List(Discrete), Var(Discrete), Var(Discrete)],
            |v| check_element(v, 0, false),
            true,
            false,
        ),
        ConstraintDef::new(
            "watchsumgeq", // CT_WATCHED_GEQSUM
            vec![List(Bool), Var(Constant)],
            |v| v[0].iter().cloned().sum::<i64>() >= v[1][0],
            true,
            true,
        ),
        ConstraintDef::new_with_validator(
            "hamming", // CT_WATCHED_HAMMING
            vec![List(Discrete), List(Discrete), Var(Constant)],
            |v| v[0].iter().zip(v[1]).filter(|(a, b)| *a != *b).count() as i64 >= v[2][0],
            true,
            true,
            |c| c.vars()[0].len() == c.vars()[1].len(),
        ),
        ConstraintDef::new_with_validator(
            "w-inintervalset", // CT_WATCHED_ININTERVALSET
            vec![Var(Discrete), List(Constant)],
            |v| {
                for i in (0..v[1].len()).step_by(2) {
                    if v[1][i] <= v[0][0] && v[0][0] <= v[1][i + 1] {
                        return true;
                    }
                }
                false
            },
            true,
            true,
            |c| {
                let vals: Vec<i64> = c.vars()[1].iter().map(|x| (*x).get_value()).collect();
                if vals.len().is_odd() || vals.len() < 2 {
                    return false;
                }
                if vals.len() > 0 {
                    for i in (0..vals.len()).step_by(2) {
                        if vals[i] > vals[i + 1] {
                            return false;
                        }
                    }
                    for i in (1..vals.len() - 1).step_by(2) {
                        if vals[i] >= vals[i + 1] - 1 {
                            return false;
                        }
                    }
                }
                true
            },
        ),
        ConstraintDef::new_with_validator(
            "w-inrange", // CT_WATCHED_INRANGE
            vec![Var(Discrete), List(Constant)],
            |v| v[1][0] <= v[0][0] && v[0][0] <= v[1][1],
            true,
            true,
            |c| {
                let vals: Vec<i64> = c.vars()[1].iter().map(|x| (*x).get_value()).collect();
                vals.len() == 2 && vals[0] <= vals[1]
            },
        ),
        ConstraintDef::new_with_validator(
            "w-inset", // CT_WATCHED_INSET
            vec![Var(Discrete), List(Constant)],
            |v| v[1].contains(&v[0][0]),
            true,
            true,
            |c| {
                let vals: Vec<i64> = c.vars()[1].iter().map(|x| (*x).get_value()).collect();
                vals.windows(2).all(|s| s[0] < s[1])
            },
        ),
        ConstraintDef::new(
            "watchsumleq", // CT_WATCHED_LEQSUM
            vec![List(Bool), Var(Constant)],
            |v| v[0].iter().cloned().sum::<i64>() <= v[1][0],
            true,
            true,
        ),
        ConstraintDef::new(
            "watchless", // CT_WATCHED_LESS
            vec![Var(Discrete), Var(Discrete)],
            |v| (v[0][0] < v[1][0]),
            true,
            true,
        ),
        ConstraintDef::new(
            "w-literal", // CT_WATCHED_LIT
            vec![Var(Discrete), Var(Constant)],
            |v| (v[0][0] == v[1][0]),
            true,
            true,
        ),
        ConstraintDef::new_with_validator(
            "litsumgeq", // CT_WATCHED_LITSUM
            vec![List(Discrete), List(Constant), Var(Constant)],
            |v| v[0].iter().zip(v[1]).filter(|(a, b)| *a == *b).count() as i64 >= v[2][0],
            true,
            true,
            |c| c.vars()[0].len() == c.vars()[1].len(),
        ),
        // TODO: CT_WATCHED_NEGATIVE_TABLE
        ConstraintDef::new(
            "watchneq", // CT_WATCHED_NEQ
            vec![Var(Discrete), Var(Discrete)],
            |v| (v[0][0] != v[1][0]),
            true,
            true,
        ),
        // TODO: WATCHED_NEW_AND, WATCHED_NEW_OR
        ConstraintDef::new_with_validator(
            "not-hamming", // CT_WATCHED_NOT_HAMMING
            vec![List(Discrete), List(Discrete), Var(Constant)],
            |v| (v[0].iter().zip(v[1]).filter(|(a, b)| *a != *b).count() as i64) < v[2][0],
            false,
            false,
            |c| c.vars()[0].len() == c.vars()[1].len(),
        ),
        ConstraintDef::new_with_validator(
            "w-notinrange", // CT_WATCHED_NOT_INRANGE
            vec![Var(Discrete), List(Constant)],
            |v| !(v[1][0] <= v[0][0] && v[0][0] <= v[1][1]),
            true,
            true,
            |c| {
                let vals: Vec<i64> = c.vars()[1].iter().map(|x| (*x).get_value()).collect();
                vals.len() == 2 && vals[0] <= vals[1]
            },
        ),
        ConstraintDef::new_with_validator(
            "w-notinset", // CT_WATCHED_NOT_INSET
            vec![Var(Discrete), List(Constant)],
            |v| !v[1].contains(&v[0][0]),
            true,
            true,
            |c| {
                let vals: Vec<i64> = c.vars()[1].iter().map(|x| (*x).get_value()).collect();
                vals.windows(2).all(|s| s[0] < s[1])
            },
        ),
        ConstraintDef::new(
            "w-notliteral", // CT_WATCHED_NOTLIT
            vec![Var(Discrete), Var(Constant)],
            |v| !(v[0][0] == v[1][0]),
            true,
            true,
        ),
        // TODO: CT_WATCHED_TABLE
        ConstraintDef::new_with_validator(
            "watchvecexists_less", // CT_WATCHED_VEC_OR_LESS
            vec![List(Discrete), List(Discrete)],
            |v| v[0].iter().zip(v[1]).any(|(a, b)| *a < *b),
            true,
            true,
            |c| c.vars()[0].len() == c.vars()[1].len(),
        ),
        ConstraintDef::new_with_validator(
            "watchvecneq", // CT_WATCHED_VECNEQ
            vec![List(Discrete), List(Discrete)],
            |v| v[0].iter().zip(v[1]).any(|(a, b)| *a != *b),
            true,
            true,
            |c| c.vars()[0].len() == c.vars()[1].len(),
        ),
        ConstraintDef::new_with_validator(
            "weightedsumgeq", // CT_WEIGHTGEQSUM
            vec![List(Constant), List(Discrete), Var(Discrete)],
            |v| v[0].iter().zip(v[1]).map(|(a, b)| *a * *b).sum::<i64>() >= v[2][0],
            true,
            true,
            |c| c.vars()[0].len() == c.vars()[1].len(),
        ),
        ConstraintDef::new_with_validator(
            "weightedsumleq", // CT_WEIGHTLEQSUM
            vec![List(Constant), List(Discrete), Var(Discrete)],
            |v| v[0].iter().zip(v[1]).map(|(a, b)| *a * *b).sum::<i64>() <= v[2][0],
            true,
            true,
            |c| c.vars()[0].len() == c.vars()[1].len(),
        ),
    ]
}
