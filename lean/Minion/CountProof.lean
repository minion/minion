import Minion.BoundsProof

/-! Shared must/may bounds for counts of binary predicates. Occurrence, literal
    sums and Hamming use one count lemma, including their false polarity. -/
namespace Minion.CountProof
open DomainProof

def pairHolds (a : Assignment) (rel : Comparison) (p : Term × Term) : Bool :=
  rel.eval (p.1.eval a) (p.2.eval a)

def pairMay (d : Domains) (rel : Comparison) (p : Term × Term) : Bool :=
  SupportProof.possibleCompare d p.1 p.2 rel true

def pairMust (d : Domains) (rel : Comparison) (p : Term × Term) : Bool :=
  !SupportProof.possibleCompare d p.1 p.2 rel false

theorem count_bounds (d : Domains) (rel : Comparison) (pairs : List (Term × Term))
    (a : Assignment) (hd : Compatible d a) :
    pairs.countP (pairMust d rel) ≤ pairs.countP (pairHolds a rel) ∧
      pairs.countP (pairHolds a rel) ≤ pairs.countP (pairMay d rel) := by
  constructor
  · apply List.countP_mono_left
    intro p _ hm
    cases hp : pairHolds a rel p with
    | true => rfl
    | false =>
      have hn := SupportProof.possibleCompare_sound d p.1 p.2 rel false a hd
        (by simpa only [pairHolds] using hp)
      simp [pairMust, hn] at hm
  · apply List.countP_mono_left
    intro p _ hp
    exact SupportProof.possibleCompare_sound d p.1 p.2 rel true a hd hp

def possibleRange (d : Domains) (expected : Bool) (bounds : Domains → Nat × Nat)
    (op : Comparison) (count : Term) : Bool :=
  (values d count).any fun r =>
    let dr := bindTerm d count r
    let (lower, upper) := bounds dr
    BoundsProof.intervalPossible op expected (lower : Int) (upper : Int) r r

theorem possibleRange_sound (d : Domains) (expected : Bool) (bounds : Domains → Nat × Nat)
    (measure : Assignment → Nat)
    (hbounds : ∀ dr ar, Compatible dr ar → (bounds dr).1 ≤ measure ar ∧ measure ar ≤ (bounds dr).2)
    (op : Comparison) (count : Term) (a : Assignment) (hd : Compatible d a)
    (hs : op.eval (measure a : Int) (count.eval a) = expected) :
    possibleRange d expected bounds op count = true := by
  refine List.any_eq_true.mpr ⟨count.eval a, values_sound d a hd count, ?_⟩
  have hb := hbounds (bindTerm d count (count.eval a)) a (compatible_bindTerm d count a hd)
  exact BoundsProof.intervalPossible_sound op expected _ _ (count.eval a) (count.eval a)
    (measure a : Int) (count.eval a)
    (by omega) (by omega) (by omega) (by omega) hs

/-- Check the universal count range before binding candidate count values.
    This rejects a wide impossible tail without recomputing argument bounds
    once for every value in that tail. -/
def quickCount (d : Domains) (expected : Bool) (maximum : Nat)
    (op : Comparison) (count : Term) : Bool :=
  BoundsProof.intervalPossible op expected 0 (maximum : Int) (lower d count) (upper d count)

theorem quickCount_sound (d : Domains) (expected : Bool) (maximum measure : Nat)
    (op : Comparison) (count : Term) (a : Assignment) (hd : Compatible d a)
    (hm : measure ≤ maximum) (hs : op.eval (measure : Int) (count.eval a) = expected) :
    quickCount d expected maximum op count = true := by
  exact BoundsProof.intervalPossible_sound op expected 0 maximum _ _ measure (count.eval a)
    (by omega) (by omega) (lower_sound d a hd count) (upper_sound d a hd count) hs

def possibleCount (d : Domains) (expected : Bool) (pairs : List (Term × Term))
    (rel op : Comparison) (count : Term) : Bool :=
  quickCount d expected pairs.length op count &&
    possibleRange d expected (fun dr => (pairs.countP (pairMust dr rel), pairs.countP (pairMay dr rel))) op count

theorem possibleCount_sound (d : Domains) (expected : Bool) (pairs : List (Term × Term))
    (rel op : Comparison) (count : Term) (a : Assignment) (hd : Compatible d a)
    (hs : op.eval (pairs.countP (pairHolds a rel) : Int) (count.eval a) = expected) :
    possibleCount d expected pairs rel op count = true := by
  simp only [possibleCount, Bool.and_eq_true]
  exact ⟨quickCount_sound d expected pairs.length _ op count a hd (List.countP_le_length) hs,
    possibleRange_sound d expected _ (fun ar => pairs.countP (pairHolds ar rel))
      (fun dr ar hr => count_bounds dr rel pairs ar hr) op count a hd hs⟩

theorem any_eq_count_ge (a : Assignment) (rel : Comparison) (pairs : List (Term × Term)) :
    pairs.any (pairHolds a rel) = Comparison.ge.eval (pairs.countP (pairHolds a rel) : Int) 1 := by
  have he : pairs.any (pairHolds a rel) = decide (1 ≤ pairs.countP (pairHolds a rel)) := by
    rw [Bool.eq_iff_iff]
    simp
  rw [he]
  simp only [Comparison.eval, decide_eq_decide]
  omega

def possibleValue (d : Domains) (expected : Bool) : Constraint → Bool
  | .occurrence xs value count op =>
      possibleCount d expected (xs.map (fun x => (x, value))) .eq op count
  | .literalSum xs vs count =>
      if xs.length == vs.length then possibleCount d expected (xs.zip vs) .eq .ge count else !expected
  | .hamming xs ys count op =>
      if xs.length == ys.length then possibleCount d expected (xs.zip ys) .ne op count else !expected
  | .vectorExists xs ys rel =>
      if xs.length == ys.length then possibleCount d expected (xs.zip ys) rel .ge (.constant 1) else !expected
  | _ => true

theorem possibleValue_sound (d : Domains) (expected : Bool) (c : Constraint) (a : Assignment)
    (hd : Compatible d a) (hs : c.eval a = expected) : possibleValue d expected c = true := by
  cases c with
  | occurrence xs value count op =>
    apply possibleCount_sound d expected _ .eq op count a hd
    have hp : (xs.map (fun x => (x, value))).countP (pairHolds a .eq) =
        xs.countP (fun x => x.eval a == value.eval a) := by
      rw [List.countP_map]
      congr 1
    rw [hp, List.countP_eq_length_filter]
    simpa only [Constraint.eval] using hs
  | literalSum xs vs count =>
    cases hlen : xs.length == vs.length with
    | false =>
      have he : false = expected := by simpa [Constraint.eval, hlen] using hs
      simp [possibleValue, hlen, ← he]
    | true =>
      simp only [possibleValue, hlen, ↓reduceIte]
      apply possibleCount_sound d expected _ .eq .ge count a hd
      have hp : pairHolds a .eq = (fun (p : Term × Term) => p.1.eval a == p.2.eval a) := rfl
      rw [hp, List.countP_eq_length_filter]
      simpa only [Constraint.eval, Comparison.eval, hlen, Bool.true_and] using hs
  | hamming xs ys count op =>
    cases hlen : xs.length == ys.length with
    | false =>
      have he : false = expected := by simpa [Constraint.eval, hlen] using hs
      simp [possibleValue, hlen, ← he]
    | true =>
      simp only [possibleValue, hlen, ↓reduceIte]
      apply possibleCount_sound d expected _ .ne op count a hd
      have hp : pairHolds a .ne = (fun (p : Term × Term) => p.1.eval a != p.2.eval a) := rfl
      rw [hp, List.countP_eq_length_filter]
      simpa only [Constraint.eval, hlen, Bool.true_and] using hs
  | vectorExists xs ys rel =>
    cases hlen : xs.length == ys.length with
    | false =>
      have he : false = expected := by simpa [Constraint.eval, hlen] using hs
      simp [possibleValue, hlen, ← he]
    | true =>
      simp only [possibleValue, hlen, ↓reduceIte]
      apply possibleCount_sound d expected _ rel .ge (.constant 1) a hd
      change Comparison.ge.eval ((xs.zip ys).countP (pairHolds a rel) : Int) 1 = expected
      rw [← any_eq_count_ge]
      have hp : pairHolds a rel = (fun (p : Term × Term) => rel.eval (p.1.eval a) (p.2.eval a)) := rfl
      rw [hp]
      simpa only [Constraint.eval, hlen, Bool.true_and] using hs
  | _ => rfl

def supported : Constraint → Bool
  | .occurrence .. | .literalSum .. | .hamming .. | .vectorExists .. => true
  | _ => false

end Minion.CountProof
