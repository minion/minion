import Lean

/-! Untrusted table decoding without an intermediate decoded array per row.
    The semantic representation and integer decoder are unchanged. -/
namespace Minion.JsonTables
open Lean

def fullRows (arity : Nat) (j : Json) : Except String (List (List Int)) := do
  let rows ← j.getArr?
  rows.foldrM (fun row result => do
    let cells ← row.getArr?
    unless cells.size == arity do throw "wrong table arity"
    let values ← cells.foldrM (fun cell values => do return (← cell.getInt?) :: values) []
    return values :: result) []

def shortRows (arity : Nat) (j : Json) : Except String (List (List (Nat × Int))) := do
  let rows ← j.getArr?
  rows.foldrM (fun row result => do
    let cells ← row.getArr?
    let values ← cells.foldrM (fun cell values => do
      let pair ← cell.getArr?
      unless pair.size == 2 do throw "short tuple entry must be an index/value pair"
      let index ← pair[0]!.getNat?
      let value ← pair[1]!.getInt?
      unless index < arity do throw "short tuple index out of range"
      return (index, value) :: values) []
    return values :: result) []

end Minion.JsonTables
