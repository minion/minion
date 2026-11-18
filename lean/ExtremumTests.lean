import Minion.LeafProof

open Minion Minion.DomainProof Minion.LeafProof

private def d : Domains := ofLists [[-2, -1, 0, 1, 2], [1, 2, 3], [-3, -2, -1, 0, 1, 2, 3, 4]]

example : rule.prune d (.extremum [.var 0, .var 1] (.var 2) false)
    2 .lowerBound (-2) = true := by decide +kernel
example : rule.prune d (.extremum [.var 0, .var 1] (.var 2) false)
    2 .upperBound 2 = true := by decide +kernel
example : rule.prune d (.extremum [.var 0, .var 1] (.var 2) true)
    2 .lowerBound 1 = true := by decide +kernel
example : rule.prune d (.extremum [.var 0, .var 1] (.var 2) true)
    2 .lowerBound 2 = false := by decide +kernel
example : checkConflict d (.extremum [] (.constant 0) false) = true := by decide +kernel
example : ExtremumProof.possibleValue d false (.extremum [] (.constant 0) true) = true := by decide +kernel

#print axioms Constraint.normalizeExtremum_eval
#print axioms ExtremumProof.possibleValue_sound
