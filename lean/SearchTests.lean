import Minion.SearchProof

open Minion Minion.DomainProof Minion.SearchProof

private def m : Model := ⟨[⟨"x", [0, 1]⟩, ⟨"y", [0, 1]⟩], [.compare .eq (.var 0) (.var 1)]⟩
private def tree : Tree := .split 0 0
  (.prune (some 0) 1 .assign 0 (.solution [0, 0]))
  (.prune (some 0) 1 .assign 1 (.solution [1, 1]))

example : check m (m.variables.map Variable.domain) tree = true := by decide +kernel
example : solutions tree = [[0, 0], [1, 1]] := by decide +kernel
example : ∀ a, a ∈ solutions tree ↔ m.IsSolution a :=
  check_complete m tree (by decide +kernel)
example : (solutions tree).Nodup :=
  check_nodup m (m.variables.map Variable.domain) tree (by decide +kernel)
example : check m [[0, 1], [0, 1]] (.prune (some 0) 0 .assign 0 (.solution [0, 0])) = false := by decide +kernel
example : check m [[0, 1], [0, 1]] (.conflict 0) = false := by decide +kernel
example : check m [[0, 1], [0, 1]] (.solution [0, 0]) = false := by decide +kernel
example : check m [[0, 1], [0, 1]] (.empty) = false := by decide +kernel
example : check m [[0], [1]] (.conflict 0) = true := by decide +kernel
example : check m [[], [0, 1]] (.empty) = true := by decide +kernel

-- An out-of-range split would leave both domain lists unchanged and could
-- otherwise duplicate the same solution into both children, even at width zero.
example : check ⟨[], []⟩ [] (.solution []) = true := by decide +kernel
example : check ⟨[], []⟩ [] (.split 0 0 (.solution []) (.solution [])) = false := by decide +kernel
example : check m [[0], [0]]
    (.split 2 0 (.solution [0, 0]) (.solution [0, 0])) = false := by decide +kernel

-- Exact reverse discovery order takes the linear path; arbitrary permutations
-- still use the multiplicity-preserving fallback.
example : sameAssignments [[0], [1], [2]] [[2], [1], [0]] = true := by decide +kernel
#eval show IO Unit from do
  unless sameAssignments [[0], [1], [2]] [[1], [0], [2]] do
    throw (IO.userError "permutation fallback rejected")
#eval show IO Unit from do
  if sameAssignments [[0], [1], [2]] [[1], [1], [2]] then
    throw (IO.userError "permutation fallback accepted duplicates")
example : verify m tree [[1, 1], [0, 0]] = true := by decide +kernel
#eval show IO Unit from do
  if verify m tree [[0, 0], [0, 0]] then
    throw (IO.userError "verify accepted duplicates")

#print axioms check_correct
#print axioms check_nodup
#print axioms check_complete
#print axioms verify_correct
#print axioms verify_nodup
#print axioms verify_unsat
