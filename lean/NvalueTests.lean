import Minion.LeafProof

open Minion Minion.DomainProof Minion.LeafProof

private def d : Domains := ofLists [[0, 1, 2, 3], [2], [0, 1, 2, 3]]

example : rule.prune d (.nvalue [.var 0, .var 1] (.constant 1) .le)
    0 .lowerBound 2 = true := by decide +kernel
example : rule.prune d (.nvalue [.var 0, .var 1] (.constant 1) .le)
    0 .upperBound 2 = true := by decide +kernel
example : rule.prune d (.nvalue [.var 0, .var 1] (.constant 1) .le)
    0 .upperBound 1 = false := by decide +kernel
example : rule.prune d (.nvalue [.var 1, .var 1, .var 0] (.var 2) .ge)
    2 .upperBound 2 = true := by decide +kernel
example : checkConflict d (.nvalue [] (.constant 1) .ge) = true := by decide +kernel

#print axioms SetProof.eraseDups_nodup
#print axioms SetProof.card_mono
#print axioms NvalueProof.bounds_sound
#print axioms NvalueProof.possibleValue_sound
