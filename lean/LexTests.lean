import Minion.LeafProof

open Minion Minion.DomainProof Minion.LeafProof

private def d : Domains := ofLists [[0, 1, 2], [0, 1, 2], [0, 1]]

example : rule.prune d (.lex [.var 0] [.constant 2] true) 0 .upperBound 1 = true := by decide +kernel
example : rule.prune d (.lex [.var 0] [.constant 2] true) 0 .upperBound 0 = false := by decide +kernel
example : checkConflict d (.lex [.var 0] [.var 0] true) = true := by decide +kernel
example : checkConflict d (.lex [.var 0] [.var 0] false) = false := by decide +kernel
example : checkConflict d (.lex [] [] true) = true := by decide +kernel
example : checkConflict d (.lex [.constant 0] [] false) = true := by decide +kernel
-- Equal-prefix bindings correlate later occurrences, rather than treating the
-- suffix as an independent assignment.
example : rule.prune d (.lex [.var 0, .var 1] [.var 1, .var 0] true)
    0 .upperBound 1 = true := by decide +kernel
example : LexProof.possibleValue d false (.lex [.var 0] [.var 0] false) = false := by decide +kernel

-- A nonzero scaling view still binds the underlying variable at the unique
-- equality prefix, so the suffix cannot choose a different value for it.
example : LexProof.possibleValue d true
    (.lex [.scale (.var 0) 2, .var 0] [.var 0, .constant (-1)] false) = false := by decide +kernel

private def zeroPrefix (n : Nat) (reverse : Bool) : Constraint :=
  let left := ((List.range n).map fun i => .scale (.var (2 * i)) 0) ++ [.var 1]
  let right := ((List.range n).map fun i => .var (2 * i + 1)) ++ [.constant 0]
  if reverse then .lex right left true else .lex left right false

-- Both sides of a comparison can contain duplicate view values. Previously
-- these two 24-position checks each expanded 2^24 identical prefix paths.
set_option maxRecDepth 10000 in
example : LexProof.possibleValue (ofLists (List.replicate 48 [0, 1])) false
    (zeroPrefix 24 false) = false := by decide +kernel
set_option maxRecDepth 10000 in
example : LexProof.possibleValue (ofLists (List.replicate 48 [0, 1])) true
    (zeroPrefix 24 true) = false := by decide +kernel

#print axioms LexProof.possibleValue_sound
#print axioms LocalProof.choosePossible_sound
#print axioms checkConflict_sound
#print axioms hasEmptyDomain_excludes
#print axioms hasEmptyDomain_prune_preserves
