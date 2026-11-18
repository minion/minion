import Minion.AllDiffProof
import Minion.CountProof

namespace Minion.MatrixProof
open DomainProof SetProof

def dimension (xs : List Term) : Option Nat :=
  (List.range (xs.length + 1)).find? (fun n => n * n == xs.length)

def rowColumns (xs : List Int) (n i : Nat) (v : Int) : List Nat :=
  (List.range n).filter fun j => xs[i * n + j]? == some v

def columnRows (xs : List Int) (n j : Nat) (v : Int) : List Nat :=
  (List.range n).filter fun i => xs[i * n + j]? == some v

def chosenColumn (xs : List Int) (n i : Nat) (v : Int) : Nat :=
  (rowColumns xs n i v).headD 0

theorem chosenColumn_mem (xs : List Int) (n i : Nat) (v : Int)
    (h : (rowColumns xs n i v).length = 1) :
    chosenColumn xs n i v ∈ rowColumns xs n i v := by
  obtain ⟨j, hj⟩ := List.length_eq_one_iff.mp h
  simp [chosenColumn, hj]

theorem singleton_unique {α : Type} (xs : List α) (h : xs.length = 1)
    (x y : α) (hx : x ∈ xs) (hy : y ∈ xs) : x = y := by
  obtain ⟨z, hz⟩ := List.length_eq_one_iff.mp h
  simp only [hz, List.mem_singleton] at hx hy
  exact hx.trans hy.symm

theorem chosenColumn_injective (xs : List Int) (n : Nat) (v : Int)
    (hrows : ∀ i ∈ List.range n, (rowColumns xs n i v).length = 1)
    (hcols : ∀ j ∈ List.range n, (columnRows xs n j v).length = 1)
    (i k : Nat) (hi : i ∈ List.range n) (hk : k ∈ List.range n)
    (he : chosenColumn xs n i v = chosenColumn xs n k v) : i = k := by
  have hci := List.mem_filter.mp (chosenColumn_mem xs n i v (hrows i hi))
  have hck := List.mem_filter.mp (chosenColumn_mem xs n k v (hrows k hk))
  apply singleton_unique (columnRows xs n (chosenColumn xs n i v) v)
    (hcols _ hci.1) i k
  · exact List.mem_filter.mpr ⟨hi, hci.2⟩
  · exact List.mem_filter.mpr ⟨hk, by simpa [he] using hck.2⟩

def cellMay (d : Domains) (xs : List Term) (index : Nat) (v : Int) : Bool :=
  match xs[index]? with
  | none => false
  | some t => (values d t).contains v

def cellMust (d : Domains) (xs : List Term) (index : Nat) (v : Int) : Bool :=
  match xs[index]? with
  | none => false
  | some t => (values d t).all fun w => w == v

theorem cellMay_sound (d : Domains) (xs : List Term) (index : Nat) (v : Int)
    (a : Assignment) (hd : Compatible d a) (hs : (xs.map (Term.eval a))[index]? = some v) :
    cellMay d xs index v = true := by
  cases hi : xs[index]? with
  | none => simp [List.getElem?_map, hi] at hs
  | some t =>
    have he : t.eval a = v := by simpa [List.getElem?_map, hi] using hs
    simp only [cellMay, hi]
    simpa [← he] using values_sound d a hd t

theorem cellMust_sound (d : Domains) (xs : List Term) (index : Nat) (v : Int)
    (a : Assignment) (hd : Compatible d a) (hs : cellMust d xs index v = true) :
    (xs.map (Term.eval a))[index]? = some v := by
  cases hi : xs[index]? with
  | none => simp [cellMust, hi] at hs
  | some t =>
    have hall : (values d t).all (fun w => w == v) = true := by simpa only [cellMust, hi] using hs
    have he := List.all_eq_true.mp hall (t.eval a) (values_sound d a hd t)
    simpa [List.getElem?_map, hi] using he

def possibleColumns (d : Domains) (xs : List Term) (n i : Nat) (v : Int) : List Int :=
  let forced := (List.range n).filter fun j => cellMust d xs (i * n + j) v
  (if forced.isEmpty then (List.range n).filter fun j => cellMay d xs (i * n + j) v
   else forced).map fun (j : Nat) => (j : Int)

theorem possibleColumns_sound (d : Domains) (xs : List Term) (n i : Nat) (v : Int)
    (a : Assignment) (hd : Compatible d a)
    (hrow : (rowColumns (xs.map (Term.eval a)) n i v).length = 1) :
    (chosenColumn (xs.map (Term.eval a)) n i v : Int) ∈ possibleColumns d xs n i v := by
  let j := chosenColumn (xs.map (Term.eval a)) n i v
  have hj := List.mem_filter.mp (chosenColumn_mem _ n i v hrow)
  simp only [possibleColumns]
  split
  · exact List.mem_map.mpr ⟨j, List.mem_filter.mpr
      ⟨hj.1, cellMay_sound d xs _ v a hd (by simpa using hj.2)⟩, rfl⟩
  · rename_i hn
    have hnonempty : ((List.range n).filter fun k => cellMust d xs (i * n + k) v) ≠ [] := by
      intro he
      simp [he] at hn
    obtain ⟨k, hk⟩ := List.exists_mem_of_ne_nil _ hnonempty
    have hparts := List.mem_filter.mp hk
    have hactual : k ∈ rowColumns (xs.map (Term.eval a)) n i v := List.mem_filter.mpr
      ⟨hparts.1, by simpa using cellMust_sound d xs _ v a hd hparts.2⟩
    have he : j = k := singleton_unique _ hrow j k (chosenColumn_mem _ n i v hrow) hactual
    exact List.mem_map.mpr ⟨k, hk, congrArg (fun r : Nat => (r : Int)) he.symm⟩

def hallCheck (d : Domains) (xs : List Term) (n : Nat) (v : Int) (indices : List Nat) : Bool :=
  let chosen := (List.range n).filter indices.contains
  card (chosen.flatMap fun i => possibleColumns d xs n i v) < chosen.length

theorem hallCheck_excludes (d : Domains) (xs : List Term) (n : Nat) (v : Int)
    (indices : List Nat) (a : Assignment) (hd : Compatible d a)
    (hrows : ∀ i ∈ List.range n, (rowColumns (xs.map (Term.eval a)) n i v).length = 1)
    (hcols : ∀ j ∈ List.range n, (columnRows (xs.map (Term.eval a)) n j v).length = 1)
    (hc : hallCheck d xs n v indices = true) : False := by
  let chosen := (List.range n).filter indices.contains
  let col := fun i => chosenColumn (xs.map (Term.eval a)) n i v
  have hsub : ∀ i ∈ chosen, i ∈ List.range n := fun _ hi => (List.mem_filter.mp hi).1
  have hp : chosen.Pairwise (fun i j => (col i : Int) ≠ (col j : Int)) := by
    have hnd : chosen.Nodup := (List.nodup_range (n := n)).sublist List.filter_sublist
    apply List.Pairwise.imp_of_mem (R := fun i j : Nat => i ≠ j) (p := hnd)
    intro i j hi hj hij he
    dsimp [col] at he
    exact hij (chosenColumn_injective _ n v hrows hcols i j (hsub i hi) (hsub j hj) (by omega))
  have hn : (chosen.map fun (i : Nat) => (col i : Int)).Nodup := hp.map _ (fun _ _ h => h)
  have hcover : (chosen.map fun (i : Nat) => (col i : Int)) ⊆
      (chosen.flatMap fun i => possibleColumns d xs n i v).eraseDups := by
    intro j hj
    obtain ⟨i, hi, rfl⟩ := List.mem_map.mp hj
    apply List.mem_eraseDups.mpr
    apply List.mem_flatMap.mpr
    refine ⟨i, hi, ?_⟩
    exact possibleColumns_sound d xs n i v a hd (hrows i (hsub i hi))
  have hb := List.Nodup.length_le_of_subset hn hcover
  have hbad : card (chosen.flatMap fun i => possibleColumns d xs n i v) < chosen.length := by
    simpa only [hallCheck, chosen, decide_eq_true_eq] using hc
  simp only [List.length_map] at hb
  exact Nat.not_lt_of_ge hb hbad

def linePossible (d : Domains) (xs : List Term) (indices : List Nat) (v : Int)
    (expected : Bool) : Bool :=
  CountProof.possibleRange d expected (fun dr =>
    (indices.countP fun i => cellMust dr xs i v, indices.countP fun i => cellMay dr xs i v)) .eq (.constant 1)

theorem linePossible_sound (d : Domains) (xs : List Term) (indices : List Nat) (v : Int)
    (expected : Bool) (a : Assignment) (hd : Compatible d a)
    (hs : ((indices.filter fun i => (xs.map (Term.eval a))[i]? == some v).length == 1) = expected) :
    linePossible d xs indices v expected = true := by
  apply CountProof.possibleRange_sound d expected _
    (fun ar => indices.countP fun i => (xs.map (Term.eval ar))[i]? == some v) _ .eq (.constant 1) a hd
  · have hcast (k : Nat) : ((k : Int) == 1) = (k == 1) := by
      apply Bool.eq_iff_iff.mpr
      simp only [beq_iff_eq]
      omega
    simpa only [Comparison.eval, List.countP_eq_length_filter, Term.eval, hcast] using hs
  · intro dr ar hr
    constructor
    · apply List.countP_mono_left
      intro i _ hi
      simpa using cellMust_sound dr xs i v ar hr hi
    · apply List.countP_mono_left
      intro i _ hi
      exact cellMay_sound dr xs i v ar hr (by simpa using hi)

def gridCheck (xs : List Int) (n : Nat) (v : Int) : Bool :=
  (List.range n).all fun i => (rowColumns xs n i v).length == 1 &&
    (columnRows xs n i v).length == 1

theorem matrixCheck_eq (xs : List Term) (v : Int) (a : Assignment) :
    matrixCheck (xs.map (Term.eval a)) v =
      match dimension xs with
      | none => false
      | some n => gridCheck (xs.map (Term.eval a)) n v := by
  simp only [matrixCheck, List.length_map]
  cases hn : (List.range (xs.length + 1)).find? (fun n => n * n == xs.length) <;>
    simp [hn, dimension, gridCheck, rowColumns, columnRows]

def gridPossible (d : Domains) (xs : List Term) (n : Nat) (v : Int) (expected : Bool) : Bool :=
  if expected then
    (List.range n).all (fun i =>
      linePossible d xs ((List.range n).map fun j => i * n + j) v true &&
      linePossible d xs ((List.range n).map fun j => j * n + i) v true) &&
    match Matching.discover ((List.range n).map fun i => possibleColumns d xs n i v) with
    | none => true
    | some indices => !hallCheck d xs n v indices
  else (List.range n).any fun i =>
    linePossible d xs ((List.range n).map fun j => i * n + j) v false ||
    linePossible d xs ((List.range n).map fun j => j * n + i) v false

theorem gridPossible_sound (d : Domains) (xs : List Term) (n : Nat) (v : Int) (expected : Bool)
    (a : Assignment) (hd : Compatible d a)
    (hs : gridCheck (xs.map (Term.eval a)) n v = expected) : gridPossible d xs n v expected = true := by
  have hline (i) (column : Bool) (exp : Bool)
      (hh : ((if column then columnRows (xs.map (Term.eval a)) n i v
        else rowColumns (xs.map (Term.eval a)) n i v).length == 1) = exp) :
      linePossible d xs ((List.range n).map fun j => if column then j * n + i else i * n + j) v exp = true := by
    apply linePossible_sound d xs _ v exp a hd
    cases column <;> simpa only [Bool.false_eq_true, ↓reduceIte, rowColumns, columnRows,
      ← List.countP_eq_length_filter, List.countP_map, Function.comp_def] using hh
  simp only [gridCheck] at hs
  cases expected with
  | true =>
    have hparts (i) (hi : i ∈ List.range n) := List.all_eq_true.mp hs i hi
    have hrows (i) (hi : i ∈ List.range n) : (rowColumns (xs.map (Term.eval a)) n i v).length = 1 := by
      have h := hparts i hi
      simp only [Bool.and_eq_true, beq_iff_eq] at h
      exact h.1
    have hcols (i) (hi : i ∈ List.range n) : (columnRows (xs.map (Term.eval a)) n i v).length = 1 := by
      have h := hparts i hi
      simp only [Bool.and_eq_true, beq_iff_eq] at h
      exact h.2
    simp only [gridPossible, ↓reduceIte, Bool.and_eq_true]
    constructor
    · apply List.all_eq_true.mpr
      intro i hi
      simp only [Bool.and_eq_true]
      exact ⟨hline i false true (by simp [hrows i hi]), hline i true true (by simp [hcols i hi])⟩
    · cases hm : Matching.discover ((List.range n).map fun i => possibleColumns d xs n i v) with
      | none => rfl
      | some indices =>
        cases hc : hallCheck d xs n v indices with
        | false => simp [hc]
        | true => exact False.elim (hallCheck_excludes d xs n v indices a hd hrows hcols hc)
  | false =>
    obtain ⟨i, hi, he⟩ := List.all_eq_false.mp hs
    have he' : (!((rowColumns (xs.map (Term.eval a)) n i v).length == 1) ||
        !((columnRows (xs.map (Term.eval a)) n i v).length == 1)) = true := by
      simpa only [Bool.not_and, Bool.not_false] using congrArg Bool.not (Bool.eq_false_iff.mpr he)
    simp only [Bool.or_eq_true] at he'
    simp only [gridPossible, Bool.false_eq_true, ↓reduceIte]
    apply List.any_eq_true.mpr
    refine ⟨i, hi, ?_⟩
    simp only [Bool.or_eq_true]
    cases he' with
    | inl hr => exact Or.inl (hline i false false (by simpa using hr))
    | inr hc => exact Or.inr (hline i true false (by simpa using hc))

def possibleValue (d : Domains) (expected : Bool) : Constraint → Bool
  | .allDifferentMatrix xs value =>
    match dimension xs with
    | none => !expected
    | some n => (values d value).any fun v => gridPossible (bindTerm d value v) xs n v expected
  | _ => true

theorem possibleValue_sound (d : Domains) (expected : Bool) (c : Constraint) (a : Assignment)
    (hd : Compatible d a) (hs : c.eval a = expected) : possibleValue d expected c = true := by
  cases c <;> try rfl
  rename_i xs value
  simp only [Constraint.eval, matrixCheck_eq] at hs
  cases hn : dimension xs with
  | none =>
    simp only [hn] at hs
    simp [possibleValue, hn, ← hs]
  | some n =>
    simp only [hn] at hs
    simp only [possibleValue, hn]
    exact List.any_eq_true.mpr ⟨value.eval a, values_sound d a hd value,
      gridPossible_sound (bindTerm d value (value.eval a)) xs n (value.eval a) expected a
        (compatible_bindTerm d value a hd) hs⟩

def supported : Constraint → Bool
  | .allDifferentMatrix .. => true
  | _ => false

end Minion.MatrixProof
