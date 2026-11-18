import Minion.LeafProof

open Minion Minion.DomainProof

private def d : Domains := ofLists [[0, 2, 4], [1, 3]]

example : bindTerm d (.var 0) 2 0 = [2] := by decide +kernel
example : bindTerm d (.notBool 0) (-1) 0 = [2] := by decide +kernel
example : bindTerm d (.neg (.var 0)) (-2) 0 = [2] := by decide +kernel
example : bindTerm d (.shift (.var 0) 3) 5 0 = [2] := by decide +kernel
example : bindTerm d (.scale (.var 0) (-3)) (-6) 0 = [2] := by decide +kernel
example : bindTerm d (.scale (.var 0) 2) 3 0 = [] := by decide +kernel
example : bindTerm d (.scale (.var 0) 0) 0 0 = [0, 2, 4] := by decide +kernel
example : bindTerm d (.shift (.scale (.var 0) 0) 5) 5 0 = [0, 2, 4] := by decide +kernel
example : bindTerm d (.var 0) 2 1 = [1, 3] := by decide +kernel
example : LeafProof.checkConflict d (.compare .ne (.var 0) (.var 0)) = true := by decide +kernel
example : LeafProof.checkConflict d (.compare .eq (.scale (.var 0) 0) (.constant 1)) = true := by decide +kernel

#print axioms Term.preimageValue_sound
#print axioms compatible_bindTerm
