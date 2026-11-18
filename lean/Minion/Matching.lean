import Lean

/-! Total, bounded witness discovery. Soundness of any proof using its output
    comes from independently checking the returned Hall subset, not from
    trusting this matching algorithm or proving it complete. -/
namespace Minion.Matching

abbrev Owners := List (Int × Nat)

structure Result where
  matching : Option Owners
  rows : List Nat
  seen : List Int

def own (owners : Owners) (value : Int) (row : Nat) : Owners :=
  (value, row) :: owners.filter (fun p => p.1 != value)

def augment (domains : List (List Int)) (fuel row : Nat) (owners : Owners)
    (rows : List Nat) (seen : List Int) : Result :=
  match fuel with
  | 0 => ⟨none, rows, seen⟩
  | fuel + 1 =>
    if rows.contains row then ⟨none, rows, seen⟩ else
      (domains[row]?.getD []).foldl (fun result value =>
          if result.matching.isSome || result.seen.contains value then result else
            match owners.find? (fun p => p.1 == value) with
            | none => ⟨some (own owners value row), result.rows, value :: result.seen⟩
            | some (_, other) =>
              let next := augment domains fuel other owners result.rows (value :: result.seen)
              match next.matching with
              | some updated => ⟨some (own updated value row), next.rows, next.seen⟩
              | none => next) ⟨none, row :: rows, seen⟩
termination_by structural fuel

def discoverFrom (domains : List (List Int)) (fuel : Nat) : List Nat → Owners → Option (List Nat)
  | [], _ => none
  | row :: rest, owners =>
    let result := augment domains fuel row owners [] []
    match result.matching with
    | some updated => discoverFrom domains fuel rest updated
    | none => some result.rows

def discover (domains : List (List Int)) : Option (List Nat) :=
  discoverFrom domains (domains.length + 1) (List.range domains.length) []

end Minion.Matching
