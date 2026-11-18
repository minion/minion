import Minion.LeafProof

open Minion Minion.DomainProof Minion.BoundsProof

private def d : Domains := ofLists [[-3, -2, -1], [2, 3, 4], [0, 1]]

example : quickPossible d true (.compare .eq (.var 0) (.var 1)) = false := by decide +kernel
example : quickPossible d false (.compare .ne (.var 0) (.var 1)) = false := by decide +kernel
example : quickPossible d true (.minusEq (.var 0) (.var 1)) = true := by decide +kernel
example : quickPossible d true (.absolute (.var 0) (.var 1)) = false := by decide +kernel
example : quickPossible d true (.difference (.var 0) (.var 1) (.var 2)) = false := by decide +kernel
example : quickPossible d true (.product (.var 0) (.var 1) (.var 2)) = false := by decide +kernel
example : quickPossible d true (.inequality (.var 1) (.var 0) (.constant 2)) = false := by decide +kernel
example : absBounds (-5) 2 = (0, 5) := by decide +kernel
example : absBounds (-5) (-2) = (2, 5) := by decide +kernel
example : productBounds (-3) 1 2 4 = (-12, 4) := by decide +kernel
example : LeafProof.checkConflict d (.compare .ne (.var 0) (.var 0)) = true := by decide +kernel

#print axioms intervalPossible_sound
#print axioms absBounds_sound
#print axioms productBounds_sound
#print axioms possibleValue_sound
