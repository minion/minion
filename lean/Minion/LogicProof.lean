import Minion.LeafProof

/-! Compositional feasibility rules for reification, implication and Boolean
    combinations. Both child polarities are checked from the original relation;
    a runtime child call is never itself an activation assumption. -/
namespace Minion.LogicProof
open DomainProof

def possible (d : Domains) (expected : Bool) (c : Constraint) : Bool :=
  match c with
  | .reify child guard =>
      (values d guard).any fun g =>
        if g == 0 then possible (bindTerm d guard g) (!expected) child
        else if g == 1 then possible (bindTerm d guard g) expected child
        else !expected
  | .imply child guard =>
      (values d guard).any fun g =>
        if g == 0 then expected
        else if g == 1 then possible (bindTerm d guard g) expected child
        else !expected
  | .conjunction children =>
      if expected then children.attach.all (fun child => possible d true child.val)
      else children.attach.any (fun child => possible d false child.val)
  | .disjunction children =>
      if expected then children.attach.any (fun child => possible d true child.val)
      else children.attach.all (fun child => possible d false child.val)
  | c => LeafProof.possibleValue d expected c
termination_by sizeOf c
decreasing_by
  all_goals simp only [Constraint.reify.sizeOf_spec, Constraint.imply.sizeOf_spec,
    Constraint.conjunction.sizeOf_spec, Constraint.disjunction.sizeOf_spec]
  all_goals first | omega | exact Nat.lt_trans (List.sizeOf_lt_of_mem child.property) (by omega)

theorem possible_sound (d : Domains) (expected : Bool) (c : Constraint) (a : Assignment)
    (hd : Compatible d a) (hs : c.eval a = expected) : possible d expected c = true := by
  cases c with
  | reify child guard =>
    simp only [possible]
    apply List.any_eq_true.mpr
    refine ⟨guard.eval a, values_sound d a hd guard, ?_⟩
    by_cases hz : guard.eval a = 0
    · simp only [hz, beq_self_eq_true, ↓reduceIte]
      apply possible_sound _ (!expected) child a (by simpa only [hz] using compatible_bindTerm d guard a hd)
      cases expected <;> cases hc : child.eval a <;> simp_all [Constraint.eval]
    · by_cases ho : guard.eval a = 1
      · simp only [ho, ↓reduceIte, beq_self_eq_true]
        apply possible_sound _ expected child a (by simpa only [ho] using compatible_bindTerm d guard a hd)
        simpa [Constraint.eval, ho] using hs
      · simp only [beq_iff_eq, hz, ho, ↓reduceIte]
        cases expected <;> simp_all [Constraint.eval]
  | imply child guard =>
    simp only [possible]
    apply List.any_eq_true.mpr
    refine ⟨guard.eval a, values_sound d a hd guard, ?_⟩
    by_cases hz : guard.eval a = 0
    · simp only [hz, beq_self_eq_true, ↓reduceIte]
      simpa [Constraint.eval, hz] using hs.symm
    · by_cases ho : guard.eval a = 1
      · simp only [ho, ↓reduceIte, beq_self_eq_true]
        apply possible_sound _ expected child a (by simpa only [ho] using compatible_bindTerm d guard a hd)
        simpa [Constraint.eval, ho] using hs
      · simp only [beq_iff_eq, hz, ho, ↓reduceIte]
        cases expected <;> simp_all [Constraint.eval]
  | conjunction children =>
    simp only [Constraint.eval] at hs
    cases expected with
    | true =>
      simp only [possible, ↓reduceIte]
      apply List.all_eq_true.mpr
      intro child hm
      exact possible_sound d true child.val a hd (List.all_eq_true.mp hs child hm)
    | false =>
      simp only [possible, Bool.false_eq_true, ↓reduceIte]
      obtain ⟨child, hm, hc⟩ := List.all_eq_false.mp hs
      exact List.any_eq_true.mpr ⟨child, hm,
        possible_sound d false child.val a hd (Bool.eq_false_iff.mpr hc)⟩
  | disjunction children =>
    simp only [Constraint.eval] at hs
    cases expected with
    | true =>
      simp only [possible, ↓reduceIte]
      obtain ⟨child, hm, hc⟩ := List.any_eq_true.mp hs
      exact List.any_eq_true.mpr ⟨child, hm, possible_sound d true child.val a hd hc⟩
    | false =>
      simp only [possible, Bool.false_eq_true, ↓reduceIte]
      apply List.all_eq_true.mpr
      intro child hm
      exact possible_sound d false child.val a hd
        (Bool.eq_false_iff.mpr (List.any_eq_false.mp hs child hm))
  | _ =>
    simp only [possible]
    exact LeafProof.possibleValue_sound d expected _ a hd hs
termination_by sizeOf c
decreasing_by
  all_goals simp_all only [Constraint.reify.sizeOf_spec, Constraint.imply.sizeOf_spec,
    Constraint.conjunction.sizeOf_spec, Constraint.disjunction.sizeOf_spec]
  all_goals first | omega | exact Nat.lt_trans (List.sizeOf_lt_of_mem child.property) (by omega)

/-- A source path selects a useful necessary condition, never an activation
    assumption. Conjunction truth and disjunction falsity imply each child's
    corresponding polarity; reification still checks every allowed guard case.
    Other polarities use the ordinary whole-relation check. -/
def possibleAt (d : Domains) (expected : Bool) (c : Constraint) (path : List Nat) : Bool :=
  match path with
  | [] => possible d expected c
  | index :: rest =>
    match c with
    | .conjunction children =>
      if expected then (children[index]?).all (fun child => possibleAt d true child rest)
      else possible d expected c
    | .disjunction children =>
      if !expected then (children[index]?).all (fun child => possibleAt d false child rest)
      else possible d expected c
    | .reify child guard =>
      if index != 0 then true else
        (values d guard).any fun g =>
          if g == 0 then possibleAt (bindTerm d guard g) (!expected) child rest
          else if g == 1 then possibleAt (bindTerm d guard g) expected child rest
          else !expected
    | .imply child guard =>
      if index != 0 then true else
        (values d guard).any fun g =>
          if g == 0 then expected
          else if g == 1 then possibleAt (bindTerm d guard g) expected child rest
          else !expected
    | _ => true

theorem possibleAt_sound (d : Domains) (expected : Bool) (c : Constraint) (path : List Nat)
    (a : Assignment) (hd : Compatible d a) (hs : c.eval a = expected) :
    possibleAt d expected c path = true := by
  induction path generalizing d expected c with
  | nil => exact possible_sound d expected c a hd hs
  | cons index rest ih =>
    cases c <;> try rfl
    case conjunction children =>
      cases expected with
      | false => exact possible_sound d false _ a hd hs
      | true =>
        cases hc : children[index]? with
        | none => simp [possibleAt, hc]
        | some child =>
          have hsem := hs
          simp only [Constraint.eval] at hsem
          have he : child.eval a = true := List.all_eq_true.mp hsem
            ⟨child, List.mem_of_getElem? hc⟩ (List.mem_attach _ _)
          simpa [possibleAt, hc] using ih d true child hd he
    case disjunction children =>
      cases expected with
      | true => exact possible_sound d true _ a hd hs
      | false =>
        cases hc : children[index]? with
        | none => simp [possibleAt, hc]
        | some child =>
          have hsem := hs
          simp only [Constraint.eval] at hsem
          have he : child.eval a = false := by
            simpa using List.any_eq_false.mp hsem
              ⟨child, List.mem_of_getElem? hc⟩ (List.mem_attach _ _)
          simpa [possibleAt, hc] using ih d false child hd he
    case reify child guard =>
      by_cases hi : index = 0
      · simp only [possibleAt, hi, bne_self_eq_false, Bool.false_eq_true, ↓reduceIte]
        apply List.any_eq_true.mpr
        refine ⟨guard.eval a, values_sound d a hd guard, ?_⟩
        by_cases hz : guard.eval a = 0
        · simp only [hz, beq_self_eq_true, ↓reduceIte]
          apply ih _ (!expected) child (by simpa only [hz] using compatible_bindTerm d guard a hd)
          cases expected <;> cases hc : child.eval a <;> simp_all [Constraint.eval]
        · by_cases ho : guard.eval a = 1
          · simp only [ho, ↓reduceIte, beq_self_eq_true]
            apply ih _ expected child (by simpa only [ho] using compatible_bindTerm d guard a hd)
            simpa [Constraint.eval, ho] using hs
          · cases expected <;> simp_all [Constraint.eval]
      · simp [possibleAt, hi]
    case imply child guard =>
      by_cases hi : index = 0
      · simp only [possibleAt, hi, bne_self_eq_false, Bool.false_eq_true, ↓reduceIte]
        apply List.any_eq_true.mpr
        refine ⟨guard.eval a, values_sound d a hd guard, ?_⟩
        by_cases hz : guard.eval a = 0
        · simp only [hz, beq_self_eq_true, ↓reduceIte]
          simpa [Constraint.eval, hz] using hs.symm
        · by_cases ho : guard.eval a = 1
          · simp only [ho, ↓reduceIte, beq_self_eq_true]
            apply ih _ expected child (by simpa only [ho] using compatible_bindTerm d guard a hd)
            simpa [Constraint.eval, ho] using hs
          · cases expected <;> simp_all [Constraint.eval]
      · simp [possibleAt, hi]

def checkConflict (d : Domains) (c : Constraint) : Bool := !possible d true c

theorem checkConflict_sound (d : Domains) (c : Constraint) (hc : checkConflict d c = true)
    (a : Assignment) (hd : Compatible d a) (hs : c.eval a = true) : False := by
  have hp := possible_sound d true c a hd hs
  simp [checkConflict, hp] at hc

def rule : LocalProof.Rule := ⟨checkConflict, checkConflict_sound⟩

def isComposite : Constraint → Bool
  | .reify .. | .imply .. | .conjunction .. | .disjunction .. => true
  | _ => false

/-- Coverage gate: unsupported leaves keep the enclosing tree explicitly
    unchecked, instead of rejecting a deduction that needs a future rule. -/
def supported (c : Constraint) : Bool :=
  match c with
  | .reify child _ | .imply child _ => supported child
  | .conjunction children | .disjunction children =>
      children.attach.all (fun child => supported child.val)
  | c => LeafProof.supported c
termination_by sizeOf c
decreasing_by
  all_goals simp only [Constraint.reify.sizeOf_spec, Constraint.imply.sizeOf_spec,
    Constraint.conjunction.sizeOf_spec, Constraint.disjunction.sizeOf_spec]
  all_goals first | omega | exact Nat.lt_trans (List.sizeOf_lt_of_mem child.property) (by omega)

end Minion.LogicProof
