import Minion.BoundsProof

/-! Fixed index/result/selected-entry support rules for every element semantic
    variant. The rule supports both truth and falsity, for logical composition.
    It does not enumerate assignments to the whole array. -/
namespace Minion.ElementProof
open DomainProof

def entryPossible (d : Domains) (xs : List Term) (iv rv offset : Int)
    (undefZero expected : Bool) : Bool :=
  let i := iv - offset
  if i < 0 || i ≥ (xs.length : Int) then (undefZero && rv == 0) == expected
  else
    match xs[i.toNat]? with
    | some t => (values d t).any fun v => (v == rv) == expected
    | none => !expected

theorem entryPossible_sound (d : Domains) (xs : List Term) (index rhs : Term)
    (offset : Int) (undefZero expected : Bool) (a : Assignment) (hd : Compatible d a)
    (hs : (Constraint.element xs index rhs offset undefZero).eval a = expected) :
    entryPossible d xs (index.eval a) (rhs.eval a) offset undefZero expected = true := by
  cases hbad : (index.eval a - offset < 0 || index.eval a - offset ≥ (xs.length : Int) : Bool) with
  | true =>
    have hsem : (undefZero && rhs.eval a == 0) = expected := by
      simpa [Constraint.eval, hbad] using hs
    simpa [entryPossible, hbad] using (beq_iff_eq.mpr hsem)
  | false =>
    cases ht : xs[(index.eval a - offset).toNat]? with
    | none =>
      have hsem : false = expected := by simpa [Constraint.eval, hbad, ht] using hs
      simp [entryPossible, hbad, ht, ← hsem]
    | some t =>
      have hsem : (t.eval a == rhs.eval a) = expected := by
        simpa [Constraint.eval, hbad, ht] using hs
      simp only [entryPossible, hbad, Bool.false_eq_true, ↓reduceIte, ht]
      exact List.any_eq_true.mpr ⟨t.eval a, values_sound d a hd t, beq_iff_eq.mpr hsem⟩

def entryConstraint (xs : List Term) (iv : Int) (rhs : Term) (offset : Int) (undefZero : Bool) : Constraint :=
  let i := iv - offset
  if i < 0 || i ≥ (xs.length : Int) then
    if undefZero then .compare .eq rhs (.constant 0) else .truth false
  else match xs[i.toNat]? with
    | some t => .compare .eq t rhs
    | none => .truth false

theorem entryConstraint_eval (xs : List Term) (index rhs : Term) (offset : Int)
    (undefZero : Bool) (a : Assignment) :
    (entryConstraint xs (index.eval a) rhs offset undefZero).eval a =
      (Constraint.element xs index rhs offset undefZero).eval a := by
  cases hbad : (index.eval a - offset < 0 || index.eval a - offset ≥ (xs.length : Int) : Bool) with
  | true => cases undefZero <;> simp [entryConstraint, Constraint.eval, Comparison.eval, Term.eval, hbad]
  | false =>
    cases ht : xs[(index.eval a - offset).toNat]? <;>
      simp [entryConstraint, Constraint.eval, Comparison.eval, hbad, ht]

def possibleValue (d : Domains) (expected : Bool) : Constraint → Bool
  | .element xs index rhs offset undefZero =>
      (values d index).any fun iv =>
        BoundsProof.possibleValue (bindTerm d index iv) expected (entryConstraint xs iv rhs offset undefZero)
  | _ => true

theorem possibleValue_sound (d : Domains) (expected : Bool) (c : Constraint) (a : Assignment)
    (hd : Compatible d a) (hs : c.eval a = expected) : possibleValue d expected c = true := by
  cases c <;> try rfl
  rename_i xs index rhs offset undefZero
  exact List.any_eq_true.mpr ⟨index.eval a, values_sound d a hd index,
    BoundsProof.possibleValue_sound _ expected _ a (compatible_bindTerm d index a hd)
      ((entryConstraint_eval xs index rhs offset undefZero a).trans hs)⟩

def checkConflict (d : Domains) (c : Constraint) : Bool := !possibleValue d true c

theorem checkConflict_sound (d : Domains) (c : Constraint) (hc : checkConflict d c = true)
    (a : Assignment) (hd : Compatible d a) (hs : c.eval a = true) : False := by
  have hp := possibleValue_sound d true c a hd hs
  simp [checkConflict, hp] at hc

def rule : LocalProof.Rule := ⟨checkConflict, checkConflict_sound⟩

def supported : Constraint → Bool
  | .element .. => true
  | _ => false

end Minion.ElementProof
