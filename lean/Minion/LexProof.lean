import Minion.BoundsProof

/-! Lexicographic feasibility follows the first differing position. Equal-prefix
    candidates bind original variables, preserving aliases across positions.
    Uniformly equal pairs can be skipped without enumerating their values.
    Each distinct equality value is explored once, including non-injective views. -/
namespace Minion.LexProof
open DomainProof

theorem cons_eval (a : Assignment) (x y : Term) (xs ys : List Term) (strict : Bool)
    (hlen : xs.length = ys.length) :
    (Constraint.lex (x :: xs) (y :: ys) strict).eval a =
      if x.eval a < y.eval a then true
      else if x.eval a == y.eval a then (Constraint.lex xs ys strict).eval a else false := by
  by_cases hlt : x.eval a < y.eval a
  · simp [Constraint.eval, lexLess, hlen, hlt]
  · by_cases he : x.eval a = y.eval a
    · simp [Constraint.eval, lexLess, hlen, he]
    · simp [Constraint.eval, lexLess, hlen, hlt, he]

def prefixPossible (d : Domains) (expected : Bool) (x y : Term) : Bool :=
  if expected then SupportProof.possible2 d x y (fun xv yv => xv < yv)
  else SupportProof.possible2 d y x (fun yv xv => yv < xv)

theorem equal_of_no_prefix (d : Domains) (expected : Bool) (x y : Term)
    (xs ys : List Term) (strict : Bool) (a : Assignment) (hd : Compatible d a)
    (hlen : xs.length = ys.length)
    (hs : (Constraint.lex (x :: xs) (y :: ys) strict).eval a = expected)
    (hp : prefixPossible d expected x y = false) : x.eval a = y.eval a := by
  by_cases hlt : x.eval a < y.eval a
  · have he : expected = true := by simpa [cons_eval a x y xs ys strict hlen, hlt] using hs.symm
    rw [he] at hp
    have hpos := SupportProof.possible2_sound d x y (fun xv yv => xv < yv) a hd (by simp [hlt])
    simp [prefixPossible, hpos] at hp
  · by_cases heq : x.eval a = y.eval a
    · exact heq
    · have hgt : y.eval a < x.eval a := by omega
      have he : expected = false := by
        simpa [cons_eval a x y xs ys strict hlen, hlt, heq] using hs.symm
      rw [he] at hp
      have hpos := SupportProof.possible2_sound d y x (fun yv xv => yv < xv) a hd (by simp [hgt])
      simp [prefixPossible, hpos] at hp

def lexPossible (d : Domains) (expected : Bool) (xs ys : List Term) (strict : Bool) : Bool :=
  if xs.length != ys.length then !expected else
    match xs, ys with
    | [], [] => (!strict) == expected
    | x :: xs, y :: ys =>
        if prefixPossible d expected x y then true
        else if !SupportProof.possible2 d x y (fun xv yv => xv != yv) then
          lexPossible d expected xs ys strict
        else
          (values d x).any fun xv =>
            let dx := bindTerm d x xv
            (values dx y).contains xv &&
              lexPossible (bindTerm dx y xv) expected xs ys strict
    | _, _ => !expected
termination_by xs.length

theorem lexPossible_sound (d : Domains) (expected : Bool) (xs ys : List Term)
    (strict : Bool) (a : Assignment) (hd : Compatible d a)
    (hs : (Constraint.lex xs ys strict).eval a = expected) : lexPossible d expected xs ys strict = true := by
  induction xs generalizing d ys with
  | nil =>
    cases ys with
    | nil =>
      have he : (!strict) = expected := by simpa [Constraint.eval, lexLess] using hs
      simpa [lexPossible] using (beq_iff_eq.mpr he)
    | cons y ys =>
      have he : false = expected := by simpa [Constraint.eval] using hs
      simp [lexPossible, ← he]
  | cons x xs ih =>
    cases ys with
    | nil =>
      have he : false = expected := by simpa [Constraint.eval] using hs
      simp [lexPossible, ← he]
    | cons y ys =>
      by_cases hlen : xs.length = ys.length
      · rw [lexPossible]
        simp only [List.length_cons, hlen, bne_self_eq_false, Bool.false_eq_true, ↓reduceIte]
        cases hp : prefixPossible d expected x y with
        | true => rfl
        | false =>
          simp only [Bool.false_eq_true, ↓reduceIte]
          have he := equal_of_no_prefix d expected x y xs ys strict a hd hlen hs hp
          have htail : (Constraint.lex xs ys strict).eval a = expected := by
            simpa [cons_eval a x y xs ys strict hlen, he] using hs
          split
          · exact ih d ys hd htail
          · refine List.any_eq_true.mpr ⟨x.eval a, values_sound d a hd x, ?_⟩
            simp only [Bool.and_eq_true]
            constructor
            · simpa only [List.contains_iff_mem, ← he] using
                values_sound _ a (compatible_bindTerm d x a hd) y
            · have hbound := compatible_bindTerm _ y a (compatible_bindTerm d x a hd)
              simpa only [← he] using ih _ ys hbound htail
      · have hb : (xs.length == ys.length) = false :=
          Bool.eq_false_iff.mpr (fun h => hlen (beq_iff_eq.mp h))
        have he : false = expected := by simpa [Constraint.eval, hb] using hs
        simp [lexPossible, hlen, ← he]

-- A relaxation without candidate bindings. Bounds can refute an entire
-- equal-prefix case; only inconclusive/alias-sensitive cases use the old search.
def intervalPair (d : Domains) (op : Comparison) (x y : Term) : Bool :=
  BoundsProof.intervalPossible op true (lower d x) (upper d x) (lower d y) (upper d y)

theorem intervalPair_sound (d : Domains) (op : Comparison) (x y : Term)
    (a : Assignment) (hd : Compatible d a) (hs : op.eval (x.eval a) (y.eval a) = true) :
    intervalPair d op x y = true :=
  BoundsProof.intervalPossible_sound op true _ _ _ _ _ _
    (lower_sound d a hd x) (upper_sound d a hd x)
    (lower_sound d a hd y) (upper_sound d a hd y) hs

def quickLex (d : Domains) (expected : Bool) (xs ys : List Term) (strict : Bool) : Bool :=
  if xs.length != ys.length then !expected else
    match xs, ys with
    | [], [] => (!strict) == expected
    | x :: xs, y :: ys =>
      (if expected then intervalPair d .lt x y else intervalPair d .lt y x) ||
        (intervalPair d .eq x y && quickLex d expected xs ys strict)
    | _, _ => !expected
termination_by xs.length

theorem quickLex_sound (d : Domains) (expected : Bool) (xs ys : List Term) (strict : Bool)
    (a : Assignment) (hd : Compatible d a) (hs : (Constraint.lex xs ys strict).eval a = expected) :
    quickLex d expected xs ys strict = true := by
  induction xs generalizing ys with
  | nil => cases ys <;> cases expected <;> simp_all [quickLex, Constraint.eval, lexLess]
  | cons x xs ih =>
    cases ys with
    | nil => cases expected <;> simp_all [quickLex, Constraint.eval]
    | cons y ys =>
      by_cases hlen : xs.length = ys.length
      · have hshape : ((x :: xs).length != (y :: ys).length) = false := by simp [hlen]
        simp only [quickLex, hshape, Bool.false_eq_true, ↓reduceIte, Bool.or_eq_true]
        by_cases he : x.eval a = y.eval a
        · right
          have ht : (Constraint.lex xs ys strict).eval a = expected := by
            simpa [cons_eval a x y xs ys strict hlen, he] using hs
          simp only [Bool.and_eq_true]
          exact ⟨intervalPair_sound d .eq x y a hd (by simp [Comparison.eval, he]), ih ys ht⟩
        · left
          by_cases hl : x.eval a < y.eval a
          · have hexp : expected = true := by
              simpa [cons_eval a x y xs ys strict hlen, hl] using hs.symm
            simp only [hexp, ↓reduceIte]
            exact intervalPair_sound d .lt x y a hd (by simp [Comparison.eval, hl])
          · have hg : y.eval a < x.eval a := by omega
            have hexp : expected = false := by
              simpa [cons_eval a x y xs ys strict hlen, hl, he] using hs.symm
            simp only [hexp, Bool.false_eq_true, ↓reduceIte]
            exact intervalPair_sound d .lt y x a hd (by simp [Comparison.eval, hg])
      · have hb : (xs.length == ys.length) = false := Bool.eq_false_iff.mpr (fun h => hlen (beq_iff_eq.mp h))
        cases expected <;> simp_all [quickLex, Constraint.eval]

def possibleValue (d : Domains) (expected : Bool) : Constraint → Bool
  | .lex xs ys strict => if quickLex d expected xs ys strict then lexPossible d expected xs ys strict else false
  | _ => true

theorem possibleValue_sound (d : Domains) (expected : Bool) (c : Constraint) (a : Assignment)
    (hd : Compatible d a) (hs : c.eval a = expected) : possibleValue d expected c = true := by
  cases c <;> try rfl
  simp only [possibleValue, quickLex_sound d expected _ _ _ a hd hs, ↓reduceIte]
  exact lexPossible_sound d expected _ _ _ a hd hs

def supported : Constraint → Bool
  | .lex .. => true
  | _ => false

end Minion.LexProof
