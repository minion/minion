import Minion.DomainProof

/-! Merge candidate values when their order can be checked in linear time.
    Arbitrarily ordered abstract domains retain the ordinary candidate list.
    The final view binding still checks aliases; intersecting domains alone
    never establishes a joint support. -/
namespace Minion.DomainOverlap

def ordered : List Int → Bool
  | [] | [_] => true
  | x :: y :: xs => x ≤ y && ordered (y :: xs)

theorem ordered_tail (x : Int) (xs : List Int) (h : ordered (x :: xs) = true) :
    ordered xs = true := by
  cases xs with
  | nil => rfl
  | cons y ys =>
    have hp : (x ≤ y) ∧ ordered (y :: ys) = true := by
      simpa only [ordered, Bool.and_eq_true, decide_eq_true_eq] using h
    exact hp.2

theorem head_lower (x : Int) (xs : List Int) (h : ordered (x :: xs) = true)
    (v : Int) (hv : v ∈ x :: xs) : x ≤ v := by
  induction xs generalizing x with
  | nil =>
    have he : v = x := by simpa using hv
    omega
  | cons y ys ih =>
    rcases List.mem_cons.mp hv with he | hr
    · subst v; omega
    · have hp : x ≤ y ∧ ordered (y :: ys) = true := by
        simpa only [ordered, Bool.and_eq_true, decide_eq_true_eq] using h
      exact Int.le_trans hp.1 (ih y hp.2 hr)

def mergeValues : Nat → List Int → List Int → List Int
  | 0, _, _ => []
  | _ + 1, [], _ | _ + 1, _, [] => []
  | fuel + 1, x :: xs, y :: ys =>
      if x < y then mergeValues fuel xs (y :: ys)
      else if y < x then mergeValues fuel (x :: xs) ys
      else x :: mergeValues fuel xs ys

theorem mergeValues_complete (fuel : Nat) (xs ys : List Int)
    (budget : xs.length + ys.length ≤ fuel)
    (hx : ordered xs = true) (hy : ordered ys = true)
    (v : Int) (hxs : v ∈ xs) (hys : v ∈ ys) : v ∈ mergeValues fuel xs ys := by
  induction fuel generalizing xs ys with
  | zero =>
    cases xs with
    | nil => simp at hxs
    | cons x xs => simp only [List.length_cons] at budget; omega
  | succ fuel ih =>
    cases xs with
    | nil => simp at hxs
    | cons x xs =>
      cases ys with
      | nil => simp at hys
      | cons y ys =>
        simp only [List.length_cons] at budget
        by_cases hxy : x < y
        · have hne : v ≠ x := by
            intro he; subst v
            have := head_lower y ys hy x hys
            omega
          have htail := (List.mem_cons.mp hxs).resolve_left hne
          simp only [mergeValues, hxy, ↓reduceIte]
          exact ih xs (y :: ys) (by simp only [List.length_cons]; omega)
            (ordered_tail x xs hx) hy htail hys
        · by_cases hyx : y < x
          · have hne : v ≠ y := by
              intro he; subst v
              have := head_lower x xs hx y hxs
              omega
            have htail := (List.mem_cons.mp hys).resolve_left hne
            simp only [mergeValues, hxy, hyx, ↓reduceIte]
            exact ih (x :: xs) ys (by simp only [List.length_cons]; omega)
              hx (ordered_tail y ys hy) hxs htail
          · have he : x = y := by omega
            simp only [mergeValues, hxy, hyx, ↓reduceIte]
            by_cases hvx : v = x
            · exact List.mem_cons.mpr (Or.inl hvx)
            · apply List.mem_cons.mpr
              right
              exact ih xs ys (by omega) (ordered_tail x xs hx) (ordered_tail y ys hy)
                ((List.mem_cons.mp hxs).resolve_left hvx)
                ((List.mem_cons.mp hys).resolve_left (by simpa [he] using hvx))

-- The native implementation accumulates its result on the heap. The theorem
-- used by the compiler also keeps the kernel's simple mathematical definition.
private def mergeValuesAux : Nat → List Int → List Int → List Int → List Int
  | 0, _, _, acc => acc.reverse
  | _ + 1, [], _, acc | _ + 1, _, [], acc => acc.reverse
  | fuel + 1, x :: xs, y :: ys, acc =>
      if x < y then mergeValuesAux fuel xs (y :: ys) acc
      else if y < x then mergeValuesAux fuel (x :: xs) ys acc
      else mergeValuesAux fuel xs ys (x :: acc)

private theorem mergeValuesAux_eq (fuel : Nat) (xs ys acc : List Int) :
    mergeValuesAux fuel xs ys acc = acc.reverse ++ mergeValues fuel xs ys := by
  induction fuel generalizing xs ys acc with
  | zero => simp [mergeValuesAux, mergeValues]
  | succ fuel ih =>
    cases xs with
    | nil => simp [mergeValuesAux, mergeValues]
    | cons x xs =>
      cases ys with
      | nil => simp [mergeValuesAux, mergeValues]
      | cons y ys =>
        by_cases hxy : x < y <;> by_cases hyx : y < x <;>
          simp [mergeValuesAux, mergeValues, hxy, hyx, ih, List.reverse_cons, List.append_assoc]

private def mergeValuesTR (fuel : Nat) (xs ys : List Int) : List Int :=
  mergeValuesAux fuel xs ys []

@[csimp] private theorem mergeValues_eq_TR : mergeValues = mergeValuesTR := by
  funext fuel xs ys
  simp only [mergeValuesTR, mergeValuesAux_eq, List.reverse_nil, List.nil_append]


def ascending (xs : List Int) : List Int := if ordered xs then xs else xs.reverse

theorem ascending_mem (xs : List Int) (v : Int) : v ∈ ascending xs ↔ v ∈ xs := by
  unfold ascending
  split
  · rfl
  · exact List.mem_reverse

def orderedCandidates (xs ys : List Int) : List Int :=
  let left := ascending xs
  let right := ascending ys
  if ordered left && ordered right then mergeValues (left.length + right.length) left right else xs

theorem orderedCandidates_complete (xs ys : List Int) (v : Int) (hx : v ∈ xs) (hy : v ∈ ys) :
    v ∈ orderedCandidates xs ys := by
  unfold orderedCandidates
  dsimp only
  split
  · rename_i h
    have hp : ordered (ascending xs) = true ∧ ordered (ascending ys) = true := by
      simpa only [Bool.and_eq_true] using h
    exact mergeValues_complete _ _ _ (Nat.le_refl _) hp.1 hp.2 v
      ((ascending_mem xs v).mpr hx) ((ascending_mem ys v).mpr hy)
  · exact hx

def candidates (xs ys : List Int) : List Int :=
  match xs, ys with
  | [], _ => []
  | [_], _ => xs
  | _, [] => []
  | _, [v] => if xs.contains v then [v] else []
  | _, _ => orderedCandidates xs ys

theorem candidates_complete (xs ys : List Int) (v : Int) (hx : v ∈ xs) (hy : v ∈ ys) :
    v ∈ candidates xs ys := by
  cases xs with
  | nil => simp at hx
  | cons x xs =>
    cases xs with
    | nil => simpa only [candidates] using hx
    | cons xx rest =>
      cases ys with
      | nil => simp at hy
      | cons y ys =>
        cases ys with
        | nil =>
          have he : v = y := by simpa using hy
          subst v
          simp [candidates, hx]
        | cons yy tail => exact orderedCandidates_complete _ _ v hx hy

end Minion.DomainOverlap
