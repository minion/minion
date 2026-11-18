import Minion

open Minion

private def checkFile (path : String) (maxAssignments : Nat) : IO Bool := do
  let text ← IO.FS.readFile path
  let decoded := Lean.Json.parse text >>= decodeSolutionFile
  let file ← match decoded with
    | .ok file => pure file
    | .error message =>
      IO.eprintln s!"REJECT {path}: {message}"
      return false
  let candidates := (file.model.variables.map (fun v => v.domain.length)).foldl (· * ·) 1
  if candidates > maxAssignments then
    IO.eprintln s!"REJECT {path}: {candidates} assignments exceed budget {maxAssignments}"
    return false
  -- Compute once, then check exact membership in both directions. Sorting makes
  -- the executable comparison efficient even when most assignments are solutions.
  if verifySolutions file.model file.solutions then
    IO.println s!"OK {path}: {file.solutions.length} solutions, {candidates} assignments"
    return true
  else
    let expected := file.model.solutions
    let reported := file.solutions
    IO.eprintln s!"REJECT {path}: reported {reported.length}, expected {expected.length} solutions"
    match reported.find? (fun a => !file.model.satisfies a) with
    | some a => IO.eprintln s!"invalid solution: {a}"
    | none => pure ()
    match expected.find? (fun a => !reported.contains a) with
    | some a => IO.eprintln s!"missing solution: {a}"
    | none => pure ()
    if reported.eraseDups.length != reported.length then IO.eprintln "duplicate solutions"
    return false

def main (args : List String) : IO UInt32 := do
  let (maxAssignments, paths) ← match args with
    | "--max-assignments" :: value :: paths =>
      match value.toNat? with
      | some n => pure (n, paths)
      | none => throw (IO.userError "invalid --max-assignments")
    | paths => pure (100000, paths)
  if paths.isEmpty then
    IO.eprintln "usage: minion-semantics [--max-assignments N] FILE.json ..."
    return 2
  let mut success := true
  for path in paths do
    try
      let ok ← checkFile path maxAssignments
      success := success && ok
    catch e =>
      IO.eprintln s!"REJECT {path}: {e}"
      success := false
  return if success then 0 else 1
