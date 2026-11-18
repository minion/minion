import Minion.Semantics

/-! A balanced positional index built without a sequence of persistent writes.
    Its lookup theorem includes out-of-range indices. -/
namespace Minion.SequenceIndex

inductive Index (α : Type) where
  | empty
  | node (leftSize : Nat) (left : Index α) (value : α) (right : Index α)

def get : Index α → Nat → Option α
  | .empty, _ => none
  | .node n left value right, i =>
    if i < n then get left i else if i = n then some value else get right (i - n - 1)

def build (xs : List α) : Index α :=
  if h : xs.length = 0 then .empty
  else
    let n := xs.length / 2
    have hn : n < xs.length := Nat.div_lt_self (by omega) (by omega)
    .node n (build (xs.take n)) xs[n] (build (xs.drop (n + 1)))
termination_by xs.length
decreasing_by
  · simp only [List.length_take]
    exact Nat.lt_of_le_of_lt (Nat.min_le_left _ _) hn
  · simp only [List.length_drop]
    omega

theorem get_build (xs : List α) (i : Nat) : get (build xs) i = xs[i]? := by
  rw [build]
  split
  · rename_i h
    have he : xs = [] := List.length_eq_zero_iff.mp h
    subst xs
    rfl
  · rename_i h
    simp only [get]
    split
    · rename_i hi
      rw [get_build, List.getElem?_take_of_lt hi]
    · rename_i hi
      split
      · rename_i he
        subst i
        exact (List.getElem?_eq_getElem (Nat.div_lt_self (by omega) (by omega))).symm
      · rename_i he
        rw [get_build, List.getElem?_drop]
        congr 1
        omega
termination_by xs.length
decreasing_by
  all_goals simp only [List.length_take, List.length_drop]
  all_goals omega

end Minion.SequenceIndex
