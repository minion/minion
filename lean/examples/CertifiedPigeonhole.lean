import Minion.SearchProof

set_option maxRecDepth 100000
set_option maxHeartbeats 0

namespace MinionCertificate

def model : Minion.Model :=
{ variables := [{ name := "x_0__", domain := [0, 1] },
                { name := "x_1__", domain := [0, 1] },
                { name := "x_2__", domain := [0, 1] }],
  constraints := [Minion.Constraint.allDifferent [Minion.Term.var 0, Minion.Term.var 1, Minion.Term.var 2]] }

def tree : Minion.SearchProof.Tree :=
Minion.SearchProof.Tree.conflict 0

def reported : List Minion.Assignment :=
[]

set_option maxRecDepth 100000 in
set_option maxHeartbeats 0 in
theorem checked : Minion.SearchProof.check model (model.variables.map Minion.Variable.domain) tree = true := by decide +kernel

theorem reported_eq : reported = Minion.SearchProof.solutions tree := by decide +kernel

theorem solutions_correct :
    ∀ a, a ∈ reported ↔ model.IsSolution a := by
  rw [reported_eq]
  exact Minion.SearchProof.check_complete model tree checked

theorem unsat : ∀ a, ¬model.IsSolution a :=
  fun a hs => List.not_mem_nil ((solutions_correct a).mpr hs)

#print axioms solutions_correct
#print axioms unsat

end MinionCertificate
