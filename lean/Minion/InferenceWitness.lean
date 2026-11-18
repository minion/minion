import Minion.LogicProof

/-! Supplied cuts are data, not trusted assumptions. The validators below do
    no matching/flow discovery and reuse the fixed Hall/capacity theorems. -/
namespace Minion.InferenceWitness
open DomainProof

inductive Evidence where
  | hall (positions : List Nat)
  | gccUpper (positions : List Nat)
  | gccLower (values : List Int)
  | focus (path : List Nat)
  | preferFocus (path : List Nat)
  | bounds
deriving Repr, BEq

-- A saturated set becomes deficient when an outside position is forced to a
-- forbidden value. Resolve that position from the semantic terms, not a claimed
-- runtime argument order. Any candidate subset still has to pass the cut test.
def extend (xs : List Term) (positions : List Nat) (target : Option Nat) : List Nat :=
  match target with
  | none => positions
  | some target =>
    -- Include every occurrence: a domain operation also changes aliased views.
    (xs.zipIdx.filter (fun p => p.1.baseIndex == some target)).map Prod.snd ++ positions

def check (d : Domains) (c : Constraint) (e : Evidence) (target : Option Nat := none) : Bool :=
  match e, c with
  | .focus path, c => !LogicProof.possibleAt d true c path
  | .preferFocus path, c => !LogicProof.possibleAt d true c path || LogicProof.checkConflict d c
  | .bounds, c => !BoundsProof.quickPossible d true c
  | .hall positions, .allDifferent xs => AllDiffProof.hallCheck d xs (extend xs positions target)
  | .gccUpper positions, .gcc xs vs ns =>
    match constantValues vs with
    | none => false
    | some ws => GccCutProof.upperCut d xs (ws.zip ns) (extend xs positions target)
  | .gccLower values, .gcc xs vs ns =>
    match constantValues vs with
    | none => false
    | some ws => GccCutProof.lowerCut d xs (ws.zip ns) values
  | _, _ => false

theorem check_excludes (d : Domains) (c : Constraint) (e : Evidence) (target : Option Nat)
    (hc : check d c e target = true) (a : Assignment) (hd : Compatible d a)
    (hs : c.eval a = true) : False := by
  cases e with
  | focus path =>
    have hp := LogicProof.possibleAt_sound d true c path a hd hs
    simp [check, hp] at hc
  | preferFocus path =>
    have hp := LogicProof.possibleAt_sound d true c path a hd hs
    have hl := LogicProof.possible_sound d true c a hd hs
    simp [check, hp, LogicProof.checkConflict, hl] at hc
  | bounds =>
    have hp := BoundsProof.quickPossible_sound d true c a hd hs
    simp [check, hp] at hc
  | hall positions =>
    cases c <;> try { simp [check] at hc }
    rename_i xs
    have hn : (xs.map (Term.eval a)).Nodup := SetProof.nodup_of_card_eq_length _
      (by simpa only [Constraint.eval, SetProof.card, beq_iff_eq] using hs)
    exact AllDiffProof.hallCheck_excludes d xs (extend xs positions target) a hd hn hc
  | gccUpper positions =>
    cases c <;> try { simp [check] at hc }
    rename_i xs vs ns
    cases hw : constantValues vs with
    | none => simp [check, hw] at hc
    | some ws =>
      exact GccCutProof.upperCut_excludes d xs (ws.zip ns) (extend xs positions target) a hd
        (GccCutProof.countsHold_of_gcc xs vs ns ws hw a hs) (by simpa [check, hw] using hc)
  | gccLower values =>
    cases c <;> try { simp [check] at hc }
    rename_i xs vs ns
    cases hw : constantValues vs with
    | none => simp [check, hw] at hc
    | some ws =>
      exact GccCutProof.lowerCut_excludes d xs (ws.zip ns) values a hd
        (GccCutProof.countsHold_of_gcc xs vs ns ws hw a hs) (by simpa [check, hw] using hc)

def rule (e : Evidence) (target : Option Nat := none) : LocalProof.Rule :=
  ⟨fun d c => check d c e target, fun d c hc a hd hs => check_excludes d c e target hc a hd hs⟩

end Minion.InferenceWitness
