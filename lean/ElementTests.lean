import Minion.ElementProof

open Minion Minion.DomainProof Minion.ElementProof

private def d : Domains := ofLists [[1, 2], [3], [0, 1], [1, 2, 3, 4]]
private def e : Constraint := .element [.var 0, .var 1] (.var 2) (.var 3) 0 false

example : rule.prune d e 3 .remove 4 = true := by decide +kernel
example : rule.prune d e 3 .remove 2 = false := by decide +kernel
-- Unselected array entries need not equal the result.
example : rule.prune d e 0 .remove 1 = false := by decide +kernel
example : rule.prune d (.element [.var 0, .var 1] (.constant 0) (.constant 2) 0 false)
    0 .remove 1 = true := by decide +kernel
example : rule.prune d (.element [.var 0, .var 1] (.var 2) (.constant 2) 0 false)
    2 .remove 1 = true := by decide +kernel
example : possibleValue d true (.element [] (.constant (-1)) (.constant 0) 0 true) = true := by decide +kernel
example : possibleValue d false (.element [] (.constant (-1)) (.constant 0) 0 true) = false := by decide +kernel
example : possibleValue d true (.element [] (.constant (-1)) (.constant 0) 0 false) = false := by decide +kernel
example : possibleValue d true (.element [.constant 1] (.constant 1) (.constant 1) 1 false) = true := by decide +kernel

#print axioms possibleValue_sound
#print axioms checkConflict_sound
