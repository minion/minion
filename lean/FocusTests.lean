import Minion.SearchProof

open Minion Minion.DomainProof

private def d : Domains := ofLists [[0, 1], [0, 1]]
private def andConstraint : Constraint := .conjunction [
  .truth true, .compare .eq (.var 0) (.constant 0)]

example : (InferenceWitness.rule (.focus [1])).prune d andConstraint 0 .assign 0 = true := by decide +kernel
example : (InferenceWitness.rule (.focus [0])).prune d andConstraint 0 .assign 0 = false := by decide +kernel
example : InferenceWitness.check d andConstraint (.focus [99]) = false := by decide +kernel

-- A false child cannot refute a true disjunction.
example : InferenceWitness.check d (.disjunction [.truth false, .truth true])
    (.focus [0]) = false := by decide +kernel

-- A source path never activates a reified child without checking its guard.
example : (InferenceWitness.rule (.focus [0, 1])).prune d
    (.reify andConstraint (.var 1)) 0 .assign 0 = false := by decide +kernel
example : (InferenceWitness.rule (.focus [0, 1])).prune (ofLists [[0, 1], [1]])
    (.reify andConstraint (.var 1)) 0 .assign 0 = true := by decide +kernel
example : (InferenceWitness.rule (.focus [0, 1])).prune (ofLists [[0, 1], [0]])
    (.reify andConstraint (.var 1)) 0 .assign 0 = false := by decide +kernel

-- Falsity of a disjunction does imply falsity of its selected child.
example : (InferenceWitness.rule (.focus [0, 0])).prune d
    (.reify (.disjunction [.compare .eq (.var 0) (.constant 0), .truth false]) (.constant 0))
    0 .remove 0 = true := by decide +kernel

#print axioms Minion.LogicProof.possibleAt_sound
#print axioms Minion.InferenceWitness.check_excludes
