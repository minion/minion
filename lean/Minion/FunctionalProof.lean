import Minion.SupportProof

/-! Compute a functional relation's result once per input pair, then check
    membership, instead of recomputing the relation for every result candidate.
    Bindings retain aliases and views. False-polarity support keeps the general
    fallback, since its relation is no longer functional. -/
namespace Minion.FunctionalProof
open DomainProof

def possibleResult (d : Domains) (x y z : Term) (f : Int → Int → Option Int) : Bool :=
  (values d x).any fun xv =>
    let dx := bindTerm d x xv
    (values dx y).any fun yv =>
      match f xv yv with
      | none => false
      | some value => (values (bindTerm dx y yv) z).contains value

theorem possibleResult_sound (d : Domains) (x y z : Term) (f : Int → Int → Option Int)
    (a : Assignment) (hd : Compatible d a) (hf : f (x.eval a) (y.eval a) = some (z.eval a)) :
    possibleResult d x y z f = true := by
  refine List.any_eq_true.mpr ⟨x.eval a, values_sound d a hd x, ?_⟩
  refine List.any_eq_true.mpr ⟨y.eval a, values_sound _ a (compatible_bindTerm d x a hd) y, ?_⟩
  simp only [hf]
  simpa using values_sound _ a
    (compatible_bindTerm _ y a (compatible_bindTerm d x a hd)) z

def divideResult (x y : Int) (undefZero : Bool) : Option Int :=
  if y == 0 then if undefZero then some 0 else none else some (x.fdiv y)

def moduloResult (x y : Int) (undefZero : Bool) : Option Int :=
  if y == 0 then if undefZero then some 0 else none else some (x.fmod y)

def powerResult (x exponent : Int) : Option Int :=
  if exponent < 0 then
    if x == 1 then some 1
    else if x == -1 then some (if exponent % 2 == 0 then 1 else -1)
    else none
  else some (x ^ exponent.toNat)

def resultCap : List Int → Nat
  | [] => 0
  | v :: vs => max v.natAbs (resultCap vs)

theorem resultCap_bound (vs : List Int) (v : Int) (hv : v ∈ vs) : v.natAbs ≤ resultCap vs := by
  induction vs with
  | nil => simp at hv
  | cons w ws ih =>
    rcases List.mem_cons.mp hv with rfl | hv
    · exact Nat.le_max_left _ _
    · exact Nat.le_trans (ih hv) (Nat.le_max_right _ _)

def boundedPowerResult (cap : Nat) (x exponent : Int) : Option Int :=
  if PowerProof.quickBound x exponent cap then powerResult x exponent else none

def possibleValue (d : Domains) (expected : Bool) (c : Constraint) : Bool :=
  if !expected then SupportProof.possibleValue d expected c else
    match c with
    | .product x y z => possibleResult d x y z (fun xv yv => some (xv * yv))
    | .difference x y z => possibleResult d x y z (fun xv yv => some ((xv - yv).natAbs : Int))
    | .divide x y z undefZero => possibleResult d x y z (fun xv yv => divideResult xv yv undefZero)
    | .modulo x y z undefZero => possibleResult d x y z (fun xv yv => moduloResult xv yv undefZero)
    | .power x y z => possibleResult d x y z (boundedPowerResult (resultCap (values d z)))
    | c => SupportProof.possibleValue d expected c

theorem possibleValue_sound (d : Domains) (expected : Bool) (c : Constraint) (a : Assignment)
    (hd : Compatible d a) (hs : c.eval a = expected) : possibleValue d expected c = true := by
  cases expected with
  | false => exact SupportProof.possibleValue_sound d false c a hd hs
  | true =>
    simp only [possibleValue, Bool.not_true, Bool.false_eq_true, ↓reduceIte]
    cases c with
    | product x y z =>
      apply possibleResult_sound _ _ _ _ _ a hd
      simpa only [Constraint.eval, beq_iff_eq, Option.some.injEq] using hs
    | difference x y z =>
      apply possibleResult_sound _ _ _ _ _ a hd
      simpa only [Constraint.eval, beq_iff_eq, Option.some.injEq] using hs
    | divide x y z undefZero =>
      apply possibleResult_sound _ _ _ _ _ a hd
      by_cases hy : y.eval a = 0 <;> cases undefZero <;> simp_all [divideResult, Constraint.eval]
    | modulo x y z undefZero =>
      apply possibleResult_sound _ _ _ _ _ a hd
      by_cases hy : y.eval a = 0 <;> cases undefZero <;> simp_all [moduloResult, Constraint.eval]
    | power x y z =>
      apply possibleResult_sound _ _ _ _ _ a hd
      have hb := PowerProof.quickBound_sound (x.eval a) (y.eval a) (z.eval a)
        (resultCap (values d z)) (by simpa only [Constraint.eval] using hs)
        (resultCap_bound _ _ (values_sound d a hd z))
      simp only [boundedPowerResult, hb, ↓reduceIte]
      by_cases hy : y.eval a < 0 <;> by_cases hx : x.eval a = 1 <;>
        by_cases hm : x.eval a = -1 <;> simp_all [powerResult, powerEq, Constraint.eval]
      all_goals omega
    | _ => exact SupportProof.possibleValue_sound d true _ a hd hs

end Minion.FunctionalProof
