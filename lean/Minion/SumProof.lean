import Minion.LocalProof
import Minion.Views

/-! Fixed, kernel-checked bounds rules for integer sums. -/
namespace Minion.SumProof

export DomainProof (Domains Compatible values values_sound lower upper lower_sound upper_sound
  nonempty_values Operation checkNoop checkNoop_sound rejectedDomains compatible_rejected
  ofLists inDomains_compatible inDomains_restrict)

/-! Each term's bound is read once into a running total, for either direction
    of comparison. An empty referenced domain is a conflict too, including a
    failed restriction whose C++ container leaves its old bounds in place. -/

def lowerTotal (d : Domains) (xs : List Term) : Int := xs.foldl (fun total t => total + lower d t) 0
def upperTotal (d : Domains) (xs : List Term) : Int := xs.foldl (fun total t => total + upper d t) 0

private theorem lowerTotal_from (d : Domains) (a : Assignment) (h : Compatible d a) (xs : List Term)
    (total exact : Int) (ht : total ≤ exact) :
    xs.foldl (fun total t => total + lower d t) total ≤ exact + (xs.map (Term.eval a)).sum := by
  induction xs generalizing total exact with
  | nil => simpa using ht
  | cons x xs ih =>
    have hx := lower_sound d a h x
    have hr := ih (total + lower d x) (exact + x.eval a) (by omega)
    simp only [List.foldl_cons, List.map_cons, List.sum_cons]
    omega

private theorem upperTotal_from (d : Domains) (a : Assignment) (h : Compatible d a) (xs : List Term)
    (total exact : Int) (ht : exact ≤ total) :
    exact + (xs.map (Term.eval a)).sum ≤ xs.foldl (fun total t => total + upper d t) total := by
  induction xs generalizing total exact with
  | nil => simpa using ht
  | cons x xs ih =>
    have hx := upper_sound d a h x
    have hr := ih (total + upper d x) (exact + x.eval a) (by omega)
    simp only [List.foldl_cons, List.map_cons, List.sum_cons]
    omega

theorem lowerTotal_sound (d : Domains) (a : Assignment) (h : Compatible d a) (xs : List Term) :
    lowerTotal d xs ≤ (xs.map (Term.eval a)).sum := by
  simpa [lowerTotal] using lowerTotal_from d a h xs 0 0 (Int.le_refl 0)

theorem upperTotal_sound (d : Domains) (a : Assignment) (h : Compatible d a) (xs : List Term) :
    (xs.map (Term.eval a)).sum ≤ upperTotal d xs := by
  simpa [upperTotal] using upperTotal_from d a h xs 0 0 (Int.le_refl 0)

/-- A sum cannot hold when a term has no value left, or when its bounds rule
    the comparison out. -/
def checkConflict (d : Domains) : Constraint → Bool
  | .sum xs rhs .le =>
      xs.any (fun t => (values d t).isEmpty) || (values d rhs).isEmpty ||
      upper d rhs < lowerTotal d xs
  | .sum xs rhs .ge =>
      xs.any (fun t => (values d t).isEmpty) || (values d rhs).isEmpty ||
      upperTotal d xs < lower d rhs
  | _ => false

theorem checkConflict_sound (d : Domains) (c : Constraint) (hc : checkConflict d c = true)
    (a : Assignment) (hd : Compatible d a) (hs : c.eval a = true) : False := by
  cases c <;> try { simp [checkConflict] at hc }
  rename_i xs rhs op
  have ne (t : Term) := nonempty_values d a hd t
  have hlo := lowerTotal_sound d a hd xs
  have hhi := upperTotal_sound d a hd xs
  have hrlo := lower_sound d a hd rhs
  have hrhi := upper_sound d a hd rhs
  cases op <;> simp [checkConflict, ne] at hc
  all_goals
    simp [Constraint.eval, Comparison.eval, ← List.sum_eq_foldl] at hs
    omega

def rule : LocalProof.Rule := ⟨checkConflict, checkConflict_sound⟩

def supported : Constraint → Bool
  | .sum _ _ .le | .sum _ _ .ge => true
  | _ => false

/-- Truth and falsity of sum inequalities share the same canonical bounds rule.
    The complement conversion is proved in Views.lean. -/
def possibleValue (d : Domains) (expected : Bool) (c : Constraint) : Bool :=
  if expected then rule.possible d c else
    match c.negateSum with
    | some nc => rule.possible d nc
    | none => true

theorem possibleValue_sound (d : Domains) (expected : Bool) (c : Constraint) (a : Assignment)
    (hd : Compatible d a) (hs : c.eval a = expected) : possibleValue d expected c = true := by
  cases expected with
  | true => exact rule.possible_sound d c a hd hs
  | false =>
    cases hn : c.negateSum with
    | none => simp [possibleValue, hn]
    | some nc =>
      have hnc : nc.eval a = true := by rw [Constraint.negateSum_eval a c nc hn, hs]; rfl
      simpa only [possibleValue, Bool.false_eq_true, ↓reduceIte, hn] using
        rule.possible_sound d nc a hd hnc

/-- Negate the claimed restriction and check a bounds conflict. This one rule
    handles term bounds, result bounds, assignments, removals, aliases and holes. -/
def checkPrune (d : Domains) (c : Constraint) (target : Nat) (op : Operation) (bound : Int) : Bool :=
  checkConflict (rejectedDomains d target op bound) c

theorem checkPrune_sound (d : Domains) (c : Constraint) (target : Nat) (op : Operation) (bound : Int)
    (hc : checkPrune d c target op bound = true) (a : Assignment)
    (hd : Compatible d a) (hs : c.eval a = true) : op.holds bound (a[target]?.getD 0) = true := by
  cases hbad : op.holds bound (a[target]?.getD 0) with
  | true => rfl
  | false =>
    exact False.elim (checkConflict_sound _ c hc a (compatible_rejected d a hd target op bound hbad) hs)

theorem checkPrune_sound_finite (d : List (List Int)) (c : Constraint)
    (target : Nat) (op : Operation) (bound : Int)
    (hc : checkPrune (ofLists d) c target op bound = true)
    (a : Assignment) (hd : InDomains d a) (hs : c.eval a = true) :
    op.holds bound (a[target]?.getD 0) = true :=
  checkPrune_sound _ c target op bound hc a (inDomains_compatible d a hd) hs

/-- The selected relation must actually occur in the model. A claimed runtime
    name or arbitrary constraint declaration is not a logical assumption. -/
theorem checkModelPrune_sound (m : Model) (d : List (List Int)) (c : Constraint)
    (hm : c ∈ m.constraints) (target : Nat) (op : Operation) (bound : Int)
    (hc : checkPrune (ofLists d) c target op bound = true)
    (a : Assignment) (hs : m.IsSolution a) (hd : InDomains d a) :
    op.holds bound (a[target]?.getD 0) = true :=
  checkPrune_sound_finite d c target op bound hc a hd (List.all_eq_true.mp hs.2 c hm)

theorem checkModelConflict_sound (m : Model) (d : List (List Int)) (c : Constraint)
    (hm : c ∈ m.constraints) (hc : checkConflict (ofLists d) c = true)
    (a : Assignment) (hs : m.IsSolution a) (hd : InDomains d a) : False :=
  checkConflict_sound _ c hc a (inDomains_compatible d a hd) (List.all_eq_true.mp hs.2 c hm)

/-- The actual finite-domain update retains every model solution compatible
    with the pre-update domains. No distinctness assumption on sum terms. -/
theorem checkPrune_preservesSolutions (m : Model) (d : List (List Int)) (c : Constraint)
    (hm : c ∈ m.constraints) (target : Nat) (op : Operation) (bound : Int)
    (hc : checkPrune (ofLists d) c target op bound = true)
    (a : Assignment) (hs : m.IsSolution a) (hd : InDomains d a) :
    InDomains (d.modify target (List.filter (op.holds bound))) a :=
  inDomains_restrict d a target op bound hd (checkModelPrune_sound m d c hm target op bound hc a hs hd)

end Minion.SumProof
