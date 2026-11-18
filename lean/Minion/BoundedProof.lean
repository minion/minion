import Minion.NvalueProof

/-! A bounded forward-checking rule. Once all argument views are fixed, evaluate
    the relation. Otherwise split on at most `fuel` argument values; exhausting
    fuel returns unknown/possible. This is useful for Minion's generic negation
    wrapper, which prunes only when at most one underlying variable is free. -/
namespace Minion.BoundedProof
open DomainProof NvalueProof

def unresolved (d : Domains) (xs : List Term) : Option Term :=
  xs.find? fun t => (singletonValue d t).isNone

def snapshot (d : Domains) (xs : List Term) : List Int :=
  xs.map fun t => (singletonValue d t).getD 0

theorem snapshot_sound (d : Domains) (xs : List Term) (a : Assignment)
    (hd : Compatible d a) (hu : unresolved d xs = none) :
    snapshot d xs = xs.map (Term.eval a) := by
  apply List.map_congr_left
  intro t ht
  have hn := List.find?_eq_none.mp hu t ht
  cases hs : singletonValue d t with
  | none => simp [hs] at hn
  | some v => simpa [hs] using (singletonValue_sound d t v hs a hd).symm

def possible (d : Domains) (xs : List Term) (predicate : List Int → Bool) : Nat → Bool
  | 0 => match unresolved d xs with
    | none => predicate (snapshot d xs)
    | some _ => true
  | fuel + 1 => match unresolved d xs with
    | none => predicate (snapshot d xs)
    | some t => (values d t).any fun v => possible (bindTerm d t v) xs predicate fuel

theorem possible_sound (d : Domains) (xs : List Term) (predicate : List Int → Bool)
    (fuel : Nat) (a : Assignment) (hd : Compatible d a)
    (hs : predicate (xs.map (Term.eval a)) = true) : possible d xs predicate fuel = true := by
  induction fuel generalizing d with
  | zero =>
    cases hu : unresolved d xs with
    | none => simpa [possible, hu, snapshot_sound d xs a hd hu] using hs
    | some _ => simp [possible, hu]
  | succ fuel ih =>
    cases hu : unresolved d xs with
    | none => simpa [possible, hu, snapshot_sound d xs a hd hu] using hs
    | some t =>
      simp only [possible, hu]
      exact List.any_eq_true.mpr ⟨t.eval a, values_sound d a hd t,
        ih _ (compatible_bindTerm d t a hd)⟩

end Minion.BoundedProof
