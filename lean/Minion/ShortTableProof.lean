import Minion.BoundedProof

/-! Short rows are conjunctions of position predicates. A compressed row gives
    alternative allowed values at each repeated position. The negation uses
    conservative row failures plus a one-free-variable forward-checking rule. -/
namespace Minion.ShortTableProof
open DomainProof

def groupCheck (row : List (Nat × Int)) (i : Nat) (value : Option Int) : Bool :=
  row.any fun (j, v) => i == j && value == some v

def groupMay (d : Domains) (xs : List Term) (row : List (Nat × Int))
    (i : Nat) (expected : Bool) : Bool :=
  match xs[i]? with
  | none => groupCheck row i none == expected
  | some t => (values d t).any fun v => groupCheck row i (some v) == expected

theorem groupMay_sound (d : Domains) (xs : List Term) (row : List (Nat × Int))
    (i : Nat) (expected : Bool) (a : Assignment) (hd : Compatible d a)
    (hs : groupCheck row i ((xs.map (Term.eval a))[i]?) = expected) :
    groupMay d xs row i expected = true := by
  cases hi : xs[i]? with
  | none => simpa [groupMay, hi, List.getElem?_map] using hs
  | some t =>
    simp only [List.getElem?_map, hi, Option.map_some] at hs
    simp only [groupMay, hi]
    exact List.any_eq_true.mpr ⟨t.eval a, values_sound d a hd t, by simp [hs]⟩

def literalMay (d : Domains) (xs : List Term) (p : Nat × Int) (expected : Bool) : Bool :=
  groupMay d xs [p] p.1 expected

theorem literalMay_sound (d : Domains) (xs : List Term) (p : Nat × Int)
    (expected : Bool) (a : Assignment) (hd : Compatible d a)
    (hs : ((xs.map (Term.eval a))[p.1]? == some p.2) = expected) :
    literalMay d xs p expected = true := by
  apply groupMay_sound d xs [p] p.1 expected a hd
  simpa [groupCheck] using hs

def rowMay (d : Domains) (xs : List Term) (row : List (Nat × Int))
    (compressed expected : Bool) : Bool :=
  if compressed then
    if expected then (row.map Prod.fst).eraseDups.all (fun i => groupMay d xs row i true)
    else (row.map Prod.fst).eraseDups.any (fun i => groupMay d xs row i false)
  else if expected then row.all (fun p => literalMay d xs p true)
    else row.any (fun p => literalMay d xs p false)

theorem rowMay_sound (d : Domains) (xs : List Term) (row : List (Nat × Int))
    (compressed expected : Bool) (a : Assignment) (hd : Compatible d a)
    (hs : shortRowCheck (xs.map (Term.eval a)) row compressed = expected) :
    rowMay d xs row compressed expected = true := by
  cases compressed <;> cases expected
  · simp only [shortRowCheck, Bool.false_eq_true, ↓reduceIte] at hs
    obtain ⟨p, hp, he⟩ := List.all_eq_false.mp hs
    exact List.any_eq_true.mpr ⟨p, hp, literalMay_sound d xs p false a hd (by simpa using he)⟩
  · simp only [shortRowCheck, Bool.false_eq_true, ↓reduceIte] at hs
    exact List.all_eq_true.mpr fun p hp => literalMay_sound d xs p true a hd (List.all_eq_true.mp hs p hp)
  · simp only [shortRowCheck, ↓reduceIte] at hs
    obtain ⟨i, hi, he⟩ := List.all_eq_false.mp hs
    exact List.any_eq_true.mpr ⟨i, hi, groupMay_sound d xs row i false a hd (by simpa [groupCheck] using he)⟩
  · simp only [shortRowCheck, ↓reduceIte] at hs
    exact List.all_eq_true.mpr fun i hi => groupMay_sound d xs row i true a hd (List.all_eq_true.mp hs i hi)

def possibleValue (d : Domains) (expected : Bool) : Constraint → Bool
  | .shortTable xs rows compressed =>
      (if expected then rows.any (fun row => rowMay d xs row compressed true)
       else rows.all (fun row => rowMay d xs row compressed false)) &&
      BoundedProof.possible d xs (fun vs => shortTableCheck vs rows compressed == expected) 1
  | _ => true

theorem possibleValue_sound (d : Domains) (expected : Bool) (c : Constraint) (a : Assignment)
    (hd : Compatible d a) (hs : c.eval a = expected) : possibleValue d expected c = true := by
  cases c <;> try rfl
  rename_i xs rows compressed
  simp only [Constraint.eval] at hs
  simp only [possibleValue, Bool.and_eq_true]
  constructor
  · cases expected with
    | true =>
      obtain ⟨row, hr, he⟩ := List.any_eq_true.mp hs
      exact List.any_eq_true.mpr ⟨row, hr, rowMay_sound d xs row compressed true a hd he⟩
    | false =>
      exact List.all_eq_true.mpr fun row hr =>
        rowMay_sound d xs row compressed false a hd (by simpa using List.any_eq_false.mp hs row hr)
  · exact BoundedProof.possible_sound d xs _ 1 a hd (by simp [hs])

def supported : Constraint → Bool
  | .shortTable .. => true
  | _ => false

end Minion.ShortTableProof
