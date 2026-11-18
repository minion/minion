import Minion.LeafProof

open Minion Minion.DomainProof Minion.LeafProof

private def d : Domains := ofLists [[0, 1], [0, 1], [0, 1, 2], [0, 1, 2, 3]]
private def c : Constraint := .gcc [.var 0, .var 1, .var 2]
  [.constant 0, .constant 1, .constant 2] [.constant 1, .constant 1, .var 3]

example : rule.prune d c 3 .assign 1 = true := by decide +kernel
example : rule.prune d c 3 .remove 1 = false := by decide +kernel
example : GccCutProof.upperCut d [.var 0, .var 1, .var 2]
    [(0, .constant 1), (1, .constant 1), (2, .constant 0)] [0, 1, 2] = true := by decide +kernel
example : GccCutProof.lowerCut d [.var 0, .var 1, .var 2]
    [(2, .constant 2)] [2, 2, 2] = true := by decide +kernel

private def lowerDomains : Domains := ofLists [[0, 1], [0, 1], [2, 3], [2, 3]]
private def upperDomains : Domains := ofLists [[0, 1], [0, 1], [0, 1], [2, 3]]
private def four : List Term := [.var 0, .var 1, .var 2, .var 3]
private def lowerEntries : GccCutProof.Entries :=
  [(0, .constant 2), (1, .constant 1), (2, .constant 1)]
private def upperEntries : GccCutProof.Entries :=
  [(0, .constant 1), (1, .constant 1), (2, .constant 0)]

-- Each flow-discovered witness is rejected by the independent numeric checker.
example : GccCutProof.discoveredCutPossible lowerDomains four lowerEntries
    (FlowWitness.discover (four.map (values lowerDomains)) [0, 1, 2]
      (GccCutProof.lowerCap lowerDomains lowerEntries)
      (GccCutProof.upperCap lowerDomains four lowerEntries)) = false := by decide +kernel
example : GccCutProof.discoveredCutPossible upperDomains four upperEntries
    (FlowWitness.discover (four.map (values upperDomains)) [0, 1, 2]
      (GccCutProof.lowerCap upperDomains upperEntries)
      (GccCutProof.upperCap upperDomains four upperEntries)) = false := by decide +kernel

-- A lower-demand augmenting path has to move an earlier assignment.
example : (match FlowWitness.discover [[0, 1], [0]] [0, 1]
    (fun _ => 1) (fun _ => 1) with
    | .matched owners => owners.length == 2
    | _ => false) = true := by decide +kernel

#print axioms GccCutProof.upperCut_excludes
#print axioms GccCutProof.lowerCut_excludes
#print axioms GccCutProof.possibleValue_sound
