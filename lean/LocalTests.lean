import Minion.SupportProof

open Minion Minion.DomainProof Minion.SupportProof

private def d : Domains := ofLists [[0, 1, 2, 3], [1, 3], [-2, -1, 0, 1, 2]]

example : rule.prune d (.compare .eq (.var 0) (.var 1)) 0 .remove 2 = true := by decide +kernel
example : rule.prune d (.compare .eq (.var 0) (.var 1)) 0 .remove 1 = false := by decide +kernel
example : rule.prune d (.compare .ne (.var 0) (.constant 1)) 0 .remove 1 = true := by decide +kernel
example : rule.prune d (.compare .lt (.var 0) (.constant 2)) 0 .upperBound 1 = true := by decide +kernel
example : rule.prune d (.inequality (.var 0) (.constant 0) (.constant 2))
    0 .upperBound 2 = true := by decide +kernel
example : rule.prune d (.absolute (.var 2) (.var 1)) 2 .lowerBound 0 = true := by decide +kernel
example : rule.prune d (.minusEq (.var 0) (.var 1)) 0 .upperBound 0 = true := by decide +kernel
example : checkConflict d (.truth false) = true := by decide +kernel
example : checkConflict d (.compare .ne (.var 0) (.var 0)) = true := by decide +kernel
example : checkConflict d (.compare .eq (.var 0) (.shift (.var 0) 1)) = true := by decide +kernel
example : rule.prune d (.inSet (.var 0) [.constant 0, .constant 2] true)
    0 .remove 1 = true := by decide +kernel
example : rule.prune d (.inIntervals (.var 0) [.constant 1, .constant 2] false)
    0 .remove 1 = true := by decide +kernel
example : rule.prune d (.divide (.var 0) (.constant 2) (.constant 1) false)
    0 .lowerBound 2 = true := by decide +kernel
example : rule.prune d (.divide (.var 0) (.constant 2) (.constant 1) false)
    0 .lowerBound 3 = false := by decide +kernel
example : rule.prune d (.product (.var 0) (.constant (-2)) (.constant (-4)))
    0 .assign 2 = true := by decide +kernel

#print axioms LocalProof.Rule.prune_preservesSolutions
#print axioms LocalProof.Rule.conflict_excludesSolutions
#print axioms checkConflict_sound
