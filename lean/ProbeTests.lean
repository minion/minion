import Minion.SearchProof

open Minion Minion.DomainProof Minion.SearchProof

private def m : Model := ⟨[⟨"x", [0, 1]⟩, ⟨"y", [0, 1]⟩, ⟨"z", [0, 1]⟩],
  [.compare .ne (.var 0) (.var 1), .compare .ne (.var 1) (.var 2), .compare .ne (.var 2) (.var 0)]⟩
private def p0 : Tree := .prune (some 0) 1 .assign 1 (.prune (some 1) 2 .assign 0 (.conflict 2))
private def p1 : Tree := .prune (some 0) 1 .assign 0 (.prune (some 1) 2 .assign 1 (.conflict 2))
private def certificate : Tree := .refute 0 .lowerBound 1 p0 p1

example : refutationCheck m (m.variables.map Variable.domain) 0 .remove 0 p0 = true := by decide +kernel
example : refutationCheck m (m.variables.map Variable.domain) 0 .remove 1 p0 = false := by decide +kernel
example : check m (m.variables.map Variable.domain) certificate = true := by decide +kernel
example : verify m certificate [] = true := by decide +kernel
example : ∀ a, ¬m.IsSolution a := verify_unsat m certificate (by decide +kernel)

#print axioms refutation_preserves
#print axioms check_complete
