import Minion.LeafProof

open Minion Minion.DomainProof Minion.LeafProof

private def d : Domains := ofLists [[-3, -2, -1, 0, 1, 2, 3], [0, 1, 2], [-2, 0, 2]]
private def w : Constraint := .weightedSum [.constant 2, .constant (-3)] [.var 0, .var 1] (.constant 0) .le

example : rule.prune d w 0 .upperBound 3 = true := by decide +kernel
example : rule.prune d (.weightedSum [.constant 2] [.var 0] (.constant 3) .le)
    0 .upperBound 1 = true := by decide +kernel
example : rule.prune d (.weightedSum [.constant 2] [.var 0] (.constant 3) .le)
    0 .upperBound 0 = false := by decide +kernel
example : rule.prune d (.weightedSum [.constant (-3)] [.var 0] (.constant (-2)) .ge)
    0 .upperBound 0 = true := by decide +kernel
example : rule.prune d (.weightedSum [.constant 0] [.var 0] (.var 2) .le)
    2 .lowerBound 0 = true := by decide +kernel
example : checkConflict d (.weightedSum [.constant 1] [] (.constant 3) .le) = true := by decide +kernel

#print axioms Constraint.expandWeighted_eval
#print axioms WeightedProof.possibleValue_sound
#print axioms checkConflict_sound
