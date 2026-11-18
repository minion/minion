import Minion.JsonTables

open Lean Minion

private def sameOutcome [BEq α] : Except String α → Except String α → Bool
  | .ok a, .ok b => a == b
  | .error _, .error _ => true
  | _, _ => false

private def oldFull (arity : Nat) (j : Json) : Except String (List (List Int)) := do
  let rows ← fromJson? j
  unless rows.all (fun row => row.length == arity) do throw "wrong arity"
  return rows

private def oldShort (arity : Nat) (j : Json) : Except String (List (List (Nat × Int))) := do
  let rows ← fromJson? j
  unless rows.all (fun row => row.all (fun (i, _) => i < arity)) do throw "wrong index"
  return rows

#eval show IO Unit from do
  let cases := ["null", "false", "0", "{}", "\"a\"", "[]", "[0]", "[[]]",
    "[[0]]", "[[0,1],[-2,3]]", "[[0,1],[2]]", "[[0,1],[2,\"x\"]]",
    "[[1.0,2e0]]", "[[1.5,2]]", "[[999999999999999999999999999999,-2]]",
    "[[[0,1]]]", "[[[0,1],[1,-2]],[]]", "[[[0,1],[0,2]]]", "[[[-1,0]]]",
    "[[[99,0]]]", "[[[0,1,2]]]", "[[[0]]]", "[[[0.5,1]]]", "[[[0,1.5]]]"]
  for source in cases do
    let j ← match Json.parse source with
      | .ok j => pure j
      | .error e => throw (IO.userError e)
    for arity in List.range 4 do
      unless sameOutcome (oldFull arity j) (JsonTables.fullRows arity j) do
        throw (IO.userError s!"full-table decoding changed for {arity}: {source}")
      unless sameOutcome (oldShort arity j) (JsonTables.shortRows arity j) do
        throw (IO.userError s!"short-table decoding changed for {arity}: {source}")
  let rows : List (List Int) := (List.range 4096).map fun (i : Nat) =>
    [(i : Int), -(i : Int), 1000000000000000000, 0]
  unless sameOutcome (oldFull 4 (toJson rows)) (JsonTables.fullRows 4 (toJson rows)) do
    throw (IO.userError "full-table decoder changed row ordering or integer values")
