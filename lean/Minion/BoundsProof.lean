import Minion.FunctionalProof

/-! Proved interval checks short-circuit expensive local support searches.
    Support enumeration remains the fallback when bounds alone are inconclusive. -/
namespace Minion.BoundsProof
open DomainProof

def intervalPossible (op : Comparison) (expected : Bool) (lx ux ly uy : Int) : Bool :=
  match op, expected with
  | .eq, true | .ne, false => lx ≤ uy && ly ≤ ux
  | .eq, false | .ne, true => lx < uy || ly < ux
  | .lt, true => lx < uy
  | .lt, false => ly ≤ ux
  | .le, true => lx ≤ uy
  | .le, false => ly < ux
  | .ge, true => ly ≤ ux
  | .ge, false => lx < uy

theorem intervalPossible_sound (op : Comparison) (expected : Bool) (lx ux ly uy x y : Int)
    (hxl : lx ≤ x) (hxu : x ≤ ux) (hyl : ly ≤ y) (hyu : y ≤ uy)
    (hs : op.eval x y = expected) : intervalPossible op expected lx ux ly uy = true := by
  cases op <;> cases expected <;>
    simp_all only [intervalPossible, Comparison.eval, Bool.and_eq_true, Bool.or_eq_true,
      beq_iff_eq, bne_iff_ne, beq_eq_false_iff_ne, bne_eq_false_iff_eq,
      decide_eq_true_eq, decide_eq_false_iff_not]
  all_goals omega

def absBounds (lo hi : Int) : Int × Int :=
  (if 0 ≤ lo then lo else if hi ≤ 0 then -hi else 0, max (lo.natAbs : Int) (hi.natAbs : Int))

theorem absBounds_sound (lo hi v : Int) (hl : lo ≤ v) (hu : v ≤ hi) :
    (absBounds lo hi).1 ≤ (v.natAbs : Int) ∧ (v.natAbs : Int) ≤ (absBounds lo hi).2 := by
  have hmaxlo := Int.le_max_left (lo.natAbs : Int) (hi.natAbs : Int)
  have hmaxhi := Int.le_max_right (lo.natAbs : Int) (hi.natAbs : Int)
  have hla : lo ≤ (lo.natAbs : Int) := Int.le_natAbs
  have hha : hi ≤ (hi.natAbs : Int) := Int.le_natAbs
  have hlna : -lo ≤ (lo.natAbs : Int) := by simpa using (Int.le_natAbs (a := -lo))
  by_cases hv : 0 ≤ v
  · rw [Int.natAbs_of_nonneg hv]
    simp only [absBounds]
    split <;> (try split) <;> omega
  · have hn : v ≤ 0 := by omega
    rw [Int.ofNat_natAbs_of_nonpos hn]
    simp only [absBounds]
    split <;> (try split) <;> omega

theorem mul_endpoint_bounds (lo hi v k low high : Int) (hl : lo ≤ v) (hu : v ≤ hi)
    (hlo : low ≤ lo * k ∧ lo * k ≤ high) (hhi : low ≤ hi * k ∧ hi * k ≤ high) :
    low ≤ v * k ∧ v * k ≤ high := by
  by_cases hk : 0 ≤ k
  · have ha := Int.mul_le_mul_of_nonneg_right hl hk
    have hb := Int.mul_le_mul_of_nonneg_right hu hk
    omega
  · have hn : k ≤ 0 := by omega
    have ha := Int.mul_le_mul_of_nonpos_right hl hn
    have hb := Int.mul_le_mul_of_nonpos_right hu hn
    omega

def productBounds (lx ux ly uy : Int) : Int × Int :=
  (min (min (lx * ly) (lx * uy)) (min (ux * ly) (ux * uy)),
   max (max (lx * ly) (lx * uy)) (max (ux * ly) (ux * uy)))

theorem productBounds_sound (lx ux ly uy x y : Int) (hxl : lx ≤ x) (hxu : x ≤ ux)
    (hyl : ly ≤ y) (hyu : y ≤ uy) :
    (productBounds lx ux ly uy).1 ≤ x * y ∧ x * y ≤ (productBounds lx ux ly uy).2 := by
  let low := (productBounds lx ux ly uy).1
  let high := (productBounds lx ux ly uy).2
  have hc : (low ≤ lx * ly ∧ lx * ly ≤ high) ∧ (low ≤ lx * uy ∧ lx * uy ≤ high) ∧
      (low ≤ ux * ly ∧ ux * ly ≤ high) ∧ (low ≤ ux * uy ∧ ux * uy ≤ high) := by
    dsimp [low, high, productBounds]
    constructor
    · exact ⟨Int.le_trans (Int.min_le_left _ _) (Int.min_le_left _ _),
        Int.le_trans (Int.le_max_left _ _) (Int.le_max_left _ _)⟩
    constructor
    · exact ⟨Int.le_trans (Int.min_le_left _ _) (Int.min_le_right _ _),
        Int.le_trans (Int.le_max_right _ _) (Int.le_max_left _ _)⟩
    constructor
    · exact ⟨Int.le_trans (Int.min_le_right _ _) (Int.min_le_left _ _),
        Int.le_trans (Int.le_max_left _ _) (Int.le_max_right _ _)⟩
    · exact ⟨Int.le_trans (Int.min_le_right _ _) (Int.min_le_right _ _),
        Int.le_trans (Int.le_max_right _ _) (Int.le_max_right _ _)⟩
  have hlo := mul_endpoint_bounds lx ux x ly low high hxl hxu hc.1 hc.2.2.1
  have hhi := mul_endpoint_bounds lx ux x uy low high hxl hxu hc.2.1 hc.2.2.2
  have hb := mul_endpoint_bounds ly uy y x low high hyl hyu
    (by simpa only [Int.mul_comm] using hlo) (by simpa only [Int.mul_comm] using hhi)
  simpa only [Int.mul_comm] using hb

def quickPossible (d : Domains) (expected : Bool) : Constraint → Bool
  | .compare op x y => intervalPossible op expected (lower d x) (upper d x) (lower d y) (upper d y)
  | .minusEq x y => intervalPossible .eq expected (lower d x) (upper d x) (-upper d y) (-lower d y)
  | .absolute x y =>
    let b := absBounds (lower d y) (upper d y)
    intervalPossible .eq expected (lower d x) (upper d x) b.1 b.2
  | .difference x y z =>
    let b := absBounds (lower d x - upper d y) (upper d x - lower d y)
    intervalPossible .eq expected b.1 b.2 (lower d z) (upper d z)
  | .product x y z =>
    let b := productBounds (lower d x) (upper d x) (lower d y) (upper d y)
    intervalPossible .eq expected b.1 b.2 (lower d z) (upper d z)
  | .inequality x y k =>
    intervalPossible .le expected (lower d x) (upper d x) (lower d y + lower d k) (upper d y + upper d k)
  | _ => true

theorem quickPossible_sound (d : Domains) (expected : Bool) (c : Constraint) (a : Assignment)
    (hd : Compatible d a) (hs : c.eval a = expected) : quickPossible d expected c = true := by
  have hb (t) : lower d t ≤ t.eval a ∧ t.eval a ≤ upper d t :=
    ⟨lower_sound d a hd t, upper_sound d a hd t⟩
  cases c with
  | compare op x y =>
    exact intervalPossible_sound op expected _ _ _ _ _ _ (hb x).1 (hb x).2 (hb y).1 (hb y).2
      (by simpa only [Constraint.eval] using hs)
  | minusEq x y =>
    apply intervalPossible_sound .eq expected _ _ _ _ (x.eval a) (-y.eval a) (hb x).1 (hb x).2
    · have := (hb y).2; omega
    · have := (hb y).1; omega
    · simpa only [Constraint.eval, Comparison.eval] using hs
  | absolute x y =>
    have ha := absBounds_sound (lower d y) (upper d y) (y.eval a) (hb y).1 (hb y).2
    exact intervalPossible_sound .eq expected _ _ _ _ (x.eval a) (y.eval a).natAbs
      (hb x).1 (hb x).2 ha.1 ha.2 (by simpa only [Constraint.eval, Comparison.eval] using hs)
  | difference x y z =>
    have hxl := (hb x).1; have hxu := (hb x).2
    have hyl := (hb y).1; have hyu := (hb y).2
    have ha := absBounds_sound (lower d x - upper d y) (upper d x - lower d y) (x.eval a - y.eval a)
      (by omega) (by omega)
    exact intervalPossible_sound .eq expected _ _ _ _ (x.eval a - y.eval a).natAbs (z.eval a)
      ha.1 ha.2 (hb z).1 (hb z).2 (by simpa only [Constraint.eval, Comparison.eval] using hs)
  | product x y z =>
    have hp := productBounds_sound (lower d x) (upper d x) (lower d y) (upper d y) (x.eval a) (y.eval a)
      (hb x).1 (hb x).2 (hb y).1 (hb y).2
    exact intervalPossible_sound .eq expected _ _ _ _ (x.eval a * y.eval a) (z.eval a)
      hp.1 hp.2 (hb z).1 (hb z).2 (by simpa only [Constraint.eval, Comparison.eval] using hs)
  | inequality x y k =>
    have hyl := (hb y).1; have hyu := (hb y).2
    have hkl := (hb k).1; have hku := (hb k).2
    apply intervalPossible_sound .le expected _ _ _ _ (x.eval a) (y.eval a + k.eval a) (hb x).1 (hb x).2
    · omega
    · omega
    · simpa only [Constraint.eval, Comparison.eval] using hs
  | _ => rfl

def possibleValue (d : Domains) (expected : Bool) (c : Constraint) : Bool :=
  if quickPossible d expected c then FunctionalProof.possibleValue d expected c else false

theorem possibleValue_sound (d : Domains) (expected : Bool) (c : Constraint) (a : Assignment)
    (hd : Compatible d a) (hs : c.eval a = expected) : possibleValue d expected c = true := by
  simp only [possibleValue, quickPossible_sound d expected c a hd hs, ↓reduceIte]
  exact FunctionalProof.possibleValue_sound d expected c a hd hs

def supported := SupportProof.supported

end Minion.BoundsProof
