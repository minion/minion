import Minion.LogicProof

open Minion Minion.DomainProof Minion.LogicProof

private def d : Domains := ofLists [[0, 1, 2, 3], [0, 1], [1]]
private def sum : Constraint := .sum [.var 0] (.constant 1) .le

example : rule.prune d (.reify sum (.var 2)) 0 .upperBound 1 = true := by decide +kernel
example : rule.prune d (.reify sum (.var 2)) 0 .upperBound 0 = false := by decide +kernel
example : rule.prune d (.reify sum (.constant 0)) 0 .lowerBound 2 = true := by decide +kernel
example : rule.prune d (.imply sum (.constant 0)) 0 .upperBound 1 = false := by decide +kernel
example : rule.prune d (.imply sum (.constant 1)) 0 .upperBound 1 = true := by decide +kernel
example : checkConflict d (.conjunction [.truth true, .truth false]) = true := by decide +kernel
example : checkConflict d (.disjunction [.truth false, .truth false]) = true := by decide +kernel
example : possible d false (.reify sum (.constant 2)) = true := by decide +kernel
example : possible d true (.reify sum (.constant 2)) = false := by decide +kernel
example : rule.prune d (.reify (.sum [.var 0] (.constant 2) .ge) (.constant 0))
    0 .upperBound 1 = true := by decide +kernel

#print axioms possible_sound
#print axioms checkConflict_sound
#print axioms Constraint.negateSum_eval
