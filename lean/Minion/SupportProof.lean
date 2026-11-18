import Minion.LocalProof
import Minion.Views
import Minion.PowerProof
import Minion.DomainOverlap

/-! Small local support rules. Candidate bindings preserve correlations between
    repeated variables and their views, without distinctness assumptions.
    These checks range over a relation's arguments, not all model assignments. -/
namespace Minion.SupportProof
open DomainProof

def possible2 (d : Domains) (x y : Term) (relation : Int → Int → Bool) : Bool :=
  (values d x).any fun xv => (values (bindTerm d x xv) y).any fun yv => relation xv yv

theorem possible2_sound (d : Domains) (x y : Term) (relation : Int → Int → Bool)
    (a : Assignment) (hd : Compatible d a) (hs : relation (x.eval a) (y.eval a) = true) :
    possible2 d x y relation = true :=
  List.any_eq_true.mpr ⟨x.eval a, values_sound d a hd x,
    List.any_eq_true.mpr ⟨y.eval a, values_sound _ a (compatible_bindTerm d x a hd) y, hs⟩⟩

def possible3 (d : Domains) (x y z : Term) (relation : Int → Int → Int → Bool) : Bool :=
  (values d x).any fun xv =>
    let dx := bindTerm d x xv
    (values dx y).any fun yv =>
      (values (bindTerm dx y yv) z).any fun zv => relation xv yv zv

theorem possible3_sound (d : Domains) (x y z : Term) (relation : Int → Int → Int → Bool)
    (a : Assignment) (hd : Compatible d a) (hs : relation (x.eval a) (y.eval a) (z.eval a) = true) :
    possible3 d x y z relation = true :=
  List.any_eq_true.mpr ⟨x.eval a, values_sound d a hd x,
    List.any_eq_true.mpr ⟨y.eval a, values_sound _ a (compatible_bindTerm d x a hd) y,
      List.any_eq_true.mpr ⟨z.eval a,
        values_sound _ a (compatible_bindTerm _ y a (compatible_bindTerm d x a hd)) z, hs⟩⟩⟩

/-- Candidate intersection is only a speedup. Binding the selected view and
    checking the other one retains correlations between aliases and views. -/
def possibleEqual (d : Domains) (x y : Term) : Bool :=
  ((values d x).head?).any (fun v => (values (bindTerm d x v) y).contains v) ||
    (DomainOverlap.candidates (values d x) (values d y)).any fun v =>
      (values (bindTerm d x v) y).contains v

theorem possibleEqual_sound (d : Domains) (x y : Term) (a : Assignment)
    (hd : Compatible d a) (he : x.eval a = y.eval a) : possibleEqual d x y = true := by
  simp only [possibleEqual, Bool.or_eq_true]
  right
  refine List.any_eq_true.mpr ⟨x.eval a, ?_, ?_⟩
  · exact DomainOverlap.candidates_complete _ _ _ (values_sound d a hd x)
      (by rw [he]; exact values_sound d a hd y)
  · have hy := values_sound _ a (compatible_bindTerm d x a hd) y
    simpa only [List.contains_iff_mem, he] using hy

def possibleCompare (d : Domains) (x y : Term) (op : Comparison) (expected : Bool) : Bool :=
  match op, expected with
  | .eq, true | .ne, false => possibleEqual d x y
  | _, _ => possible2 d x y (fun xv yv => op.eval xv yv == expected)

theorem possibleCompare_sound (d : Domains) (x y : Term) (op : Comparison) (expected : Bool)
    (a : Assignment) (hd : Compatible d a) (hs : op.eval (x.eval a) (y.eval a) = expected) :
    possibleCompare d x y op expected = true := by
  cases op <;> cases expected
  all_goals first
    | exact possibleEqual_sound d x y a hd (by simpa [Comparison.eval] using hs)
    | exact possible2_sound d x y _ a hd (by simp [hs])

/-- Unsupported relations return true, so they can never establish conflict. -/
def possibleValue (d : Domains) (expected : Bool) : Constraint → Bool
  | .truth b => b == expected
  | .compare op x y => possibleCompare d x y op expected
  | .minusEq x y => possibleCompare d x (.neg y) .eq expected
  | .absolute x y => possible2 d x y (fun xv yv => (xv == yv.natAbs) == expected)
  | .difference x y z => possible3 d x y z (fun xv yv zv => (((xv - yv).natAbs : Int) == zv) == expected)
  | .product x y z => possible3 d x y z (fun xv yv zv => (xv * yv == zv) == expected)
  | .divide x y z undefZero => possible3 d x y z (fun xv yv zv =>
      (if yv == 0 then undefZero && zv == 0 else xv.fdiv yv == zv) == expected)
  | .modulo x y z undefZero => possible3 d x y z (fun xv yv zv =>
      (if yv == 0 then undefZero && zv == 0 else xv.fmod yv == zv) == expected)
  | .power x y z => possible3 d x y z (fun xv yv zv => PowerProof.boundedEq xv yv zv == expected)
  | .inequality x y k => possible3 d x y k (fun xv yv kv => (xv ≤ yv + kv) == expected)
  | .inSet x vs positive =>
      match constantValues vs with
      | some ws => (values d x).any fun v => ((ws.any (fun w => w == v)) == positive) == expected
      | none => true
  | .inIntervals x bounds positive =>
      match constantValues bounds with
      | some bs => (values d x).any fun v => (containsInterval v bs == positive) == expected
      | none => true
  | _ => true

theorem possibleValue_sound (d : Domains) (expected : Bool) (c : Constraint) (a : Assignment)
    (hd : Compatible d a) (hs : c.eval a = expected) : possibleValue d expected c = true := by
  have hvalue : (c.eval a == expected) = true := by simp [hs]
  clear hs
  have hs := hvalue
  clear hvalue
  cases c <;> try rfl
  all_goals simp only [Constraint.eval] at hs
  · exact hs
  · rename_i op x y
    exact possibleCompare_sound d x y op expected a hd (by simpa using hs)
  · exact possible2_sound _ _ _ _ a hd hs
  · exact possible3_sound _ _ _ _ _ a hd hs
  · rename_i x y
    exact possibleCompare_sound d x (.neg y) .eq expected a hd
      (by simpa [Comparison.eval, Term.eval] using hs)
  · exact possible3_sound _ _ _ _ _ a hd hs
  · exact possible3_sound _ _ _ _ _ a hd hs
  · rename_i x y z
    simpa only [possibleValue, PowerProof.boundedEq_eq] using
      possible3_sound d x y z (fun xv yv zv => powerEq xv yv zv == expected) a hd hs
  · exact possible3_sound _ _ _ _ _ a hd hs
  · exact possible3_sound _ _ _ _ _ a hd hs
  · rename_i x vs positive
    cases hvs : constantValues vs with
    | none => simp [possibleValue, hvs]
    | some ws =>
      have he := constantValues_sound vs ws hvs a
      have hany : vs.any (fun t => t.eval a == x.eval a) = ws.any (fun v => v == x.eval a) := by
        rw [← he]
        simp only [List.any_map, Function.comp_def]
      rw [hany] at hs
      simp only [possibleValue, hvs]
      exact List.any_eq_true.mpr ⟨x.eval a, values_sound d a hd x, hs⟩
  · rename_i x bounds positive
    cases hb : constantValues bounds with
    | none => simp [possibleValue, hb]
    | some bs =>
      have he := constantValues_sound bounds bs hb a
      rw [he] at hs
      simp only [possibleValue, hb]
      exact List.any_eq_true.mpr ⟨x.eval a, values_sound d a hd x, hs⟩

def possible (d : Domains) (c : Constraint) : Bool := possibleValue d true c

theorem possible_sound (d : Domains) (c : Constraint) (a : Assignment)
    (hd : Compatible d a) (hs : c.eval a = true) : possible d c = true :=
  possibleValue_sound d true c a hd hs

def checkConflict (d : Domains) (c : Constraint) : Bool := !possible d c

theorem checkConflict_sound (d : Domains) (c : Constraint) (hc : checkConflict d c = true)
    (a : Assignment) (hd : Compatible d a) (hs : c.eval a = true) : False := by
  have hp := possible_sound d c a hd hs
  simp [checkConflict, hp] at hc

def rule : LocalProof.Rule := ⟨checkConflict, checkConflict_sound⟩

def supported : Constraint → Bool
  | .truth _ | .compare .. | .minusEq .. | .absolute .. | .inequality .. => true
  | .difference .. | .product .. | .divide .. | .modulo .. | .power .. => true
  | .inSet _ vs _ | .inIntervals _ vs _ => (constantValues vs).isSome
  | _ => false

end Minion.SupportProof
