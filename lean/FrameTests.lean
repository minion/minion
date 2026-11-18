import Minion.LeafProof

open Minion Minion.DomainProof Minion.LeafProof

private def d : Domains := ofLists [[0, 1, 2], [0, 1, 2], [0, 1, 2]]
private def c : Constraint := .frameUpdate [.constant 1, .var 0] [.var 1, .var 2] [] [] (.constant 1)

example : rule.prune d c 1 .assign 1 = true := by decide +kernel
example : rule.prune d c 1 .remove 1 = false := by decide +kernel
example : FrameProof.possibleValue d false (.frameUpdate [.var 0] [.constant 1] [] [] (.constant 1)) = true := by decide +kernel
example : checkConflict d (.frameUpdate [.var 0] [.var 1] [.constant 0] [] (.constant 1)) = true := by decide +kernel
example : checkConflict d (.frameUpdate [.var 0] [.var 1] [.constant 1, .constant 1] [] (.constant 1)) = true := by decide +kernel
example : FrameProof.possibleValue d true (.frameUpdate [.constant 0] [.constant 1] [.constant 1] [] (.constant 1)) = true := by decide +kernel

#print axioms FrameProof.rowsPossible_sound
#print axioms FrameProof.possibleValue_sound
