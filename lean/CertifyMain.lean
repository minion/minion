import Minion.TraceCertificate
import Minion.BinaryTrace
import Minion.OptimalProof
import Minion.ProjectionProof

open Minion

structure Options where
  auditBudget : Option Nat := none
  exportLean : Option String := none
  requireWitnesses : Bool := false
  layout : Option TraceCertificate.Layout := none
  reportShape : Bool := false
  trim : Bool := false
  boundsEvidence : Bool := true

private def fail {α : Type} (message : String) : IO α := throw (IO.userError message)

/-- Read a binary trace, build its proof program and have the proved checker
    accept or reject it. Everything before the check is untrusted. -/
private def certify (path : String) (o : Options) : IO Unit := do
  let extracted ← match BinaryTrace.direct (← IO.FS.readBinFile path) with
    | .ok extracted => pure extracted
    | .error message => fail message
  -- An optimisation run which found a solution is checked as a refutation of
  -- the model with "strictly better than the best solution" added. The
  -- solutions found on the way are leaves of that refutation.
  let best := extracted.solutions.getLast?
  let optimum := do
    let (minimising, objective) ← extracted.objective
    let best ← best
    pure (minimising, objective, best, OptimalProof.value objective best)
  if let some (minimising, objective, _, _) := optimum then
    let values := extracted.solutions.map (OptimalProof.value objective)
    unless extracted.solutions.all extracted.model.satisfies do
      fail "a solution reported during optimisation does not satisfy the model"
    unless (values.zip values.tail).all fun (earlier, later) =>
        if minimising then later < earlier else earlier < later do
      fail "solutions reported during optimisation do not strictly improve"
  -- A search which stopped at a solution is checked as a complete search of
  -- the model restricted to the decisions whose other branch it never explored.
  let model := match optimum with
    | some (minimising, objective, _, v) => OptimalProof.improved extracted.model minimising objective v
    | none => PartialProof.restricted extracted.model extracted.unexplored
  -- With auxiliary variables, each skipped branch names the solution it was
  -- skipped for; that solution's non-auxiliary values must be a reported one's.
  let kept := extracted.kept.getD []
  unless extracted.skips.all (fun s => s.values.length == kept.length &&
      extracted.solutions.any fun r => ProjectionProof.project kept r == s.values) do
    fail "a branch was skipped for a solution which is not among those reported"
  let reported := if optimum.isSome then [] else extracted.solutions
  if o.requireWitnesses &&
      BinaryTrace.missingWitness model (model.variables.map Variable.domain).toArray extracted.program then
    fail "matching/flow discovery disabled: supplied witness required"
  -- Untrusted rewrites of the program: name the interval rule where it
  -- suffices, and optionally keep only the steps the leaves depend on.
  let mut program := extracted.program
  if o.boundsEvidence then program := BoundsEvidence.annotate model program
  if o.trim then
    match TraceCertificate.trim model program with
    | .ok (trimmed, report) =>
      program := trimmed
      IO.println ("TRIM " ++ report.compress)
    | .error message => fail message
  match TraceCertificate.verify model reported program (o.layout.getD .blocksArray) with
  | .error message =>
    fail ((BinaryTrace.explain model program).getD
      ("every step holds, but the trace's reported solutions are not exactly its solution leaves (" ++ message ++ ")"))
  | .ok _ => pure ()
  -- A fault after the tree closed is reported only once the tree itself
  -- holds, so that an unjustified leaf which closed it early is named first.
  if let some message := extracted.trailing then fail message
  if let some budget := o.auditBudget then
    let size := (model.variables.map (fun v => v.domain.length)).foldl (· * ·) 1
    unless size ≤ budget do fail s!"{size} assignments exceed audit budget {budget}"
    match optimum with
    | none =>
      if extracted.complete && extracted.kept.isSome then
        -- Enumerate the original model: its solutions and the reported ones
        -- have the same non-auxiliary parts, each reported once.
        let found := extracted.solutions.map (ProjectionProof.project kept)
        let all := extracted.model.solutions.map (ProjectionProof.project kept)
        unless found.eraseDups.length == found.length && found.all all.contains && all.all found.contains do
          fail "projected solution-set audit failed"
      else unless verifySolutions model extracted.solutions do fail "solution-set audit failed"
    | some (minimising, objective, _, v) =>
      -- Enumerate the original model: nothing beats the claimed optimum, and something attains it.
      let values := extracted.model.solutions.map (OptimalProof.value objective)
      unless values.contains v && values.all (fun w => if minimising then v ≤ w else w ≤ v) do
        fail "optimum audit failed"
  if let some output := o.exportLean then
    match TraceCertificate.exportLean model program (o.layout.getD .automatic) with
    | .ok source => IO.FS.writeFile output source
    | .error message => fail message
  if o.reportShape then
    match TraceCertificate.shape model program with
    | .ok shape => IO.println ("SHAPE " ++ shape.compress)
    | .error message => fail message
  match optimum with
  | some (minimising, objective, _, v) =>
    let name := (extracted.model.variables[objective]?.map (·.name)).getD s!"variable {objective}"
    IO.println s!"OPTIMAL {path}: {if minimising then "minimum" else "maximum"} {name} = {v}, after {extracted.solutions.length} improving solutions; {BinaryTrace.summary program}{if o.auditBudget.isSome then "; optimum audited by enumeration" else ""}; optimality CERTIFIED"
  | none =>
    if extracted.complete && extracted.kept.isSome then
      IO.println s!"PROJECTED {path}: {extracted.solutions.length} solutions, one for each assignment of the {kept.length} non-auxiliary variables which extends to a solution; {extracted.skips.length} auxiliary branches skipped; {BinaryTrace.summary program}{if o.auditBudget.isSome then "; projected solution set audited" else ""}; projected solution set CERTIFIED"
    else if extracted.complete then
      IO.println s!"CERTIFIED {path}: {extracted.solutions.length} solutions; {BinaryTrace.summary program}{if o.auditBudget.isSome then "; complete solution set audited" else ""}; complete search tree CERTIFIED"
    else
      IO.println s!"SOLUTION {path}: {extracted.solutions.length} solutions before the search stopped, {extracted.unexplored.length} decisions left unexplored; {BinaryTrace.summary program}{if o.auditBudget.isSome then "; searched region audited by enumeration" else ""}; search up to the solution CERTIFIED"

/-- Rewrite a trace as JSON lines, one event per line. -/
private def toJson (path output : String) : IO Unit := do
  match BinaryTrace.events (← IO.FS.readBinFile path) with
  | .error message => fail message
  | .ok events =>
    IO.FS.withFile output .write fun handle => do
      for event in events do handle.putStrLn event.compress

private def usage : String :=
  "usage: minion-certify [--to-json OUT.jsonl TRACE] | [--audit-solutions N] [--no-matching-discovery] " ++
  "[--trim] [--no-bounds-evidence] [--certificate-shape] " ++
  "[--certificate-layout auto|tree|blocks-array|blocks-index|blocks-index-build] [--export-lean FILE.lean] TRACE ..."

def main (args : List String) : IO UInt32 := do
  if let ["--to-json", output, path] := args then
    toJson path output
    return 0
  let mut o : Options := {}
  let mut paths : List String := []
  let mut rest := args
  while !rest.isEmpty do
    match rest with
    | "--audit-solutions" :: value :: tail =>
      match value.toNat? with
      | some n => o := { o with auditBudget := some n }; rest := tail
      | none => fail "invalid audit budget"
    | "--no-matching-discovery" :: tail => o := { o with requireWitnesses := true }; rest := tail
    | "--no-bounds-evidence" :: tail => o := { o with boundsEvidence := false }; rest := tail
    | "--trim" :: tail => o := { o with trim := true }; rest := tail
    | "--certificate-shape" :: tail => o := { o with reportShape := true }; rest := tail
    | "--certificate-layout" :: value :: tail =>
      match TraceCertificate.parseLayout value with
      | .ok selected => o := { o with layout := some selected }
      | .error message => fail message
      rest := tail
    | "--export-lean" :: output :: tail => o := { o with exportLean := some output }; rest := tail
    | path :: tail =>
      if path.startsWith "--" then
        IO.eprintln s!"unknown option {path}\n{usage}"
        return 2
      paths := paths ++ [path]
      rest := tail
    | [] => pure ()
  if paths.isEmpty then
    IO.eprintln usage
    return 2
  if o.exportLean.isSome && paths.length != 1 then fail "--export-lean requires exactly one input trace"
  let mut success := true
  for path in paths do
    try certify path o
    catch error =>
      IO.eprintln s!"REJECT {path}: {error}"
      success := false
  return if success then 0 else 1
