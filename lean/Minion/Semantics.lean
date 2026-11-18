import Lean

/-! Assignment semantics only: no search orders, propagation algorithms or mutable state.
    Distinct propagators implementing the same relation share a constructor. -/
namespace Minion

abbrev Assignment := List Int

structure Variable where
  name : String
  domain : List Int
  deriving Repr, BEq

inductive Term where
  | var (index : Nat)
  | constant (value : Int)
  | notBool (index : Nat)
  | neg (term : Term)
  | shift (term : Term) (offset : Int)
  | scale (term : Term) (factor : Int)
  deriving Repr, BEq

def Term.eval (a : Assignment) : Term → Int
  | .var i => a[i]?.getD 0
  | .constant v => v
  | .notBool i => 1 - a[i]?.getD 0
  | .neg t => -t.eval a
  | .shift t k => t.eval a + k
  | .scale t k => k * t.eval a

def Term.wellFormed (vars : List Variable) : Term → Bool
  | .constant _ => true
  | .var i => i < vars.length
  | .notBool i => i < vars.length &&
      (vars[i]?.getD ⟨"", []⟩).domain.all (fun x => x == 0 || x == 1)
  | .neg t => t.wellFormed vars
  | .shift t _ => t.wellFormed vars
  | .scale t _ => t.wellFormed vars

inductive Comparison where
  | eq | ne | lt | le | ge
  deriving Repr, BEq

def Comparison.eval : Comparison → Int → Int → Bool
  | .eq, x, y => x == y
  | .ne, x, y => x != y
  | .lt, x, y => x < y
  | .le, x, y => x ≤ y
  | .ge, x, y => x ≥ y

inductive Constraint where
  | truth (value : Bool)
  | compare (op : Comparison) (x y : Term)
  | absolute (x y : Term)
  | difference (x y z : Term)
  | minusEq (x y : Term)
  | divide (x y z : Term) (undefZero : Bool)
  | modulo (x y z : Term) (undefZero : Bool)
  | power (x y z : Term)
  | product (x y z : Term)
  | inequality (x y offset : Term)
  | allDifferent (xs : List Term)
  | element (xs : List Term) (index rhs : Term) (offset : Int) (undefZero : Bool)
  | lex (xs ys : List Term) (strict : Bool)
  | sum (xs : List Term) (rhs : Term) (op : Comparison)
  | weightedSum (weights xs : List Term) (rhs : Term) (op : Comparison)
  | occurrence (xs : List Term) (value count : Term) (op : Comparison)
  | nvalue (xs : List Term) (count : Term) (op : Comparison)
  | extremum (xs : List Term) (rhs : Term) (isMax : Bool)
  | hamming (xs ys : List Term) (count : Term) (op : Comparison)
  | literalSum (xs values : List Term) (count : Term)
  | inSet (x : Term) (values : List Term) (positive : Bool)
  | inIntervals (x : Term) (bounds : List Term) (positive : Bool)
  | vectorExists (xs ys : List Term) (op : Comparison)
  | gcc (xs values counts : List Term)
  | allDifferentMatrix (xs : List Term) (value : Term)
  | frameUpdate (source target removedSource removedTarget : List Term) (blockSize : Term)
  | table (xs : List Term) (tuples : List (List Int)) (positive : Bool)
  | shortTable (xs : List Term) (tuples : List (List (Nat × Int))) (compressed : Bool)
  | reify (child : Constraint) (guard : Term)
  | imply (child : Constraint) (guard : Term)
  | conjunction (children : List Constraint)
  | disjunction (children : List Constraint)
  deriving Repr

def lexLess : List Int → List Int → Bool
  | [], [] => false
  | [], _ :: _ => true
  | _ :: _, [] => false
  | x :: xs, y :: ys => x < y || (x == y && lexLess xs ys)

def containsInterval (x : Int) : List Int → Bool
  | lo :: hi :: rest => (lo ≤ x && x ≤ hi) || containsInterval x rest
  | _ => false

def powerEq (x exponent result : Int) : Bool :=
  if exponent < 0 then
    (x == 1 && result == 1) || (x == -1 && result == (if exponent % 2 == 0 then 1 else -1))
  else x ^ exponent.toNat == result

def shortRowCheck (xs : List Int) (tuple : List (Nat × Int)) (compressed : Bool) : Bool :=
  if compressed then
    (tuple.map Prod.fst).eraseDups.all fun i => tuple.any fun (j, v) =>
      i == j && xs[i]? == some v
  else tuple.all fun (i, v) => xs[i]? == some v

def shortTableCheck (xs : List Int) (tuples : List (List (Nat × Int))) (compressed : Bool) : Bool :=
  tuples.any fun tuple => shortRowCheck xs tuple compressed

def matrixCheck (xs : List Int) (v : Int) : Bool := Id.run do
  let n := (List.range (xs.length + 1)).find? (fun n => n * n == xs.length)
  match n with
  | none => return false
  | some n =>
    return (List.range n).all fun i =>
      ((List.range n).filter (fun j => xs[i * n + j]? == some v)).length == 1 &&
      ((List.range n).filter (fun j => xs[j * n + i]? == some v)).length == 1

def frameInvalid (srcLength tgtLength : Nat) (removedSrc removedTgt : List Int) (bs : Int) : Bool :=
  let validIndices := fun (indices : List Int) (n : Nat) =>
    indices.all (fun i => 0 < i && i ≤ (n : Int)) && indices.eraseDups.length == indices.length
  bs ≤ 0 || srcLength != tgtLength || srcLength % bs.toNat != 0 ||
    !validIndices removedSrc srcLength || !validIndices removedTgt tgtLength

def frameBlocks {α : Type} (xs : List α) (removed : List Int) (bs : Int) : List (List α) :=
  ((List.range (xs.length / bs.toNat)).filter
    (fun i => !removed.contains (Int.ofNat (i + 1)))).map
    (fun i => (xs.drop (i * bs.toNat)).take bs.toNat)

def frameCheck (src tgt removedSrc removedTgt : List Int) (bs : Int) : Bool :=
  if frameInvalid src.length tgt.length removedSrc removedTgt bs then false
  else
    -- Minion compares the surviving prefixes; one side may have trailing blocks.
    ((frameBlocks src removedSrc bs).zip (frameBlocks tgt removedTgt bs)).all (fun (x, y) => x == y)

def Constraint.eval (a : Assignment) : Constraint → Bool
  | .truth b => b
  | .compare op x y => op.eval (x.eval a) (y.eval a)
  | .absolute x y => x.eval a == (y.eval a).natAbs
  | .difference x y z => ((x.eval a - y.eval a).natAbs : Int) == z.eval a
  | .minusEq x y => x.eval a == -y.eval a
  | .divide x y z undefZero =>
      if y.eval a == 0 then undefZero && z.eval a == 0
      else (x.eval a).fdiv (y.eval a) == z.eval a
  | .modulo x y z undefZero =>
      if y.eval a == 0 then undefZero && z.eval a == 0
      else (x.eval a).fmod (y.eval a) == z.eval a
  | .power x y z => powerEq (x.eval a) (y.eval a) (z.eval a)
  | .product x y z => x.eval a * y.eval a == z.eval a
  | .inequality x y k => x.eval a ≤ y.eval a + k.eval a
  | .allDifferent xs => let vs := xs.map (Term.eval a); vs.eraseDups.length == vs.length
  | .element xs index rhs offset undefZero =>
      let i := index.eval a - offset
      if i < 0 || i ≥ (xs.length : Int) then undefZero && rhs.eval a == 0
      else (xs[i.toNat]?.map (Term.eval a)) == some (rhs.eval a)
  | .lex xs ys strict =>
      let x := xs.map (Term.eval a); let y := ys.map (Term.eval a)
      xs.length == ys.length && (lexLess x y || (!strict && x == y))
  | .sum xs rhs op => op.eval ((xs.map (Term.eval a)).foldl (· + ·) 0) (rhs.eval a)
  | .weightedSum ws xs rhs op => ws.length == xs.length &&
      op.eval (((ws.zip xs).map (fun (w, x) => w.eval a * x.eval a)).foldl (· + ·) 0) (rhs.eval a)
  | .occurrence xs v n op =>
      op.eval ((xs.filter (fun x => x.eval a == v.eval a)).length : Int) (n.eval a)
  | .nvalue xs n op => op.eval ((xs.map (Term.eval a)).eraseDups.length : Int) (n.eval a)
  | .extremum xs rhs isMax =>
      match xs.map (Term.eval a) with
      | [] => false
      | x :: rest => rest.foldl (if isMax then max else min) x == rhs.eval a
  | .hamming xs ys n op => xs.length == ys.length &&
      op.eval (((xs.zip ys).filter (fun (x, y) => x.eval a != y.eval a)).length : Int) (n.eval a)
  | .literalSum xs ys n => xs.length == ys.length &&
      (((xs.zip ys).filter (fun (x, y) => x.eval a == y.eval a)).length : Int) ≥ n.eval a
  | .inSet x vs positive => (vs.any (fun v => v.eval a == x.eval a)) == positive
  | .inIntervals x bounds positive => (containsInterval (x.eval a) (bounds.map (Term.eval a))) == positive
  | .vectorExists xs ys op => xs.length == ys.length &&
      (xs.zip ys).any (fun (x, y) => op.eval (x.eval a) (y.eval a))
  | .gcc xs vs ns => vs.length == ns.length && (vs.zip ns).all (fun (v, n) =>
      ((xs.filter (fun x => x.eval a == v.eval a)).length : Int) == n.eval a)
  | .allDifferentMatrix xs v => matrixCheck (xs.map (Term.eval a)) (v.eval a)
  | .frameUpdate src tgt rs rt bs => frameCheck (src.map (Term.eval a)) (tgt.map (Term.eval a))
      (rs.map (Term.eval a)) (rt.map (Term.eval a)) (bs.eval a)
  | .table xs tuples positive => tuples.contains (xs.map (Term.eval a)) == positive
  | .shortTable xs tuples compressed => shortTableCheck (xs.map (Term.eval a)) tuples compressed
  | .reify child guard => (guard.eval a == 0 || guard.eval a == 1) &&
      ((guard.eval a == 1) == child.eval a)
  | .imply child guard => (guard.eval a == 0 || guard.eval a == 1) &&
      (guard.eval a != 1 || child.eval a)
  | .conjunction children => children.attach.all (fun child => child.val.eval a)
  | .disjunction children => children.attach.any (fun child => child.val.eval a)
termination_by c => sizeOf c
decreasing_by
  all_goals simp only [Constraint.reify.sizeOf_spec, Constraint.imply.sizeOf_spec,
    Constraint.conjunction.sizeOf_spec, Constraint.disjunction.sizeOf_spec]
  all_goals first | omega | exact Nat.lt_trans (List.sizeOf_lt_of_mem child.property) (by omega)

structure Model where
  variables : List Variable
  constraints : List Constraint
  deriving Repr

end Minion
