import Minion.FastSearchProof

set_option maxRecDepth 100000
set_option maxHeartbeats 0

namespace MinionPowerCertificate

def model : Minion.Model :=
{ variables := [], constraints := [Minion.Constraint.power (Minion.Term.constant 3) (Minion.Term.constant 16777216) (Minion.Term.constant (-1))] }

def tree : Minion.SearchProof.Tree :=
Minion.SearchProof.Tree.conflict 0

def reported : List Minion.Assignment :=
[]

set_option maxRecDepth 100000 in
set_option maxHeartbeats 0 in
theorem fast_checked : Minion.FastSearchProof.check model (model.variables.map Minion.Variable.domain) tree = true := by decide +kernel

theorem checked : Minion.SearchProof.check model (model.variables.map Minion.Variable.domain) tree = true := by
  simpa only [Minion.FastSearchProof.check_eq] using fast_checked

theorem reported_eq : reported = Minion.SearchProof.solutions tree := by decide +kernel

theorem solutions_correct :
    ∀ a, a ∈ reported ↔ model.IsSolution a := by
  rw [reported_eq]
  exact Minion.SearchProof.check_complete model tree checked

theorem solutions_nodup : reported.Nodup := by
  rw [reported_eq]
  exact Minion.SearchProof.check_nodup model (model.variables.map Minion.Variable.domain) tree checked

theorem unsat : ∀ a, ¬model.IsSolution a :=
  fun a hs => List.not_mem_nil ((solutions_correct a).mpr hs)

#print axioms solutions_correct
#print axioms solutions_nodup
#print axioms unsat

end MinionPowerCertificate
