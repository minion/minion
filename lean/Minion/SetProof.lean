import Minion.DomainProof

/-! Finite set cardinality facts shared by distinct-value and global-constraint
    rules. Sets are duplicate-free lists; no new mathematical axioms are used. -/
namespace Minion.SetProof

theorem eraseDups_nodup {α : Type} [BEq α] [LawfulBEq α] (xs : List α) : xs.eraseDups.Nodup := by
  cases xs with
  | nil => simp
  | cons x xs =>
    rw [List.eraseDups_cons, List.nodup_cons]
    constructor
    · simp
    · exact eraseDups_nodup (xs.filter (fun y => !(y == x)))
termination_by xs.length
decreasing_by exact Nat.lt_succ_of_le (List.length_filter_le _ _)

theorem eraseDups_sublist {α : Type} [BEq α] [LawfulBEq α] (xs : List α) :
    List.Sublist xs.eraseDups xs := by
  cases xs with
  | nil => exact .refl _
  | cons x xs =>
    rw [List.eraseDups_cons]
    exact .cons_cons x ((eraseDups_sublist (xs.filter (fun y => !(y == x)))).trans List.filter_sublist)
termination_by xs.length
decreasing_by exact Nat.lt_succ_of_le (List.length_filter_le _ _)

def card {α : Type} [BEq α] (xs : List α) : Nat := xs.eraseDups.length

/-- Count only up to a requested bound. The number of passes is bounded by
    `cap`, rather than by the number of distinct values in a wide domain. -/
def cappedCard {α : Type} [BEq α] : Nat → List α → Nat
  | 0, _ => 0
  | _ + 1, [] => 0
  | cap + 1, x :: xs => cappedCard cap (xs.filter (fun y => !(y == x))) + 1

theorem cappedCard_eq {α : Type} [BEq α] (cap : Nat) (xs : List α) :
    cappedCard cap xs = min cap (card xs) := by
  induction cap generalizing xs with
  | zero => simp [cappedCard]
  | succ cap ih =>
    cases xs with
    | nil => simp [cappedCard, card]
    | cons x xs => simp [cappedCard, card, List.eraseDups_cons, ih, Nat.add_min_add_right]

theorem card_le_length {α : Type} [BEq α] [LawfulBEq α] (xs : List α) : card xs ≤ xs.length :=
  (eraseDups_sublist xs).length_le

theorem card_cons_of_not_mem {α : Type} [BEq α] [LawfulBEq α] (x : α) (xs : List α)
    (hx : x ∉ xs) : card (x :: xs) = card xs + 1 := by
  have hf : xs.filter (fun y => !(y == x)) = xs := by
    apply List.filter_eq_self.mpr
    intro y hy
    have hne : y ≠ x := by intro he; subst y; exact hx hy
    simpa using hne
  simp [card, List.eraseDups_cons, hf]

theorem card_mono {α : Type} [BEq α] [LawfulBEq α] (xs ys : List α) (h : xs ⊆ ys) :
    card xs ≤ card ys := by
  apply List.Nodup.length_le_of_subset (eraseDups_nodup xs)
  intro x hx
  exact List.mem_eraseDups.mpr (h (List.mem_eraseDups.mp hx))

theorem card_append_le {α : Type} [BEq α] [LawfulBEq α] (xs ys : List α) :
    card (xs ++ ys) ≤ card xs + ys.length := by
  have hsub : (xs ++ ys).eraseDups ⊆ xs.eraseDups ++ ys := by
    intro x hx
    cases List.mem_append.mp (List.mem_eraseDups.mp hx) with
    | inl hl => exact List.mem_append_left _ (List.mem_eraseDups.mpr hl)
    | inr hr => exact List.mem_append_right _ hr
  simpa [card] using List.Nodup.length_le_of_subset (eraseDups_nodup (xs ++ ys)) hsub

theorem nodup_of_card_eq_length {α : Type} [BEq α] [LawfulBEq α] (xs : List α)
    (h : card xs = xs.length) : xs.Nodup := by
  have he : xs.eraseDups = xs := (eraseDups_sublist xs).eq_of_length h
  simpa only [he] using eraseDups_nodup xs

theorem card_eq_length_of_nodup {α : Type} [BEq α] [LawfulBEq α] (xs : List α)
    (h : xs.Nodup) : card xs = xs.length := by
  apply Nat.le_antisymm (card_le_length xs)
  exact List.Nodup.length_le_of_subset h (fun _ hx => List.mem_eraseDups.mpr hx)

end Minion.SetProof
