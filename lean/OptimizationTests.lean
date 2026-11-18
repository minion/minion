import Minion.SearchProof

open Minion Minion.DomainProof

-- Zero-scale views used to revisit the same forbidden prefix exponentially.
example : TableProof.hasAllowed (fun _ => [0, 1])
    ((List.range 24).map fun i => .scale (.var i) 0) [List.replicate 24 0] = false := by decide +kernel

-- Neither polarity needs to construct this enormous integer.
example : PowerProof.boundedEq 3 16777216 (-1) = false := by decide +kernel
example : FunctionalProof.possibleValue (fun _ => []) true
    (.power (.constant 3) (.constant 16777216) (.constant (-1))) = false := by decide +kernel
example : FunctionalProof.possibleValue (fun _ => []) false
    (.power (.constant 3) (.constant 16777216) (.constant (-1))) = true := by decide +kernel

-- Check negative exponents, sign/parity, and the zero exponent boundary.
example : ([-3, -1, 0, 1, 3].all fun x =>
    [-3, -2, -1, 0, 1, 2, 3].all fun e =>
      [-27, -9, -3, -1, 0, 1, 3, 9, 27].all fun z =>
        PowerProof.boundedEq x e z == powerEq x e z) = true := by decide +kernel

private def impossible (positive : Bool) : Constraint :=
  .reify (.table [.var 0, .var 0] [[0, 1], [1, 0]] positive)
    (.constant (if positive then 1 else 0))
private def m (positive : Bool) : Model := ⟨[⟨"x", [0, 1]⟩], [impossible positive]⟩
example : SearchProof.check (m true) [[0, 1]] (.filterTables 0 (.conflict 0)) = true := by decide +kernel
example : SearchProof.check (m false) [[0, 1]] (.filterTables 0 (.conflict 0)) = true := by decide +kernel
example : (InferenceWitness.rule (.preferFocus [1])).prune (ofLists [[0, 1]])
    (.conjunction [.truth true, .compare .eq (.var 0) (.constant 0)]) 0 .assign 0 = true := by decide +kernel

example : values (fun _ => [0, 1, 2, 3]) (.scale (.var 0) 0) = [0] := by decide +kernel
example : values (fun _ => []) (.scale (.var 0) 0) = [] := by decide +kernel
example : SetProof.cappedCard 2 ([0, 0, 1, 2, 3] : List Int) = 2 := by decide +kernel
example : NvalueProof.singletonValue (fun _ => [3, 3, 3]) (.var 0) = some 3 := by decide +kernel
example : NvalueProof.singletonValue (fun _ => [3, 4, 3]) (.var 0) = none := by decide +kernel

-- Ordered, reverse-ordered, unsorted, and aliased equality candidates.
example : SupportProof.possibleEqual (ofLists [[0, 2, 4], [1, 3, 5]]) (.var 0) (.var 1) = false := by decide +kernel
example : SupportProof.possibleEqual (ofLists [[4, 2, 0], [0, 2, 4]]) (.var 0) (.var 1) = true := by decide +kernel
example : SupportProof.possibleEqual (ofLists [[2, 0, 1]]) (.var 0) (.constant 0) = true := by decide +kernel
example : SupportProof.possibleEqual (ofLists [[0, 1]]) (.var 0) (.notBool 0) = false := by decide +kernel
example : SupportProof.possibleEqual (ofLists [[0, 1, 2]]) (.var 0) (.shift (.var 0) 1) = false := by decide +kernel
example : CountProof.quickCount (ofLists [[3, 4, 5]]) true 2 .ge (.var 0) = false := by decide +kernel

#print axioms Minion.DomainOverlap.mergeValues_complete
#print axioms Minion.SupportProof.possibleEqual_sound
#print axioms Minion.CountProof.quickCount_sound
#print axioms Minion.DomainProof.values_sound
#print axioms Minion.SetProof.cappedCard_eq
#print axioms Minion.NvalueProof.bounds_sound
#print axioms Minion.PowerProof.boundedEq_eq
#print axioms Minion.TableProjection.filterTables_eval
#print axioms Minion.SearchProof.check_correct
#print axioms Minion.SearchProof.check_nodup
