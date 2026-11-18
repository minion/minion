import Minion.SetProof
import Minion.CountProof

/-! Distinct-value bounds: forced singleton values give a lower bound. Every
    remaining occurrence contributes at most one new value, and every value
    must belong to the union of domains. Counts are independent of aliases. -/
namespace Minion.NvalueProof
open DomainProof SetProof

def singletonValue (d : Domains) (t : Term) : Option Int :=
  match values d t with
  | [] => none
  | v :: vs => if vs.all (fun w => w == v) then some v else none

theorem singletonValue_sound (d : Domains) (t : Term) (v : Int)
    (hv : singletonValue d t = some v) (a : Assignment) (hd : Compatible d a) : t.eval a = v := by
  have hm := values_sound d a hd t
  cases hs : values d t with
  | nil => simp [singletonValue, hs] at hv
  | cons x xs =>
    cases ha : xs.all (fun w => w == x) with
    | false => simp [singletonValue, hs, ha] at hv
    | true =>
      have he : x = v := by simpa [singletonValue, hs, ha] using hv
      rw [hs] at hm
      rcases List.mem_cons.mp hm with hx | hr
      · exact hx.trans he
      · have hh := List.all_eq_true.mp ha (t.eval a) hr
        exact (by simpa using hh : t.eval a = x).trans he

def forcedValues (d : Domains) (xs : List Term) : List Int := xs.filterMap (singletonValue d)
def unassignedTerms (d : Domains) (xs : List Term) : List Term :=
  xs.filter (fun t => (singletonValue d t).isNone)
def unionValues (d : Domains) (xs : List Term) : List Int := xs.flatMap (values d)

def requiresFresh (d : Domains) (xs : List Term) : Bool :=
  let forced := forcedValues d xs
  xs.any (fun t => (values d t).all (fun v => !forced.contains v))

theorem forced_subset (d : Domains) (xs : List Term) (a : Assignment) (hd : Compatible d a) :
    forcedValues d xs ⊆ xs.map (Term.eval a) := by
  intro v hv
  obtain ⟨t, ht, he⟩ := List.mem_filterMap.mp hv
  exact List.mem_map.mpr ⟨t, ht, singletonValue_sound d t v he a hd⟩

theorem union_covers (d : Domains) (xs : List Term) (a : Assignment) (hd : Compatible d a) :
    xs.map (Term.eval a) ⊆ unionValues d xs := by
  intro v hv
  obtain ⟨t, ht, rfl⟩ := List.mem_map.mp hv
  exact List.mem_flatMap.mpr ⟨t, ht, values_sound d a hd t⟩

theorem partition_covers (d : Domains) (xs : List Term) (a : Assignment) (hd : Compatible d a) :
    xs.map (Term.eval a) ⊆ forcedValues d xs ++ (unassignedTerms d xs).map (Term.eval a) := by
  intro v hv
  obtain ⟨t, ht, rfl⟩ := List.mem_map.mp hv
  cases hs : singletonValue d t with
  | none =>
    apply List.mem_append_right
    exact List.mem_map.mpr ⟨t, List.mem_filter.mpr ⟨ht, by simp [hs]⟩, rfl⟩
  | some w =>
    apply List.mem_append_left
    have he := singletonValue_sound d t w hs a hd
    rw [he]
    exact List.mem_filterMap.mpr ⟨t, ht, hs⟩

def bounds (d : Domains) (xs : List Term) : Nat × Nat :=
  let forced := card (forcedValues d xs)
  (forced + (if requiresFresh d xs then 1 else 0),
    cappedCard (forced + (unassignedTerms d xs).length) (unionValues d xs))

theorem bounds_sound (d : Domains) (xs : List Term) (a : Assignment) (hd : Compatible d a) :
    (bounds d xs).1 ≤ card (xs.map (Term.eval a)) ∧
      card (xs.map (Term.eval a)) ≤ (bounds d xs).2 := by
  simp only [bounds, cappedCard_eq]
  constructor
  · cases hf : requiresFresh d xs with
    | false => simpa [bounds, cappedCard_eq, hf] using card_mono _ _ (forced_subset d xs a hd)
    | true =>
      obtain ⟨t, ht, hall⟩ := List.any_eq_true.mp hf
      have hv := List.all_eq_true.mp hall (t.eval a) (values_sound d a hd t)
      have hn : t.eval a ∉ forcedValues d xs := by simpa using hv
      have hsub : t.eval a :: forcedValues d xs ⊆ xs.map (Term.eval a) := by
        intro v hm
        cases List.mem_cons.mp hm with
        | inl he => subst v; exact List.mem_map.mpr ⟨t, ht, rfl⟩
        | inr hr => exact forced_subset d xs a hd hr
      have hc := card_mono _ _ hsub
      rw [card_cons_of_not_mem _ _ hn] at hc
      simpa [bounds, cappedCard_eq, hf] using hc
  · apply Nat.le_min.mpr
    constructor
    · have hsub := card_mono _ _ (partition_covers d xs a hd)
      have happ := card_append_le (forcedValues d xs) ((unassignedTerms d xs).map (Term.eval a))
      simpa only [List.length_map] using Nat.le_trans hsub happ
    · exact card_mono _ _ (union_covers d xs a hd)

def possibleValue (d : Domains) (expected : Bool) : Constraint → Bool
  | .nvalue xs count op => CountProof.quickCount d expected xs.length op count &&
      CountProof.possibleRange d expected (fun dr => bounds dr xs) op count
  | _ => true

theorem possibleValue_sound (d : Domains) (expected : Bool) (c : Constraint) (a : Assignment)
    (hd : Compatible d a) (hs : c.eval a = expected) : possibleValue d expected c = true := by
  cases c <;> try rfl
  rename_i xs count op
  simp only [possibleValue, Bool.and_eq_true]
  constructor
  · apply CountProof.quickCount_sound d expected xs.length (card (xs.map (Term.eval a))) op count a hd
    · simpa only [List.length_map] using card_le_length (xs.map (Term.eval a))
    · simpa only [Constraint.eval, card] using hs
  · apply CountProof.possibleRange_sound d expected (fun dr => bounds dr xs)
      (fun ar => card (xs.map (Term.eval ar))) (fun dr ar hr => bounds_sound dr xs ar hr) op count a hd
    simpa only [Constraint.eval, card] using hs

def supported : Constraint → Bool
  | .nvalue .. => true
  | _ => false

end Minion.NvalueProof
