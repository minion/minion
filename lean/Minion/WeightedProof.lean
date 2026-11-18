import Minion.SumProof
import Minion.SupportProof

/-! Weighted inequalities reuse canonical sum rules after a proved conversion
    to scaling views, including negative and zero coefficients. -/
namespace Minion.WeightedProof
open DomainProof

def possibleValue (d : Domains) (expected : Bool) (c : Constraint) : Bool :=
  if SupportProof.supported c.expandWeighted then SupportProof.possibleValue d expected c.expandWeighted
  else SumProof.possibleValue d expected c.expandWeighted

theorem possibleValue_sound (d : Domains) (expected : Bool) (c : Constraint) (a : Assignment)
    (hd : Compatible d a) (hs : c.eval a = expected) : possibleValue d expected c = true := by
  have he : c.expandWeighted.eval a = expected := by rw [Constraint.expandWeighted_eval]; exact hs
  simp only [possibleValue]
  split
  · exact SupportProof.possibleValue_sound d expected c.expandWeighted a hd he
  · exact SumProof.possibleValue_sound d expected c.expandWeighted a hd he

def supported : Constraint → Bool
  | .weightedSum ws _ _ .le | .weightedSum ws _ _ .ge => (constantValues ws).isSome
  | _ => false

end Minion.WeightedProof
