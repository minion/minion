import Minion.FastSearchProof

set_option maxRecDepth 100000
set_option maxHeartbeats 0

namespace MinionZeroScaleCertificate

def model : Minion.Model :=
{ variables := [{ name := "x0", domain := [0, 1] }, { name := "x1", domain := [0, 1] }, { name := "x2", domain := [0, 1] }, { name := "x3", domain := [0, 1] }, { name := "x4", domain := [0, 1] }, { name := "x5", domain := [0, 1] }, { name := "x6", domain := [0, 1] }, { name := "x7", domain := [0, 1] }, { name := "x8", domain := [0, 1] }, { name := "x9", domain := [0, 1] }, { name := "x10", domain := [0, 1] }, { name := "x11", domain := [0, 1] }, { name := "x12", domain := [0, 1] }, { name := "x13", domain := [0, 1] }, { name := "x14", domain := [0, 1] }, { name := "x15", domain := [0, 1] }, { name := "x16", domain := [0, 1] }, { name := "x17", domain := [0, 1] }, { name := "x18", domain := [0, 1] }, { name := "x19", domain := [0, 1] }, { name := "x20", domain := [0, 1] }, { name := "x21", domain := [0, 1] }, { name := "x22", domain := [0, 1] }, { name := "x23", domain := [0, 1] }], constraints := [Minion.Constraint.table [Minion.Term.scale (Minion.Term.var 0) 0, Minion.Term.scale (Minion.Term.var 1) 0, Minion.Term.scale (Minion.Term.var 2) 0, Minion.Term.scale (Minion.Term.var 3) 0, Minion.Term.scale (Minion.Term.var 4) 0, Minion.Term.scale (Minion.Term.var 5) 0, Minion.Term.scale (Minion.Term.var 6) 0, Minion.Term.scale (Minion.Term.var 7) 0, Minion.Term.scale (Minion.Term.var 8) 0, Minion.Term.scale (Minion.Term.var 9) 0, Minion.Term.scale (Minion.Term.var 10) 0, Minion.Term.scale (Minion.Term.var 11) 0, Minion.Term.scale (Minion.Term.var 12) 0, Minion.Term.scale (Minion.Term.var 13) 0, Minion.Term.scale (Minion.Term.var 14) 0, Minion.Term.scale (Minion.Term.var 15) 0, Minion.Term.scale (Minion.Term.var 16) 0, Minion.Term.scale (Minion.Term.var 17) 0, Minion.Term.scale (Minion.Term.var 18) 0, Minion.Term.scale (Minion.Term.var 19) 0, Minion.Term.scale (Minion.Term.var 20) 0, Minion.Term.scale (Minion.Term.var 21) 0, Minion.Term.scale (Minion.Term.var 22) 0, Minion.Term.scale (Minion.Term.var 23) 0] [[0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]] false] }

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

end MinionZeroScaleCertificate
