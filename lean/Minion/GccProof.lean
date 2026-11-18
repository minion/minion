import Minion.CountProof

/-! GCC's individual count equations share the occurrence bounds rule. Joint
    capacity reasoning will add independently checked cuts to this base rule. -/
namespace Minion.GccProof
open DomainProof

def possibleValue (d : Domains) (expected : Bool) : Constraint → Bool
  | .gcc xs vs ns =>
      if vs.length == ns.length then
        if expected then (vs.zip ns).all fun (v, n) => CountProof.possibleValue d true (.occurrence xs v n .eq)
        else (vs.zip ns).any fun (v, n) => CountProof.possibleValue d false (.occurrence xs v n .eq)
      else !expected
  | _ => true

theorem possibleValue_sound (d : Domains) (expected : Bool) (c : Constraint) (a : Assignment)
    (hd : Compatible d a) (hs : c.eval a = expected) : possibleValue d expected c = true := by
  cases c <;> try rfl
  rename_i xs vs ns
  cases hlen : vs.length == ns.length with
  | false =>
    have he : false = expected := by simpa [Constraint.eval, hlen] using hs
    simp [possibleValue, hlen, ← he]
  | true =>
    simp only [Constraint.eval, hlen, Bool.true_and] at hs
    simp only [possibleValue, hlen, ↓reduceIte]
    cases expected with
    | true =>
      apply List.all_eq_true.mpr
      intro p hp
      apply CountProof.possibleValue_sound d true (.occurrence xs p.1 p.2 .eq) a hd
      simpa only [Constraint.eval, Comparison.eval] using List.all_eq_true.mp hs p hp
    | false =>
      obtain ⟨p, hp, he⟩ := List.all_eq_false.mp hs
      apply List.any_eq_true.mpr
      refine ⟨p, hp, CountProof.possibleValue_sound d false (.occurrence xs p.1 p.2 .eq) a hd ?_⟩
      simpa only [Constraint.eval, Comparison.eval, Bool.not_eq_true] using he

def supported : Constraint → Bool
  | .gcc .. => true
  | _ => false

end Minion.GccProof
