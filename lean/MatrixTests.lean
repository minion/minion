import Minion.LeafProof

open Minion Minion.DomainProof Minion.LeafProof

private def d : Domains := ofLists (List.replicate 7 [0, 1])
private def c : Constraint := .allDifferentMatrix
  [.var 0, .var 1, .constant 0, .var 2, .var 3, .constant 0, .var 4, .var 5, .var 6] (.constant 1)

example : rule.prune d c 6 .assign 1 = true := by decide +kernel
example : rule.prune d c 4 .assign 0 = true := by decide +kernel
example : rule.prune d c 6 .remove 1 = false := by decide +kernel
example : checkConflict d (.allDifferentMatrix [.constant 1, .constant 1, .var 0, .var 1] (.constant 1)) = true := by decide +kernel
example : MatrixProof.possibleValue d false (.allDifferentMatrix [] (.constant 1)) = false := by decide +kernel

#print axioms MatrixProof.hallCheck_excludes
#print axioms MatrixProof.possibleValue_sound
