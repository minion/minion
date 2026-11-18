import Minion

open Minion

private def elementExample : Model := {
  variables := [⟨"index", [0, 1]⟩, ⟨"result", [1, 2, 3]⟩]
  constraints := [.element [.constant 1, .constant 3] (.var 0) (.var 1) 0 false]
}

-- A small concrete certificate checked by the kernel, without native_decide.
private theorem elementCheck : verifySolutions elementExample [[0, 1], [1, 3]] = true := by
  simp +decide [verifySolutions, sortAssignments, Model.solutions, assignments,
    elementExample, Constraint.eval, Term.eval]

example (a : Assignment) :
    a ∈ ([[0, 1], [1, 3]] : List Assignment) ↔ elementExample.IsSolution a :=
  verifySolutions_correct elementExample _ elementCheck a

example : verifySolutions ⟨[], []⟩ [[]] = true := by decide +kernel
example : verifySolutions ⟨[], [.truth false]⟩ [] = true := by decide +kernel

#print axioms verifySolutions_correct
#print axioms elementCheck
