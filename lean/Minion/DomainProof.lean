import Minion.Check
import Minion.Views

/-! Shared finite-domain and variable-view reasoning for local proof rules. -/
namespace Minion.DomainProof

abbrev Domains := Nat → List Int

def Compatible (d : Domains) (a : Assignment) : Prop :=
  ∀ i, a[i]?.getD 0 ∈ d i

def values (d : Domains) : Term → List Int
  | .constant v => [v]
  | .var i => d i
  | .notBool i => (d i).map (fun v => 1 - v)
  | .neg t => (values d t).map (fun v => -v)
  | .shift t k => (values d t).map (fun v => v + k)
  | .scale t k =>
      let vs := values d t
      if k == 0 then if vs.isEmpty then [] else [0] else vs.map (fun v => k * v)

theorem values_sound (d : Domains) (a : Assignment) (h : Compatible d a) (t : Term) :
    t.eval a ∈ values d t := by
  induction t with
  | constant v => simp [values, Term.eval]
  | var i => exact h i
  | notBool i => exact List.mem_map.mpr ⟨a[i]?.getD 0, h i, rfl⟩
  | neg t ih => exact List.mem_map.mpr ⟨t.eval a, ih, rfl⟩
  | shift t k ih => exact List.mem_map.mpr ⟨t.eval a, ih, rfl⟩
  | scale t k ih =>
    by_cases hk : k = 0
    · subst k
      have hn : (values d t).isEmpty = false := by
        cases he : values d t with
        | nil => simp [he] at ih
        | cons _ _ => rfl
      simp [values, hn, Term.eval]
    · simp only [values, beq_iff_eq, hk, ↓reduceIte]
      exact List.mem_map.mpr ⟨t.eval a, ih, rfl⟩

/-- Bind a candidate view value at its original variable using a unique inverse.
    Candidate enumeration establishes old-domain membership; this helper does
    not scan the old domain again. Non-injective zero-scale views leave it
    unchanged. This is an internal support-search approximation, not a pruning
    rule. The compatibility theorem is the prerequisite its callers use. -/
def bindTerm (d : Domains) (t : Term) (value : Int) : Domains :=
  match t.baseIndex, t.preimageValue value with
  | some i, some v => fun j => if j == i then
      (if t.baseValue v == value then [v] else []) else d j
  | _, _ => d

theorem compatible_bindTerm (d : Domains) (t : Term) (a : Assignment)
    (hd : Compatible d a) : Compatible (bindTerm d t (t.eval a)) a := by
  cases hi : t.baseIndex with
  | none => simpa [bindTerm, hi] using hd
  | some i =>
    cases hv : t.preimageValue (t.eval a) with
    | none => simpa [bindTerm, hi, hv] using hd
    | some v =>
      intro j
      by_cases hj : j = i
      · subst j
        have hp := Term.preimageValue_sound t i v a hi hv
        have he := Term.baseValue_sound t i hi a
        rw [hp] at he
        simpa [bindTerm, hi, hv, he] using hp
      · simpa [bindTerm, hi, hv, hj] using hd j

def lower (d : Domains) (t : Term) : Int := (values d t).min?.getD 0
def upper (d : Domains) (t : Term) : Int := (values d t).max?.getD 0
private theorem min_bound {vs : List Int} {v : Int} (h : v ∈ vs) :
    vs.min?.getD 0 ≤ v := by
  cases hm : vs.min? with
  | none => have := List.min?_eq_none_iff.mp hm; simp_all
  | some lo =>
    have bound := (List.min?_eq_some_iff.mp hm).2 v h
    simpa [hm] using bound

private theorem max_bound {vs : List Int} {v : Int} (h : v ∈ vs) :
    v ≤ vs.max?.getD 0 := by
  cases hm : vs.max? with
  | none => have := List.max?_eq_none_iff.mp hm; simp_all
  | some hi =>
    have bound := (List.max?_eq_some_iff.mp hm).2 v h
    simpa [hm] using bound

theorem lower_sound (d : Domains) (a : Assignment) (h : Compatible d a) (t : Term) :
    lower d t ≤ t.eval a := min_bound (values_sound d a h t)

theorem upper_sound (d : Domains) (a : Assignment) (h : Compatible d a) (t : Term) :
    t.eval a ≤ upper d t := max_bound (values_sound d a h t)

theorem nonempty_values (d : Domains) (a : Assignment) (h : Compatible d a) (t : Term) :
    (values d t).isEmpty = false := by
  have hv := values_sound d a h t
  cases hvv : values d t <;> simp_all

inductive Operation where
  | assign | remove | lowerBound | upperBound
  deriving Repr, BEq

def Operation.holds (op : Operation) (bound value : Int) : Bool :=
  match op with
  | .assign => value == bound
  | .remove => value != bound
  | .lowerBound => value ≥ bound
  | .upperBound => value ≤ bound

/-- A decision event already applies its restriction during replay. The ensuing
    low-level domain event is a genuine no-op in that replay state. -/
def checkNoop (d : Domains) (target : Nat) (op : Operation) (bound : Int) : Bool :=
  (d target).all (op.holds bound)

theorem checkNoop_sound (d : Domains) (target : Nat) (op : Operation) (bound : Int)
    (hc : checkNoop d target op bound = true) (a : Assignment) (hd : Compatible d a) :
    op.holds bound (a[target]?.getD 0) = true :=
  List.all_eq_true.mp hc _ (hd target)

/-- Overapproximate rejected assignments. A single-value deletion can test its
    singleton directly, without repeatedly filtering a wide old domain. An
    absent candidate is conservative; no-op deletion events are handled before
    this rule is needed. -/
def rejectedDomains (d : Domains) (target : Nat) (op : Operation) (bound : Int) : Domains :=
  fun i => if i == target then
    match op with
    | .remove => [bound]
    | op => (d i).filter (fun v => !op.holds bound v)
  else d i

theorem compatible_rejected (d : Domains) (a : Assignment) (hd : Compatible d a)
    (target : Nat) (op : Operation) (bound : Int) (hbad : op.holds bound (a[target]?.getD 0) = false) :
    Compatible (rejectedDomains d target op bound) a := by
  intro i
  by_cases hi : i = target
  · subst i
    cases op with
    | remove =>
      have he : a[target]?.getD 0 = bound := by simpa [Operation.holds] using hbad
      simp [rejectedDomains, he]
    | assign | lowerBound | upperBound =>
      simpa [rejectedDomains, List.mem_filter, hbad] using hd target
  · simpa [rejectedDomains, hi] using hd i

/-- Bridge finite model domains to the total domain function used by the rules.
    The out-of-range default matches Term.eval's default. -/
def ofLists (d : List (List Int)) : Domains := fun i => d[i]?.getD [0]

theorem inDomains_compatible (d : List (List Int)) (a : Assignment) (h : InDomains d a) :
    Compatible (ofLists d) a := by
  induction d generalizing a with
  | nil =>
    cases a with
    | nil => intro i; simp [ofLists]
    | cons _ _ => simp [InDomains] at h
  | cons domain rest ih =>
    cases a with
    | nil => simp [InDomains] at h
    | cons value values =>
      have hh := h
      change value ∈ domain ∧ InDomains rest values at hh
      intro i
      cases i with
      | zero => simpa [ofLists] using hh.1
      | succ i => simpa [ofLists] using ih values hh.2 i

theorem inDomains_restrict (d : List (List Int)) (a : Assignment) (target : Nat)
    (op : Operation) (bound : Int) (hd : InDomains d a)
    (hp : op.holds bound (a[target]?.getD 0) = true) :
    InDomains (d.modify target (List.filter (op.holds bound))) a := by
  induction d generalizing a target with
  | nil => cases a <;> simp_all [InDomains]
  | cons domain rest ih =>
    cases a with
    | nil => simp [InDomains] at hd
    | cons value values =>
      change value ∈ domain ∧ InDomains rest values at hd
      cases target with
      | zero =>
        change value ∈ domain.filter (op.holds bound) ∧ InDomains rest values
        exact ⟨List.mem_filter.mpr ⟨hd.1, hp⟩, hd.2⟩
      | succ target =>
        change value ∈ domain ∧ InDomains (rest.modify target (List.filter (op.holds bound))) values
        exact ⟨hd.1, ih values target hd.2 hp⟩

def hasEmptyDomain (d : List (List Int)) : Bool := d.any List.isEmpty

theorem hasEmptyDomain_excludes (d : List (List Int)) (a : Assignment)
    (hc : hasEmptyDomain d = true) (hd : InDomains d a) : False := by
  induction d generalizing a with
  | nil => simp [hasEmptyDomain] at hc
  | cons domain rest ih =>
    cases a with
    | nil => simp [InDomains] at hd
    | cons value values =>
      change value ∈ domain ∧ InDomains rest values at hd
      have hnonempty : domain.isEmpty = false := by
        cases domain with
        | nil => simp at hd
        | cons _ _ => rfl
      have hr : hasEmptyDomain rest = true := by simpa [hasEmptyDomain, hnonempty] using hc
      exact ih values hr hd.2

theorem hasEmptyDomain_prune_preserves (d : List (List Int)) (a : Assignment)
    (hc : hasEmptyDomain d = true) (hd : InDomains d a)
    (target : Nat) (op : Operation) (bound : Int) :
    InDomains (d.modify target (List.filter (op.holds bound))) a :=
  False.elim (hasEmptyDomain_excludes d a hc hd)

end Minion.DomainProof
