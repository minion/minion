import Minion.LocalProof

/-! Shared full-table rules. Positive support filters domains along a row.
    Negative support searches only prefixes present in the forbidden table;
    an absent prefix supplies an allowed tuple without expanding the full
    Cartesian product. Both directions preserve aliases through view bindings. -/
namespace Minion.TableProof
open DomainProof

def rowPossible (d : Domains) : List Term → List Int → Bool
  | [], [] => true
  | x :: xs, v :: vs => (values d x).contains v && rowPossible (bindTerm d x v) xs vs
  | _, _ => false

theorem rowPossible_sound (d : Domains) (xs : List Term) (a : Assignment) (hd : Compatible d a) :
    rowPossible d xs (xs.map (Term.eval a)) = true := by
  induction xs generalizing d with
  | nil => rfl
  | cons x xs ih =>
    simp only [List.map_cons, rowPossible, Bool.and_eq_true]
    exact ⟨by simpa using values_sound d a hd x, ih _ (compatible_bindTerm d x a hd)⟩

def rowTails (rows : List (List Int)) (v : Int) : List (List Int) :=
  rows.filterMap fun row =>
    match row with
    | x :: xs => if x == v then some xs else none
    | [] => none

theorem mem_rowTails (rows : List (List Int)) (v : Int) (tail : List Int) :
    tail ∈ rowTails rows v ↔ v :: tail ∈ rows := by
  constructor
  · intro ht
    obtain ⟨row, hr, he⟩ := List.mem_filterMap.mp ht
    cases row with
    | nil => simp at he
    | cons x xs =>
      by_cases hx : x = v
      · subst x
        simp only [beq_self_eq_true, ↓reduceIte, Option.some.injEq] at he
        rw [← he]
        exact hr
      · simp [hx] at he
  · intro ht
    exact List.mem_filterMap.mpr ⟨v :: tail, ht, by simp⟩

def hasAllowed (d : Domains) (xs : List Term) (rows : List (List Int)) : Bool :=
  if rows.isEmpty then true else
    match xs with
    | [] => !rows.contains []
    | x :: xs => (values d x).any fun v => hasAllowed (bindTerm d x v) xs (rowTails rows v)
termination_by xs.length

theorem hasAllowed_sound (d : Domains) (xs : List Term) (rows : List (List Int))
    (a : Assignment) (hd : Compatible d a) (hn : xs.map (Term.eval a) ∉ rows) :
    hasAllowed d xs rows = true := by
  induction xs generalizing d rows with
  | nil =>
    have hc : rows.contains [] = false := by simpa using hn
    have hnil : [] ∉ rows := by simpa only [List.map_nil] using hn
    simp [hasAllowed, hnil]
  | cons x xs ih =>
    cases he : rows.isEmpty with
    | true => simp [hasAllowed, he]
    | false =>
      simp only [hasAllowed, he, Bool.false_eq_true, ↓reduceIte]
      refine List.any_eq_true.mpr ⟨x.eval a, values_sound d a hd x, ?_⟩
      apply ih _ _ (compatible_bindTerm d x a hd)
      intro ht
      exact hn (by simpa only [List.map_cons] using (mem_rowTails rows (x.eval a) _).mp ht)

def possibleValue (d : Domains) (expected : Bool) : Constraint → Bool
  | .table xs rows positive =>
      if positive == expected then rows.any (rowPossible d xs) else hasAllowed d xs rows
  | _ => true

theorem possibleValue_sound (d : Domains) (expected : Bool) (c : Constraint) (a : Assignment)
    (hd : Compatible d a) (hs : c.eval a = expected) : possibleValue d expected c = true := by
  cases c with
  | table xs rows positive =>
    have he : rows.contains (xs.map (Term.eval a)) = (positive == expected) := by
      have hsem : (rows.contains (xs.map (Term.eval a)) == positive) = expected := by
        simpa only [Constraint.eval] using hs
      cases positive <;> cases expected <;> cases hm : rows.contains (xs.map (Term.eval a)) <;> simp_all
    cases hp : positive == expected with
    | true =>
      have hm : xs.map (Term.eval a) ∈ rows := by simpa using he.trans hp
      simpa only [possibleValue, hp, ↓reduceIte] using
        List.any_eq_true.mpr ⟨xs.map (Term.eval a), hm, rowPossible_sound d xs a hd⟩
    | false =>
      have hn : xs.map (Term.eval a) ∉ rows := by simpa using he.trans hp
      simpa only [possibleValue, hp, Bool.false_eq_true, ↓reduceIte] using hasAllowed_sound d xs rows a hd hn
  | _ => rfl

def supported : Constraint → Bool
  | .table .. => true
  | _ => false

end Minion.TableProof
