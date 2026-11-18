import Minion.LeafProof

open Minion Minion.DomainProof

private def d : Domains := ofLists [[-2, -1, 0, 1, 2], [-1, 0, 1, 2], [-2, 0, 2, 4]]

example : FunctionalProof.possibleValue d true (.product (.var 0) (.var 1) (.var 2)) = true := by decide +kernel
example : FunctionalProof.possibleValue d true (.product (.var 0) (.constant 0) (.constant 1)) = false := by decide +kernel
example : FunctionalProof.possibleValue d true (.divide (.var 0) (.constant 0) (.constant 0) true) = true := by decide +kernel
example : FunctionalProof.possibleValue d true (.divide (.var 0) (.constant 0) (.constant 0) false) = false := by decide +kernel
example : FunctionalProof.possibleValue d true (.modulo (.constant (-3)) (.constant 2) (.constant 1) false) = true := by decide +kernel
example : FunctionalProof.possibleValue d true (.power (.constant (-1)) (.constant (-3)) (.constant (-1))) = true := by decide +kernel
example : FunctionalProof.possibleValue d true (.power (.constant 2) (.constant (-1)) (.constant 0)) = false := by decide +kernel
example : FunctionalProof.possibleValue (ofLists [[0, 1]]) true
    (.product (.var 0) (.constant 0) (.var 0)) = true := by decide +kernel

-- This used to allocate/enumerate a trillion-element range.
example : CountProof.possibleRange d true (fun _ => (1, 1000000000000))
    .lt (.constant 2) = true := by decide +kernel
example : LexProof.quickLex d true [.constant 2, .var 0] [.constant 2, .constant (-3)] false = false := by decide +kernel
example : LexProof.quickLex d false [.constant 2] [.constant 2] false = false := by decide +kernel

#print axioms FunctionalProof.possibleValue_sound
#print axioms CountProof.possibleRange_sound
#print axioms LexProof.quickLex_sound
