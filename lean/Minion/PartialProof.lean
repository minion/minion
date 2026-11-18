import Minion.BlockSearchProof

/-! A search which stops once it has enough solutions. Wherever the search
    took the left branch of a decision and never returned for the right one,
    the reader adds a constraint saying that below that point the decision
    holds, and closes the unexplored branch as a conflict with it. The
    explored part of the search is then a complete search of the model with
    those constraints added, so the existing search theorem applies unchanged:
    the solutions found are exactly the model's solutions which satisfy every
    added constraint. In particular each is a solution, and everything
    searched before it was correctly refuted. -/
namespace Minion.PartialProof
open SearchProof

def restricted (m : Model) (assumed : List Constraint) : Model :=
  { m with constraints := m.constraints ++ assumed }

theorem restricted_solution (m : Model) (assumed : List Constraint) (a : Assignment) :
    (restricted m assumed).IsSolution a ↔
      m.IsSolution a ∧ ∀ c ∈ assumed, c.eval a = true := by
  simp only [Model.IsSolution, restricted, List.all_append, Bool.and_eq_true, List.all_eq_true]
  constructor
  · rintro ⟨hd, hc, hp⟩
    exact ⟨⟨hd, hc⟩, hp⟩
  · rintro ⟨⟨hd, hc⟩, hp⟩
    exact ⟨hd, hc, hp⟩

def check (m : Model) (assumed : List Constraint) (p : BlockSearchProof.Program)
    (reported : List Assignment) (indexed : Bool := false) : Bool :=
  BlockSearchProof.verify (restricted m assumed) p reported indexed

theorem check_correct (m : Model) (assumed : List Constraint) (p : BlockSearchProof.Program)
    (reported : List Assignment) (indexed : Bool)
    (h : check m assumed p reported indexed = true) :
    ∀ a, a ∈ reported ↔ m.IsSolution a ∧ ∀ c ∈ assumed, c.eval a = true := by
  intro a
  rw [← restricted_solution]
  exact verify_correct _ _ reported (by simpa only [check, BlockSearchProof.verify_eq] using h) a

/-- "Below this point in the search, the variable has this value": either some
    earlier decision on the path does not hold, or it does. -/
def below (path : List (Nat × Int × Bool)) (target : Nat) (value : Int) : Constraint :=
  .disjunction (path.map (fun p => .compare (if p.2.2 then .ne else .eq) (.var p.1) (.constant p.2.1)) ++
    [.compare .eq (.var target) (.constant value)])

end Minion.PartialProof
