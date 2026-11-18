import Minion.LocalProof

/-! A small result cannot equal an exponentially large power. Check the result's
    bit length before constructing the integer. The bound is proved using exact
    natural logarithms, and also applies when checking a false/reified relation. -/
namespace Minion.PowerProof

 theorem exponent_bound (x result : Int) (n cap : Nat) (hx : 1 < x.natAbs)
    (he : x ^ n = result) (hr : result.natAbs ≤ cap) : n ≤ cap.log2 := by
  have hp : 2 ^ n ≤ result.natAbs := by
    rw [← he, Int.natAbs_pow]
    exact Nat.pow_le_pow_left (by omega) n
  have hn : cap ≠ 0 := by
    have ht := Nat.two_pow_pos n
    omega
  exact (Nat.le_log2 hn).mpr (Nat.le_trans hp hr)

def quickBound (x exponent : Int) (cap : Nat) : Bool :=
  exponent < 0 || x.natAbs ≤ 1 || exponent.toNat ≤ cap.log2

theorem quickBound_sound (x exponent result : Int) (cap : Nat)
    (hs : powerEq x exponent result = true) (hr : result.natAbs ≤ cap) :
    quickBound x exponent cap = true := by
  by_cases he : exponent < 0
  · simp [quickBound, he]
  · by_cases hx : x.natAbs ≤ 1
    · simp [quickBound, hx]
    · have hp : x ^ exponent.toNat = result := by simpa [powerEq, he] using hs
      have hb := exponent_bound x result exponent.toNat cap (by omega) hp hr
      simp [quickBound, hb]

def boundedEq (x exponent result : Int) : Bool :=
  quickBound x exponent result.natAbs && powerEq x exponent result

theorem boundedEq_eq (x exponent result : Int) : boundedEq x exponent result = powerEq x exponent result := by
  cases hs : powerEq x exponent result with
  | false => simp [boundedEq, hs]
  | true => simp [boundedEq, hs, quickBound_sound x exponent result result.natAbs hs (Nat.le_refl _)]

end Minion.PowerProof
