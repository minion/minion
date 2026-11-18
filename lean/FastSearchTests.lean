import Minion.FastSearchProof

open Minion Minion.DomainProof Minion.SearchProof

private def m (domain : List Int) : Model := ⟨[⟨"x", domain⟩], [
  .compare .ne (.var 0) (.constant 0),
  .compare .ne (.var 0) (.constant 1),
  .compare .ne (.var 0) (.constant 2)]⟩
private def tree : Tree := .prune (some 0) 0 .remove 0
  (.prune (some 1) 0 .remove 1 (.prune (some 2) 0 .remove 2 (.solution [3])))

example : FastSearchProof.check (m [0,1,2,3]) [[0,1,2,3]] tree = true := by decide +kernel
example : FastSearchProof.check (m [2,0,3,1]) [[2,0,3,1]] tree = true := by decide +kernel
example : FastSearchProof.check (m [0,0,1,2,3]) [[0,0,1,2,3]] tree = true := by decide +kernel
example : FastSearchProof.check (m [0,1,2,3]) [[0,1,2,3]] (.solution [3]) = false := by decide +kernel
example : FastSearchProof.check (m [0,1,2,3]) [[0,1,2,3]]
    (.split 99 0 (.solution [3]) (.solution [3])) = false := by decide +kernel

private def many : Model := ⟨[⟨"x", [0,1]⟩],
  List.replicate 512 (.truth true) ++ [.compare .eq (.var 0) (.constant 0)]⟩
example : FastSearchProof.check many [[0,1]]
    (.prune (some 512) 0 .assign 0 (.solution [0])) = true := by decide +kernel

-- A cached check outside the model domains still performs the original
-- membership check; the validated root-domain shortcut cannot be forged.
private def outside : Model := ⟨[⟨"x", [0]⟩], List.replicate 512 (.truth true)⟩
example : FastSearchProof.check outside [[1]] (.solution [1]) = false := by decide +kernel
private def wide : Model := ⟨[⟨"x", (List.range 512).map Int.ofNat⟩],
  [.compare .eq (.var 0) (.constant 1)]⟩
example : FastSearchProof.check wide (wide.variables.map Variable.domain)
    (.prune (some 0) 0 .remove 0 (.prune (some 0) 0 .assign 1 (.solution [1]))) = true := by decide +kernel

example (model : Model) (d : List (List Int)) (proof : Tree) :
    FastSearchProof.check model d proof = SearchProof.check model d proof :=
  FastSearchProof.check_eq model d proof

example : FastSearchProof.solutionsReverse (.split 0 0 (.solution [0]) (.solution [1])) = [[1], [0]] := by decide +kernel

#print axioms Minion.FastSearchProof.solutionsReverse_eq
#print axioms Minion.FastSearchProof.check_eq
#print axioms Minion.FastSearchProof.verify_eq
