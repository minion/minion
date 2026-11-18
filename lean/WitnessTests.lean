import Minion.SearchProof

open Minion Minion.DomainProof Minion.SearchProof

private def d : Domains := ofLists [[0, 1], [0, 1], [0, 1, 2]]
private def c : Constraint := .allDifferent [.var 0, .var 1, .var 2]

example : (InferenceWitness.rule (.hall [0, 1]) (some 2)).prune d c 2 .remove 0 = true := by decide +kernel
example : (InferenceWitness.rule (.hall [0]) (some 2)).prune d c 2 .remove 0 = false := by decide +kernel
example : (InferenceWitness.rule (.hall [0, 0]) (some 2)).prune d c 2 .remove 0 = false := by decide +kernel
example : InferenceWitness.check d (.truth true) (.hall [0, 1]) = false := by decide +kernel

private def model : Model := {
  variables := [{ name := "x", domain := [0, 1] }, { name := "y", domain := [0, 1] }]
  constraints := [.table [.var 0, .var 1] [[0, 0], [1, 1]] true] }

private def tree : Tree := .split 0 0
  (.tableReduce 0 [0] (.prune (some 0) 1 .assign 0 (.solution [0, 0])))
  (.tableReduce 0 [1] (.prune (some 0) 1 .assign 1 (.solution [1, 1])))

example : check model (model.variables.map Variable.domain) tree = true := by decide +kernel
example : solutions tree = [[0, 0], [1, 1]] := by decide +kernel
example : check model (model.variables.map Variable.domain)
    (.tableReduce 0 [] .empty) = false := by decide +kernel
example : check model (model.variables.map Variable.domain)
    (.split 0 0
      (.tableReduce 0 [0] (.prune (some 0) 1 .assign 0 (.solution [0, 0])))
      (.tableReduce 0 [0] (.solution [1, 1]))) = false := by decide +kernel
example : TableProjection.checkConstraint (ofLists [[0], [1]])
    (.table [.var 0, .var 1] [[0, 0], [1, 1]] false) [] = true := by decide +kernel
example : TableProjection.selectRows [[0], [1], [2]] [1, 1, 99] = [[1]] := by decide +kernel

#print axioms InferenceWitness.check_excludes
#print axioms TableProjection.solution_iff
#print axioms SearchProof.verify_correct
