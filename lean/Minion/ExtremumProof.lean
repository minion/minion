import Minion.LocalProof
import Minion.Views

/-! Min feasibility uses all arguments >= result and one equal argument.
    Its complement has an argument below the result or all arguments above it.
    Max reuses min through a proved negated-view normalization. -/
namespace Minion.ExtremumProof
open DomainProof

def can (d : Domains) (t : Term) (predicate : Int → Bool) : Bool :=
  (values d t).any predicate

theorem can_sound (d : Domains) (t : Term) (predicate : Int → Bool) (a : Assignment)
    (hd : Compatible d a) (hp : predicate (t.eval a) = true) : can d t predicate = true :=
  List.any_eq_true.mpr ⟨t.eval a, values_sound d a hd t, hp⟩

def candidate (d : Domains) (expected : Bool) (r : Int) : List Term → Bool
  | [] => !expected
  | xs@(_ :: _) =>
      if expected then
        xs.all (fun t => can d t (fun v => r ≤ v)) && xs.any (fun t => can d t (fun v => v == r))
      else
        xs.any (fun t => can d t (fun v => v < r)) || xs.all (fun t => can d t (fun v => r < v))

theorem candidate_sound (d : Domains) (expected : Bool) (xs : List Term) (rhs : Term)
    (a : Assignment) (hd : Compatible d a)
    (hs : (Constraint.extremum xs rhs false).eval a = expected) :
    candidate d expected (rhs.eval a) xs = true := by
  cases xs with
  | nil =>
    have he : false = expected := by simpa [Constraint.eval] using hs
    simp [candidate, ← he]
  | cons x xs =>
    let m := (xs.map (Term.eval a)).foldl min (x.eval a)
    have hmin : ((x :: xs).map (Term.eval a)).min? = some m := by
      simp only [List.map_cons, List.min?_cons', m]
    have hm := List.min?_eq_some_iff.mp hmin
    obtain ⟨t, ht, htm⟩ := List.mem_map.mp hm.1
    have hsem : (m == rhs.eval a) = expected := by
      simpa only [Constraint.eval, List.map_cons, Bool.false_eq_true, ↓reduceIte, m] using hs
    cases expected with
    | true =>
      have he : m = rhs.eval a := beq_iff_eq.mp hsem
      simp only [candidate, ↓reduceIte]
      simp only [Bool.and_eq_true]
      constructor
      · apply List.all_eq_true.mpr
        intro u hu
        apply can_sound d u _ a hd
        have hb := hm.2 (u.eval a) (List.mem_map.mpr ⟨u, hu, rfl⟩)
        simpa only [decide_eq_true_eq, he] using hb
      · apply List.any_eq_true.mpr
        refine ⟨t, ht, can_sound d t _ a hd ?_⟩
        simp [htm, he]
    | false =>
      have hne : m ≠ rhs.eval a := by intro he; simp [he] at hsem
      simp only [candidate, Bool.false_eq_true, ↓reduceIte]
      by_cases hlt : m < rhs.eval a
      · simp only [Bool.or_eq_true]
        left
        apply List.any_eq_true.mpr
        refine ⟨t, ht, can_sound d t _ a hd ?_⟩
        simpa only [decide_eq_true_eq, htm] using hlt
      · have hgt : rhs.eval a < m := by omega
        simp only [Bool.or_eq_true]
        right
        apply List.all_eq_true.mpr
        intro u hu
        apply can_sound d u _ a hd
        have hb := hm.2 (u.eval a) (List.mem_map.mpr ⟨u, hu, rfl⟩)
        simp only [decide_eq_true_eq]
        omega

def possibleMin (d : Domains) (expected : Bool) : Constraint → Bool
  | .extremum xs rhs false =>
      (values d rhs).any fun r => candidate (bindTerm d rhs r) expected r xs
  | _ => true

theorem possibleMin_sound (d : Domains) (expected : Bool) (c : Constraint) (a : Assignment)
    (hd : Compatible d a) (hs : c.eval a = expected) : possibleMin d expected c = true := by
  cases c <;> try rfl
  rename_i xs rhs isMax
  cases isMax <;> try rfl
  exact List.any_eq_true.mpr ⟨rhs.eval a, values_sound d a hd rhs,
    candidate_sound _ expected xs rhs a (compatible_bindTerm d rhs a hd) hs⟩

def possibleValue (d : Domains) (expected : Bool) (c : Constraint) : Bool :=
  possibleMin d expected c.normalizeExtremum

theorem possibleValue_sound (d : Domains) (expected : Bool) (c : Constraint) (a : Assignment)
    (hd : Compatible d a) (hs : c.eval a = expected) : possibleValue d expected c = true :=
  possibleMin_sound d expected c.normalizeExtremum a hd (by simpa only [Constraint.normalizeExtremum_eval] using hs)

def supported : Constraint → Bool
  | .extremum .. => true
  | _ => false

end Minion.ExtremumProof
