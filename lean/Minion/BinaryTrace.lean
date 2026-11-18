import Minion.TraceCertificate
import Minion.ProjectionProof

/-! Reader for Minion's compact annotation trace. Nothing here is trusted: the
    proof program it builds is accepted only by the proved checker.

    A file starts with `MINIONTR` and a version byte. Every record has a tag
    byte. Unsigned integers are LEB128, signed integers zigzag. Each record
    except a string definition is one event; events are numbered from 1, and
    the identifiers of contexts, worlds, splits and witnesses are the numbers
    of the events which introduced them, so they are not stored. -/
namespace Minion.BinaryTrace
open Lean

def magic : ByteArray := "MINIONTR".toUTF8.push 1

def isBinary (bytes : ByteArray) : Bool :=
  bytes.size ≥ magic.size && (List.range magic.size).all fun i => bytes[i]! == magic[i]!

structure Decoder where
  bytes : ByteArray
  position : Nat := magic.size
  sequence : Nat := 0
  strings : Array String := #[]
  contexts : Array Nat := #[]
  worlds : Array Nat := #[]

abbrev Read := StateT Decoder (Except String)

def byte : Read Nat := do
  let d ← get
  if h : d.position < d.bytes.size then
    set { d with position := d.position + 1 }
    return d.bytes[d.position].toNat
  else throw "binary trace: unexpected end of file"

partial def unsigned (shift value : Nat := 0) : Read Nat := do
  let b ← byte
  let value := value + ((b % 128) <<< shift)
  if b < 128 then return value else unsigned (shift + 7) value

def signed : Read Int := do
  let n ← unsigned
  return if n % 2 == 0 then ((n / 2 : Nat) : Int) else -(((n / 2 : Nat) : Int)) - 1

def text : Read String := do
  let length ← unsigned
  let d ← get
  if d.position + length > d.bytes.size then throw "binary trace: truncated text"
  set { d with position := d.position + length }
  match String.fromUTF8? (d.bytes.extract d.position (d.position + length)) with
  | some s => return s
  | none => throw "binary trace: invalid text"

def string : Read String := do
  let i ← unsigned
  match (← get).strings[i]? with
  | some s => return s
  | none => throw "binary trace: undefined string"

def operationName : Nat → String
  | 0 => "remove"
  | 1 => "assign"
  | 2 => "lower_bound"
  | _ => "upper_bound"

private def reference (distance : Nat) : Read Nat := do
  let d ← get
  if distance > d.sequence then throw "binary trace: reference before the start"
  return d.sequence - distance

private def event (name : String) (fields : List (String × Json)) : Read Json := do
  return Json.mkObj (("seq", toJson (← get).sequence) :: ("event", toJson name) :: fields)

private def integers : Read (Array Int) := do
  let n ← unsigned
  let mut out := Array.mkEmpty n
  for _ in [0:n] do out := out.push (← signed)
  return out

/-- The next event as a JSON object, or `none` at the end. This rendering is
    for inspection and for tests which edit a trace. -/
partial def next : Read (Option Json) := do
  let d ← get
  if d.position ≥ d.bytes.size then return none
  let tag ← byte
  if tag == 1 then
    let s ← text
    modify fun d => { d with strings := d.strings.push s }
    return ← next
  modify fun d => { d with sequence := d.sequence + 1 }
  let sequence := (← get).sequence
  let context := (← get).contexts.back?.getD 0
  if tag == 0 then
    let body ← text
    match Json.parse ("{\"seq\":" ++ toString sequence ++ "," ++ body ++ "}") with
    | .ok j => return some j
    | .error message => throw s!"binary trace: {message}"
  else if tag == 2 || tag == 3 then
    let constraint ← unsigned
    let rule ← string
    let info ← if tag == 3 then
        match Json.parse (← text) with
        | .ok j => pure j
        | .error message => throw s!"binary trace: {message}"
      else pure (Json.mkObj [])
    modify fun d => { d with contexts := d.contexts.push sequence }
    some <$> event "context_open" [("id", toJson sequence), ("parent", toJson context),
      ("constraint", toJson constraint), ("rule", toJson rule), ("info", info)]
  else if tag == 4 then
    if context == 0 then throw "binary trace: context close without an open context"
    modify fun d => { d with contexts := d.contexts.pop }
    some <$> event "context_close" [("id", toJson context)]
  else if 5 ≤ tag && tag < 13 then
    let operation := operationName ((tag - 5) % 4)
    let target ← unsigned
    let value ← signed
    let site ← string
    let witness ← if tag ≥ 9 then do pure [("witness", toJson (← reference (← unsigned)))] else pure []
    some <$> event "prune" ([("context", toJson context), ("variable", toJson target),
      ("operation", toJson operation), ("value", toJson value),
      ("rule", toJson (site ++ ":" ++ operation)), ("info", Json.mkObj [])] ++ witness)
  else if tag == 13 then
    modify fun d => { d with worlds := d.worlds.push sequence }
    some <$> event "push" [("id", toJson sequence)]
  else if tag == 14 then
    let some world := (← get).worlds.back? | throw "binary trace: pop without a saved world"
    modify fun d => { d with worlds := d.worlds.pop }
    some <$> event "pop" [("id", toJson world)]
  else if tag == 15 || tag == 16 then
    let target ← unsigned
    let value ← signed
    let split ← if tag == 15 then pure sequence else do reference (← unsigned)
    some <$> event "branch" [("split", toJson split), ("variable", toJson target),
      ("value", toJson value), ("equal", toJson (tag == 15))]
  else if tag == 17 then
    some <$> event "branch_close" [("split", toJson (← reference (← unsigned)))]
  else if tag == 22 then
    some <$> event "branch_skip" [("split", toJson (← reference (← unsigned)))]
  else if tag == 18 then some <$> event "conflict" [("context", toJson context)]
  else if tag == 19 then
    some <$> event "conflict" [("context", toJson context), ("witness", toJson (← reference (← unsigned)))]
  else if tag == 20 then some <$> event "backtrack" []
  else if tag == 21 then some <$> event "solution" [("values", toJson (← integers))]
  else if tag == 23 then
    let constraint ← unsigned
    let kind ← string
    let key ← byte
    let items ← integers
    some <$> event "witness" [("id", toJson sequence), ("constraint", toJson constraint),
      ("kind", toJson kind), (if key == 0 then "positions" else "values", toJson items)]
  else throw s!"binary trace: unknown record tag {tag}"

/-- Every event of a binary trace, as JSON objects. -/
def events (bytes : ByteArray) : Except String (Array Json) := do
  unless isBinary bytes do throw "not a Minion binary trace"
  let mut decoder : Decoder := { bytes }
  let mut out := #[]
  repeat
    let (event, after) ← next.run decoder
    decoder := after
    match event with
    | some j => out := out.push j
    | none => break
  return out

/-! ## Reading the proof program

Builds the proof program straight from the records. A deletion is attributed to the top-level constraint which
encloses its context; inside a reification, conjunction or disjunction it also
carries the path to the child which acted. Failed probes (SAC and SSAC) become
refutation subprograms. Large tables are narrowed to the rows still possible
at the start of each search node; Minion's own table-row reductions are not
used. Deletions recorded after a
conflict at the same node are skipped: that node is already a leaf. -/
open DomainProof BlockSearchProof

structure Extraction where
  model : Model
  program : Program
  solutions : List Assignment
  complete : Bool
  /-- For a search which stopped after finding a solution: one constraint for
      each decision whose other branch was never explored, saying that below
      that point the decision holds. The program closes each such branch as a
      conflict with its constraint, which follow the model's own in this order. -/
  unexplored : List Constraint := []
  /-- The non-auxiliary variables, when the model has auxiliary ones. -/
  kept : Option (List Nat) := none
  /-- Branches skipped because a solution had been found and only auxiliary
      variables remained. Each has a constraint in `unexplored`. -/
  skips : List ProjectionProof.Skip := []
  /-- For an optimisation run: whether it minimises, and the objective variable. -/
  objective : Option (Bool × Nat) := none
  /-- A structural fault after the search tree closed. The tree is checked
      first, so that an unjustified leaf which closed it early is reported. -/
  trailing : Option String := none

/-- What the innermost open context stands for. -/
private inductive Scope where
  | constraint (root : Option Nat) (path : List Nat)
  | probe (id : Nat)

private structure Direct where
  /-- Source position of each declared constraint: its root, then child indices. -/
  sources : Std.HashMap Nat (List Nat) := {}
  parents : Std.HashMap Nat Nat := {}
  /-- Roots for which a child path can be used: composite and fully covered. -/
  focus : Array Bool := #[]
  largeTables : List Nat := []
  witnesses : Std.HashMap Nat InferenceWitness.Evidence := {}
  refutations : Std.HashMap Nat Program := {}
  /-- The restriction each probe assumed: variable, lower and upper value. -/
  assumptions : Std.HashMap Nat (Nat × Int × Int) := {}
  /-- For an optimisation run, the index the "strictly better than the best
      solution" constraint will have once the best solution is known. Bounds
      applied from earlier solutions, and the solutions themselves, are steps
      and leaves justified by that constraint. -/
  bound : Option Nat := none
  /-- Constraints for decisions left unexplored when the search stopped after a solution. -/
  unexplored : Array Constraint := #[]
  kept : List Nat := []
  skips : Array ProjectionProof.Skip := #[]
  /-- The decisions from the root to the current node: variable, value, and
      whether the branch taken is the one which assigns the value. -/
  path : List (Nat × Int × Bool) := []
  /-- The index of the first constraint added for an unexplored decision. -/
  unexploredBase : Nat := 0
  /-- The most recent right-branch record: its variable and value. -/
  right : Option (Nat × Int) := none
  stack : Array Scope := #[]
  solutions : Array Assignment := #[]
  complete : Option Bool := none

private inductive Item where
  | step (step : Step)
  | split (target : Nat) (value : Int)
  | right
  | close
  | leaf (program : Program)
  | probeStart (id : Nat)
  | probeEnd (failed : Bool)
  | skip
  | refute (target : Nat) (operation : Operation) (value : Int) (probe : Nat)
  | other
  | finished

private abbrev Extract := StateT Direct (StateT Decoder (Except String))

private def operationOf : Nat → Operation
  | 0 => .remove
  | 1 => .assign
  | 2 => .lowerBound
  | _ => .upperBound

private def hasLargeTable : Nat → Constraint → Bool
  | 0, _ => false
  | _ + 1, .table _ rows _ => rows.length ≥ 512
  | fuel + 1, .reify c _ | fuel + 1, .imply c _ => hasLargeTable fuel c
  | fuel + 1, .conjunction cs | fuel + 1, .disjunction cs => cs.any (hasLargeTable fuel)
  | _ + 1, _ => false

/-- The nearest enclosing constraint which has a source position. -/
private def location (s : Direct) : Nat → Nat → List Nat
  | 0, _ => []
  | fuel + 1, id =>
    match s.sources[id]? with
    | some path => path
    | none => match s.parents[id]? with
      | some parent => location s fuel parent
      | none => []

private def evidence (distance : Nat) : Extract InferenceWitness.Evidence := do
  let id ← (reference distance : Read Nat)
  match (← get).witnesses[id]? with
  | some e => return e
  | none => throw "binary trace: reference to a missing witness"

/-- The enclosing root, and the child path when the checker can use one. -/
private def scope : Extract (Option Nat × Option (List Nat)) := do
  let s ← get
  match s.stack.back? with
  | some (.constraint (some c) path) =>
    return (some c, if !path.isEmpty && s.focus[c]?.getD false then some path else none)
  | _ => return (none, none)

private def field {α : Type} [FromJson α] (j : Json) (name : String) : Extract α :=
  match j.getObjValAs? α name with
  | .ok value => pure value
  | .error message => throw s!"binary trace: {message}"

private def json (body : String) : Extract Item := do
  let j ← match Json.parse ("{" ++ body ++ "}") with
    | .ok j => pure j
    | .error message => throw s!"binary trace: {message}"
  let kind ← match j.getObjValAs? String "event" with
    | .ok kind => pure kind
    | .error message => throw message
  if kind == "probe" then
    let id := (← (get : Read Decoder)).sequence
    let assumed : Nat × Int × Int := (← field j "variable", ← field j "lower", ← field j "upper")
    modify fun s => { s with assumptions := s.assumptions.insert id assumed }
    return .probeStart id
  if kind == "probe_result" then
    match j.getObjValAs? Bool "failed" with
    | .ok failed => return .probeEnd failed
    | .error message => throw message
  if kind == "constraint" then
    let id : Nat ← field j "id"
    if let .ok path := j.getObjValAs? String "source_path" then
      if let some indices := (path.splitOn "/").mapM String.toNat? then
        modify fun s => { s with sources := s.sources.insert id indices }
    return .other
  else if kind == "child" then
    let parent : Nat ← field j "parent"
    let child : Nat ← field j "child"
    modify fun s => { s with parents := s.parents.insert child parent }
    return .other
  else if kind == "table_reduce" then return .other
  else if kind == "end" then
    let complete ← match j.getObjValAs? Bool "complete" with
      | .ok complete => pure complete
      | .error message => throw message
    modify fun s => { s with complete := some complete }
    return .finished
  else throw s!"binary trace: unexpected event {kind}"

/-- One record, as an item of the search structure. -/
private def item : Extract Item := do
  let d ← (get : Read Decoder)
  if d.position ≥ d.bytes.size then return .finished
  let tag ← (byte : Read Nat)
  if tag == 1 then
    let s ← (text : Read String)
    modifyThe Decoder fun d => { d with strings := d.strings.push s }
    return .other
  modifyThe Decoder fun d => { d with sequence := d.sequence + 1 }
  if tag == 0 then json (← (text : Read String))
  else if tag == 2 then
    let constraint ← (unsigned : Read Nat)
    let rule ← (string : Read String)
    let s ← get
    if rule == "optimise.bound" && s.bound.isNone then
      throw "binary trace: objective bound in a search which does not optimise"
    let scope := if rule == "optimise.bound" then Scope.constraint s.bound [] else
      match location s 256 constraint with
      | root :: path => Scope.constraint (some root) path
      | [] => Scope.constraint none []
    modify fun s => { s with stack := s.stack.push scope }
    return .other
  else if tag == 3 then
    let _constraint ← (unsigned : Read Nat)
    let rule ← (string : Read String)
    let info ← (text : Read String)
    unless rule == "probe.refutation" do throw s!"binary trace: unexpected context {rule}"
    let probe ← match Json.parse info >>= (·.getObjValAs? Nat "probe") with
      | .ok probe => pure probe
      | .error message => throw s!"binary trace: {message}"
    modify fun s => { s with stack := s.stack.push (.probe probe) }
    return .other
  else if tag == 4 then
    modify fun s => { s with stack := s.stack.pop }
    return .other
  else if 5 ≤ tag && tag < 13 then
    let target ← (unsigned : Read Nat)
    let value ← (signed : Read Int)
    let _site ← (unsigned : Read Nat)
    let operation := operationOf ((tag - 5) % 4)
    if let some (.probe probe) := (← get).stack.back? then
      return .refute target operation value probe
    let (root, path) ← scope
    if tag ≥ 9 then
      let e ← evidence (← (unsigned : Read Nat))
      match root with
      | some c => return .step (.explainedPrune c target operation value e)
      | none => throw "binary trace: witnessed deletion outside a constraint"
    else match root, path with
      | some c, some path => return .step (.explainedPrune c target operation value (.preferFocus path))
      | root, _ => return .step (.prune root target operation value)
  else if tag == 13 || tag == 14 || tag == 20 then return .other
  else if tag == 15 then return .split (← (unsigned : Read Nat)) (← (signed : Read Int))
  else if tag == 16 then
    let target ← (unsigned : Read Nat)
    let value ← (signed : Read Int)
    let _ ← (unsigned : Read Nat)
    modify fun s => { s with right := some (target, value) }
    return .right
  else if tag == 17 then
    let _ ← (unsigned : Read Nat)
    return .close
  else if tag == 22 then
    let _ ← (unsigned : Read Nat)
    return .skip
  else if tag == 18 then
    return .leaf (match ← scope with
      | (some c, some path) => .explainedConflict c (.preferFocus path)
      | (some c, none) => .conflict c
      | (none, _) => .empty)
  else if tag == 19 then
    let e ← evidence (← (unsigned : Read Nat))
    match (← scope).1 with
    | some c => return .leaf (.explainedConflict c e)
    | none => throw "binary trace: witnessed conflict outside a constraint"
  else if tag == 21 then
    let a := (← (integers : Read (Array Int))).toList
    modify fun s => { s with solutions := s.solutions.push a }
    match (← get).bound with
    | some k => return .leaf (.conflict k)
    | none => return .leaf (.solution a)
  else if tag == 23 then
    let _constraint ← (unsigned : Read Nat)
    let kind ← (string : Read String)
    let _key ← (byte : Read Nat)
    let items := (← (integers : Read (Array Int))).toList
    let e ← match kind with
      | "hall" => pure (InferenceWitness.Evidence.hall (items.map Int.toNat))
      | "gcc_upper" => pure (InferenceWitness.Evidence.gccUpper (items.map Int.toNat))
      | "gcc_lower" => pure (InferenceWitness.Evidence.gccLower items)
      | _ => throw "binary trace: unknown witness kind"
    let id := (← (get : Read Decoder)).sequence
    modify fun s => { s with witnesses := s.witnesses.insert id e }
    return .other
  else throw s!"binary trace: unknown record tag {tag}"

/-- The stream stopped inside the search tree. -/
private def stopped {α : Type} : Extract α := do
  if (← get).complete == some false then
    throw "the search stopped before finding a solution: there is nothing to certify"
  else throw "binary trace: search ended inside the search tree"

/-- The trace ended because the search stopped once it had a solution. -/
private def stoppedAfterSolution : Extract Bool := do
  let s ← get
  return s.complete == some false && !s.solutions.isEmpty && s.bound.isNone

private def blockOf (steps : Array Step) (next : Program) : Program :=
  if steps.isEmpty then next else .block steps.toList next

/-- What follows a subtree: its sibling or the close of its branch, the end of
    a search which stopped at a solution, or a skip of the sibling because
    only auxiliary variables remained. -/
private inductive After where
  | continues | stoppedHere | skipped
  deriving BEq

/-- Skip the records of an already closed node, up to the wanted marker. -/
private partial def skipTo (right : Bool) : Extract After := do
  match ← item with
  | .right => if right then return .continues else throw "binary trace: unexpected right branch"
  | .close => if right then throw "binary trace: branch closed before its right side" else return .continues
  | .skip => if right then return .skipped else throw "binary trace: branch skipped after its right side"
  | .finished => if ← stoppedAfterSolution then return .stoppedHere else stopped
  | .split .. => throw "binary trace: branch inside a closed node"
  | .leaf (.solution _) => throw "binary trace: solution after a closed subtree"
  | _ => skipTo right

/-- The refutation a deduction rests on. The deduction must be the negation
    of what that probe assumed. -/
private def refutation (probe target : Nat) (operation : Operation) (value : Int) : Extract Program := do
  let s ← get
  let some (assumed, lower, upper) := s.assumptions[probe]?
    | throw "binary trace: deduction from an unknown probe"
  -- The lower end of a range probe is not compared with the variable's
  -- current minimum: this reader does not follow the domains.
  let negation := assumed == target && lower ≤ upper && match operation with
    | .remove => lower == value && upper == value
    | .lowerBound => upper == value - 1
    | .upperBound => lower == value + 1
    | .assign => false
  unless negation do throw "binary trace: probe deduction is not the negation of its assumption"
  match s.refutations[probe]? with
  | some proof => return proof
  | none => throw "binary trace: deduction from a probe which did not fail"

/-- The records of one probe, after its opening record. A failed probe ends
    in a conflict and yields a refutation; a probe which found no conflict
    yields nothing. Probes nest. -/
private partial def probeBody (steps : Array Step := #[]) : Extract (Option Program) := do
  match ← item with
  | .step step => probeBody (steps.push step)
  | .other => probeBody steps
  | .leaf program =>
    let rec close : Extract Unit := do
      match ← item with
      | .probeEnd true => return
      | .probeEnd false => throw "binary trace: probe reached a conflict but did not fail"
      | .finished => stopped
      | .probeStart _ | .split .. => throw "binary trace: search inside a closed probe"
      | _ => close
    close
    return some (blockOf steps program)
  | .probeEnd false => return none
  | .probeEnd true => throw "binary trace: failed probe has no closing conflict"
  | .probeStart id =>
    if let some proof ← probeBody then
      modify fun s => { s with refutations := s.refutations.insert id proof }
    probeBody steps
  | .refute target operation value probe =>
    let proof ← refutation probe target operation value
    return (← probeBody).map fun next => blockOf steps (.refute target operation value proof next)
  | .split .. | .right | .close | .skip => throw "binary trace: branching inside a probe"
  | .finished => stopped

private partial def node (steps : Array Step := #[]) : Extract Program := do
  match ← item with
  | .step step => node (steps.push step)
  | .other => node steps
  | .probeStart id =>
    if let some proof ← probeBody then
      modify fun s => { s with refutations := s.refutations.insert id proof }
    node steps
  | .refute target operation value probe =>
    let proof ← refutation probe target operation value
    return blockOf steps (.refute target operation value proof (← node))
  | .probeEnd _ => throw "binary trace: probe result without a probe"
  | .leaf program => return blockOf steps program
  | .split target value =>
    let narrow := ((← get).largeTables.map Step.filterTables).toArray
    let path := (← get).path
    modify fun s => { s with path := (target, value, true) :: path }
    let left ← node narrow
    modify fun s => { s with path := path }
    let after ← skipTo true
    if after == .continues then
      unless (← get).right == some (target, value) do
        throw "binary trace: right branch does not negate its left branch"
      modify fun s => { s with path := (target, value, false) :: path }
      let right ← node narrow
      modify fun s => { s with path := path }
      -- The trace may end here when the solution it stopped at is in this branch.
      let _ ← skipTo false
      return blockOf steps (.split target value left right)
    else if after == .skipped then
      -- A solution was found below, and the other side would only vary
      -- auxiliary variables. Close it with a constraint it violates wherever
      -- every other variable already has that solution's value.
      let s ← get
      let some solution := s.solutions.back? | throw "binary trace: branch skipped before any solution"
      let outside := path.reverse.map (fun (p : Nat × Int × Bool) =>
          Constraint.compare (if p.2.2 then .ne else .eq) (.var p.1) (.constant p.2.1)) ++
        [Constraint.compare .eq (.var target) (.constant value)]
      let skip : ProjectionProof.Skip := ⟨outside, ProjectionProof.project s.kept solution⟩
      set { s with unexplored := s.unexplored.push (skip.constraint s.kept), skips := s.skips.push skip }
      return blockOf steps (.split target value left (.conflict (s.unexploredBase + s.unexplored.size)))
    else
      -- The search stopped inside the left branch. The right branch is
      -- outside what was searched: record the decision and close it.
      let s ← get
      set { s with unexplored := s.unexplored.push (PartialProof.below path.reverse target value) }
      return blockOf steps (.split target value left (.conflict (s.unexploredBase + s.unexplored.size)))
  | .right | .close | .skip => throw "binary trace: branch marker without a branch"
  | .finished => stopped

private partial def finishStream : Extract Unit := do
  match ← item with
  | .finished => return
  | .split .. => throw "binary trace: branch after the search tree"
  | .leaf (.solution _) => throw "binary trace: solution after a closed subtree"
  | _ => finishStream

/-- A witnessed step has no clause for a domain which is already empty, nor
    for a restriction which changes nothing; the plain step forms do. Follow
    the domains and use the plain form in those states. -/
private partial def normalize (d : Array (List Int)) : Program → Program
  | .explainedConflict c e => if d.any List.isEmpty then .empty else .explainedConflict c e
  | .block steps next => Id.run do
    let mut d := d
    let mut out := Array.mkEmpty steps.length
    for step in steps do
      match step with
      | .explainedPrune c t op b _ =>
        let plain := d.any List.isEmpty || (d[t]?.getD [0]).all (op.holds b)
        out := out.push (if plain then .prune (some c) t op b else step)
        d := d.modify t (List.filter (op.holds b))
      | .prune _ t op b =>
        out := out.push step
        d := d.modify t (List.filter (op.holds b))
      | _ => out := out.push step
    return .block out.toList (normalize d next)
  | .split t v left right =>
    .split t v (normalize (d.modify t (List.filter (Operation.assign.holds v))) left)
      (normalize (d.modify t (List.filter (Operation.remove.holds v))) right)
  | .refute t op b proof next =>
    let c := SearchProof.complement op b
    .refute t op b (normalize (d.modify t (List.filter (c.1.holds c.2))) proof)
      (normalize (d.modify t (List.filter (op.holds b))) next)
  | p => p

def direct (bytes : ByteArray) : Except String Extraction := do
  unless isBinary bytes do throw "not a Minion binary trace"
  let start : Decoder := { bytes }
  let (first, afterModel) ← (next.run start)
  let some first := first | throw "binary trace: empty"
  unless (first.getObjValAs? String "event").toOption == some "model" &&
      (first.getObjValAs? String "format").toOption == some "minion-annotations-v1" do
    throw "binary trace: the first record must declare the model"
  let model := (← decodeSolutionFile (← first.getObjVal? "model")).model
  let objective ← match first.getObjVal? "optimise" with
    | .error _ => pure none
    | .ok o => do
      let minimising ← o.getObjValAs? Bool "minimising"
      let term ← o.getObjVal? "variable"
      unless (term.getObjValAs? String "kind").toOption == some "var" do
        throw "binary trace: the objective must be a plain variable"
      pure (some (minimising, ← term.getObjValAs? Nat "index"))
  let auxiliary : List Nat := (first.getObjValAs? (List Nat) "auxiliary").toOption.getD []
  let kept := (List.range model.variables.length).filter (!auxiliary.contains ·)
  if objective.isSome && !auxiliary.isEmpty then
    throw "optimisation with auxiliary variables is not supported yet"
  let initial : Direct := {
    kept := kept
    bound := objective.map fun _ => model.constraints.length
    unexploredBase := model.constraints.length
    focus := (model.constraints.map fun c => LogicProof.isComposite c && LogicProof.supported c).toArray
    largeTables := (model.constraints.zipIdx.filter fun p => hasLargeTable 256 p.1).map Prod.snd }
  let run : Extract (Program × Option String) := do
    let program ← node (initial.largeTables.map Step.filterTables).toArray
    let trailing ← try finishStream; pure none catch message => pure (some message)
    return (program, trailing)
  let (((program, trailing), s), _) ← (run.run initial).run afterModel
  let program := normalize (model.variables.map Variable.domain).toArray program
  if trailing.isSome then return ⟨model, program, s.solutions.toList, true, [], none, [], objective, trailing⟩
  let some complete := s.complete | throw "missing end marker (truncated trace)"
  if !complete && (s.solutions.isEmpty || objective.isSome) then
    throw "the search stopped before finishing, and not at a solution: there is nothing to certify"
  return ⟨model, program, s.solutions.toList, complete, s.unexplored.toList,
    if auxiliary.isEmpty then none else some kept, s.skips.toList, objective, none⟩

/-! ## Naming the step which fails

When the proved checker rejects a program, walk it once more, applying the
same proved rules step by step, and describe the first step which does not
hold. This only produces the message; it never turns a rejection into an
acceptance. -/

private def constraintName (m : Model) (c : Nat) : String :=
  match m.constraints[c]? with
  | none => s!"constraint {c} (not in the model)"
  | some k => s!"constraint {c} ({((((repr k).pretty 1000000).splitOn " ").headD "?").replace "Minion.Constraint." ""})"

private def variableName (m : Model) (t : Nat) : String :=
  match m.variables[t]? with
  | some v => v.name
  | none => s!"variable {t} (not in the model)"

private def restriction (m : Model) (t : Nat) (op : Operation) (b : Int) : String :=
  variableName m t ++ (match op with
    | .remove => " ≠ " | .assign => " = " | .lowerBound => " ≥ " | .upperBound => " ≤ ") ++ toString b

private structure Walk where
  steps : Nat := 0
  path : List String := []

private def located (w : Walk) (message : String) : String :=
  s!"step {w.steps + 1}" ++ (if w.path.isEmpty then "" else " under " ++ ", ".intercalate w.path.reverse) ++
    ": " ++ message

private partial def firstFailure (m : Model) (d : List (List Int)) (w : Walk) :
    Program → Except String Walk
  | .empty =>
    if hasEmptyDomain d then pure w else throw (located w "leaf claims an empty domain, but none is empty")
  | .conflict c =>
    if SearchProof.check m d (.conflict c) then pure w
    else throw (located w s!"conflict not justified by {constraintName m c}")
  | .explainedConflict c e =>
    if SearchProof.check m d (.explainedConflict c e) then pure w
    else throw (located w s!"conflict not justified by {constraintName m c} with the supplied reason")
  | .solution a =>
    if d != a.map (fun v => [v]) then
      throw (located w "solution reported before the domains were reduced to exactly that assignment")
    else if m.satisfies a then pure w
    else throw (located w "reported solution does not satisfy the model")
  | .block steps next => do
    let mut d := d
    let mut m := m
    let mut w := w
    for step in steps do
      match step with
      | .prune c t op b =>
        unless SearchProof.pruneCheck m d c t op b do
          throw (located w s!"deletion {restriction m t op b} not justified by " ++
            (match c with | some c => constraintName m c | none => "any constraint"))
        d := SearchProof.restrict d t op b
      | .explainedPrune c t op b e =>
        unless (m.constraints[c]?).any (fun k => (InferenceWitness.rule e (some t)).prune (ofLists d) k t op b) do
          throw (located w s!"deletion {restriction m t op b} not justified by {constraintName m c} with the supplied reason")
        d := SearchProof.restrict d t op b
      | .tableReduce c kept =>
        unless TableProjection.checkModel m d c kept do
          throw (located w s!"table rows discarded from {constraintName m c} are still possible")
        m := TableProjection.reduceModel m c kept
      | .filterTables c => m := TableProjection.filterModel m d c
      w := { w with steps := w.steps + 1 }
    firstFailure m d w next
  | .split t v left right => do
    unless t < d.length do throw (located w s!"branch on {variableName m t}")
    let afterLeft ← firstFailure m (SearchProof.restrict d t .assign v)
      { w with path := restriction m t .assign v :: w.path } left
    let afterRight ← firstFailure m (SearchProof.restrict d t .remove v)
      { afterLeft with path := restriction m t .remove v :: w.path } right
    pure { afterRight with path := w.path }
  | .refute t op b proof next => do
    let c := SearchProof.complement op b
    let afterProof ← firstFailure m (SearchProof.reject d t op b)
      { w with path := s!"probe {restriction m t c.1 c.2}" :: w.path } proof
    unless (BlockSearchProof.solutions proof).isEmpty do
      throw (located w s!"probe {restriction m t c.1 c.2} did not fail")
    firstFailure m (SearchProof.restrict d t op b) { afterProof with path := w.path } next

private partial def tally : Program → Nat × Nat × Nat × Nat
  | .block steps next =>
    let (d, l, p, f) := tally next
    let filters := steps.countP fun step => match step with | .filterTables _ => true | _ => false
    (d + steps.length - filters, l, p, f + filters)
  | .split _ _ left right =>
    let (d, l, p, f) := tally left
    let (d', l', p', f') := tally right
    (d + d', l + l', p + p', f + f')
  | .refute _ _ _ proof next =>
    let (d, l, p, f) := tally proof
    let (d', l', p', f') := tally next
    (d + d' + 1, l + l', p + p' + 1, f + f')
  | _ => (0, 1, 0, 0)

/-- What an accepted program contained. Every counted item was checked. -/
def summary (p : Program) : String :=
  let (deductions, leaves, probes, filters) := tally p
  s!"{deductions} deductions, {leaves} leaves, {probes} probe assumptions, " ++
    s!"{filters} computed table filters CHECKED; 0 prunings and 0 conflicts UNCHECKED"

/-- A description of the first step which does not hold, if there is one. -/
def explain (m : Model) (p : Program) : Option String :=
  match firstFailure m (m.variables.map Variable.domain) {} p with
  | .ok _ => none
  | .error message => some message

/-- Whether some all-different or GCC step lacks a supplied witness. Steps
    which change nothing, or follow a domain wipe-out, need none. -/
partial def missingWitness (m : Model) (d : Array (List Int)) : Program → Bool
  | .conflict c => !d.any List.isEmpty && (m.constraints[c]?).any SearchProof.needsMatching
  | .explainedConflict c (.preferFocus _) | .explainedConflict c (.focus _) =>
    (m.constraints[c]?).any SearchProof.needsMatching
  | .block steps next => Id.run do
    let mut d := d
    for step in steps do
      match step with
      | .prune c t op b =>
        let idle := d.any List.isEmpty || (d[t]?.getD [0]).all (op.holds b)
        if !idle && (c.bind (m.constraints[·]?)).any SearchProof.needsMatching then return true
        d := d.modify t (List.filter (op.holds b))
      | .explainedPrune c t op b e =>
        let focused := match e with | .preferFocus _ | .focus _ => true | _ => false
        if focused && (m.constraints[c]?).any SearchProof.needsMatching then return true
        d := d.modify t (List.filter (op.holds b))
      | _ => pure ()
    return missingWitness m d next
  | .split t v left right =>
    missingWitness m (d.modify t (List.filter (Operation.assign.holds v))) left ||
      missingWitness m (d.modify t (List.filter (Operation.remove.holds v))) right
  | .refute t op b proof next =>
    let c := SearchProof.complement op b
    missingWitness m (d.modify t (List.filter (c.1.holds c.2))) proof ||
      missingWitness m (d.modify t (List.filter (op.holds b))) next
  | _ => false

end Minion.BinaryTrace
