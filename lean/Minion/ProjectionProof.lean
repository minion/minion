import Minion.PartialProof

/-! Auxiliary variables. Once the other variables are fixed, the search finds
    one assignment to the auxiliary ones and does not look for more. Wherever
    it skips the other side of a decision for that reason, the reader adds a
    constraint to the model which the skipped side violates, and closes that
    side as a conflict with it. The constraint is a disjunction ending in
    "some non-auxiliary variable differs from the solution just found", so the
    conflict holds only where every non-auxiliary variable is already fixed
    to that solution's value. The existing search theorem then gives the
    reported solutions exactly as the solutions of the extended model, and
    from that: every solution of the original model agrees with a reported
    one on the non-auxiliary variables. -/
namespace Minion.ProjectionProof
open SearchProof

def value (a : Assignment) (index : Nat) : Int := a[index]?.getD 0

def project (kept : List Nat) (a : Assignment) : List Int := kept.map (value a)

/-- A skipped side: the conditions which place an assignment outside it, and
    the values the kept variables have throughout it. -/
structure Skip where
  outside : List Constraint
  values : List Int

def differs (kept : List Nat) (values : List Int) : List Constraint :=
  (kept.zip values).map fun p => Constraint.compare .ne (.var p.1) (.constant p.2)

def Skip.constraint (kept : List Nat) (s : Skip) : Constraint :=
  .disjunction (s.outside ++ differs kept s.values)

def check (m : Model) (kept : List Nat) (skips : List Skip) (p : BlockSearchProof.Program)
    (reported : List Assignment) (indexed : Bool := false) : Bool :=
  PartialProof.check m (skips.map (Skip.constraint kept)) p reported indexed &&
    skips.all fun s => s.values.length == kept.length &&
      reported.any fun r => project kept r == s.values

private theorem eval_disjunction (cs : List Constraint) (a : Assignment) :
    (Constraint.disjunction cs).eval a = cs.any (fun c => c.eval a) := by
  simp only [Constraint.eval, List.any_eq, List.mem_attach, true_and, Subtype.exists, exists_prop]

private theorem project_eq (a : Assignment) (kept : List Nat) (values : List Int)
    (hl : values.length = kept.length)
    (h : ∀ c ∈ differs kept values, c.eval a = false) : project kept a = values := by
  induction kept generalizing values with
  | nil => cases values <;> simp_all [project]
  | cons k ks ih =>
    cases values with
    | nil => simp at hl
    | cons v vs =>
      have hk := h (.compare .ne (.var k) (.constant v)) (by simp [differs])
      have hrest := ih vs (by simpa using hl) (fun c hc => h c (by
        simp only [differs, List.zip_cons_cons, List.map_cons, List.mem_cons]
        exact Or.inr hc))
      simp only [Constraint.eval, Comparison.eval, Term.eval, bne_eq_false_iff_eq] at hk
      simp only [project, List.map_cons, List.cons.injEq] at hrest ⊢
      exact ⟨hk, hrest⟩

theorem check_correct (m : Model) (kept : List Nat) (skips : List Skip)
    (p : BlockSearchProof.Program) (reported : List Assignment) (indexed : Bool)
    (h : check m kept skips p reported indexed = true) :
    (∀ r ∈ reported, m.IsSolution r) ∧
    ∀ a, m.IsSolution a → ∃ r ∈ reported, project kept r = project kept a := by
  simp only [check, Bool.and_eq_true, List.all_eq_true, List.any_eq_true, beq_iff_eq] at h
  have hc := PartialProof.check_correct m _ p reported indexed h.1
  refine ⟨fun r hr => ((hc r).mp hr).1, ?_⟩
  intro a ha
  by_cases hall : ∀ c ∈ skips.map (Skip.constraint kept), c.eval a = true
  · exact ⟨a, (hc a).mpr ⟨ha, hall⟩, rfl⟩
  · obtain ⟨c, hc⟩ := Classical.not_forall.mp hall
    obtain ⟨hcm, hne⟩ := Classical.not_imp.mp hc
    obtain ⟨s, hs, rfl⟩ := List.mem_map.mp hcm
    have hfalse : (Skip.constraint kept s).eval a = false := by simpa using hne
    rw [Skip.constraint, eval_disjunction] at hfalse
    have hparts : ∀ c ∈ differs kept s.values, c.eval a = false := by
      have hsplit := hfalse
      simp only [List.any_append, Bool.or_eq_false_iff, List.any_eq_false, Bool.not_eq_true] at hsplit
      exact hsplit.2
    obtain ⟨hl, r, hr, hv⟩ := h.2 s hs
    refine ⟨r, hr, ?_⟩
    rw [hv]
    exact (project_eq a kept s.values hl hparts).symm

end Minion.ProjectionProof
