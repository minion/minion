import Minion.GccProof
import Minion.CapacityProof
import Minion.AllDiffProof
import Minion.FlowWitness

namespace Minion.GccCutProof
open DomainProof SetProof

abbrev Entries := List (Int × Term)

def CountsHold (xs : List Term) (entries : Entries) (a : Assignment) : Prop :=
  ∀ p ∈ entries, ((xs.map (Term.eval a)).count p.1 : Int) = p.2.eval a

theorem countsHold_of_gcc (xs vs ns : List Term) (ws : List Int)
    (hw : constantValues vs = some ws) (a : Assignment)
    (hs : (Constraint.gcc xs vs ns).eval a = true) : CountsHold xs (ws.zip ns) a := by
  have hparts := hs
  simp only [Constraint.eval, Bool.and_eq_true] at hparts
  have hall : (vs.zip ns).all (fun (v, n) =>
      ((xs.filter (fun x => x.eval a == v.eval a)).length : Int) == n.eval a) = true :=
    hparts.2
  have he := constantValues_sound vs ws hw a
  intro p hp
  rw [← he, List.zip_map_left] at hp
  obtain ⟨q, hq, rfl⟩ := List.mem_map.mp hp
  have hc := List.all_eq_true.mp hall q hq
  change ((xs.map (Term.eval a)).count (q.1.eval a) : Int) = q.2.eval a
  rw [List.count_eq_countP, List.countP_map]
  simpa only [Function.comp_def, List.countP_eq_length_filter, beq_iff_eq] using hc

def upperCap (d : Domains) (xs : List Term) (entries : Entries) (v : Int) : Nat :=
  match entries.find? (fun p => p.1 == v) with
  | none => xs.length
  | some (_, count) => (upper d count).toNat

def lowerCap (d : Domains) (entries : Entries) (v : Int) : Nat :=
  match entries.find? (fun p => p.1 == v) with
  | none => 0
  | some (_, count) => (lower d count).toNat

theorem upperCap_sound (d : Domains) (xs : List Term) (entries : Entries)
    (v : Int) (a : Assignment) (hd : Compatible d a) (hc : CountsHold xs entries a) :
    (xs.map (Term.eval a)).count v ≤ upperCap d xs entries v := by
  cases hf : entries.find? (fun p => p.1 == v) with
  | none => simpa [upperCap, hf] using List.count_le_length (a := v) (l := xs.map (Term.eval a))
  | some p =>
    have hp := List.mem_of_find?_eq_some hf
    have hv : p.1 = v := by simpa using List.find?_some hf
    have he := hc p hp
    have hu := upper_sound d a hd p.2
    simp only [upperCap, hf]
    rw [hv] at he
    omega

theorem lowerCap_sound (d : Domains) (xs : List Term) (entries : Entries)
    (v : Int) (a : Assignment) (hd : Compatible d a) (hc : CountsHold xs entries a) :
    lowerCap d entries v ≤ (xs.map (Term.eval a)).count v := by
  cases hf : entries.find? (fun p => p.1 == v) with
  | none => simp [lowerCap, hf]
  | some p =>
    have hp := List.mem_of_find?_eq_some hf
    have hv : p.1 = v := by simpa using List.find?_some hf
    have he := hc p hp
    have hl := lower_sound d a hd p.2
    simp only [lowerCap, hf]
    rw [hv] at he
    omega

def upperCut (d : Domains) (xs : List Term) (entries : Entries) (indices : List Nat) : Bool :=
  let chosen := AllDiffProof.select xs indices
  if chosen.any (fun t => (values d t).any (fun v => upperCap d xs entries v ≥ chosen.length))
  then false else
    let vs := (chosen.flatMap (values d)).eraseDups
    (vs.map (upperCap d xs entries)).sum < chosen.length

theorem upperCut_excludes (d : Domains) (xs : List Term) (entries : Entries)
    (indices : List Nat) (a : Assignment) (hd : Compatible d a)
    (hc : CountsHold xs entries a) (hcut : upperCut d xs entries indices = true) : False := by
  let chosen := AllDiffProof.select xs indices
  let vs := (chosen.flatMap (values d)).eraseDups
  have hcover : chosen.map (Term.eval a) ⊆ vs := by
    intro v hv
    obtain ⟨t, ht, rfl⟩ := List.mem_map.mp hv
    exact List.mem_eraseDups.mpr (List.mem_flatMap.mpr ⟨t, ht, values_sound d a hd t⟩)
  have hb := CapacityProof.upper_cut (xs.map (Term.eval a)) (chosen.map (Term.eval a)) vs
    (upperCap d xs entries) ((AllDiffProof.select_sublist xs indices).map (Term.eval a))
    hcover (eraseDups_nodup _) (fun v _ => upperCap_sound d xs entries v a hd hc)
  have hbad : (vs.map (upperCap d xs entries)).sum < chosen.length := by
    unfold upperCut at hcut
    dsimp only at hcut
    split at hcut
    · contradiction
    · simpa only [chosen, vs, decide_eq_true_eq] using hcut
  simp only [List.length_map] at hb
  omega

def lowerCut (d : Domains) (xs : List Term) (entries : Entries) (vs : List Int) : Bool :=
  let chosen := vs.eraseDups
  xs.countP (fun t => (values d t).any chosen.contains) < (chosen.map (lowerCap d entries)).sum

theorem lowerCut_excludes (d : Domains) (xs : List Term) (entries : Entries)
    (vs : List Int) (a : Assignment) (hd : Compatible d a)
    (hc : CountsHold xs entries a) (hcut : lowerCut d xs entries vs = true) : False := by
  let chosen := vs.eraseDups
  have hl := CapacityProof.mass_lower (xs.map (Term.eval a)) chosen (lowerCap d entries)
    (fun v _ => lowerCap_sound d xs entries v a hd hc)
  rw [CapacityProof.mass_partition _ _ (eraseDups_nodup vs), List.countP_map] at hl
  have hu : xs.countP (fun t => chosen.contains (t.eval a)) ≤
      xs.countP (fun t => (values d t).any chosen.contains) := by
    apply List.countP_mono_left
    intro t _ ht
    exact List.any_eq_true.mpr ⟨t.eval a, values_sound d a hd t, ht⟩
  have hbad : xs.countP (fun t => (values d t).any chosen.contains) <
      (chosen.map (lowerCap d entries)).sum := by
    simpa only [lowerCut, chosen, decide_eq_true_eq] using hcut
  exact Nat.not_lt_of_ge (Nat.le_trans (by simpa [chosen, Function.comp_def] using hl) hu) hbad

/-! The slot matching below only proposes cuts. Optional slots are available to
    dummy rows; a deficient row set suggests either an upper-capacity cut on
    its actual variables or a lower-demand cut on the complementary values. -/
def discoverSlots (d : Domains) (xs : List Term) (entries : Entries) : Option (List Nat × List Int) :=
  let valueSet := ((xs.flatMap (values d)) ++ entries.map Prod.fst).eraseDups
  let slots := valueSet.flatMap fun v =>
    (List.range (min xs.length (upperCap d xs entries v))).map fun k => (v, k)
  let numbered := slots.zipIdx
  let actual := xs.map fun t =>
    (numbered.filter fun p => (values d t).contains p.1.1).map fun p => (p.2 : Int)
  let optional := (numbered.filter fun p => lowerCap d entries p.1.1 ≤ p.1.2).map fun p => (p.2 : Int)
  match Matching.discover (actual ++ List.replicate (slots.length - xs.length) optional) with
  | none => none
  | some rows =>
    let indices := rows.filter fun i => i < xs.length
    let covered := (AllDiffProof.select xs indices).flatMap (values d)
    some (indices, valueSet.filter fun v => !covered.contains v)

def slotCutsPossible (d : Domains) (xs : List Term) (entries : Entries) : Bool :=
  match discoverSlots d xs entries with
  | none => true
  | some (indices, vs) => !upperCut d xs entries indices && !lowerCut d xs entries vs

def discoveredCutPossible (d : Domains) (xs : List Term) (entries : Entries)
    (result : FlowWitness.Outcome) : Bool :=
  match result with
  | .matched _ => true
  | .lower vs => !lowerCut d xs entries vs
  | .upper rows => !upperCut d xs entries rows

def cutsPossible (d : Domains) (xs : List Term) (entries : Entries) : Bool :=
  !upperCut d xs entries (List.range xs.length) &&
  !lowerCut d xs entries (entries.map Prod.fst) &&
  let domains := xs.map (values d)
  discoveredCutPossible d xs entries
    (FlowWitness.discover domains (entries.map Prod.fst)
      (lowerCap d entries) (upperCap d xs entries)) &&
  -- Retain the older discovery on small instances. Budget exhaustion only
  -- declines to propose an additional cut; it never asserts inconsistency.
  if xs.length * (domains.map List.length).sum ≤ 4096
  then slotCutsPossible d xs entries else true

theorem cutsPossible_sound (d : Domains) (xs : List Term) (entries : Entries)
    (a : Assignment) (hd : Compatible d a) (hc : CountsHold xs entries a) :
    cutsPossible d xs entries = true := by
  have hu (indices) : upperCut d xs entries indices = false := by
    cases h : upperCut d xs entries indices with
    | false => rfl
    | true => exact False.elim (upperCut_excludes d xs entries indices a hd hc h)
  have hl (vs) : lowerCut d xs entries vs = false := by
    cases h : lowerCut d xs entries vs with
    | false => rfl
    | true => exact False.elim (lowerCut_excludes d xs entries vs a hd hc h)
  have hflow (result) : discoveredCutPossible d xs entries result = true := by
    cases result <;> simp [discoveredCutPossible, hu, hl]
  have hslots : slotCutsPossible d xs entries = true := by
    unfold slotCutsPossible
    cases discoverSlots d xs entries with
    | none => rfl
    | some p => simp [hu, hl]
  simp only [cutsPossible, hu, hl, hflow, hslots, Bool.not_false, Bool.true_and]
  split <;> rfl

def possibleValue (d : Domains) (expected : Bool) (c : Constraint) : Bool :=
  GccProof.possibleValue d expected c &&
  match c with
  | .gcc xs vs ns =>
    if expected then
      match constantValues vs with
      | none => true
      | some ws =>
        let entries := ws.zip ns
        cutsPossible d xs entries && entries.all fun (_, n) =>
          if (values d n).eraseDups.length ≥ (upper d n - lower d n + 1).toNat then true
          else (values d n).any fun r => cutsPossible (bindTerm d n r) xs entries
    else true
  | _ => true

theorem possibleValue_sound (d : Domains) (expected : Bool) (c : Constraint) (a : Assignment)
    (hd : Compatible d a) (hs : c.eval a = expected) : possibleValue d expected c = true := by
  simp only [possibleValue, Bool.and_eq_true]
  constructor
  · exact GccProof.possibleValue_sound d expected c a hd hs
  · cases c <;> try rfl
    rename_i xs vs ns
    cases expected with
    | false => rfl
    | true =>
      cases hw : constantValues vs with
      | none => simp [hw]
      | some ws =>
        simp only [↓reduceIte, hw]
        have hc := countsHold_of_gcc xs vs ns ws hw a hs
        simp only [Bool.and_eq_true]
        constructor
        · exact cutsPossible_sound d xs (ws.zip ns) a hd hc
        · apply List.all_eq_true.mpr
          intro p _
          split
          · rfl
          · exact List.any_eq_true.mpr ⟨p.2.eval a, values_sound d a hd p.2,
              cutsPossible_sound (bindTerm d p.2 (p.2.eval a)) xs (ws.zip ns) a
                (compatible_bindTerm d p.2 a hd) hc⟩

def supported := GccProof.supported

end Minion.GccCutProof
