import Minion.Semantics

/-! Reusable arithmetic views and proved relation normalization. Views refer to
    the original variables; they do not introduce independent domain state. -/
namespace Minion

def Term.baseIndex : Term → Option Nat
  | .constant _ => none
  | .var i | .notBool i => some i
  | .neg t | .shift t _ | .scale t _ => t.baseIndex

def Term.baseValue (value : Int) : Term → Int
  | .constant v => v
  | .var _ => value
  | .notBool _ => 1 - value
  | .neg t => -t.baseValue value
  | .shift t k => t.baseValue value + k
  | .scale t k => k * t.baseValue value

theorem Term.baseValue_sound (t : Term) (i : Nat) (h : t.baseIndex = some i) (a : Assignment) :
    t.baseValue (a[i]?.getD 0) = t.eval a := by
  induction t with
  | constant v => simp [Term.baseIndex] at h
  | var j => simp [Term.baseIndex] at h; subst j; rfl
  | notBool j => simp [Term.baseIndex] at h; subst j; rfl
  | neg t ih => simp only [Term.baseIndex] at h; simp [Term.baseValue, Term.eval, ih h]
  | shift t k ih => simp only [Term.baseIndex] at h; simp [Term.baseValue, Term.eval, ih h]
  | scale t k ih => simp only [Term.baseIndex] at h; simp [Term.baseValue, Term.eval, ih h]

/-- Recover a unique base candidate through arithmetic views. A zero scale
    is non-injective and deliberately supplies no unique preimage. -/
def Term.preimageValue : Term → Int → Option Int
  | .constant _, _ => none
  | .var _, value => some value
  | .notBool _, value => some (1 - value)
  | .neg t, value => t.preimageValue (-value)
  | .shift t k, value => t.preimageValue (value - k)
  | .scale t k, value => if k = 0 then none else t.preimageValue (value.fdiv k)

theorem Term.preimageValue_sound (t : Term) (i : Nat) (value : Int) (a : Assignment)
    (hi : t.baseIndex = some i) (hv : t.preimageValue (t.eval a) = some value) :
    a[i]?.getD 0 = value := by
  induction t generalizing value with
  | constant v => simp [Term.baseIndex] at hi
  | var j =>
    simp only [Term.baseIndex, Option.some.injEq] at hi
    subst j
    simpa only [Term.preimageValue, Term.eval, Option.some.injEq] using hv
  | notBool j =>
    simp only [Term.baseIndex, Option.some.injEq] at hi
    subst j
    simp only [Term.preimageValue, Term.eval, Option.some.injEq] at hv
    omega
  | neg t ih =>
    exact ih value hi (by simpa only [Term.preimageValue, Term.eval, Int.neg_neg] using hv)
  | shift t k ih =>
    exact ih value hi (by simpa only [Term.preimageValue, Term.eval, Int.add_sub_cancel] using hv)
  | scale t k ih =>
    by_cases hk : k = 0
    · simp [Term.preimageValue, hk] at hv
    · exact ih value hi (by simpa [Term.preimageValue, Term.eval, hk] using hv)

/-- Fold constants and cancel double arithmetic negation. Boolean complement
    remains a distinct view, so negating !b denotes -(1-b). -/
def Term.negate : Term → Term
  | .constant v => .constant (-v)
  | .neg t => t
  | t => .neg t

@[simp] theorem Term.eval_negate (a : Assignment) (t : Term) :
    t.negate.eval a = -t.eval a := by
  cases t <;> simp [Term.negate, Term.eval]

@[simp] theorem Term.wellFormed_negate (vars : List Variable) (t : Term) :
    t.negate.wellFormed vars = t.wellFormed vars := by
  cases t <;> simp [Term.negate, Term.wellFormed]

def constantValues : List Term → Option (List Int)
  | [] => some []
  | .constant v :: rest => (constantValues rest).map (v :: ·)
  | _ => none

theorem constantValues_sound (xs : List Term) (vs : List Int)
    (hc : constantValues xs = some vs) (a : Assignment) : xs.map (Term.eval a) = vs := by
  induction xs generalizing vs with
  | nil => simpa [constantValues] using hc
  | cons t ts ih =>
    cases t <;> try { simp [constantValues] at hc }
    rename_i v
    cases ht : constantValues ts with
    | none => simp [constantValues, ht] at hc
    | some rest =>
      simp [constantValues, ht] at hc
      subst vs
      simp [Term.eval, ih rest ht]

private theorem sum_eval_negate (a : Assignment) (xs : List Term) :
    ((xs.map Term.negate).map (Term.eval a)).sum = -(xs.map (Term.eval a)).sum := by
  induction xs with
  | nil => simp
  | cons x xs ih =>
    simp only [List.map_cons, List.sum_cons, Term.eval_negate, ih, Int.neg_add]

/-- Keep the source relation intact; normalize at the proof-rule boundary. -/
def Constraint.normalizeSum : Constraint → Constraint
  | .sum xs rhs .ge => .sum (xs.map Term.negate) rhs.negate .le
  | c => c

@[simp] theorem Constraint.normalizeSum_eval (a : Assignment) (c : Constraint) :
    c.normalizeSum.eval a = c.eval a := by
  cases c <;> try rfl
  rename_i xs rhs op
  cases op <;> try rfl
  simp only [Constraint.normalizeSum, Constraint.eval, Comparison.eval,
    ← List.sum_eq_foldl, sum_eval_negate]
  simp

/-- The integer complement of a sum inequality also uses only sumleq, with
    negation and a one-unit shift. Unsupported comparison shapes fail closed. -/
def Constraint.negateSum : Constraint → Option Constraint
  | .sum xs rhs .le => some (.sum (xs.map Term.negate) (.shift rhs.negate (-1)) .le)
  | .sum xs rhs .ge => some (.sum xs (.shift rhs (-1)) .le)
  | _ => none

theorem Constraint.negateSum_eval (a : Assignment) (c nc : Constraint)
    (hc : c.negateSum = some nc) : nc.eval a = !c.eval a := by
  cases c <;> try { simp [Constraint.negateSum] at hc }
  rename_i xs rhs op
  cases op <;> simp [Constraint.negateSum] at hc
  all_goals subst nc
  all_goals
    simp only [Constraint.eval, Comparison.eval, ← List.sum_eq_foldl,
      sum_eval_negate, Term.eval, Term.eval_negate]
    simp only [← decide_not, decide_eq_decide]
    omega

def weightedTerms (weights : List Int) (xs : List Term) : List Term :=
  (weights.zip xs).map (fun (weight, x) => Term.scale x weight)

/-- Weight multiplication is a view of the original variable. Keep every
    occurrence, including zero weights, so source correspondence stays simple. -/
def Constraint.expandWeighted : Constraint → Constraint
  | c@(.weightedSum weights xs rhs op) =>
      match constantValues weights with
      | some ws => if ws.length == xs.length then .sum (weightedTerms ws xs) rhs op else .truth false
      | none => c
  | c => c

theorem Constraint.expandWeighted_eval (a : Assignment) (c : Constraint) :
    c.expandWeighted.eval a = c.eval a := by
  cases c <;> try rfl
  rename_i weights xs rhs op
  cases hw : constantValues weights with
  | none => simp [Constraint.expandWeighted, hw]
  | some ws =>
    have he := constantValues_sound weights ws hw a
    have hl : weights.length = ws.length := by simpa using congrArg List.length he
    have terms : (weightedTerms ws xs).map (Term.eval a) =
        (weights.zip xs).map (fun (w, x) => w.eval a * x.eval a) := by
      rw [← he]
      simp [weightedTerms, List.zip_map_left, List.map_map, Function.comp_def, Term.eval]
    by_cases hlen : ws.length = xs.length
    · simp [Constraint.expandWeighted, hw, hlen, Constraint.eval, hl, terms]
    · simp [Constraint.expandWeighted, hw, hlen, Constraint.eval, hl]

private theorem fold_negate_min (a : Assignment) (xs : List Term) (v : Int) :
    ((xs.map Term.negate).map (Term.eval a)).foldl min (-v) =
      -((xs.map (Term.eval a)).foldl max v) := by
  induction xs generalizing v with
  | nil => rfl
  | cons x xs ih =>
    simp only [List.map_cons, List.foldl_cons, Term.eval_negate, Int.neg_min_neg]
    exact ih (max v (x.eval a))

/-- Max is min over negated views, as in Minion's implementation. -/
def Constraint.normalizeExtremum : Constraint → Constraint
  | .extremum xs rhs true => .extremum (xs.map Term.negate) rhs.negate false
  | c => c

theorem Constraint.normalizeExtremum_eval (a : Assignment) (c : Constraint) :
    c.normalizeExtremum.eval a = c.eval a := by
  cases c <;> try rfl
  rename_i xs rhs isMax
  cases isMax <;> try rfl
  cases xs with
  | nil => simp [Constraint.normalizeExtremum, Constraint.eval]
  | cons x xs =>
    simp only [Constraint.normalizeExtremum, Constraint.eval, List.map_cons,
      Term.eval_negate, fold_negate_min, Bool.false_eq_true, ↓reduceIte]
    rw [Bool.eq_iff_iff]
    simp

end Minion
