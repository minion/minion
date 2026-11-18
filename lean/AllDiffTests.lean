import Minion.LeafProof

open Minion Minion.DomainProof Minion.LeafProof

private def d : Domains := ofLists [[0, 1], [0, 1], [0, 1, 2]]
private def c : Constraint := .allDifferent [.var 0, .var 1, .var 2]

example : rule.prune d c 2 .assign 2 = true := by decide +kernel
example : rule.prune d c 2 .remove 2 = false := by decide +kernel
example : AllDiffProof.hallCheck d [.var 0, .var 1, .var 2] [0, 1, 2] = false := by decide +kernel
example : AllDiffProof.hallCheck d [.var 0, .var 1] [0, 0, 1, 1, 999] = false := by decide +kernel
example : AllDiffProof.hallCheck d [.var 0, .var 1, .var 0] [0, 1, 2] = true := by decide +kernel
example : checkConflict d (.allDifferent [.var 0, .var 0]) = true := by decide +kernel
example : AllDiffProof.possibleValue d false (.allDifferent [.var 0, .var 0]) = true := by decide +kernel

#print axioms AllDiffProof.hallCheck_excludes
#print axioms AllDiffProof.possibleValue_sound
#print axioms checkConflict_sound
