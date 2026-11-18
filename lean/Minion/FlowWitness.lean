import Lean

/-! Bounded capacitated matching discovery. Its output is only a proposed cut;
    callers must validate that cut independently before rejecting a domain.
    Capacities are numbers, rather than expanded lists of value slots. -/
namespace Minion.FlowWitness

abbrev Owners := List (Nat × Int)

inductive Outcome where
  | matched (owners : Owners)
  | lower (values : List Int)
  | upper (rows : List Nat)
deriving Repr, BEq

structure Result where
  matching : Option Owners
  rows : List Nat
  seen : List Int

def own (owners : Owners) (row : Nat) (value : Int) : Owners :=
  (row, value) :: owners.filter (fun p => p.1 != row)

def usage (owners : Owners) (value : Int) : Nat :=
  owners.countP (fun p => p.2 == value)

/- Fill one unit of a lower demand, alternating through already assigned rows.
   Each successful path ends at an unassigned row. -/
def augmentLower (domains : List (List Int)) (fuel : Nat) (value : Int)
    (owners : Owners) (rows : List Nat) (seen : List Int) : Result :=
  match fuel with
  | 0 => ⟨none, rows, seen⟩
  | fuel + 1 =>
    if seen.contains value then ⟨none, rows, seen⟩ else
      (List.range domains.length).foldl (fun result row =>
        if result.matching.isSome || result.rows.contains row ||
            !(domains[row]?.getD []).contains value then result else
          match owners.find? (fun p => p.1 == row) with
          | none => ⟨some (own owners row value), row :: result.rows, result.seen⟩
          | some (_, other) =>
            if other == value then ⟨none, row :: result.rows, result.seen⟩ else
              let next := augmentLower domains fuel other owners (row :: result.rows) result.seen
              match next.matching with
              | some updated => ⟨some (own updated row value), next.rows, next.seen⟩
              | none => next) ⟨none, rows, value :: seen⟩
termination_by structural fuel

def meetDemand (domains : List (List Int)) (lower : Int → Nat) (value : Int)
    (fuel : Nat) (owners : Owners) : Outcome :=
  if usage owners value ≥ lower value then .matched owners else
    match fuel with
    | 0 => .lower [value]
    | fuel + 1 =>
      let next := augmentLower domains (domains.length + 1) value owners [] []
      match next.matching with
      | none => .lower next.seen
      | some updated => meetDemand domains lower value fuel updated
termination_by structural fuel

def meetDemands (domains : List (List Int)) (lower : Int → Nat) :
    List Int → Owners → Outcome
  | [], owners => .matched owners
  | value :: rest, owners =>
    match meetDemand domains lower value (domains.length + 1) owners with
    | .matched updated => meetDemands domains lower rest updated
    | result => result

/- Fill an unassigned row, alternating through full values. A successful path
   ends at a value with spare capacity and preserves earlier lower demands. -/
def augmentUpper (domains : List (List Int)) (upper : Int → Nat) (fuel row : Nat)
    (owners : Owners) (rows : List Nat) (seen : List Int) : Result :=
  match fuel with
  | 0 => ⟨none, rows, seen⟩
  | fuel + 1 =>
    if rows.contains row then ⟨none, rows, seen⟩ else
      (domains[row]?.getD []).foldl (fun result value =>
        if result.matching.isSome || result.seen.contains value then result else
          if usage owners value < upper value then
            ⟨some (own owners row value), result.rows, value :: result.seen⟩
          else
            (owners.filter (fun p => p.2 == value)).foldl (fun attempt p =>
              if attempt.matching.isSome then attempt else
                let next := augmentUpper domains upper fuel p.1 owners attempt.rows attempt.seen
                match next.matching with
                | some updated => ⟨some (own updated row value), next.rows, next.seen⟩
                | none => next) ⟨none, result.rows, value :: result.seen⟩)
        ⟨none, row :: rows, seen⟩
termination_by structural fuel

def fillRows (domains : List (List Int)) (upper : Int → Nat) :
    List Nat → Owners → Outcome
  | [], owners => .matched owners
  | row :: rest, owners =>
    if owners.any (fun p => p.1 == row) then fillRows domains upper rest owners else
      let next := augmentUpper domains upper (domains.length + 1) row owners [] []
      match next.matching with
      | none => .upper next.rows
      | some updated => fillRows domains upper rest updated

def discover (domains : List (List Int)) (tracked : List Int)
    (lower upper : Int → Nat) : Outcome :=
  match meetDemands domains lower tracked [] with
  | .matched owners => fillRows domains upper (List.range domains.length) owners
  | result => result

end Minion.FlowWitness
