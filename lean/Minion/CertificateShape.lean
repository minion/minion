import Minion.BlockSearchProof

/-! Diagnostic sizes distinguish sequence length from actual search/probe depth. -/
namespace Minion.CertificateShape

/-- Counts recursive proof nodes, not raw Lean expression depth. List spines
    are measured separately by propagationSteps and maxBlockLength. -/
structure Shape where
  nodes : Nat := 1
  height : Nat := 1
  propagationSteps : Nat := 0
  maxBlockLength : Nat := 0
  splitDepth : Nat := 0
  probeDepth : Nat := 0
  deriving Lean.ToJson

private def fork (left right : Shape) (split probe : Bool) : Shape :=
  ⟨1 + left.nodes + right.nodes, 1 + max left.height right.height,
    left.propagationSteps + right.propagationSteps,
    max left.maxBlockLength right.maxBlockLength,
    (if split then 1 else 0) + max left.splitDepth right.splitDepth,
    max (left.probeDepth + if probe then 1 else 0) right.probeDepth⟩

def tree : SearchProof.Tree → Shape
  | .empty | .conflict _ | .explainedConflict .. | .solution _ => {}
  | .prune _ _ _ _ next | .explainedPrune _ _ _ _ _ next
  | .tableReduce _ _ next | .filterTables _ next =>
    let s := tree next
    { s with
      nodes := s.nodes + 1
      height := s.height + 1
      propagationSteps := s.propagationSteps + 1 }
  | .split _ _ left right => fork (tree left) (tree right) true false
  | .refute _ _ _ proof next => fork (tree proof) (tree next) false true

def program : BlockSearchProof.Program → Shape
  | .empty | .conflict _ | .explainedConflict .. | .solution _ => {}
  | .block steps next =>
    let s := program next
    { s with
      nodes := s.nodes + 1
      height := s.height + 1
      propagationSteps := s.propagationSteps + steps.length
      maxBlockLength := max s.maxBlockLength steps.length }
  | .split _ _ left right => fork (program left) (program right) true false
  | .refute _ _ _ proof next => fork (program proof) (program next) false true

/-- The size of the equivalent legacy tree without allocating its unary nodes. -/
def expandedProgram : BlockSearchProof.Program → Shape
  | .empty | .conflict _ | .explainedConflict .. | .solution _ => {}
  | .block steps next =>
    let s := expandedProgram next
    { s with
      nodes := s.nodes + steps.length
      height := s.height + steps.length
      propagationSteps := s.propagationSteps + steps.length }
  | .split _ _ left right => fork (expandedProgram left) (expandedProgram right) true false
  | .refute _ _ _ proof next => fork (expandedProgram proof) (expandedProgram next) false true

end Minion.CertificateShape
