import Minion.SumProof
import Minion.SupportProof
import Minion.BoundsProof
import Minion.ElementProof
import Minion.WeightedProof
import Minion.LexProof
import Minion.CountProof
import Minion.ExtremumProof
import Minion.NvalueProof
import Minion.AllDiffProof
import Minion.TableProof
import Minion.ShortTableProof
import Minion.GccCutProof
import Minion.MatrixProof
import Minion.FrameProof

/-! Registry of proved leaf rules. Adding a family supplies its coverage
    predicate, executable feasibility check and fixed soundness theorem. -/
namespace Minion.LeafProof
open DomainProof

def registry : List LocalProof.FeasibilityRule := [
  ⟨FrameProof.supported, FrameProof.possibleValue, FrameProof.possibleValue_sound⟩,
  ⟨MatrixProof.supported, MatrixProof.possibleValue, MatrixProof.possibleValue_sound⟩,
  ⟨GccCutProof.supported, GccCutProof.possibleValue, GccCutProof.possibleValue_sound⟩,
  ⟨ShortTableProof.supported, ShortTableProof.possibleValue, ShortTableProof.possibleValue_sound⟩,
  ⟨TableProof.supported, TableProof.possibleValue, TableProof.possibleValue_sound⟩,
  ⟨AllDiffProof.supported, AllDiffProof.possibleValue, AllDiffProof.possibleValue_sound⟩,
  ⟨NvalueProof.supported, NvalueProof.possibleValue, NvalueProof.possibleValue_sound⟩,
  ⟨ExtremumProof.supported, ExtremumProof.possibleValue, ExtremumProof.possibleValue_sound⟩,
  ⟨CountProof.supported, CountProof.possibleValue, CountProof.possibleValue_sound⟩,
  ⟨LexProof.supported, LexProof.possibleValue, LexProof.possibleValue_sound⟩,
  ⟨WeightedProof.supported, WeightedProof.possibleValue, WeightedProof.possibleValue_sound⟩,
  ⟨ElementProof.supported, ElementProof.possibleValue, ElementProof.possibleValue_sound⟩,
  ⟨BoundsProof.supported, BoundsProof.possibleValue, BoundsProof.possibleValue_sound⟩,
  ⟨SumProof.supported, SumProof.possibleValue, SumProof.possibleValue_sound⟩]

def possibleValue (d : Domains) (expected : Bool) (c : Constraint) : Bool :=
  LocalProof.choosePossible registry d expected c

theorem possibleValue_sound (d : Domains) (expected : Bool) (c : Constraint) (a : Assignment)
    (hd : Compatible d a) (hs : c.eval a = expected) : possibleValue d expected c = true :=
  LocalProof.choosePossible_sound registry d expected c a hd hs

def checkConflict (d : Domains) (c : Constraint) : Bool := !possibleValue d true c

theorem checkConflict_sound (d : Domains) (c : Constraint) (hc : checkConflict d c = true)
    (a : Assignment) (hd : Compatible d a) (hs : c.eval a = true) : False := by
  have hp := possibleValue_sound d true c a hd hs
  simp [checkConflict, hp] at hc

def rule : LocalProof.Rule := ⟨checkConflict, checkConflict_sound⟩

def supported (c : Constraint) : Bool := registry.any (fun r => r.supported c)

end Minion.LeafProof
