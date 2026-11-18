import Minion.BlockSearchProof

/-! Untrusted backward trimming of a search certificate. Nothing here is
    proved: the result is an ordinary `Program`, accepted only if the fixed
    checker accepts it. Trimming too much therefore causes rejection.

    A forward pass tracks domains, solely to recognise no-op restrictions and
    which variable has an empty domain. The backward pass keeps a deduction
    only when a retained check below it reads the restricted variable. A check
    is assumed to read exactly the variables of its root constraint. -/
namespace Minion.Trim
open DomainProof BlockSearchProof

private def bit (i : Nat) : Nat := 1 <<< i

private def termBits (ts : List Term) : Nat :=
  ts.foldl (fun bits t => match t.baseIndex with
    | some i => bits ||| bit i
    | none => bits) 0

def scope : Nat → Constraint → Nat
  | 0, _ => 0
  | _ + 1, .truth _ => 0
  | _ + 1, .compare _ x y | _ + 1, .absolute x y | _ + 1, .minusEq x y => termBits [x, y]
  | _ + 1, .difference x y z | _ + 1, .divide x y z _ | _ + 1, .modulo x y z _
  | _ + 1, .power x y z | _ + 1, .product x y z | _ + 1, .inequality x y z => termBits [x, y, z]
  | _ + 1, .allDifferent xs | _ + 1, .table xs _ _ | _ + 1, .shortTable xs _ _ => termBits xs
  | _ + 1, .element xs index rhs _ _ => termBits (index :: rhs :: xs)
  | _ + 1, .lex xs ys _ | _ + 1, .vectorExists xs ys _ => termBits xs ||| termBits ys
  | _ + 1, .sum xs rhs _ | _ + 1, .nvalue xs rhs _ | _ + 1, .extremum xs rhs _
  | _ + 1, .allDifferentMatrix xs rhs | _ + 1, .inSet rhs xs _ | _ + 1, .inIntervals rhs xs _ =>
      termBits (rhs :: xs)
  | _ + 1, .weightedSum ws xs rhs _ => termBits (rhs :: ws) ||| termBits xs
  | _ + 1, .occurrence xs v n _ => termBits (v :: n :: xs)
  | _ + 1, .hamming xs ys n _ | _ + 1, .literalSum xs ys n => termBits (n :: xs) ||| termBits ys
  | _ + 1, .gcc xs vs ns => termBits xs ||| termBits vs ||| termBits ns
  | _ + 1, .frameUpdate source target removedSource removedTarget blockSize =>
      termBits (blockSize :: source) ||| termBits target ||| termBits removedSource |||
        termBits removedTarget
  | fuel + 1, .reify c guard | fuel + 1, .imply c guard => termBits [guard] ||| scope fuel c
  | fuel + 1, .conjunction cs | fuel + 1, .disjunction cs =>
      cs.foldl (fun bits c => bits ||| scope fuel c) 0

/-- Variables and constraint versions which the retained checks below read. -/
private structure Need where
  variables : Nat
  constraints : Nat

private def Need.union (x y : Need) : Need := ⟨x.variables ||| y.variables, x.constraints ||| y.constraints⟩

private structure State where
  domains : Array (List Int)
  empty : Option Nat

private def State.restrict (s : State) (t : Nat) (op : Operation) (b : Int) : State :=
  let ⟨domains, empty⟩ := s
  let domains := domains.modify t (List.filter (op.holds b))
  ⟨domains, empty <|> (if (domains[t]?.any List.isEmpty) then some t else none)⟩

private inductive Reason where
  | noop
  | emptyDomain (index : Nat)
  | rule

private def classify (s : State) : Step → Reason
  | .prune _ t op b =>
    if (s.domains[t]?.getD [0]).all (op.holds b) then .noop
    else match s.empty with
      | some e => .emptyDomain e
      | none => .rule
  | _ => .rule

private def advance (s : State) : Step → State
  | .prune _ t op b | .explainedPrune _ t op b _ => s.restrict t op b
  | .tableReduce .. | .filterTables _ => s

private def block (steps : List Step) (next : Program) : Program :=
  match steps, next with
  | [], next => next
  | steps, .block more next => .block (steps ++ more) next
  | steps, next => .block steps next

private def keepStep (scopes : Array Nat) (step : Step) (reason : Reason)
    (kept : List Step) (need : Need) : Except String (List Step × Need) :=
  let uses := fun (c t : Nat) =>
    (step :: kept, Need.mk (need.variables ||| scopes[c]?.getD 0 ||| bit t) (need.constraints ||| bit c))
  match step, reason with
  | .prune _ _ _ _, .noop => pure (kept, need)
  | .prune c t _ _, reason =>
    if !need.variables.testBit t then pure (kept, need)
    else match reason, c with
      | .emptyDomain e, _ => pure (step :: kept, { need with variables := need.variables ||| bit e })
      | _, some c => pure (uses c t)
      | _, none => throw "trim: restriction without a constraint is neither a no-op nor in an inconsistent state"
  | .explainedPrune c t _ _ _, _ =>
    pure (if need.variables.testBit t then uses c t else (kept, need))
  | .tableReduce c _, _ | .filterTables c, _ =>
    pure (if need.constraints.testBit c then
      (step :: kept, { need with variables := need.variables ||| scopes[c]?.getD 0 })
    else (kept, need))

private partial def go (scopes : Array Nat) (all : Need) (s : State) :
    Program → Except String (Program × Need)
  | .empty =>
    match s.empty with
    | some e => pure (.empty, ⟨bit e, 0⟩)
    | none => throw "trim: empty-domain leaf without an empty domain"
  | .conflict c =>
    match s.empty with
    | some e => pure (.empty, ⟨bit e, 0⟩)
    | none => pure (.conflict c, ⟨scopes[c]?.getD 0, bit c⟩)
  | .explainedConflict c e => pure (.explainedConflict c e, ⟨scopes[c]?.getD 0, bit c⟩)
  | .solution a => pure (.solution a, all)
  | .block steps next => do
    let mut state := s
    let mut reasons : Array (Step × Reason) := Array.mkEmpty steps.length
    for step in steps do
      reasons := reasons.push (step, classify state step)
      state := advance state step
    let (next, need) ← go scopes all state next
    let mut kept := []
    let mut need := need
    for (step, reason) in reasons.reverse do
      (kept, need) ← keepStep scopes step reason kept need
    pure (block kept next, need)
  | .split t v left right => do
    let (left, needLeft) ← go scopes all (s.restrict t .assign v) left
    if !needLeft.variables.testBit t then return (left, needLeft)
    let (right, needRight) ← go scopes all (s.restrict t .remove v) right
    if !needRight.variables.testBit t then return (right, needRight)
    pure (.split t v left right, needLeft.union needRight)
  | .refute t op b proof next => do
    let (next, needNext) ← go scopes all (s.restrict t op b) next
    if !needNext.variables.testBit t then return (next, needNext)
    let complement := SearchProof.complement op b
    let (proof, needProof) ← go scopes all (s.restrict t complement.1 complement.2) proof
    if !needProof.variables.testBit t then return (proof, needProof)
    pure (.refute t op b proof next, needNext.union needProof)

def trim (m : Model) (p : Program) : Except String Program := do
  let scopes := (m.constraints.map (scope 256)).toArray
  let all : Need := ⟨bit m.variables.length - 1, bit m.constraints.length - 1⟩
  let domains := (m.variables.map Variable.domain).toArray
  let empty := (List.range domains.size).find? fun i => domains[i]?.any List.isEmpty
  return (← go scopes all ⟨domains, empty⟩ p).1

end Minion.Trim
