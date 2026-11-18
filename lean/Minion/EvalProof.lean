import Minion.ConstraintIndex
import Minion.SequenceIndex
import Minion.SetProof

/-! A proved-equivalent assignment evaluator. Variable lookup can use a binary
    index under kernel reduction, and GCC evaluates its input terms once. The
    original assignment semantics remain unchanged. -/
namespace Minion

namespace ValueCounts

private def key : Int → Nat
  | .ofNat n => 2 * n
  | .negSucc n => 2 * n + 1

private theorem key_eq (x y : Int) : key x = key y ↔ x = y := by
  constructor
  · intro h
    cases x <;> cases y <;> simp only [key] at h
    all_goals first | omega | (congr 1 <;> omega)
  · intro h
    subst y
    rfl

private def build : List Int → ConstraintIndex.Index Nat × Nat
  | [] => (.empty, 0)
  | x :: xs =>
    let rest := build xs
    let old := (ConstraintIndex.get rest.1 (key x)).getD 0
    (ConstraintIndex.set rest.1 (key x) (old + 1),
      if old = 0 then rest.2 + 1 else rest.2)

private theorem lookup_build (xs : List Int) (v : Int) :
    (ConstraintIndex.get (build xs).1 (key v)).getD 0 = xs.count v := by
  induction xs with
  | nil => simp only [build, ConstraintIndex.get, ConstraintIndex.read, Option.getD_none, List.count_nil]
  | cons x xs ih =>
    simp only [build, ConstraintIndex.get_set]
    by_cases hx : x = v
    · subst x
      simp only [↓reduceIte, Option.getD_some, ih, List.count_cons_self]
    · have hk : key x ≠ key v := fun h => hx ((key_eq x v).mp h)
      simp only [ite_eq_right hk, ih, List.count_cons_of_ne hx]

private theorem card_cons (x : Int) (xs : List Int) :
    SetProof.card (x :: xs) = if x ∈ xs then SetProof.card xs else SetProof.card xs + 1 := by
  split
  · rename_i hx
    apply Nat.le_antisymm
    · exact SetProof.card_mono _ _ (fun y hy => by
        rcases List.mem_cons.mp hy with rfl | hy
        · exact hx
        · exact hy)
    · exact SetProof.card_mono _ _ (fun _ hy => List.mem_cons_of_mem _ hy)
  · exact SetProof.card_cons_of_not_mem x xs ‹_›

private theorem cardinality_build (xs : List Int) : (build xs).2 = SetProof.card xs := by
  induction xs with
  | nil => rfl
  | cons x xs ih =>
    simp only [build, lookup_build, ih, card_cons]
    by_cases hx : x ∈ xs <;> simp_all only [List.count_eq_zero,
      not_true_eq_false, not_false_eq_true, ↓reduceIte]

inductive Counter where
  | scan (values : List Int)
  | indexed (values : List Int) (index : ConstraintIndex.Index Nat)

/-- Store the built index as data. Returning a function here allows compiler
    eta expansion to move index construction into each individual lookup. -/
def counter (xs : List Int) : Counter :=
  if xs.length < 32 || xs.any (fun v => v.natAbs > 1048576) then .scan xs
  else .indexed xs (build xs).1

/-- Small lists and huge integer keys retain direct counting, keeping index
    depth bounded independently of arbitrary-size input integers. -/
def Counter.count : Counter → Int → Nat
  | .scan xs, v => xs.count v
  | .indexed xs index, v =>
    if v.natAbs > 1048576 then xs.count v
    else (ConstraintIndex.get index (key v)).getD 0

theorem counter_eq (xs : List Int) (v : Int) : (counter xs).count v = xs.count v := by
  simp only [counter]
  split
  · rfl
  · simp only [Counter.count]
    split
    · rfl
    · exact lookup_build xs v

def cardinality (xs : List Int) : Nat :=
  if xs.length < 32 || xs.any (fun v => v.natAbs > 1048576) then xs.eraseDups.length
  else (build xs).2

theorem cardinality_eq (xs : List Int) : cardinality xs = xs.eraseDups.length := by
  unfold cardinality
  split
  · rfl
  · exact cardinality_build xs

end ValueCounts

def Term.indexed (get : Nat → Int) : Term → Int
  | .var i => get i
  | .constant v => v
  | .notBool i => 1 - get i
  | .neg t => -t.indexed get
  | .shift t k => t.indexed get + k
  | .scale t k => k * t.indexed get

theorem Term.indexed_eq (a : Assignment) (get : Nat → Int)
    (h : ∀ i, get i = a[i]?.getD 0) (t : Term) : t.indexed get = t.eval a := by
  induction t <;> simp_all only [Term.indexed, Term.eval]

def Constraint.indexed (get : Nat → Int) : Constraint → Bool
  | .truth b => b
  | .compare op x y => op.eval (x.indexed get) (y.indexed get)
  | .absolute x y => x.indexed get == (y.indexed get).natAbs
  | .difference x y z => ((x.indexed get - y.indexed get).natAbs : Int) == z.indexed get
  | .minusEq x y => x.indexed get == -y.indexed get
  | .divide x y z undefZero =>
      if y.indexed get == 0 then undefZero && z.indexed get == 0
      else (x.indexed get).fdiv (y.indexed get) == z.indexed get
  | .modulo x y z undefZero =>
      if y.indexed get == 0 then undefZero && z.indexed get == 0
      else (x.indexed get).fmod (y.indexed get) == z.indexed get
  | .power x y z => powerEq (x.indexed get) (y.indexed get) (z.indexed get)
  | .product x y z => x.indexed get * y.indexed get == z.indexed get
  | .inequality x y k => x.indexed get ≤ y.indexed get + k.indexed get
  | .allDifferent xs => let vs := xs.map (Term.indexed get); ValueCounts.cardinality vs == vs.length
  | .element xs index rhs offset undefZero =>
      let i := index.indexed get - offset
      if i < 0 || i ≥ (xs.length : Int) then undefZero && rhs.indexed get == 0
      else (xs[i.toNat]?.map (Term.indexed get)) == some (rhs.indexed get)
  | .lex xs ys strict =>
      let x := xs.map (Term.indexed get); let y := ys.map (Term.indexed get)
      xs.length == ys.length && (lexLess x y || (!strict && x == y))
  | .sum xs rhs op => op.eval ((xs.map (Term.indexed get)).foldl (· + ·) 0) (rhs.indexed get)
  | .weightedSum ws xs rhs op => ws.length == xs.length &&
      op.eval (((ws.zip xs).map (fun (w, x) => w.indexed get * x.indexed get)).foldl (· + ·) 0) (rhs.indexed get)
  | .occurrence xs v n op =>
      op.eval ((xs.filter (fun x => x.indexed get == v.indexed get)).length : Int) (n.indexed get)
  | .nvalue xs n op => op.eval (ValueCounts.cardinality (xs.map (Term.indexed get)) : Int) (n.indexed get)
  | .extremum xs rhs isMax =>
      match xs.map (Term.indexed get) with
      | [] => false
      | x :: rest => rest.foldl (if isMax then max else min) x == rhs.indexed get
  | .hamming xs ys n op => xs.length == ys.length &&
      op.eval (((xs.zip ys).filter (fun (x, y) => x.indexed get != y.indexed get)).length : Int) (n.indexed get)
  | .literalSum xs ys n => xs.length == ys.length &&
      (((xs.zip ys).filter (fun (x, y) => x.indexed get == y.indexed get)).length : Int) ≥ n.indexed get
  | .inSet x vs positive => (vs.any (fun v => v.indexed get == x.indexed get)) == positive
  | .inIntervals x bounds positive => (containsInterval (x.indexed get) (bounds.map (Term.indexed get))) == positive
  | .vectorExists xs ys op => xs.length == ys.length &&
      (xs.zip ys).any (fun (x, y) => op.eval (x.indexed get) (y.indexed get))
  | .gcc xs vs ns =>
      let values := xs.map (Term.indexed get)
      let count := ValueCounts.counter values
      vs.length == ns.length && (vs.zip ns).all (fun (v, n) =>
        ((count.count (v.indexed get)) : Int) == n.indexed get)
  | .allDifferentMatrix xs v => matrixCheck (xs.map (Term.indexed get)) (v.indexed get)
  | .frameUpdate src tgt rs rt bs => frameCheck (src.map (Term.indexed get)) (tgt.map (Term.indexed get))
      (rs.map (Term.indexed get)) (rt.map (Term.indexed get)) (bs.indexed get)
  | .table xs tuples positive => tuples.contains (xs.map (Term.indexed get)) == positive
  | .shortTable xs tuples compressed => shortTableCheck (xs.map (Term.indexed get)) tuples compressed
  | .reify child guard => (guard.indexed get == 0 || guard.indexed get == 1) &&
      ((guard.indexed get == 1) == child.indexed get)
  | .imply child guard => (guard.indexed get == 0 || guard.indexed get == 1) &&
      (guard.indexed get != 1 || child.indexed get)
  | .conjunction children => children.attach.all (fun child => child.val.indexed get)
  | .disjunction children => children.attach.any (fun child => child.val.indexed get)
termination_by c => sizeOf c
decreasing_by
  all_goals simp only [Constraint.reify.sizeOf_spec, Constraint.imply.sizeOf_spec,
    Constraint.conjunction.sizeOf_spec, Constraint.disjunction.sizeOf_spec]
  all_goals first | omega | exact Nat.lt_trans (List.sizeOf_lt_of_mem child.property) (by omega)

theorem Constraint.indexed_eq (a : Assignment) (get : Nat → Int)
    (h : ∀ i, get i = a[i]?.getD 0) (c : Constraint) : c.indexed get = c.eval a := by
  have ht : Term.indexed get = Term.eval a := funext (Term.indexed_eq a get h)
  cases c with
  | reify child guard | imply child guard =>
    simp only [Constraint.indexed, Constraint.eval, Term.indexed_eq a get h,
      Constraint.indexed_eq a get h child]
  | conjunction children | disjunction children =>
    simp only [Constraint.indexed, Constraint.eval]
    congr 1
    funext child
    exact Constraint.indexed_eq a get h child.val
  | gcc xs vs ns =>
    simp only [Constraint.indexed, Constraint.eval, ht, ValueCounts.counter_eq, List.count_eq_length_filter,
      List.filter_map, List.length_map, Function.comp_def]
  | _ => simp only [Constraint.indexed, Constraint.eval, ht, ValueCounts.cardinality_eq] <;> rfl
termination_by sizeOf c
decreasing_by
  all_goals simp_all only [Constraint.reify.sizeOf_spec, Constraint.imply.sizeOf_spec,
    Constraint.conjunction.sizeOf_spec, Constraint.disjunction.sizeOf_spec]
  all_goals first | omega | exact Nat.lt_trans (List.sizeOf_lt_of_mem child.property) (by omega)

-- This is a performance hint only: either evaluator has identical semantics.
-- Ordinary small constraints retain their direct compiled evaluation path.
private def wideCounts : Nat → Constraint → Bool
  | 0, _ => false
  | _ + 1, .gcc xs .. | _ + 1, .allDifferent xs | _ + 1, .nvalue xs .. => xs.length ≥ 32
  | fuel + 1, .reify c _ | fuel + 1, .imply c _ => wideCounts fuel c
  | fuel + 1, .conjunction cs | fuel + 1, .disjunction cs => cs.any (wideCounts fuel)
  | _ + 1, _ => false

/-- Small assignments avoid index construction and ordinary small constraints
    retain the original evaluator. Out-of-range references keep the original
    zero default, so this equivalence does not need well-formedness. -/
def evalConstraints (a : Assignment) (cs : List Constraint) : Bool :=
  if a.length < 32 then
    if cs.any (wideCounts 256) then cs.all (Constraint.indexed (fun i => a[i]?.getD 0))
    else cs.all (Constraint.eval a)
  else
    let index := SequenceIndex.build a
    cs.all (Constraint.indexed (fun i => (SequenceIndex.get index i).getD 0))

theorem evalConstraints_eq (a : Assignment) (cs : List Constraint) :
    evalConstraints a cs = cs.all (Constraint.eval a) := by
  simp only [evalConstraints]
  split
  · split
    · exact congrArg (fun f => cs.all f) (funext (Constraint.indexed_eq a _ (fun _ => rfl)))
    · rfl
  · exact congrArg (fun f => cs.all f) (funext (Constraint.indexed_eq a _
      (fun i => by rw [SequenceIndex.get_build])))

end Minion
