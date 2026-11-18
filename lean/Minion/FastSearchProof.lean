import Minion.SearchProof
import Minion.EvalProof

/-! An equivalent pure checker with cached constraint lookup and first-occurrence
    deletion on domains known to contain no duplicates. The initial linear
    order check supplies that prerequisite; arbitrary input order falls back
    to the original checker. No solver facts or native oracle are assumed. -/
namespace Minion.FastSearchProof
open DomainProof SearchProof

private def increasing : List Int → Bool
  | [] | [_] => true
  | x :: y :: xs => x < y && increasing (y :: xs)

private theorem increasing_tail (x : Int) (xs : List Int) (h : increasing (x :: xs) = true) :
    increasing xs = true := by
  cases xs with
  | nil => rfl
  | cons y ys =>
    have hp : x < y ∧ increasing (y :: ys) = true := by
      simpa only [increasing, Bool.and_eq_true, decide_eq_true_eq] using h
    exact hp.2

private theorem increasing_head (x : Int) (xs : List Int) (h : increasing (x :: xs) = true)
    (v : Int) (hv : v ∈ xs) : x < v := by
  induction xs generalizing x with
  | nil => simp at hv
  | cons y ys ih =>
    have hp : x < y ∧ increasing (y :: ys) = true := by
      simpa only [increasing, Bool.and_eq_true, decide_eq_true_eq] using h
    rcases List.mem_cons.mp hv with he | hr
    · simpa [he] using hp.1
    · exact Int.lt_trans hp.1 (ih y hp.2 hr)

private theorem increasing_nodup (xs : List Int) (h : increasing xs = true) : xs.Nodup := by
  induction xs with
  | nil => simp
  | cons x xs ih =>
    apply List.nodup_cons.mpr
    refine ⟨?_, ih (increasing_tail x xs h)⟩
    intro hm
    have := increasing_head x xs h x hm
    omega

def UniqueDomains (d : List (List Int)) : Prop := ∀ xs ∈ d, xs.Nodup

theorem unique_restrict (d : List (List Int)) (target : Nat) (op : Operation) (bound : Int)
    (hd : UniqueDomains d) : UniqueDomains (restrict d target op bound) := by
  induction d generalizing target with
  | nil => simp [restrict, UniqueDomains]
  | cons xs rest ih =>
    have hx := hd xs (by simp)
    have hr : UniqueDomains rest := fun ys hy => hd ys (by simp [hy])
    cases target with
    | zero =>
      intro ys hy
      simp only [restrict, List.modify_zero_cons, List.mem_cons] at hy
      rcases hy with rfl | hy
      · exact hx.filter _
      · exact hr ys hy
    | succ target =>
      intro ys hy
      simp only [restrict, List.modify_succ_cons, List.mem_cons] at hy
      rcases hy with rfl | hy
      · exact hx
      · exact ih target hr ys hy

def updateFast (op : Operation) (bound : Int) (xs : List Int) : List Int :=
  match op with
  | .remove => xs.erase bound
  | .assign => if xs.contains bound then [bound] else []
  | op => xs.filter (op.holds bound)

private theorem updateFast_eq (op : Operation) (bound : Int) (xs : List Int) (hx : xs.Nodup) :
    updateFast op bound xs = xs.filter (op.holds bound) := by
  cases op with
  | remove => exact hx.erase_eq_filter bound
  | lowerBound | upperBound => rfl
  | assign =>
    change (if xs.contains bound then [bound] else []) = xs.filter (Operation.holds .assign bound)
    have heq : Operation.holds .assign bound = (fun v => v == bound) := by funext v; rfl
    rw [heq]
    induction xs with
    | nil => simp
    | cons x xs ih =>
      have hp := List.nodup_cons.mp hx
      by_cases he : x = bound
      · subst x
        have hf : xs.filter (fun v => v == bound) = [] := by
          apply List.filter_eq_nil_iff.mpr
          intro v hv hh
          have he : v = bound := by simpa using hh
          subst v
          exact hp.1 hv
        simp only [List.filter_cons, beq_self_eq_true, ↓reduceIte]
        rw [hf]
        simp
      · have hb : (x == bound) = false := by simp [he]
        simp only [List.filter_cons, hb, Bool.false_eq_true, ↓reduceIte]
        have hc : (x :: xs).contains bound = xs.contains bound := by simp [Ne.symm he]
        rw [hc]
        exact ih hp.2

def restrictFast (d : List (List Int)) (target : Nat) (op : Operation) (bound : Int) : List (List Int) :=
  d.modify target (updateFast op bound)

theorem restrictFast_eq (d : List (List Int)) (target : Nat) (op : Operation) (bound : Int)
    (hd : UniqueDomains d) : restrictFast d target op bound = restrict d target op bound := by
  induction d generalizing target with
  | nil => simp [restrictFast, restrict]
  | cons xs rest ih =>
    have hx := hd xs (by simp)
    have hr : UniqueDomains rest := fun ys hy => hd ys (by simp [hy])
    cases target with
    | zero =>
      simp only [restrictFast, restrict, List.modify_zero_cons]
      exact congrArg (fun ys => ys :: rest) (updateFast_eq op bound xs hx)
    | succ target =>
      simpa only [restrictFast, restrict, List.modify_succ_cons] using
        congrArg (fun ys => xs :: ys) (ih target hr)

-- This invariant starts with an exact domain-list comparison. Restriction
-- preserves it, including inside failed probes and reduced table models.
def WithinModel (m : Model) (d : List (List Int)) : Prop :=
  ∀ a, InDomains d a → InDomains (m.variables.map Variable.domain) a

theorem within_restrict (m : Model) (d : List (List Int)) (target : Nat)
    (op : Operation) (bound : Int) (h : WithinModel m d) :
    WithinModel m (restrict d target op bound) :=
  fun a ha => h a (restrict_reflects d target op bound a ha)

private theorem singleton_domains (a : Assignment) : InDomains (a.map (fun v => [v])) a := by
  induction a with
  | nil => trivial
  | cons v vs ih => exact ⟨by simp, ih⟩

def leaf (m : Model) (d : List (List Int)) (within : Bool) (a : Assignment) : Bool :=
  d == a.map (fun v => [v]) &&
    if within then evalConstraints a m.constraints else m.satisfies a

theorem leaf_eq (m : Model) (d : List (List Int)) (within : Bool) (a : Assignment)
    (hw : within = true → WithinModel m d) :
    leaf m d within a = (d == a.map (fun v => [v]) && m.satisfies a) := by
  cases within with
  | false => rfl
  | true =>
    by_cases hd : d = a.map (fun v => [v])
    · have ha := hw rfl a (by rw [hd]; exact singleton_domains a)
      apply Bool.eq_iff_iff.mpr
      simp only [leaf, hd, beq_self_eq_true, ↓reduceIte, Bool.true_and, evalConstraints_eq]
      constructor
      · intro hc
        exact (satisfies_correct m a).mpr ⟨ha, hc⟩
      · intro hc
        exact ((satisfies_correct m a).mp hc).2
    · have hb : (d == a.map (fun v => [v])) = false := by simpa only [beq_eq_false_iff_ne] using hd
      simp only [leaf, hb, Bool.false_and]

private def pruneCached (cache : Array Constraint) (d : List (List Int)) (index : Option Nat)
    (target : Nat) (op : Operation) (bound : Int) : Bool :=
  checkNoop (ofLists d) target op bound || hasEmptyDomain d ||
    match index.bind (cache[·]?) with
    | none => false
    | some c => LogicProof.rule.prune (ofLists d) c target op bound

private theorem pruneCached_eq (m : Model) (d : List (List Int)) (index : Option Nat)
    (target : Nat) (op : Operation) (bound : Int) :
    pruneCached m.constraints.toArray d index target op bound = pruneCheck m d index target op bound := by
  simp only [pruneCached, pruneCheck, List.getElem?_toArray]
  rfl

private def cached (m : Model) (cache : Array Constraint) (d : List (List Int)) (within : Bool) : Tree → Bool
  | .empty => hasEmptyDomain d
  | .conflict index => hasEmptyDomain d || (match cache[index]? with
    | none => false
    | some c => LogicProof.checkConflict (ofLists d) c)
  | .solution a => leaf m d within a
  | .prune index target op bound next =>
    pruneCached cache d index target op bound && cached m cache (restrictFast d target op bound) within next
  | .refute target op bound proof next =>
    cached m cache (reject d target op bound) within proof && (solutions proof).isEmpty &&
      cached m cache (restrictFast d target op bound) within next
  | .split target value left right =>
    decide (target < d.length) &&
      (cached m cache (restrictFast d target .assign value) within left &&
       cached m cache (restrictFast d target .remove value) within right)
  | .explainedConflict index evidence =>
    (cache[index]?).any fun c => (InferenceWitness.rule evidence).conflict (ofLists d) c
  | .explainedPrune index target op bound evidence next =>
    ((cache[index]?).any fun c =>
      (InferenceWitness.rule evidence (some target)).prune (ofLists d) c target op bound) &&
      cached m cache (restrictFast d target op bound) within next
  | .tableReduce index kept next =>
    TableProjection.checkModel m d index kept &&
      let reduced := TableProjection.reduceModel m index kept
      cached reduced reduced.constraints.toArray d within next
  | .filterTables index next =>
    let reduced := TableProjection.filterModel m d index
    cached reduced reduced.constraints.toArray d within next

private theorem cached_eq (m : Model) (d : List (List Int)) (tree : Tree) (hd : UniqueDomains d)
    (within : Bool) (hw : within = true → WithinModel m d) :
    cached m m.constraints.toArray d within tree = SearchProof.check m d tree := by
  induction tree generalizing m d with
  | empty => rfl
  | conflict index =>
    simp only [cached, SearchProof.check, List.getElem?_toArray]
    rfl
  | solution a => exact leaf_eq m d within a hw
  | prune index target op bound next ih =>
    simp only [cached, SearchProof.check, pruneCached_eq, restrictFast_eq d target op bound hd]
    rw [ih m _ (unique_restrict d target op bound hd)
      (fun h => within_restrict m d target op bound (hw h))]
  | refute target op bound proof next ip ih =>
    simp only [cached, SearchProof.check, restrictFast_eq d target op bound hd]
    have hr : UniqueDomains (reject d target op bound) := by
      rw [reject_eq_complement]
      exact unique_restrict d target _ _ hd
    have wr : within = true → WithinModel m (reject d target op bound) := by
      intro h
      rw [reject_eq_complement]
      exact within_restrict m d target _ _ (hw h)
    rw [ip m _ hr wr, ih m _ (unique_restrict d target op bound hd)
      (fun h => within_restrict m d target op bound (hw h))]
  | split target value left right il ir =>
    simp only [cached, SearchProof.check, restrictFast_eq d target .assign value hd,
      restrictFast_eq d target .remove value hd]
    rw [il m _ (unique_restrict d target .assign value hd)
      (fun h => within_restrict m d target .assign value (hw h)),
      ir m _ (unique_restrict d target .remove value hd)
      (fun h => within_restrict m d target .remove value (hw h))]
  | explainedConflict index evidence => simp only [cached, SearchProof.check, List.getElem?_toArray]
  | explainedPrune index target op bound evidence next ih =>
    simp only [cached, SearchProof.check, List.getElem?_toArray, restrictFast_eq d target op bound hd]
    rw [ih m _ (unique_restrict d target op bound hd)
      (fun h => within_restrict m d target op bound (hw h))]
  | tableReduce index kept next ih =>
    simp only [cached, SearchProof.check]
    rw [ih _ d hd (by simpa only [WithinModel, TableProjection.reduceModel] using hw)]
  | filterTables index next ih =>
    simp only [cached, SearchProof.check]
    exact ih _ d hd (by simpa only [WithinModel, TableProjection.filterModel] using hw)

private def large (m : Model) (d : List (List Int)) : Bool :=
  m.constraints.length ≥ 512 || d.any (fun xs => xs.length ≥ 512)

/-- Prerequisite for the duplicate-free domain operations, shared by block checking. -/
def eligible (m : Model) (d : List (List Int)) : Bool := large m d && d.all increasing

theorem eligible_unique (m : Model) (d : List (List Int)) (h : eligible m d = true) :
    UniqueDomains d := by
  have hp : large m d = true ∧ d.all increasing = true := by
    simpa only [eligible, Bool.and_eq_true] using h
  exact fun xs hx => increasing_nodup xs (List.all_eq_true.mp hp.2 xs hx)

def check (m : Model) (d : List (List Int)) (tree : Tree) : Bool :=
  if large m d && d.all increasing then
    cached m m.constraints.toArray d (d == m.variables.map Variable.domain) tree
  else SearchProof.check m d tree

theorem check_eq (m : Model) (d : List (List Int)) (tree : Tree) :
    check m d tree = SearchProof.check m d tree := by
  unfold check
  split
  · rename_i h
    have hp : large m d = true ∧ d.all increasing = true := by simpa only [Bool.and_eq_true] using h
    apply cached_eq m d tree (fun xs hx => increasing_nodup xs (List.all_eq_true.mp hp.2 xs hx))
    intro he
    have hd : d = m.variables.map Variable.domain := by simpa using he
    rw [hd]
    exact fun _ ha => ha
  · rfl

private def collectReverse : Tree → List Assignment → List Assignment
  | .empty, acc | .conflict _, acc | .explainedConflict .., acc => acc
  | .solution a, acc => a :: acc
  | .prune _ _ _ _ next, acc | .explainedPrune _ _ _ _ _ next, acc
  | .tableReduce _ _ next, acc | .filterTables _ next, acc => collectReverse next acc
  | .refute _ _ _ _ next, acc => collectReverse next acc
  | .split _ _ left right, acc => collectReverse right (collectReverse left acc)

private theorem collectReverse_eq (tree : Tree) (acc : List Assignment) :
    collectReverse tree acc = (solutions tree).reverse ++ acc := by
  induction tree generalizing acc with
  | empty => simp [collectReverse, solutions]
  | conflict index => simp [collectReverse, solutions]
  | explainedConflict index evidence => simp [collectReverse, solutions]
  | solution a => simp [collectReverse, solutions]
  | prune index target op bound next ih => simp only [collectReverse, solutions, ih]
  | explainedPrune index target op bound evidence next ih => simp only [collectReverse, solutions, ih]
  | tableReduce index kept next ih => simp only [collectReverse, solutions, ih]
  | filterTables index next ih => simp only [collectReverse, solutions, ih]
  | refute target op bound proof next _ ih => simp only [collectReverse, solutions, ih]
  | split target value left right il ir =>
    simp only [collectReverse, solutions, ir, il, List.reverse_append, List.append_assoc]

def solutionsReverse (tree : Tree) : List Assignment := collectReverse tree []

theorem solutionsReverse_eq (tree : Tree) : solutionsReverse tree = (solutions tree).reverse := by
  simp only [solutionsReverse, collectReverse_eq, List.append_nil]

def verify (m : Model) (tree : Tree) (reported : List Assignment) : Bool :=
  check m (m.variables.map Variable.domain) tree &&
    (solutionsReverse tree == reported || sameAssignments (solutions tree) reported)

theorem verify_eq (m : Model) (tree : Tree) (reported : List Assignment) :
    verify m tree reported = SearchProof.verify m tree reported := by
  simp only [verify, check_eq, SearchProof.verify]
  have he : (solutionsReverse tree == reported || sameAssignments (solutions tree) reported) =
      sameAssignments (solutions tree) reported := by
    cases hr : solutionsReverse tree == reported with
    | false => simp
    | true =>
      have hrev : (solutions tree).reverse = reported := by
        simpa only [solutionsReverse_eq, beq_iff_eq] using hr
      have hs : solutions tree = reported.reverse := by
        simpa only [List.reverse_reverse] using congrArg List.reverse hrev
      have hp : sameAssignments (solutions tree) reported = true := by simp [sameAssignments, hs]
      simp [hp]
  rw [he]

end Minion.FastSearchProof
