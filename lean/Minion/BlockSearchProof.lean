import Minion.FastSearchProof
import Minion.ConstraintIndex

/-! Sequential propagation blocks. Only search splits and failed-probe
    refutations introduce recursive proof branches. The checker is related to
    the original tree checker by a fixed theorem, not an assumed conversion. -/
namespace Minion.BlockSearchProof
open DomainProof SearchProof

inductive Step where
  | prune (constraint : Option Nat) (target : Nat) (operation : Operation) (bound : Int)
  | explainedPrune (constraint target : Nat) (operation : Operation) (bound : Int)
      (evidence : InferenceWitness.Evidence)
  | tableReduce (constraint : Nat) (kept : List Nat)
  | filterTables (constraint : Nat)
  deriving Repr

inductive Program where
  | empty
  | conflict (constraint : Nat)
  | explainedConflict (constraint : Nat) (evidence : InferenceWitness.Evidence)
  | solution (assignment : Assignment)
  | block (steps : List Step) (next : Program)
  | split (target : Nat) (value : Int) (left right : Program)
  | refute (target : Nat) (operation : Operation) (bound : Int) (refutation next : Program)
  deriving Repr

def Step.expand (step : Step) (next : Tree) : Tree :=
  match step with
  | .prune c t op b => .prune c t op b next
  | .explainedPrune c t op b e => .explainedPrune c t op b e next
  | .tableReduce c kept => .tableReduce c kept next
  | .filterTables c => .filterTables c next

def expandSteps : List Step → Tree → Tree
  | [], next => next
  | step :: rest, next => step.expand (expandSteps rest next)

def expand : Program → Tree
  | .empty => .empty
  | .conflict c => .conflict c
  | .explainedConflict c e => .explainedConflict c e
  | .solution a => .solution a
  | .block steps next => expandSteps steps (expand next)
  | .split t v left right => .split t v (expand left) (expand right)
  | .refute t op b proof next => .refute t op b (expand proof) (expand next)

private def closeBlock (reversed : List Step) (next : Program) : Program :=
  if reversed.isEmpty then next else .block reversed.reverse next

private def collect (reversed : List Step) : Tree → Program
  | .prune c t op b next => collect (.prune c t op b :: reversed) next
  | .explainedPrune c t op b e next => collect (.explainedPrune c t op b e :: reversed) next
  | .tableReduce c kept next => collect (.tableReduce c kept :: reversed) next
  | .filterTables c next => collect (.filterTables c :: reversed) next
  | .empty => closeBlock reversed .empty
  | .conflict c => closeBlock reversed (.conflict c)
  | .explainedConflict c e => closeBlock reversed (.explainedConflict c e)
  | .solution a => closeBlock reversed (.solution a)
  | .split t v left right => closeBlock reversed (.split t v (collect [] left) (collect [] right))
  | .refute t op b proof next => closeBlock reversed (.refute t op b (collect [] proof) (collect [] next))

/-- Untrusted extraction convenience; acceptance checks the resulting program. -/
def ofTree (tree : Tree) : Program := collect [] tree

private theorem expandSteps_append (xs ys : List Step) (next : Tree) :
    expandSteps (xs ++ ys) next = expandSteps xs (expandSteps ys next) := by
  induction xs with
  | nil => rfl
  | cons first rest ih => simp only [List.cons_append, expandSteps, ih]

private theorem expand_closeBlock (reversed : List Step) (next : Program) :
    expand (closeBlock reversed next) = expandSteps reversed.reverse (expand next) := by
  cases reversed <;> simp [closeBlock, expand, expandSteps]

private theorem expand_collect (tree : Tree) (reversed : List Step) :
    expand (collect reversed tree) = expandSteps reversed.reverse tree := by
  induction tree generalizing reversed with
  | empty | conflict c | explainedConflict c e | solution a =>
    simp only [collect, expand_closeBlock, expand]
  | prune c t op b next ih | explainedPrune c t op b e next ih
  | tableReduce c kept next ih | filterTables c next ih =>
    simp only [collect, ih, List.reverse_cons, expandSteps_append, expandSteps, Step.expand]
  | split t v left right il ir =>
    simp only [collect, expand_closeBlock, expand, il, ir, List.reverse_nil, expandSteps]
  | refute t op b proof next ip ih =>
    simp only [collect, expand_closeBlock, expand, ip, ih, List.reverse_nil, expandSteps]

theorem expand_ofTree (tree : Tree) : expand (ofTree tree) = tree := by
  simp only [ofTree, expand_collect, List.reverse_nil, expandSteps]

/-- Cost hints affect layout selection only; every layout has the same theorem. -/
structure Cost where
  references : Nat := 0
  modelChanges : Nat := 0

def cost : Program → Cost
  | .empty | .solution _ => {}
  | .conflict _ | .explainedConflict .. => ⟨1, 0⟩
  | .block steps next =>
    steps.foldl (fun c step => match step with
      | .filterTables _ | .tableReduce .. => ⟨c.references + 1, c.modelChanges + 1⟩
      | _ => ⟨c.references + 1, c.modelChanges⟩) (cost next)
  | .split _ _ left right | .refute _ _ _ left right =>
    let l := cost left
    let r := cost right
    ⟨l.references + r.references, l.modelChanges + r.modelChanges⟩

private inductive Cache where
  | array (values : Array Constraint)
  | binary (values : ConstraintIndex.Index Constraint)

private def Cache.make (indexed : Bool) (cs : List Constraint) : Cache :=
  if indexed then .binary (ConstraintIndex.build cs) else .array cs.toArray

private def Cache.get : Cache → Nat → Option Constraint
  | .array values, i => values[i]?
  | .binary values, i => ConstraintIndex.get values i

private theorem Cache.get_make (indexed : Bool) (cs : List Constraint) (i : Nat) :
    (Cache.make indexed cs).get i = cs[i]? := by
  cases indexed <;> simp [Cache.make, Cache.get, ConstraintIndex.get_build]

/-- Domains are held in an array, so that reading or narrowing one variable
    does not walk past all the earlier ones, and a flag records whether any is
    empty, so that no step rescans them. The checker is still stated, and
    proved, against the list of domains this array holds. -/
private structure State where
  model : Model
  store : Array (List Int)
  hasEmpty : Bool
  cache : Cache

private abbrev State.domains (s : State) : List (List Int) := s.store.toList

private def State.lookup (s : State) : Domains := fun i => s.store[i]?.getD [0]

private theorem lookup_eq (s : State) : s.lookup = ofLists s.domains := by
  funext i
  simp [State.lookup, ofLists, State.domains]

private structure Valid (fast within : Bool) (s : State) : Prop where
  unique : fast = true → FastSearchProof.UniqueDomains s.domains
  within : within = true → FastSearchProof.WithinModel s.model s.domains
  cache : ∀ i, s.cache.get i = s.model.constraints[i]?
  empty : s.hasEmpty = hasEmptyDomain s.domains

private def restrictWith (fast : Bool) (d : List (List Int)) (t : Nat)
    (op : Operation) (b : Int) : List (List Int) :=
  if fast then FastSearchProof.restrictFast d t op b else restrict d t op b

private theorem restrictWith_eq (fast : Bool) (d : List (List Int)) (t : Nat)
    (op : Operation) (b : Int) (h : fast = true → FastSearchProof.UniqueDomains d) :
    restrictWith fast d t op b = restrict d t op b := by
  cases fast with
  | false => rfl
  | true => exact FastSearchProof.restrictFast_eq d t op b (h rfl)

private def update (fast : Bool) (op : Operation) (b : Int) : List Int → List Int :=
  if fast then FastSearchProof.updateFast op b else List.filter (op.holds b)

private theorem update_nil (fast : Bool) (op : Operation) (b : Int) : update fast op b [] = [] := by
  cases fast <;> cases op <;> simp [update, FastSearchProof.updateFast]

private theorem hasEmpty_modify (d : List (List Int)) (t : Nat) (f : List Int → List Int)
    (hf : f [] = []) :
    hasEmptyDomain (d.modify t f) = (hasEmptyDomain d || (d[t]?).any (fun xs => (f xs).isEmpty)) := by
  induction d generalizing t with
  | nil => simp [hasEmptyDomain]
  | cons xs rest ih =>
    cases t with
    | zero =>
      cases xs with
      | nil => simp [hasEmptyDomain, hf]
      | cons x xs' => simp [hasEmptyDomain, Bool.or_comm]
    | succ t =>
      have hr := ih t
      simp only [hasEmptyDomain] at hr ⊢
      simp only [List.modify_succ_cons, List.any_cons, hr, List.getElem?_cons_succ, Bool.or_assoc]

private def State.restrict (s : State) (fast : Bool) (t : Nat) (op : Operation) (b : Int) : State :=
  let ⟨model, store, hasEmpty, cache⟩ := s
  let emptied := (store[t]?).any fun xs => (update fast op b xs).isEmpty
  ⟨model, store.modify t (update fast op b), hasEmpty || emptied, cache⟩

private theorem restrict_domains (s : State) (fast : Bool) (t : Nat) (op : Operation) (b : Int) :
    (s.restrict fast t op b).domains = restrictWith fast s.domains t op b := by
  cases fast <;> simp [State.restrict, State.domains, restrictWith, update, restrict,
    FastSearchProof.restrictFast]

private theorem restrict_model (s : State) (fast : Bool) (t : Nat) (op : Operation) (b : Int) :
    (s.restrict fast t op b).model = s.model ∧ (s.restrict fast t op b).cache = s.cache := ⟨rfl, rfl⟩

private theorem valid_restrict (s : State) (fast within : Bool) (t : Nat)
    (op : Operation) (b : Int) (h : Valid fast within s) :
    Valid fast within (s.restrict fast t op b) := by
  have hd : (s.restrict fast t op b).domains = restrict s.domains t op b := by
    rw [restrict_domains, restrictWith_eq fast s.domains t op b h.unique]
  refine ⟨?_, ?_, h.cache, ?_⟩
  · intro hf
    rw [hd]
    exact FastSearchProof.unique_restrict s.domains t op b (h.unique hf)
  · intro hw
    rw [hd]
    exact FastSearchProof.within_restrict s.model s.domains t op b (h.within hw)
  · have hm : (s.restrict fast t op b).domains = s.domains.modify t (update fast op b) := by
      simp [State.restrict, State.domains]
    rw [hm, hasEmpty_modify _ _ _ (update_nil fast op b), ← h.empty]
    simp [State.restrict, State.domains]

private def State.withModel (s : State) (indexed : Bool) (m : Model) : State :=
  ⟨m, s.store, s.hasEmpty, Cache.make indexed m.constraints⟩

private theorem valid_withModel (s : State) (fast within indexed : Bool) (m : Model)
    (hv : m.variables = s.model.variables) (h : Valid fast within s) :
    Valid fast within (s.withModel indexed m) := by
  refine ⟨h.unique, ?_, Cache.get_make indexed m.constraints, h.empty⟩
  simpa only [FastSearchProof.WithinModel, State.withModel, hv] using h.within

private def prune (s : State) (c : Option Nat) (t : Nat) (op : Operation) (b : Int) : Bool :=
  checkNoop s.lookup t op b || s.hasEmpty ||
    match c.bind s.cache.get with
    | none => false
    | some constraint => LogicProof.rule.prune s.lookup constraint t op b

private def step (indexed fast : Bool) (s : State) : Step → Bool × State
  | .prune c t op b => (prune s c t op b, s.restrict fast t op b)
  | .explainedPrune c t op b e =>
    ((s.cache.get c).any fun constraint =>
      (InferenceWitness.rule e (some t)).prune s.lookup constraint t op b,
      s.restrict fast t op b)
  | .tableReduce c kept =>
    ((s.cache.get c).any (fun constraint => TableProjection.checkConstraint s.lookup constraint kept),
      s.withModel indexed (TableProjection.reduceModel s.model c kept))
  | .filterTables c =>
    (true, s.withModel indexed (TableProjection.filterModel s.model s.domains c))

private def run (indexed fast : Bool) (s : State) : List Step → Bool × State
  | [] => (true, s)
  | first :: rest =>
    let (accepted, next) := step indexed fast s first
    if accepted then run indexed fast next rest else (false, next)

private theorem step_valid (indexed fast within : Bool) (s : State) (first : Step)
    (h : Valid fast within s) : Valid fast within (step indexed fast s first).2 := by
  cases first with
  | prune c t op b | explainedPrune c t op b e => exact valid_restrict s fast within t op b h
  | tableReduce c kept => exact valid_withModel s fast within indexed _ rfl h
  | filterTables c => exact valid_withModel s fast within indexed _ rfl h

private theorem step_check (indexed fast within : Bool) (s : State) (first : Step)
    (next : Tree) (h : Valid fast within s) :
    ((step indexed fast s first).1 &&
      SearchProof.check (step indexed fast s first).2.model (step indexed fast s first).2.domains next) =
    SearchProof.check s.model s.domains (first.expand next) := by
  have he : s.cache.get = fun i => s.model.constraints[i]? := funext h.cache
  have hr (t : Nat) (op : Operation) (b : Int) :
      (s.restrict fast t op b).domains = restrict s.domains t op b := by
    rw [restrict_domains, restrictWith_eq fast s.domains t op b h.unique]
  cases first <;>
    simp only [step, Step.expand, SearchProof.check, prune, pruneCheck, hr, (restrict_model s fast _ _ _).1,
      State.withModel, TableProjection.checkModel, lookup_eq, h.empty, he, Bool.true_and]
  all_goals rfl

private theorem run_valid (indexed fast within : Bool) (s : State) (steps : List Step)
    (h : Valid fast within s) : Valid fast within (run indexed fast s steps).2 := by
  induction steps generalizing s with
  | nil => exact h
  | cons first rest ih =>
    simp only [run]
    split
    · exact ih _ (step_valid indexed fast within s first h)
    · exact step_valid indexed fast within s first h

private theorem run_check (indexed fast within : Bool) (s : State) (steps : List Step)
    (next : Tree) (h : Valid fast within s) :
    ((run indexed fast s steps).1 &&
      SearchProof.check (run indexed fast s steps).2.model (run indexed fast s steps).2.domains next) =
    SearchProof.check s.model s.domains (expandSteps steps next) := by
  induction steps generalizing s with
  | nil => simp [run, expandSteps]
  | cons first rest ih =>
    have hc := step_check indexed fast within s first (expandSteps rest next) h
    have hr := ih (step indexed fast s first).2 (step_valid indexed fast within s first h)
    simp only [run, expandSteps]
    split <;> rename_i ha
    · rw [hr]
      simpa only [ha, Bool.true_and] using hc
    · simpa only [ha, Bool.false_and] using hc

private def collectReverse : Program → List Assignment → List Assignment
  | .empty, acc | .conflict _, acc | .explainedConflict .., acc => acc
  | .solution a, acc => a :: acc
  | .block _ next, acc | .refute _ _ _ _ next, acc => collectReverse next acc
  | .split _ _ left right, acc => collectReverse right (collectReverse left acc)

private theorem solutions_expandSteps (steps : List Step) (next : Tree) :
    SearchProof.solutions (expandSteps steps next) = SearchProof.solutions next := by
  induction steps with
  | nil => rfl
  | cons first rest ih => cases first <;> simpa only [expandSteps, Step.expand, SearchProof.solutions] using ih

private theorem collectReverse_eq (p : Program) (acc : List Assignment) :
    collectReverse p acc = (SearchProof.solutions (expand p)).reverse ++ acc := by
  induction p generalizing acc with
  | empty | conflict c | explainedConflict c e | solution a => simp [collectReverse, expand, SearchProof.solutions]
  | block steps next ih => simp only [collectReverse, expand, solutions_expandSteps, ih]
  | refute t op b proof next _ ih => simp only [collectReverse, expand, SearchProof.solutions, ih]
  | split t v left right il ir =>
    simp only [collectReverse, expand, SearchProof.solutions, ir, il, List.reverse_append, List.append_assoc]

def solutionsReverse (p : Program) : List Assignment := collectReverse p []
def solutions (p : Program) : List Assignment := (solutionsReverse p).reverse

theorem solutions_eq (p : Program) : solutions p = SearchProof.solutions (expand p) := by
  simp only [solutions, solutionsReverse, collectReverse_eq, List.append_nil, List.reverse_reverse]

private def cached (indexed fast within : Bool) (s : State) : Program → Bool
  | .empty => s.hasEmpty
  | .conflict c => s.hasEmpty || (s.cache.get c).any (LogicProof.checkConflict s.lookup)
  | .explainedConflict c e => (s.cache.get c).any ((InferenceWitness.rule e).conflict s.lookup)
  | .solution a => FastSearchProof.leaf s.model s.domains within a
  | .block steps next =>
    let (accepted, nextState) := run indexed fast s steps
    accepted && cached indexed fast within nextState next
  | .split t v left right =>
    decide (t < s.store.size) &&
      (cached indexed fast within (s.restrict fast t .assign v) left &&
       cached indexed fast within (s.restrict fast t .remove v) right)
  | .refute t op b proof next =>
    let complement := SearchProof.complement op b
    cached indexed fast within (s.restrict fast t complement.1 complement.2) proof &&
      (solutionsReverse proof).isEmpty && cached indexed fast within (s.restrict fast t op b) next

private theorem reverse_empty (p : Program) :
    (solutionsReverse p).isEmpty = (SearchProof.solutions (expand p)).isEmpty := by
  simp [solutionsReverse, collectReverse_eq]

private theorem cached_eq (indexed fast within : Bool) (s : State) (p : Program)
    (h : Valid fast within s) : cached indexed fast within s p = SearchProof.check s.model s.domains (expand p) := by
  induction p generalizing s with
  | empty => exact h.empty
  | conflict c =>
    simp only [cached, expand, SearchProof.check, h.cache, h.empty, lookup_eq]
    cases s.model.constraints[c]? <;> rfl
  | explainedConflict c e => simp only [cached, expand, SearchProof.check, h.cache, lookup_eq]
  | solution a => exact FastSearchProof.leaf_eq s.model s.domains within a h.within
  | block steps next ih =>
    simp only [cached, expand]
    rw [ih _ (run_valid indexed fast within s steps h)]
    exact run_check indexed fast within s steps (expand next) h
  | split t v left right il ir =>
    simp only [cached, expand, SearchProof.check]
    rw [il _ (valid_restrict s fast within t .assign v h),
      ir _ (valid_restrict s fast within t .remove v h)]
    simp only [restrict_domains, restrictWith_eq fast s.domains _ _ _ h.unique,
      (restrict_model s fast _ _ _).1, State.domains, Array.length_toList]
  | refute t op b proof next ip ih =>
    simp only [cached, expand, SearchProof.check, reverse_empty]
    rw [ip _ (valid_restrict s fast within t _ _ h), ih _ (valid_restrict s fast within t op b h)]
    simp only [restrict_domains, restrictWith_eq fast s.domains _ _ _ h.unique,
      (restrict_model s fast _ _ _).1, ← reject_eq_complement]

-- Force the Boolean dispatch once. Passing its unevaluated expression to
-- every recursive call is cheap after compilation, but can repeat kernel work.
private def start (indexed fast within : Bool) (s : State) (p : Program) : Bool :=
  if fast then
    if within then cached indexed true true s p else cached indexed true false s p
  else
    if within then cached indexed false true s p else cached indexed false false s p

private theorem start_eq (indexed fast within : Bool) (s : State) (p : Program)
    (h : Valid fast within s) : start indexed fast within s p = SearchProof.check s.model s.domains (expand p) := by
  cases fast <;> cases within <;> exact cached_eq indexed _ _ s p h

/-- `indexed=false` isolates block representation from binary-index lookup. -/
def check (m : Model) (d : List (List Int)) (p : Program) (indexed : Bool := true) : Bool :=
  start indexed (FastSearchProof.eligible m d) (d == m.variables.map Variable.domain)
    ⟨m, d.toArray, hasEmptyDomain d, Cache.make indexed m.constraints⟩ p

theorem check_eq (m : Model) (d : List (List Int)) (p : Program) (indexed : Bool := true) :
    check m d p indexed = SearchProof.check m d (expand p) := by
  have hs := start_eq indexed (FastSearchProof.eligible m d) (d == m.variables.map Variable.domain)
    ⟨m, d.toArray, hasEmptyDomain d, Cache.make indexed m.constraints⟩ p
  simp only [State.domains] at hs
  apply hs
  refine ⟨FastSearchProof.eligible_unique m d, ?_, Cache.get_make indexed m.constraints, rfl⟩
  intro he
  have hd : d = m.variables.map Variable.domain := by simpa using he
  rw [hd]
  exact fun _ ha => ha

/-- Kernel exports supply ordinary index data and establish its correspondence
    once, instead of reducing the index-building expression at every use. -/
def checkWithIndex (m : Model) (d : List (List Int)) (p : Program)
    (index : ConstraintIndex.Index Constraint) : Bool :=
  start true (FastSearchProof.eligible m d) (d == m.variables.map Variable.domain)
    ⟨m, d.toArray, hasEmptyDomain d, .binary index⟩ p

theorem checkWithIndex_eq (m : Model) (d : List (List Int)) (p : Program)
    (index : ConstraintIndex.Index Constraint) (hi : index = ConstraintIndex.build m.constraints) :
    checkWithIndex m d p index = SearchProof.check m d (expand p) := by
  subst index
  exact check_eq m d p true

theorem check_complete (m : Model) (p : Program)
    (h : check m (m.variables.map Variable.domain) p = true) :
    ∀ a, a ∈ solutions p ↔ m.IsSolution a := by
  rw [solutions_eq]
  exact SearchProof.check_complete m (expand p) (by simpa only [check_eq] using h)

theorem check_nodup (m : Model) (d : List (List Int)) (p : Program)
    (h : check m d p = true) : (solutions p).Nodup := by
  rw [solutions_eq]
  exact SearchProof.check_nodup m d (expand p) (by simpa only [check_eq] using h)

/-- Fixed transport helpers keep expansion of the old tree out of each
    instance's proof term, for both array and binary-index diagnostics. -/
theorem check_complete_using (m : Model) (p : Program) (indexed : Bool)
    (h : check m (m.variables.map Variable.domain) p indexed = true) :
    ∀ a, a ∈ solutions p ↔ m.IsSolution a := by
  rw [solutions_eq]
  exact SearchProof.check_complete m (expand p) (by simpa only [check_eq] using h)

theorem check_nodup_using (m : Model) (d : List (List Int)) (p : Program) (indexed : Bool)
    (h : check m d p indexed = true) : (solutions p).Nodup := by
  rw [solutions_eq]
  exact SearchProof.check_nodup m d (expand p) (by simpa only [check_eq] using h)

theorem checkWithIndex_complete (m : Model) (p : Program)
    (index : ConstraintIndex.Index Constraint) (hi : index = ConstraintIndex.build m.constraints)
    (h : checkWithIndex m (m.variables.map Variable.domain) p index = true) :
    ∀ a, a ∈ solutions p ↔ m.IsSolution a := by
  rw [solutions_eq]
  exact SearchProof.check_complete m (expand p) (by simpa only [checkWithIndex_eq m _ p index hi] using h)

theorem checkWithIndex_nodup (m : Model) (d : List (List Int)) (p : Program)
    (index : ConstraintIndex.Index Constraint) (hi : index = ConstraintIndex.build m.constraints)
    (h : checkWithIndex m d p index = true) : (solutions p).Nodup := by
  rw [solutions_eq]
  exact SearchProof.check_nodup m d (expand p) (by simpa only [checkWithIndex_eq m d p index hi] using h)

def verify (m : Model) (p : Program) (reported : List Assignment) (indexed : Bool := true) : Bool :=
  check m (m.variables.map Variable.domain) p indexed &&
    (solutionsReverse p == reported || sameAssignments (solutions p) reported)

theorem verify_eq (m : Model) (p : Program) (reported : List Assignment) (indexed : Bool := true) :
    verify m p reported indexed = SearchProof.verify m (expand p) reported := by
  simp only [verify, check_eq, SearchProof.verify, ← solutions_eq]
  have he : (solutionsReverse p == reported || sameAssignments (solutions p) reported) =
      sameAssignments (solutions p) reported := by
    cases hr : solutionsReverse p == reported with
    | false => simp
    | true =>
      have hrev : (solutions p).reverse = reported := by
        simpa only [solutions, List.reverse_reverse, beq_iff_eq] using hr
      have hs : solutions p = reported.reverse := by
        simpa only [List.reverse_reverse] using congrArg List.reverse hrev
      have hp : sameAssignments (solutions p) reported = true := by simp [sameAssignments, hs]
      simp [hp]
  rw [he]

end Minion.BlockSearchProof
