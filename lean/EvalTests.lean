import Minion.EvalProof
import Minion.CertificateData

open Minion

-- The sparse counter handles both signs, repeated keys, absent values and
-- query values outside the bounded-key optimization.
private def sample : List Int := (List.range 64).map (fun i => Int.ofNat i - 32)
example : (ValueCounts.counter sample).count (-1) = 1 := by decide +kernel
example : (ValueCounts.counter (sample ++ sample)).count 31 = 2 := by decide +kernel
example : (ValueCounts.counter sample).count 32 = 0 := by decide +kernel
example : ValueCounts.cardinality (sample ++ sample) = 64 := by decide +kernel
example : ValueCounts.cardinality (List.replicate 64 (-3)) = 1 := by decide +kernel
example : (ValueCounts.counter sample).count 10000000000000000000000000000000000000 = 0 := by decide +kernel
example : (ValueCounts.counter [10000000000000000000000000000000000000, -1]).count
    10000000000000000000000000000000000000 = 1 := by decide +kernel

-- Indexed assignment lookup retains the semantic zero default for invalid
-- references and composes with views and enclosing Boolean constraints.
example : evalConstraints ((List.range 64).map Int.ofNat)
    [.reify (.gcc [.neg (.var 1), .constant (-1), .notBool 99]
      [.constant (-1), .constant 1] [.constant 2, .constant 1]) (.constant 1)] = true := by decide +kernel

-- Compact payload notation expands to the exact ordinary data, including
-- empty rows, arbitrary-precision integers and sparse short-table entries.
example : minion_int_rows% "[[-2,0,123456789012345678901234567890],[],[1]]" =
    ([[-2, 0, 123456789012345678901234567890], [], [1]] : List (List Int)) := by rfl
example : minion_short_rows% "[[[0,-2],[4,123456789012345678901234567890]],[]]" =
    ([[(0, -2), (4, 123456789012345678901234567890)], []] : List (List (Nat × Int))) := by rfl

#print axioms Minion.Term.indexed_eq
#print axioms Minion.ValueCounts.counter_eq
#print axioms Minion.ValueCounts.cardinality_eq
#print axioms Minion.SequenceIndex.get_build
#print axioms Minion.Constraint.indexed_eq
#print axioms Minion.evalConstraints_eq
