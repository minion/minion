import Minion.BlockSearchProof

/-! Optimality from a refutation. A branch-and-bound search which ends with
    best objective value `v` is, read as a whole, a refutation of the model
    with the extra constraint "the objective is strictly better than `v`":
    every bound the solver applied on the way was weaker than that constraint,
    and every solution it found on the way violates it. No search theorem
    beyond the existing one is needed. -/
namespace Minion.OptimalProof
open SearchProof

def value (objective : Nat) (a : Assignment) : Int := a[objective]?.getD 0

/-- The objective variable is strictly better than `v`. -/
def better (minimising : Bool) (objective : Nat) (v : Int) : Constraint :=
  if minimising then .compare .lt (.var objective) (.constant v)
  else .compare .lt (.constant v) (.var objective)

def improved (m : Model) (minimising : Bool) (objective : Nat) (v : Int) : Model :=
  { m with constraints := m.constraints ++ [better minimising objective v] }

/-- `best` is a solution, and the program refutes any strictly better one. -/
def check (m : Model) (minimising : Bool) (objective : Nat) (best : Assignment)
    (p : BlockSearchProof.Program) (indexed : Bool := false) : Bool :=
  m.satisfies best &&
    BlockSearchProof.verify (improved m minimising objective (value objective best)) p [] indexed

theorem check_optimal (m : Model) (minimising : Bool) (objective : Nat) (best : Assignment)
    (p : BlockSearchProof.Program) (indexed : Bool)
    (h : check m minimising objective best p indexed = true) :
    m.IsSolution best ∧ ∀ a, m.IsSolution a →
      if minimising then value objective best ≤ value objective a
      else value objective a ≤ value objective best := by
  simp only [check, Bool.and_eq_true] at h
  refine ⟨(satisfies_correct m best).mp h.1, ?_⟩
  intro a ha
  have hunsat := verify_unsat _ _ (by simpa only [BlockSearchProof.verify_eq] using h.2) a
  cases minimising with
  | true =>
    simp only [↓reduceIte]
    apply Classical.byContradiction
    intro hlt
    apply hunsat
    refine ⟨ha.1, ?_⟩
    simp only [improved, better, ↓reduceIte, List.all_append, ha.2, Bool.true_and, List.all_cons,
      List.all_nil, Bool.and_true, Constraint.eval, Comparison.eval, Term.eval, value] at *
    exact decide_eq_true (Int.lt_of_not_ge hlt)
  | false =>
    simp only [Bool.false_eq_true, ↓reduceIte]
    apply Classical.byContradiction
    intro hlt
    apply hunsat
    refine ⟨ha.1, ?_⟩
    simp only [improved, better, Bool.false_eq_true, ↓reduceIte, List.all_append, ha.2, Bool.true_and,
      List.all_cons, List.all_nil, Bool.and_true, Constraint.eval, Comparison.eval, Term.eval, value] at *
    exact decide_eq_true (Int.lt_of_not_ge hlt)

end Minion.OptimalProof
