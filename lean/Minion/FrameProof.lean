import Minion.BoundedProof

namespace Minion.FrameProof
open DomainProof NvalueProof

theorem frameBlocks_map {α β : Type} (xs : List α) (f : α → β) (removed : List Int) (bs : Int) :
    frameBlocks (xs.map f) removed bs = (frameBlocks xs removed bs).map (List.map f) := by
  simp [frameBlocks, List.map_map, List.map_take, List.map_drop]

def listEqPossible (d : Domains) (xs ys : List Term) : Bool :=
  xs.length == ys.length && (xs.zip ys).all fun (x, y) => SupportProof.possible2 d x y Comparison.eq.eval

theorem listEqPossible_sound (d : Domains) (xs ys : List Term) (a : Assignment)
    (hd : Compatible d a) (hs : xs.map (Term.eval a) = ys.map (Term.eval a)) :
    listEqPossible d xs ys = true := by
  induction xs generalizing ys with
  | nil => cases ys <;> simp_all [listEqPossible]
  | cons x xs ih =>
    cases ys with
    | nil => simp at hs
    | cons y ys =>
      have hparts := List.cons.inj (by simpa only [List.map_cons] using hs)
      have hr := ih ys hparts.2
      have hp := SupportProof.possible2_sound d x y Comparison.eq.eval a hd (by simp [Comparison.eval, hparts.1])
      simpa [listEqPossible, hp] using hr

def rowsPossible (d : Domains) (src tgt : List Term) (rs rt : List Int) (bs : Int) : Bool :=
  if frameInvalid src.length tgt.length rs rt bs then false else
    ((frameBlocks src rs bs).zip (frameBlocks tgt rt bs)).all fun (xs, ys) => listEqPossible d xs ys

theorem rowsPossible_sound (d : Domains) (src tgt : List Term) (rs rt : List Int) (bs : Int)
    (a : Assignment) (hd : Compatible d a)
    (hs : frameCheck (src.map (Term.eval a)) (tgt.map (Term.eval a)) rs rt bs = true) :
    rowsPossible d src tgt rs rt bs = true := by
  simp only [frameCheck, List.length_map, frameBlocks_map] at hs
  cases hi : frameInvalid src.length tgt.length rs rt bs with
  | true => simp [hi] at hs
  | false =>
    simp only [hi, Bool.false_eq_true, ↓reduceIte] at hs
    simp only [rowsPossible, hi, Bool.false_eq_true, ↓reduceIte]
    apply List.all_eq_true.mpr
    intro p hp
    have hmap : (p.1.map (Term.eval a), p.2.map (Term.eval a)) ∈
        ((frameBlocks src rs bs).map (List.map (Term.eval a))).zip
          ((frameBlocks tgt rt bs).map (List.map (Term.eval a))) := by
      rw [List.zip_map]
      exact List.mem_map.mpr ⟨p, hp, rfl⟩
    have he := List.all_eq_true.mp hs _ hmap
    exact listEqPossible_sound d p.1 p.2 a hd (by simpa using he)

def fixed (d : Domains) (t : Term) : Int := (singletonValue d t).getD 0

theorem fixed_sound (d : Domains) (controls : List Term) (t : Term)
    (ht : t ∈ controls) (a : Assignment) (hd : Compatible d a)
    (hc : BoundedProof.unresolved d controls = none) : fixed d t = t.eval a := by
  have hn := List.find?_eq_none.mp hc t ht
  cases hs : singletonValue d t with
  | none => simp [hs] at hn
  | some v => simpa [fixed, hs] using (singletonValue_sound d t v hs a hd).symm

def positivePossible (d : Domains) (src tgt rs rt : List Term) (bs : Term) : Bool :=
  match BoundedProof.unresolved d (rs ++ (rt ++ [bs])) with
  | some _ => true
  | none => rowsPossible d src tgt (rs.map (fixed d)) (rt.map (fixed d)) (fixed d bs)

theorem positivePossible_sound (d : Domains) (src tgt rs rt : List Term) (bs : Term)
    (a : Assignment) (hd : Compatible d a)
    (hs : frameCheck (src.map (Term.eval a)) (tgt.map (Term.eval a))
      (rs.map (Term.eval a)) (rt.map (Term.eval a)) (bs.eval a) = true) :
    positivePossible d src tgt rs rt bs = true := by
  cases hc : BoundedProof.unresolved d (rs ++ (rt ++ [bs])) with
  | some _ => simp [positivePossible, hc]
  | none =>
    have hfixed (t) (ht : t ∈ rs ++ (rt ++ [bs])) := fixed_sound d _ t ht a hd hc
    have hrs : rs.map (fixed d) = rs.map (Term.eval a) :=
      List.map_congr_left fun t ht => hfixed t (by simp [ht])
    have hrt : rt.map (fixed d) = rt.map (Term.eval a) :=
      List.map_congr_left fun t ht => hfixed t (by simp [ht])
    have hb : fixed d bs = bs.eval a := hfixed bs (by simp)
    simpa [positivePossible, hc, hrs, hrt, hb] using
      rowsPossible_sound d src tgt (rs.map (Term.eval a)) (rt.map (Term.eval a)) (bs.eval a) a hd hs

def fromArguments (srcLength tgtLength rsLength rtLength : Nat) (args : List Int) : Bool :=
  let src := args.take srcLength
  let rest := args.drop srcLength
  let tgt := rest.take tgtLength
  let rest := rest.drop tgtLength
  let rs := rest.take rsLength
  let rest := rest.drop rsLength
  let rt := rest.take rtLength
  let bs := (rest.drop rtLength).headD 0
  frameCheck src tgt rs rt bs

theorem fromArguments_eval (src tgt rs rt : List Term) (bs : Term) (a : Assignment) :
    fromArguments src.length tgt.length rs.length rt.length
      ((src ++ (tgt ++ (rs ++ (rt ++ [bs])))).map (Term.eval a)) =
    frameCheck (src.map (Term.eval a)) (tgt.map (Term.eval a))
      (rs.map (Term.eval a)) (rt.map (Term.eval a)) (bs.eval a) := by
  simp only [fromArguments, List.map_append, List.map_cons, List.map_nil]
  simp only [← List.length_map (f := Term.eval a), List.take_left, List.drop_left,
    List.headD_cons]

def possibleValue (d : Domains) (expected : Bool) : Constraint → Bool
  | .frameUpdate src tgt rs rt bs =>
    if expected then positivePossible d src tgt rs rt bs else
      BoundedProof.possible d (src ++ (tgt ++ (rs ++ (rt ++ [bs]))))
        (fun args => !fromArguments src.length tgt.length rs.length rt.length args) 1
  | _ => true

theorem possibleValue_sound (d : Domains) (expected : Bool) (c : Constraint) (a : Assignment)
    (hd : Compatible d a) (hs : c.eval a = expected) : possibleValue d expected c = true := by
  cases c <;> try rfl
  rename_i src tgt rs rt bs
  simp only [Constraint.eval] at hs
  cases expected with
  | true => exact positivePossible_sound d src tgt rs rt bs a hd hs
  | false =>
    simp only [possibleValue, Bool.false_eq_true, ↓reduceIte]
    apply BoundedProof.possible_sound d _ _ 1 a hd
    simp only [fromArguments_eval, hs, Bool.not_false]

def supported : Constraint → Bool
  | .frameUpdate .. => true
  | _ => false

end Minion.FrameProof
