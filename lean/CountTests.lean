import Minion.LeafProof

open Minion Minion.DomainProof Minion.LeafProof

private def d : Domains := ofLists [[0, 1, 2], [1], [-1, 0, 1, 2, 3]]
private def c : Constraint := .occurrence [.var 0, .var 1] (.constant 1) (.var 2) .eq

example : rule.prune d c 2 .lowerBound 1 = true := by decide +kernel
example : rule.prune d c 2 .upperBound 2 = true := by decide +kernel
example : rule.prune d c 2 .remove 1 = false := by decide +kernel
example : rule.prune d (.occurrence [.var 0] (.constant 1) (.constant 0) .eq)
    0 .remove 1 = true := by decide +kernel
example : checkConflict d (.hamming [.var 0] [.var 0] (.constant 1) .ge) = true := by decide +kernel
example : checkConflict d (.literalSum [.var 0] [] (.constant 0)) = true := by decide +kernel
example : CountProof.possibleValue d false (.literalSum [.var 0] [] (.constant 0)) = true := by decide +kernel
example : checkConflict d (.vectorExists [.var 0] [.var 0] .ne) = true := by decide +kernel
example : rule.prune d (.vectorExists [.var 0] [.constant 1] .lt)
    0 .upperBound 0 = true := by decide +kernel

#print axioms CountProof.count_bounds
#print axioms CountProof.possibleValue_sound
#print axioms checkConflict_sound
