import Minion.TableProof

/-! Branch-local table reductions. Every discarded row must be incompatible
    with the current domains. Kept rows may be an overapproximation. The same
    equivalence covers both positive and negative tables. -/
namespace Minion.TableProjection
open DomainProof

-- Linear construction/lookup in native checking, and reducible in exported
-- kernel proofs (unlike an opaque hash-based membership implementation).
def mask (length : Nat) (kept : List Nat) : Array Bool :=
  kept.foldl (fun selected i => selected.setIfInBounds i true) (Array.replicate length false)

def selectRows (rows : List (List Int)) (kept : List Nat) : List (List Int) :=
  let selected := mask rows.length kept
  (rows.zipIdx.filter (fun p => selected[p.2]?.getD false)).map Prod.fst

def check (d : Domains) (xs : List Term) (rows : List (List Int)) (kept : List Nat) : Bool :=
  let selected := mask rows.length kept
  rows.zipIdx.all fun p => selected[p.2]?.getD false || !TableProof.rowPossible d xs p.1

theorem membership (d : Domains) (xs : List Term) (rows : List (List Int)) (kept : List Nat)
    (a : Assignment) (hd : Compatible d a) (hc : check d xs rows kept = true) :
    (xs.map (Term.eval a) ∈ selectRows rows kept) ↔ xs.map (Term.eval a) ∈ rows := by
  constructor
  · intro hs
    obtain ⟨p, hp, he⟩ := List.mem_map.mp hs
    have hz := (List.mem_filter.mp hp).1
    have hm := List.mem_map.mpr ⟨p, hz, he⟩
    simpa only [List.zipIdx_map_fst] using hm
  · intro hs
    have hm : xs.map (Term.eval a) ∈ rows.zipIdx.map Prod.fst := by
      simpa only [List.zipIdx_map_fst] using hs
    obtain ⟨p, hp, he⟩ := List.mem_map.mp hm
    have hpossible : TableProof.rowPossible d xs p.1 = true := by
      rw [he]
      exact TableProof.rowPossible_sound d xs a hd
    have hkeep := List.all_eq_true.mp hc p hp
    simp only [hpossible, Bool.not_true, Bool.or_false] at hkeep
    exact List.mem_map.mpr ⟨p, List.mem_filter.mpr ⟨hp, hkeep⟩, he⟩

def reduce (c : Constraint) (kept : List Nat) : Constraint :=
  match c with
  | .table xs rows positive => .table xs (selectRows rows kept) positive
  | c => c

def checkConstraint (d : Domains) (c : Constraint) (kept : List Nat) : Bool :=
  match c with
  | .table xs rows _ => check d xs rows kept
  | _ => false

theorem reduce_eval (d : Domains) (c : Constraint) (kept : List Nat)
    (a : Assignment) (hd : Compatible d a) (hc : checkConstraint d c kept = true) :
    (reduce c kept).eval a = c.eval a := by
  cases c <;> try { simp [checkConstraint] at hc }
  rename_i xs rows positive
  have he : (selectRows rows kept).contains (xs.map (Term.eval a)) =
      rows.contains (xs.map (Term.eval a)) := by
    apply Bool.eq_iff_iff.mpr
    simpa using membership d xs rows kept a hd hc
  simp only [reduce, Constraint.eval, he]

def checkModel (m : Model) (d : List (List Int)) (index : Nat) (kept : List Nat) : Bool :=
  (m.constraints[index]?).any (fun c => checkConstraint (ofLists d) c kept)

def reduceModel (m : Model) (index : Nat) (kept : List Nat) : Model :=
  { m with constraints := m.constraints.modify index (fun c => reduce c kept) }

private theorem all_modify (cs : List Constraint) (index : Nat) (f : Constraint → Constraint)
    (a : Assignment) (he : ∀ c, cs[index]? = some c → (f c).eval a = c.eval a) :
    (cs.modify index f).all (Constraint.eval a) = cs.all (Constraint.eval a) := by
  induction cs generalizing index with
  | nil => simp
  | cons c cs ih =>
    cases index with
    | zero => simp [he c (by simp)]
    | succ index =>
      have hr := ih index (fun c hc => he c (by simpa using hc))
      simpa using congrArg (fun b => c.eval a && b) hr

theorem solution_iff (m : Model) (d : List (List Int)) (index : Nat) (kept : List Nat)
    (a : Assignment) (hd : InDomains d a) (hc : checkModel m d index kept = true) :
    (reduceModel m index kept).IsSolution a ↔ m.IsSolution a := by
  have he : (m.constraints.modify index (fun c => reduce c kept)).all (Constraint.eval a) =
      m.constraints.all (Constraint.eval a) := by
    apply all_modify
    intro c hi
    have hp : checkConstraint (ofLists d) c kept = true := by
      simpa only [checkModel, hi, Option.any_some] using hc
    exact reduce_eval _ c kept a (inDomains_compatible d a hd) hp
  simp only [Model.IsSolution, reduceModel, he]

/-! A computed reduction also works under arbitrary logical wrappers. Unlike
    supplied row IDs, it carries no selection payload: the kernel computes the
    compatible rows and applies this fixed equivalence at either polarity. -/
def filterTables (d : Domains) : Nat → Constraint → Constraint
  | 0, c => c
  | _ + 1, .table xs rows positive =>
      .table xs (rows.filter (TableProof.rowPossible d xs)) positive
  | fuel + 1, .reify c guard => .reify (filterTables d fuel c) guard
  | fuel + 1, .imply c guard => .imply (filterTables d fuel c) guard
  | fuel + 1, .conjunction cs => .conjunction (cs.map (filterTables d fuel))
  | fuel + 1, .disjunction cs => .disjunction (cs.map (filterTables d fuel))
  | _ + 1, c => c

private theorem eval_all_attach (cs : List Constraint) (a : Assignment) :
    cs.attach.all (fun c => c.val.eval a) = cs.all (Constraint.eval a) := by
  apply Bool.eq_iff_iff.mpr
  simp only [List.all_eq_true]
  constructor
  · intro h c hc
    exact h ⟨c, hc⟩ (List.mem_attach _ _)
  · intro h c _
    exact h c.val c.property

private theorem eval_any_attach (cs : List Constraint) (a : Assignment) :
    cs.attach.any (fun c => c.val.eval a) = cs.any (Constraint.eval a) := by
  apply Bool.eq_iff_iff.mpr
  simp only [List.any_eq_true]
  constructor
  · rintro ⟨c, _, hc⟩
    exact ⟨c.val, c.property, hc⟩
  · rintro ⟨c, hc, he⟩
    exact ⟨⟨c, hc⟩, List.mem_attach _ _, he⟩

theorem filterTables_eval (d : Domains) (fuel : Nat) (c : Constraint)
    (a : Assignment) (hd : Compatible d a) :
    (filterTables d fuel c).eval a = c.eval a := by
  induction fuel generalizing c with
  | zero => rfl
  | succ fuel ih =>
    cases c <;> try rfl
    case table xs rows positive =>
      have hmem : (xs.map (Term.eval a) ∈ rows.filter (TableProof.rowPossible d xs)) ↔
          xs.map (Term.eval a) ∈ rows := by
        simp [TableProof.rowPossible_sound d xs a hd]
      have he : (rows.filter (TableProof.rowPossible d xs)).contains (xs.map (Term.eval a)) =
          rows.contains (xs.map (Term.eval a)) := by
        apply Bool.eq_iff_iff.mpr
        simpa using hmem
      simp only [filterTables, Constraint.eval, he]
    case reify child guard => simp only [filterTables, Constraint.eval, ih child]
    case imply child guard => simp only [filterTables, Constraint.eval, ih child]
    case conjunction cs =>
      simp only [filterTables, Constraint.eval, eval_all_attach, List.all_map, Function.comp_def]
      congr 1
      funext child
      exact ih child
    case disjunction cs =>
      simp only [filterTables, Constraint.eval, eval_any_attach, List.any_map, Function.comp_def]
      congr 1
      funext child
      exact ih child

def filterModel (m : Model) (d : List (List Int)) (index : Nat) : Model :=
  { m with constraints := m.constraints.modify index (filterTables (ofLists d) 256) }

theorem filterModel_solution_iff (m : Model) (d : List (List Int)) (index : Nat)
    (a : Assignment) (hd : InDomains d a) :
    (filterModel m d index).IsSolution a ↔ m.IsSolution a := by
  have he : (m.constraints.modify index (filterTables (ofLists d) 256)).all (Constraint.eval a) =
      m.constraints.all (Constraint.eval a) := by
    apply all_modify
    intro c _
    exact filterTables_eval _ _ c a (inDomains_compatible d a hd)
  simp only [Model.IsSolution, filterModel, he]

end Minion.TableProjection
