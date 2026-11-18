import Minion.InferenceWitness
import Minion.TableProjection

/-! A small, pure search certificate. Its checker composes the fixed local rules
    with finite-domain splitting. The theorem is independent of the C++ solver,
    trace parser and any algorithm used to construct the certificate tree. -/
namespace Minion.SearchProof
open DomainProof

/-- Conservative syntactic gate for the diagnostic no-matching-discovery mode.
    Other support enumeration is deliberately outside this flag's guarantee. -/
def needsMatching (c : Constraint) : Bool :=
  match c with
  | .allDifferent .. | .gcc .. | .allDifferentMatrix .. => true
  | .reify child _ | .imply child _ => needsMatching child
  | .conjunction children | .disjunction children => children.attach.any (fun c => needsMatching c.val)
  | _ => false
termination_by sizeOf c
decreasing_by
  all_goals simp only [Constraint.reify.sizeOf_spec, Constraint.imply.sizeOf_spec,
    Constraint.conjunction.sizeOf_spec, Constraint.disjunction.sizeOf_spec]
  all_goals first | omega | exact Nat.lt_trans (List.sizeOf_lt_of_mem c.property) (by omega)

inductive Tree where
  | empty
  | conflict (constraint : Nat)
  | solution (assignment : Assignment)
  | prune (constraint : Option Nat) (target : Nat) (operation : Operation) (bound : Int) (next : Tree)
  | refute (target : Nat) (operation : Operation) (bound : Int) (refutation next : Tree)
  | split (target : Nat) (value : Int) (left right : Tree)
  | explainedConflict (constraint : Nat) (evidence : InferenceWitness.Evidence)
  | explainedPrune (constraint target : Nat) (operation : Operation) (bound : Int)
      (evidence : InferenceWitness.Evidence) (next : Tree)
  | tableReduce (constraint : Nat) (kept : List Nat) (next : Tree)
  | filterTables (constraint : Nat) (next : Tree)
  deriving Repr

def restrict (d : List (List Int)) (target : Nat) (operation : Operation) (bound : Int) : List (List Int) :=
  d.modify target (List.filter (operation.holds bound))

def reject (d : List (List Int)) (target : Nat) (operation : Operation) (bound : Int) : List (List Int) :=
  d.modify target (List.filter (fun v => !operation.holds bound v))

def complement (operation : Operation) (bound : Int) : Operation × Int :=
  match operation with
  | .assign => (.remove, bound)
  | .remove => (.assign, bound)
  | .lowerBound => (.upperBound, bound - 1)
  | .upperBound => (.lowerBound, bound + 1)

theorem complement_holds (operation : Operation) (bound value : Int) :
    (complement operation bound).1.holds (complement operation bound).2 value =
      !operation.holds bound value := by
  cases operation <;> dsimp only [complement] <;> apply Bool.eq_iff_iff.mpr <;>
    simp [Operation.holds, Bool.eq_false_iff] <;> omega

theorem reject_eq_complement (d : List (List Int)) (target : Nat) (operation : Operation) (bound : Int) :
    reject d target operation bound =
      restrict d target (complement operation bound).1 (complement operation bound).2 := by
  have he : (complement operation bound).1.holds (complement operation bound).2 =
      (fun value => !operation.holds bound value) := funext (complement_holds operation bound)
  simp only [reject, restrict, he]

def pruneCheck (m : Model) (d : List (List Int)) (constraint : Option Nat)
    (target : Nat) (operation : Operation) (bound : Int) : Bool :=
  checkNoop (ofLists d) target operation bound || hasEmptyDomain d ||
    match constraint.bind (m.constraints[·]?) with
    | none => false
    | some c => LogicProof.rule.prune (ofLists d) c target operation bound

def solutions : Tree → List Assignment
  | .empty | .conflict _ | .explainedConflict .. => []
  | .solution a => [a]
  | .prune _ _ _ _ next => solutions next
  | .refute _ _ _ _ next => solutions next
  | .split _ _ left right => solutions left ++ solutions right
  | .explainedPrune _ _ _ _ _ next | .tableReduce _ _ next | .filterTables _ next => solutions next

def check (m : Model) (d : List (List Int)) : Tree → Bool
  | .empty => hasEmptyDomain d
  | .conflict index => hasEmptyDomain d || (match m.constraints[index]? with
    | none => false
    | some c => LogicProof.checkConflict (ofLists d) c)
  | .solution a => d == a.map (fun v => [v]) && m.satisfies a
  | .prune index target operation bound next =>
    pruneCheck m d index target operation bound && check m (restrict d target operation bound) next
  | .refute target operation bound refutation next =>
    check m (reject d target operation bound) refutation && (solutions refutation).isEmpty &&
      check m (restrict d target operation bound) next
  | .split target value left right =>
    decide (target < d.length) &&
      (check m (restrict d target .assign value) left && check m (restrict d target .remove value) right)
  | .explainedConflict index evidence =>
    (m.constraints[index]?).any fun c => (InferenceWitness.rule evidence).conflict (ofLists d) c
  | .explainedPrune index target operation bound evidence next =>
    ((m.constraints[index]?).any fun c =>
      (InferenceWitness.rule evidence (some target)).prune (ofLists d) c target operation bound) &&
    check m (restrict d target operation bound) next
  | .tableReduce index kept next =>
    TableProjection.checkModel m d index kept && check (TableProjection.reduceModel m index kept) d next

  | .filterTables index next => check (TableProjection.filterModel m d index) d next

def refutationCheck (m : Model) (d : List (List Int)) (target : Nat)
    (operation : Operation) (bound : Int) (refutation : Tree) : Bool :=
  check m (reject d target operation bound) refutation && (solutions refutation).isEmpty

theorem satisfies_correct (m : Model) (a : Assignment) : m.satisfies a = true ↔ m.IsSolution a := by
  have hdomains (vs : List Variable) (a : Assignment) :
      (a.length == vs.length && (vs.zip a).all fun (v, x) => v.domain.contains x) = true ↔
        InDomains (vs.map Variable.domain) a := by
    induction vs generalizing a with
    | nil => cases a <;> simp [InDomains]
    | cons v vs ih =>
      cases a with
      | nil => simp [InDomains]
      | cons x xs =>
        simp only [List.map_cons, InDomains]
        rw [← ih xs]
        simp [Bool.and_eq_true, and_left_comm]
  simp only [Model.satisfies, Bool.and_eq_true, Model.IsSolution, hdomains]

theorem restrict_reflects (d : List (List Int)) (target : Nat) (operation : Operation) (bound : Int)
    (a : Assignment) (hd : InDomains (restrict d target operation bound) a) : InDomains d a := by
  induction d generalizing target a with
  | nil => simpa [restrict] using hd
  | cons domain rest ih =>
    cases a with
    | nil => simp [restrict, InDomains] at hd
    | cons value values =>
      cases target with
      | zero => exact ⟨(List.mem_filter.mp hd.1).1, hd.2⟩
      | succ target => exact ⟨hd.1, ih target values hd.2⟩

theorem prune_preserves (m : Model) (d : List (List Int)) (index : Option Nat)
    (target : Nat) (operation : Operation) (bound : Int)
    (hc : pruneCheck m d index target operation bound = true)
    (a : Assignment) (hs : m.IsSolution a) (hd : InDomains d a) :
    InDomains (restrict d target operation bound) a := by
  have hcases : checkNoop (ofLists d) target operation bound = true ∨ hasEmptyDomain d = true ∨
      (match index.bind (m.constraints[·]?) with
      | none => false
      | some c => LogicProof.rule.prune (ofLists d) c target operation bound) = true := by
    simpa only [pruneCheck, Bool.or_eq_true, or_assoc] using hc
  cases hcases with
  | inl hn =>
    exact inDomains_restrict d a target operation bound hd
      (checkNoop_sound _ target operation bound hn a (inDomains_compatible d a hd))
  | inr hr =>
    cases hr with
    | inl he => exact False.elim (hasEmptyDomain_excludes d a he hd)
    | inr hp =>
      cases index with
      | none => simp at hp
      | some i =>
        cases hi : m.constraints[i]? with
        | none => simp [hi] at hp
        | some c =>
          exact LogicProof.rule.prune_preservesSolutions m d c (List.mem_of_getElem? hi)
            target operation bound (by simpa [hi] using hp) a hs hd

theorem split_covers (d : List (List Int)) (target : Nat) (value : Int) (a : Assignment)
    (hd : InDomains d a) :
    InDomains (restrict d target .assign value) a ∨ InDomains (restrict d target .remove value) a := by
  by_cases he : a[target]?.getD 0 = value
  · exact Or.inl (inDomains_restrict d a target .assign value hd (by simp [Operation.holds, he]))
  · exact Or.inr (inDomains_restrict d a target .remove value hd (by simp [Operation.holds, he]))

theorem restrict_holds (d : List (List Int)) (target : Nat) (operation : Operation) (bound : Int)
    (a : Assignment) (ht : target < d.length)
    (hd : InDomains (restrict d target operation bound) a) :
    operation.holds bound (a[target]?.getD 0) = true := by
  induction d generalizing target a with
  | nil => simp at ht
  | cons domain rest ih =>
    cases a with
    | nil => simp [restrict, InDomains] at hd
    | cons value values =>
      cases target with
      | zero => exact (List.mem_filter.mp hd.1).2
      | succ target => exact ih target values (by simpa using ht) hd.2

theorem split_disjoint (d : List (List Int)) (target : Nat) (value : Int) (a : Assignment)
    (ht : target < d.length) :
    ¬(InDomains (restrict d target .assign value) a ∧
      InDomains (restrict d target .remove value) a) := by
  intro ⟨hl, hr⟩
  have he : a[target]?.getD 0 = value := by
    simpa [Operation.holds] using restrict_holds d target .assign value a ht hl
  have hn : a[target]?.getD 0 ≠ value := by
    simpa [Operation.holds] using restrict_holds d target .remove value a ht hr
  exact hn he

theorem singleton_domains (a b : Assignment) : InDomains (a.map fun v => [v]) b ↔ b = a := by
  induction a generalizing b with
  | nil => cases b <;> simp [InDomains]
  | cons v vs ih => cases b <;> simp [InDomains, ih]

/-- Every accepted subtree reports exactly the original model solutions in its
    current domains. No propagation-strength or solver-correctness assumption. -/
theorem check_correct (m : Model) (d : List (List Int)) (tree : Tree) (hc : check m d tree = true) :
    ∀ a, a ∈ solutions tree ↔ m.IsSolution a ∧ InDomains d a := by
  induction tree generalizing m d with
  | empty =>
    intro a
    simp only [solutions, List.not_mem_nil, false_iff, not_and]
    intro _ hd
    exact hasEmptyDomain_excludes d a hc hd
  | conflict index =>
    cases he : hasEmptyDomain d with
    | true =>
      intro a
      simp only [solutions, List.not_mem_nil, false_iff, not_and]
      intro _ hd
      exact hasEmptyDomain_excludes d a he hd
    | false =>
      simp only [check, he, Bool.false_or] at hc
      cases hi : m.constraints[index]? with
      | none => simp [hi] at hc
      | some c =>
        intro a
        simp only [solutions, List.not_mem_nil, false_iff, not_and]
        intro hs hd
        exact LogicProof.rule.conflict_excludesSolutions m d c (List.mem_of_getElem? hi)
          (by simpa [hi, LogicProof.rule] using hc) a hs hd
  | solution assignment =>
    have hparts : d = assignment.map (fun v => [v]) ∧ m.satisfies assignment = true := by
      simpa only [check, Bool.and_eq_true, beq_iff_eq] using hc
    have hs := satisfies_correct m assignment |>.mp hparts.2
    rw [hparts.1]
    intro a
    simp only [solutions, List.mem_singleton, singleton_domains]
    constructor
    · intro he; subst a; exact ⟨hs, rfl⟩
    · exact And.right
  | prune index target operation bound next ih =>
    have hparts : pruneCheck m d index target operation bound = true ∧
        check m (restrict d target operation bound) next = true := by
      simpa only [check, Bool.and_eq_true] using hc
    intro a
    rw [solutions, ih m _ hparts.2 a]
    constructor
    · intro ⟨hs, hd⟩; exact ⟨hs, restrict_reflects d target operation bound a hd⟩
    · intro ⟨hs, hd⟩; exact ⟨hs, prune_preserves m d index target operation bound hparts.1 a hs hd⟩
  | refute target operation bound refutation next iref inext =>
    have hparts : (check m (reject d target operation bound) refutation = true ∧
        (solutions refutation).isEmpty = true) ∧
        check m (restrict d target operation bound) next = true := by
      simpa only [check, Bool.and_eq_true] using hc
    have hreject (a) (hs : m.IsSolution a) (hd : InDomains d a)
        (hbad : operation.holds bound (a[target]?.getD 0) = false) : False := by
      have hcomp : (complement operation bound).1.holds (complement operation bound).2
          (a[target]?.getD 0) = true := by simp [complement_holds, hbad]
      have hin : InDomains (reject d target operation bound) a := by
        rw [reject_eq_complement]
        exact inDomains_restrict d a target _ _ hd hcomp
      have hm := (iref m _ hparts.1.1 a).mpr ⟨hs, hin⟩
      have hempty : solutions refutation = [] := List.isEmpty_iff.mp hparts.1.2
      simp [hempty] at hm
    intro a
    rw [solutions, inext m _ hparts.2 a]
    constructor
    · intro ⟨hs, hd⟩; exact ⟨hs, restrict_reflects d target operation bound a hd⟩
    · intro ⟨hs, hd⟩
      refine ⟨hs, inDomains_restrict d a target operation bound hd ?_⟩
      cases hgood : operation.holds bound (a[target]?.getD 0) with
      | true => rfl
      | false => exact False.elim (hreject a hs hd hgood)
  | split target value left right il ir =>
    have hparts : target < d.length ∧
        (check m (restrict d target .assign value) left = true ∧
         check m (restrict d target .remove value) right = true) := by
      simpa only [check, Bool.and_eq_true, decide_eq_true_eq] using hc
    intro a
    rw [solutions, List.mem_append, il m _ hparts.2.1 a, ir m _ hparts.2.2 a]
    constructor
    · intro h
      cases h with
      | inl h => exact ⟨h.1, restrict_reflects d target .assign value a h.2⟩
      | inr h => exact ⟨h.1, restrict_reflects d target .remove value a h.2⟩
    · intro ⟨hs, hd⟩
      cases split_covers d target value a hd with
      | inl hl => exact Or.inl ⟨hs, hl⟩
      | inr hr => exact Or.inr ⟨hs, hr⟩


  | explainedConflict index evidence =>
    cases hi : m.constraints[index]? with
    | none => simp [check, hi] at hc
    | some c =>
      have hp : (InferenceWitness.rule evidence).conflict (ofLists d) c = true := by
        simpa [check, hi] using hc
      intro a
      simp only [solutions, List.not_mem_nil, false_iff, not_and]
      intro hs hd
      exact (InferenceWitness.rule evidence).conflict_excludesSolutions m d c
        (List.mem_of_getElem? hi) hp a hs hd
  | explainedPrune index target operation bound evidence next ih =>
    have hparts := hc
    simp only [check, Bool.and_eq_true] at hparts
    cases hi : m.constraints[index]? with
    | none => simp [hi] at hparts
    | some c =>
      have hp : (InferenceWitness.rule evidence (some target)).prune (ofLists d) c target operation bound = true := by
        simpa [hi] using hparts.1
      intro a
      rw [solutions, ih m _ hparts.2 a]
      constructor
      · intro ⟨hs, hd⟩; exact ⟨hs, restrict_reflects d target operation bound a hd⟩
      · intro ⟨hs, hd⟩
        exact ⟨hs, (InferenceWitness.rule evidence (some target)).prune_preservesSolutions
          m d c (List.mem_of_getElem? hi) target operation bound hp a hs hd⟩
  | tableReduce index kept next ih =>
    have hp : TableProjection.checkModel m d index kept = true ∧
        check (TableProjection.reduceModel m index kept) d next = true := by
      simpa only [check, Bool.and_eq_true] using hc
    intro a
    rw [solutions, ih _ _ hp.2 a]
    constructor
    · intro ⟨hs, hd⟩
      exact ⟨(TableProjection.solution_iff m d index kept a hd hp.1).mp hs, hd⟩
    · intro ⟨hs, hd⟩
      exact ⟨(TableProjection.solution_iff m d index kept a hd hp.1).mpr hs, hd⟩

  | filterTables index next ih =>
    intro a
    rw [solutions, ih _ _ hc a]
    constructor
    · intro ⟨hs, hd⟩
      exact ⟨(TableProjection.filterModel_solution_iff m d index a hd).mp hs, hd⟩
    · intro ⟨hs, hd⟩
      exact ⟨(TableProjection.filterModel_solution_iff m d index a hd).mpr hs, hd⟩

/-- Uniqueness comes from disjoint, in-range splits, not a scan of the reported
    assignments. Pruning/reduction nodes have one reporting child; failed-probe
    solutions are excluded from the reported list. -/
theorem check_nodup (m : Model) (d : List (List Int)) (tree : Tree)
    (hc : check m d tree = true) : (solutions tree).Nodup := by
  induction tree generalizing m d with
  | empty => simp [solutions]
  | conflict index => simp [solutions]
  | explainedConflict index evidence => simp [solutions]
  | solution a => simp [solutions]
  | prune index target operation bound next ih =>
    simp only [check, Bool.and_eq_true] at hc
    exact ih m _ hc.2
  | refute target operation bound refutation next _ ih =>
    simp only [check, Bool.and_eq_true] at hc
    exact ih m _ hc.2
  | explainedPrune index target operation bound evidence next ih =>
    simp only [check, Bool.and_eq_true] at hc
    exact ih m _ hc.2
  | tableReduce index kept next ih =>
    simp only [check, Bool.and_eq_true] at hc
    exact ih _ _ hc.2
  | filterTables index next ih => exact ih _ _ hc
  | split target value left right il ir =>
    have hp : target < d.length ∧
        (check m (restrict d target .assign value) left = true ∧
         check m (restrict d target .remove value) right = true) := by
      simpa only [check, Bool.and_eq_true, decide_eq_true_eq] using hc
    apply List.nodup_append.mpr
    refine ⟨il m _ hp.2.1, ir m _ hp.2.2, ?_⟩
    intro a ha b hb he
    subst b
    exact split_disjoint d target value a hp.1
      ⟨((check_correct m _ left hp.2.1 a).mp ha).2,
       ((check_correct m _ right hp.2.2 a).mp hb).2⟩

theorem refutation_preserves (m : Model) (d : List (List Int)) (target : Nat)
    (operation : Operation) (bound : Int) (refutation : Tree)
    (hc : refutationCheck m d target operation bound refutation = true)
    (a : Assignment) (hs : m.IsSolution a) (hd : InDomains d a) :
    InDomains (restrict d target operation bound) a := by
  have hparts : check m (reject d target operation bound) refutation = true ∧
      (solutions refutation).isEmpty = true := by simpa only [refutationCheck, Bool.and_eq_true] using hc
  apply inDomains_restrict d a target operation bound hd
  cases hgood : operation.holds bound (a[target]?.getD 0) with
  | true => rfl
  | false =>
    have hbad : InDomains (reject d target operation bound) a := by
      rw [reject_eq_complement]
      exact inDomains_restrict d a target _ _ hd (by simp [complement_holds, hgood])
    have hm := (check_correct m _ refutation hparts.1 a).mpr ⟨hs, hbad⟩
    have he : solutions refutation = [] := List.isEmpty_iff.mp hparts.2
    simp [he] at hm

theorem check_complete (m : Model) (tree : Tree)
    (hc : check m (m.variables.map Variable.domain) tree = true) :
    ∀ a, a ∈ solutions tree ↔ m.IsSolution a := by
  intro a
  rw [check_correct m _ tree hc a]
  exact ⟨And.left, fun hs => ⟨hs, hs.1⟩⟩

/-- Replay accumulates solutions in reverse discovery order. Compare that
    exact multiplicity first; arbitrary permutations retain the sorted fallback. -/
def sameAssignments (xs ys : List Assignment) : Bool :=
  xs == ys.reverse || sortAssignments xs == sortAssignments ys

theorem sameAssignments_perm (xs ys : List Assignment)
    (hc : sameAssignments xs ys = true) : xs.Perm ys := by
  have hp : xs = ys.reverse ∨ sortAssignments xs = sortAssignments ys := by
    simpa only [sameAssignments, Bool.or_eq_true, beq_iff_eq] using hc
  rcases hp with he | he
  · rw [he]
    exact List.reverse_perm ys
  · have px : (sortAssignments xs).Perm xs := List.mergeSort_perm _ _
    have py : (sortAssignments ys).Perm ys := List.mergeSort_perm _ _
    rw [he] at px
    exact px.symm.trans py

def verify (m : Model) (tree : Tree) (reported : List Assignment) : Bool :=
  check m (m.variables.map Variable.domain) tree && sameAssignments (solutions tree) reported

theorem verify_correct (m : Model) (tree : Tree) (reported : List Assignment)
    (hc : verify m tree reported = true) : ∀ a, a ∈ reported ↔ m.IsSolution a := by
  have hp : check m (m.variables.map Variable.domain) tree = true ∧
      sameAssignments (solutions tree) reported = true := by
    simpa only [verify, Bool.and_eq_true] using hc
  have perm := sameAssignments_perm _ _ hp.2
  intro a
  exact perm.mem_iff.symm.trans (check_complete m tree hp.1 a)

theorem verify_nodup (m : Model) (tree : Tree) (reported : List Assignment)
    (hc : verify m tree reported = true) : reported.Nodup := by
  have hp : check m (m.variables.map Variable.domain) tree = true ∧
      sameAssignments (solutions tree) reported = true := by
    simpa only [verify, Bool.and_eq_true] using hc
  exact (sameAssignments_perm _ _ hp.2).nodup_iff.mp (check_nodup m _ tree hp.1)

theorem verify_unsat (m : Model) (tree : Tree) (hc : verify m tree [] = true) :
    ∀ a, ¬m.IsSolution a := by
  intro a hs
  exact List.not_mem_nil ((verify_correct m tree [] hc a).mpr hs)

end Minion.SearchProof
