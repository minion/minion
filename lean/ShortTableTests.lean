import Minion.LeafProof

open Minion Minion.DomainProof Minion.LeafProof

private def d : Domains := ofLists [[0, 1, 2], [0, 1, 2]]
private def rows : List (List (Nat × Int)) := [[(0, 0), (0, 2)], [(1, 1)]]
example : checkConflict d (.shortTable [.var 0] [[(0, 0), (0, 2)]] false) = true := by decide +kernel
example : rule.prune d (.shortTable [.var 0] [[(0, 0), (0, 2)]] true) 0 .remove 1 = true := by decide +kernel
example : ShortTableProof.possibleValue d false (.shortTable [.var 0, .var 1] rows true) = true := by decide +kernel
example : ShortTableProof.possibleValue d false (.shortTable [.var 0] [[(0, 0)], [(0, 1)], [(0, 2)]] false) = false := by decide +kernel
example : ShortTableProof.possibleValue d false (.shortTable [] [[]] true) = false := by decide +kernel
example : ShortTableProof.possibleValue d true (.shortTable [] [] true) = false := by decide +kernel

#print axioms BoundedProof.possible_sound
#print axioms ShortTableProof.possibleValue_sound
