import Minion.SearchProof

set_option maxRecDepth 100000
set_option maxHeartbeats 0

namespace MinionCertificate

def model : Minion.Model :=
{ variables := [{ name := "x", domain := [0, 1] },
                { name := "y", domain := [0, 1] },
                { name := "z", domain := [0, 1] }],
  constraints := [Minion.Constraint.compare (Minion.Comparison.ne) (Minion.Term.var 0) (Minion.Term.var 1),
                  Minion.Constraint.compare (Minion.Comparison.ne) (Minion.Term.var 1) (Minion.Term.var 2),
                  Minion.Constraint.compare (Minion.Comparison.ne) (Minion.Term.var 2) (Minion.Term.var 0)] }

def tree : Minion.SearchProof.Tree :=
Minion.SearchProof.Tree.refute
  0
  (Minion.DomainProof.Operation.lowerBound)
  1
  (Minion.SearchProof.Tree.prune
    (some 0)
    1
    (Minion.DomainProof.Operation.remove)
    0
    (Minion.SearchProof.Tree.prune
      (some 2)
      2
      (Minion.DomainProof.Operation.remove)
      0
      (Minion.SearchProof.Tree.prune
        (some 1)
        2
        (Minion.DomainProof.Operation.remove)
        1
        (Minion.SearchProof.Tree.conflict 1))))
  (Minion.SearchProof.Tree.prune
    (some 0)
    1
    (Minion.DomainProof.Operation.remove)
    1
    (Minion.SearchProof.Tree.prune
      (some 2)
      2
      (Minion.DomainProof.Operation.remove)
      1
      (Minion.SearchProof.Tree.prune
        (some 1)
        2
        (Minion.DomainProof.Operation.remove)
        0
        (Minion.SearchProof.Tree.conflict 1))))

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
