import Minion.DomainProof

/-! One reusable proof interface for every sound local conflict check. The
    conflict check can use bounds, support sets, Hall witnesses, etc. -/
namespace Minion.LocalProof
open DomainProof

structure FeasibilityRule where
  supported : Constraint → Bool
  possible : Domains → Bool → Constraint → Bool
  sound : ∀ d expected c a, Compatible d a → c.eval a = expected → possible d expected c = true

def choosePossible (rules : List FeasibilityRule) (d : Domains) (expected : Bool) (c : Constraint) : Bool :=
  match rules with
  | [] => true
  | r :: rest => if r.supported c then r.possible d expected c else choosePossible rest d expected c

theorem choosePossible_sound (rules : List FeasibilityRule) (d : Domains) (expected : Bool)
    (c : Constraint) (a : Assignment) (hd : Compatible d a) (hs : c.eval a = expected) :
    choosePossible rules d expected c = true := by
  induction rules with
  | nil => rfl
  | cons r rest ih =>
    simp only [choosePossible]
    split
    · exact r.sound d expected c a hd hs
    · exact ih

structure Rule where
  conflict : Domains → Constraint → Bool
  sound : ∀ d c, conflict d c = true → ∀ a, Compatible d a → c.eval a = true → False

def Rule.possible (r : Rule) (d : Domains) (c : Constraint) : Bool := !r.conflict d c

theorem Rule.possible_sound (r : Rule) (d : Domains) (c : Constraint) (a : Assignment)
    (hd : Compatible d a) (hs : c.eval a = true) : r.possible d c = true := by
  cases hc : r.conflict d c with
  | false => simp [Rule.possible, hc]
  | true => exact False.elim (r.sound d c hc a hd hs)

def Rule.prune (r : Rule) (d : Domains) (c : Constraint)
    (target : Nat) (op : Operation) (bound : Int) : Bool :=
  r.conflict (rejectedDomains d target op bound) c

theorem Rule.prune_sound (r : Rule) (d : Domains) (c : Constraint)
    (target : Nat) (op : Operation) (bound : Int)
    (hc : r.prune d c target op bound = true) (a : Assignment)
    (hd : Compatible d a) (hs : c.eval a = true) :
    op.holds bound (a[target]?.getD 0) = true := by
  cases hbad : op.holds bound (a[target]?.getD 0) with
  | true => rfl
  | false =>
    exact False.elim (r.sound _ c hc a (compatible_rejected d a hd target op bound hbad) hs)

theorem Rule.prune_preservesSolutions (r : Rule) (m : Model) (d : List (List Int))
    (c : Constraint) (hm : c ∈ m.constraints) (target : Nat) (op : Operation) (bound : Int)
    (hc : r.prune (ofLists d) c target op bound = true)
    (a : Assignment) (hs : m.IsSolution a) (hd : InDomains d a) :
    InDomains (d.modify target (List.filter (op.holds bound))) a :=
  inDomains_restrict d a target op bound hd
    (r.prune_sound _ c target op bound hc a (inDomains_compatible d a hd)
      (List.all_eq_true.mp hs.2 c hm))

theorem Rule.conflict_excludesSolutions (r : Rule) (m : Model) (d : List (List Int))
    (c : Constraint) (hm : c ∈ m.constraints) (hc : r.conflict (ofLists d) c = true)
    (a : Assignment) (hs : m.IsSolution a) (hd : InDomains d a) : False :=
  r.sound _ c hc a (inDomains_compatible d a hd) (List.all_eq_true.mp hs.2 c hm)

end Minion.LocalProof
