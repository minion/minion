import Minion.SumProof

open Minion Minion.SumProof

private def d : Domains := ofLists [[0, 1, 2, 3], [2, 3, 4], [0, 1, 2, 3, 4]]
private def leSum : Constraint := .sum [.var 0, .var 1] (.constant 3) .le
private def geSum : Constraint := .sum [.var 0, .var 1] (.constant 6) .ge

-- Arithmetic views retain their underlying variable and compose with Boolean views.
example : (Term.neg (.var 0)).eval [-3] = 3 := by decide +kernel
example : (Term.neg (.notBool 0)).eval [0] = -1 := by decide +kernel
example : ((Term.neg (.var 0)).negate).eval [-3] = -3 := by decide +kernel
example : values d (.neg (.var 0)) = [0, -1, -2, -3] := by decide +kernel

-- sumgeq conflicts exactly when the largest total falls below the right-hand side.
example : checkConflict d geSum = false := by decide +kernel
example : checkConflict d (.sum [.var 0, .var 1] (.constant 7) .ge) = false := by decide +kernel
example : checkConflict d (.sum [.var 0, .var 1] (.constant 8) .ge) = true := by decide +kernel
example : checkConflict d (.sum [.var 0, .neg (.var 1)] (.constant 2) .ge) = true := by decide +kernel
example : checkPrune d (.sum [.neg (.var 0), .neg (.var 1)] (.constant (-6)) .le)
    0 .lowerBound 2 = true := by decide +kernel

-- Pruning x > 1 is justified; pruning x > 0 would delete the solution x=1,y=2.
example : checkPrune d leSum 0 .upperBound 1 = true := by decide +kernel
example : checkPrune d leSum 0 .upperBound 0 = false := by decide +kernel
example : checkPrune d geSum 0 .lowerBound 2 = true := by decide +kernel
example : checkPrune d geSum 0 .lowerBound 3 = false := by decide +kernel

-- Result bounds and failure, with no assignment enumeration in the rule checker.
example : checkPrune d (.sum [.var 0, .var 1] (.var 2) .le) 2 .lowerBound 2 = true := by decide +kernel
example : checkConflict d leSum = false := by decide +kernel
example : checkConflict d (.sum [.var 0, .var 1] (.constant 1) .le) = true := by decide +kernel
example : checkConflict d (.sum [] (.constant 1) .ge) = true := by decide +kernel

-- Repeated references do not require distinct-variable assumptions.
example : checkPrune d (.sum [.var 0, .var 0] (.constant 3) .le) 0 .upperBound 1 = true := by decide +kernel

#print axioms checkPrune_sound_finite
#print axioms checkModelPrune_sound
#print axioms checkModelConflict_sound
#print axioms checkNoop_sound
#print axioms checkPrune_preservesSolutions
#print axioms Constraint.normalizeSum_eval
