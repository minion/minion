import Minion.BlockSearchProof

open Minion Minion.DomainProof Minion.BlockSearchProof

private def m : Model := ⟨[⟨"x", [0,1,2,3]⟩], [
  .compare .ne (.var 0) (.constant 0), .compare .ne (.var 0) (.constant 1),
  .compare .ne (.var 0) (.constant 2)]⟩

private def p : Program := .block [
  .prune (some 0) 0 .remove 0, .prune (some 1) 0 .remove 1,
  .prune (some 2) 0 .remove 2] (.solution [3])

example : check m [[0,1,2,3]] p false = true := by decide +kernel
example : check m [[0,1,2,3]] p true = true := by decide +kernel
example : solutions p = [[3]] := by decide +kernel
example : verify m p [[3]] = true := by decide +kernel
#eval show IO Unit from do
  unless !verify m p [[3],[3]] do throw (IO.userError "duplicate solutions accepted")
example : check m [[0,1,2,3]] (.solution [3]) = false := by decide +kernel
example : check m [[0,1,2,3]] (.split 99 0 (.solution [3]) (.solution [3])) = false := by decide +kernel
example : check m [[0,1,2,3]] (.block [.prune (some 0) 0 .assign 3] (.solution [3])) = false := by decide +kernel
example : check m [[0,1,2,3]] (.refute 0 .remove 0 (.conflict 0)
    (.block [.prune (some 1) 0 .remove 1, .prune (some 2) 0 .remove 2] (.solution [3]))) = true := by decide +kernel
example : check m [[0,1,2,3]] (.refute 0 .remove 0 (.solution [0]) p) = false := by decide +kernel

private def impossible (positive : Bool) : Model :=
  ⟨[⟨"x", [0,1]⟩], [.reify (.table [.var 0, .var 0] [[0,1],[1,0]] positive)
    (.constant (if positive then 1 else 0))]⟩
example : check (impossible true) [[0,1]] (.block [.filterTables 0] (.conflict 0)) = true := by decide +kernel
example : check (impossible false) [[0,1]] (.block [.filterTables 0] (.conflict 0)) = true := by decide +kernel

example : ((List.range 80).all fun i =>
    ConstraintIndex.get (ConstraintIndex.build (List.range 64)) i == (List.range 64)[i]?) = true := by decide +kernel

private def index : ConstraintIndex.Index Constraint :=
  .node (some (.compare .ne (.var 0) (.constant 0)))
    (.node none .empty (.node (some (.compare .ne (.var 0) (.constant 2))) .empty .empty))
    (.node (some (.compare .ne (.var 0) (.constant 1))) .empty .empty)
example : index = ConstraintIndex.build m.constraints := by with_unfolding_all rfl
example : checkWithIndex m [[0,1,2,3]] p index = true := by decide +kernel

-- A raw local check can succeed against an unrelated false constraint. The
-- correspondence premise is essential and cannot be supplied for this index.
private def satisfiable : Model := ⟨[], [.truth true]⟩
private def forged : ConstraintIndex.Index Constraint := .node (some (.truth false)) .empty .empty
example : checkWithIndex satisfiable [] (.conflict 0) forged = true := by decide +kernel
example : forged ≠ ConstraintIndex.build satisfiable.constraints := by
  intro h
  have hg := congrArg (fun i => ConstraintIndex.get i 0) h
  rw [ConstraintIndex.get_build] at hg
  simp [forged, satisfiable, ConstraintIndex.get, ConstraintIndex.read, ConstraintIndex.address] at hg

private def outside : Model := ⟨[⟨"x", [0]⟩], List.replicate 512 (.truth true)⟩
example : check outside [[1]] (.solution [1]) = false := by decide +kernel
example : check m [[2,0,3,1]] p = true := by decide +kernel
example : check ⟨[], []⟩ [] (.solution []) = true := by decide +kernel
example : check ⟨[], []⟩ [] .empty = false := by decide +kernel
example : check ⟨[], []⟩ [] (.conflict 0) = false := by decide +kernel
example : checkWithIndex ⟨[], []⟩ [] (.solution []) .empty = true := by decide +kernel

example (model : Model) (domains : List (List Int)) (program : Program) :
    check model domains program = SearchProof.check model domains (expand program) :=
  check_eq model domains program

#print axioms Minion.ConstraintIndex.get_build
#print axioms Minion.BlockSearchProof.check_eq
#print axioms Minion.BlockSearchProof.checkWithIndex_eq
#print axioms Minion.BlockSearchProof.check_complete
#print axioms Minion.BlockSearchProof.check_nodup
#print axioms Minion.BlockSearchProof.verify_eq
#print axioms Minion.BlockSearchProof.expand_ofTree
