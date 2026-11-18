import Minion.LeafProof

open Minion Minion.DomainProof Minion.LeafProof

private def d : Domains := ofLists [[0, 1, 2], [0, 1, 2]]
private def c : Constraint := .table [.var 0, .var 1] [[0, 2], [1, 1]] true

example : rule.prune d c 0 .upperBound 1 = true := by decide +kernel
example : rule.prune d c 0 .remove 1 = false := by decide +kernel
example : TableProof.possibleValue d false c = true := by decide +kernel
example : checkConflict d (.table [.var 0, .var 0] [[0, 1], [1, 0]] true) = true := by decide +kernel
example : TableProof.possibleValue d false (.table [] [[]] true) = false := by decide +kernel
example : TableProof.possibleValue d false (.table [] [] true) = true := by decide +kernel
example : checkConflict d (.table [.var 0] [[0], [1], [2]] false) = true := by decide +kernel
example : TableProof.possibleValue d true (.table [.var 0, .notBool 0] [[0, 1], [1, 0]] false) = true := by decide +kernel

#print axioms TableProof.possibleValue_sound
#print axioms checkConflict_sound
