import Minion.SupportProof
import Minion.SetProof
import Minion.Matching

/-! A Hall witness is checked by one fixed cardinality lemma. Discovery is an
    independent bounded matching calculation; its output is never trusted.
    The false polarity is witnessed by a possible equality between two entries. -/
namespace Minion.AllDiffProof
open DomainProof SetProof

def select (xs : List Term) (indices : List Nat) : List Term :=
  ((xs.zipIdx).filter (fun p => indices.contains p.2)).map Prod.fst

theorem select_sublist (xs : List Term) (indices : List Nat) : List.Sublist (select xs indices) xs := by
  have hf : List.Sublist ((xs.zipIdx).filter (fun p => indices.contains p.2)) xs.zipIdx := List.filter_sublist
  simpa only [select, List.zipIdx_map_fst] using hf.map Prod.fst

def hallCheck (d : Domains) (xs : List Term) (indices : List Nat) : Bool :=
  let chosen := select xs indices
  cappedCard chosen.length (chosen.flatMap (values d)) < chosen.length

theorem hallCheck_excludes (d : Domains) (xs : List Term) (indices : List Nat)
    (a : Assignment) (hd : Compatible d a) (hn : (xs.map (Term.eval a)).Nodup)
    (hc : hallCheck d xs indices = true) : False := by
  let chosen := select xs indices
  have hsub : List.Sublist (chosen.map (Term.eval a)) (xs.map (Term.eval a)) :=
    (select_sublist xs indices).map (Term.eval a)
  have hnd : (chosen.map (Term.eval a)).Nodup := hn.sublist hsub
  have hvalues : chosen.map (Term.eval a) ⊆ (chosen.flatMap (values d)).eraseDups := by
    intro v hv
    obtain ⟨t, ht, rfl⟩ := List.mem_map.mp hv
    exact List.mem_eraseDups.mpr (List.mem_flatMap.mpr ⟨t, ht, values_sound d a hd t⟩)
  have hb := List.Nodup.length_le_of_subset hnd hvalues
  have hbad : card (chosen.flatMap (values d)) < chosen.length := by
    have hm : min chosen.length (card (chosen.flatMap (values d))) < chosen.length := by
      simpa only [hallCheck, chosen, cappedCard_eq, decide_eq_true_eq] using hc
    omega
  simp only [List.length_map] at hb
  exact Nat.not_lt_of_ge hb hbad

def forcedCollision (d : Domains) : List Term → Bool
  | [] => false
  | x :: xs => xs.any (fun y => !SupportProof.possible2 d x y Comparison.ne.eval) || forcedCollision d xs

theorem forcedCollision_excludes (d : Domains) (xs : List Term) (a : Assignment)
    (hd : Compatible d a) (hn : (xs.map (Term.eval a)).Nodup)
    (hc : forcedCollision d xs = true) : False := by
  induction xs with
  | nil => simp [forcedCollision] at hc
  | cons x xs ih =>
    have hparts := List.nodup_cons.mp (by simpa only [List.map_cons] using hn)
    have hcases : xs.any (fun y => !SupportProof.possible2 d x y Comparison.ne.eval) = true ∨
        forcedCollision d xs = true := by simpa only [forcedCollision, Bool.or_eq_true] using hc
    cases hcases with
    | inr hr => exact ih hparts.2 hr
    | inl hl =>
      obtain ⟨y, hy, hno⟩ := List.any_eq_true.mp hl
      have hne : x.eval a ≠ y.eval a := by
        intro he
        exact hparts.1 (List.mem_map.mpr ⟨y, hy, he.symm⟩)
      have hmay := SupportProof.possible2_sound d x y Comparison.ne.eval a hd (by simp [Comparison.eval, hne])
      simp [hmay] at hno

def equalPairMay (d : Domains) : List Term → Bool
  | [] => false
  | x :: xs => xs.any (fun y => SupportProof.possibleEqual d x y) || equalPairMay d xs

theorem equalPairMay_sound (d : Domains) (xs : List Term) (a : Assignment)
    (hd : Compatible d a) (hn : ¬(xs.map (Term.eval a)).Nodup) : equalPairMay d xs = true := by
  induction xs with
  | nil => simp at hn
  | cons x xs ih =>
    simp only [equalPairMay, Bool.or_eq_true]
    by_cases hx : x.eval a ∈ xs.map (Term.eval a)
    · left
      obtain ⟨y, hy, he⟩ := List.mem_map.mp hx
      exact List.any_eq_true.mpr ⟨y, hy,
        SupportProof.possibleEqual_sound d x y a hd he.symm⟩
    · right
      apply ih
      intro ht
      exact hn (by simpa only [List.map_cons, List.nodup_cons] using And.intro hx ht)

def positivePossible (d : Domains) (xs : List Term) : Bool :=
  if forcedCollision d xs then false else
    match Matching.discover (xs.map (values d)) with
    | none => true
    | some indices => !hallCheck d xs indices

theorem positivePossible_sound (d : Domains) (xs : List Term) (a : Assignment)
    (hd : Compatible d a) (hn : (xs.map (Term.eval a)).Nodup) : positivePossible d xs = true := by
  cases hf : forcedCollision d xs with
  | true => exact False.elim (forcedCollision_excludes d xs a hd hn hf)
  | false =>
    simp only [positivePossible, hf, Bool.false_eq_true, ↓reduceIte]
    cases hm : Matching.discover (xs.map (values d)) with
    | none => rfl
    | some indices =>
      cases hh : hallCheck d xs indices with
      | false => simp [hh]
      | true => exact False.elim (hallCheck_excludes d xs indices a hd hn hh)

def possibleValue (d : Domains) (expected : Bool) : Constraint → Bool
  | .allDifferent xs => if expected then positivePossible d xs else equalPairMay d xs
  | _ => true

theorem possibleValue_sound (d : Domains) (expected : Bool) (c : Constraint) (a : Assignment)
    (hd : Compatible d a) (hs : c.eval a = expected) : possibleValue d expected c = true := by
  cases c <;> try rfl
  rename_i xs
  cases expected with
  | true =>
    have hc : card (xs.map (Term.eval a)) = (xs.map (Term.eval a)).length := by
      simpa only [Constraint.eval, card, beq_iff_eq] using hs
    exact positivePossible_sound d xs a hd (nodup_of_card_eq_length _ hc)
  | false =>
    apply equalPairMay_sound d xs a hd
    intro hn
    have he := card_eq_length_of_nodup (xs.map (Term.eval a)) hn
    have ht : (Constraint.allDifferent xs).eval a = true := by
      simpa only [Constraint.eval, card, beq_iff_eq] using he
    have hbad : true = false := ht.symm.trans hs
    cases hbad

def supported : Constraint → Bool
  | .allDifferent .. => true
  | _ => false

end Minion.AllDiffProof
