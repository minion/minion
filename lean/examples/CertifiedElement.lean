import Minion.SearchProof

set_option maxRecDepth 100000
set_option maxHeartbeats 0

namespace MinionCertificate

def model : Minion.Model :=
{ variables := [{ name := "index", domain := [0, 1] },
                { name := "result", domain := [1, 2, 3] },
                { name := "b", domain := [0, 1] }],
  constraints := [Minion.Constraint.reify
                    (Minion.Constraint.element
                      [Minion.Term.constant 1, Minion.Term.constant 3]
                      (Minion.Term.var 0)
                      (Minion.Term.var 1)
                      0
                      false)
                    (Minion.Term.var 2)] }

def tree : Minion.SearchProof.Tree :=
Minion.SearchProof.Tree.split
  0
  0
  (Minion.SearchProof.Tree.split
    1
    1
    (Minion.SearchProof.Tree.prune
      (some 0)
      2
      (Minion.DomainProof.Operation.assign)
      1
      (Minion.SearchProof.Tree.solution [0, 1, 1]))
    (Minion.SearchProof.Tree.prune
      (some 0)
      2
      (Minion.DomainProof.Operation.assign)
      0
      (Minion.SearchProof.Tree.split
        1
        2
        (Minion.SearchProof.Tree.solution [0, 2, 0])
        (Minion.SearchProof.Tree.solution [0, 3, 0]))))
  (Minion.SearchProof.Tree.split
    1
    1
    (Minion.SearchProof.Tree.prune
      (some 0)
      2
      (Minion.DomainProof.Operation.assign)
      0
      (Minion.SearchProof.Tree.solution [1, 1, 0]))
    (Minion.SearchProof.Tree.split
      1
      2
      (Minion.SearchProof.Tree.prune
        (some 0)
        2
        (Minion.DomainProof.Operation.assign)
        0
        (Minion.SearchProof.Tree.solution [1, 2, 0]))
      (Minion.SearchProof.Tree.prune
        (some 0)
        2
        (Minion.DomainProof.Operation.assign)
        1
        (Minion.SearchProof.Tree.solution [1, 3, 1]))))

def reported : List Minion.Assignment :=
[[0, 1, 1], [0, 2, 0], [0, 3, 0], [1, 1, 0], [1, 2, 0], [1, 3, 1]]

set_option maxRecDepth 100000 in
set_option maxHeartbeats 0 in
theorem checked : Minion.SearchProof.check model (model.variables.map Minion.Variable.domain) tree = true := by decide +kernel

theorem reported_eq : reported = Minion.SearchProof.solutions tree := by decide +kernel

theorem solutions_correct :
    ∀ a, a ∈ reported ↔ model.IsSolution a := by
  rw [reported_eq]
  exact Minion.SearchProof.check_complete model tree checked

#print axioms solutions_correct

end MinionCertificate
