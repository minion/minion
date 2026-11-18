import Minion.SetProof

/-! Capacity cuts: distinct values partition a list's occurrences. Summed upper
    capacities bound the size of a chosen variable set; summed lower demands
    bound how many variables must be able to take a chosen value. -/
namespace Minion.CapacityProof

def mass (xs vs : List Int) : Nat := (vs.map fun v => xs.count v).sum

theorem mass_cons (x : Int) (xs vs : List Int) :
    mass (x :: xs) vs = mass xs vs + vs.count x := by
  induction vs with
  | nil => simp [mass]
  | cons v vs ih =>
    simp only [mass, List.map_cons, List.sum_cons] at ih ⊢
    rw [List.count_cons, List.count_cons, Bool.beq_comm (a := x) (b := v)]
    split <;> omega

theorem mass_partition (xs vs : List Int) (hn : vs.Nodup) :
    mass xs vs = xs.countP (fun v => vs.contains v) := by
  induction xs with
  | nil => induction vs <;> simp_all [mass]
  | cons x xs ih =>
    rw [mass_cons, ih, hn.count]
    simp only [List.countP_cons]
    by_cases hx : x ∈ vs <;> simp [hx]

theorem mass_upper (xs vs : List Int) (cap : Int → Nat)
    (hc : ∀ v ∈ vs, xs.count v ≤ cap v) : mass xs vs ≤ (vs.map cap).sum := by
  induction vs with
  | nil => simp [mass]
  | cons v vs ih =>
    have hv := hc v (by simp)
    have hr := ih (fun w hw => hc w (by simp [hw]))
    simp only [mass, List.map_cons, List.sum_cons] at hr ⊢
    omega

theorem mass_lower (xs vs : List Int) (cap : Int → Nat)
    (hc : ∀ v ∈ vs, cap v ≤ xs.count v) : (vs.map cap).sum ≤ mass xs vs := by
  induction vs with
  | nil => simp [mass]
  | cons v vs ih =>
    have hv := hc v (by simp)
    have hr := ih (fun w hw => hc w (by simp [hw]))
    simp only [mass, List.map_cons, List.sum_cons] at hr ⊢
    omega

theorem upper_cut (actual chosen vs : List Int) (cap : Int → Nat)
    (hsub : List.Sublist chosen actual) (hcover : chosen ⊆ vs) (hn : vs.Nodup)
    (hcap : ∀ v ∈ vs, actual.count v ≤ cap v) : chosen.length ≤ (vs.map cap).sum := by
  have hb := mass_upper chosen vs cap (fun v hv => Nat.le_trans (hsub.count_le v) (hcap v hv))
  rw [mass_partition chosen vs hn] at hb
  have he : chosen.countP (fun v => vs.contains v) = chosen.length := by
    apply List.countP_eq_length.mpr
    intro v hv
    simpa using hcover hv
  rwa [he] at hb

end Minion.CapacityProof
