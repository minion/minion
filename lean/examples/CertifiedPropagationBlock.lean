import Minion.BlockSearchProof

set_option maxRecDepth 100000
set_option maxHeartbeats 0

namespace MinionBlockCertificate

def c0 : Minion.Constraint :=
Minion.Constraint.compare (Minion.Comparison.ne) (Minion.Term.var 0) (Minion.Term.constant 0)

def c1 : Minion.Constraint :=
Minion.Constraint.compare (Minion.Comparison.ne) (Minion.Term.var 0) (Minion.Term.constant 1)

def c2 : Minion.Constraint :=
Minion.Constraint.compare (Minion.Comparison.ne) (Minion.Term.var 0) (Minion.Term.constant 2)

def c3 : Minion.Constraint :=
Minion.Constraint.compare (Minion.Comparison.ne) (Minion.Term.var 0) (Minion.Term.constant 3)

def c4 : Minion.Constraint :=
Minion.Constraint.compare (Minion.Comparison.ne) (Minion.Term.var 0) (Minion.Term.constant 4)

def c5 : Minion.Constraint :=
Minion.Constraint.compare (Minion.Comparison.ne) (Minion.Term.var 0) (Minion.Term.constant 5)

def c6 : Minion.Constraint :=
Minion.Constraint.compare (Minion.Comparison.ne) (Minion.Term.var 0) (Minion.Term.constant 6)

def c7 : Minion.Constraint :=
Minion.Constraint.compare (Minion.Comparison.ne) (Minion.Term.var 0) (Minion.Term.constant 7)

def model : Minion.Model :=
{ variables := [{ name := "x", domain := [0, 1, 2, 3, 4, 5, 6, 7, 8] }], constraints := [c0, c1, c2, c3, c4, c5, c6, c7] }

def constraintIndex : Minion.ConstraintIndex.Index Minion.Constraint :=
(.node (some c0) (.node none (.node none .empty (.node (some c4) .empty .empty)) (.node (some c2) .empty (.node (some c6) .empty .empty))) (.node (some c1) (.node none .empty (.node (some c5) .empty .empty)) (.node (some c3) .empty (.node (some c7) .empty .empty))))

theorem index_eq : constraintIndex = Minion.ConstraintIndex.build model.constraints := by with_unfolding_all rfl

def program : Minion.BlockSearchProof.Program :=
Minion.BlockSearchProof.Program.block [Minion.BlockSearchProof.Step.prune (some 0) 0 (Minion.DomainProof.Operation.remove) 0, Minion.BlockSearchProof.Step.prune (some 1) 0 (Minion.DomainProof.Operation.remove) 1, Minion.BlockSearchProof.Step.prune (some 2) 0 (Minion.DomainProof.Operation.remove) 2, Minion.BlockSearchProof.Step.prune (some 3) 0 (Minion.DomainProof.Operation.remove) 3, Minion.BlockSearchProof.Step.prune (some 4) 0 (Minion.DomainProof.Operation.remove) 4, Minion.BlockSearchProof.Step.prune (some 5) 0 (Minion.DomainProof.Operation.remove) 5, Minion.BlockSearchProof.Step.prune (some 6) 0 (Minion.DomainProof.Operation.remove) 6, Minion.BlockSearchProof.Step.prune (some 7) 0 (Minion.DomainProof.Operation.remove) 7] (Minion.BlockSearchProof.Program.solution [8])

def reported : List Minion.Assignment :=
[[8]]

theorem checked : Minion.BlockSearchProof.checkWithIndex model (model.variables.map Minion.Variable.domain) program constraintIndex = true := by decide +kernel

theorem reported_eq : reported = Minion.BlockSearchProof.solutions program := by decide +kernel

theorem solutions_correct :
    ∀ a, a ∈ reported ↔ model.IsSolution a := by
  rw [reported_eq]
  exact Minion.BlockSearchProof.checkWithIndex_complete model program constraintIndex index_eq checked

theorem solutions_nodup : reported.Nodup := by
  rw [reported_eq]
  exact Minion.BlockSearchProof.checkWithIndex_nodup model (model.variables.map Minion.Variable.domain) program constraintIndex index_eq checked

#print axioms solutions_correct
#print axioms solutions_nodup

end MinionBlockCertificate
