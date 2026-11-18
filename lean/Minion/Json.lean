import Minion.Check
import Minion.JsonTables
import Std.Data.HashSet

namespace Minion
open Lean

private def field (j : Json) (key : String) : Except String Json := j.getObjVal? key

private def decodeTerm (vars : List Variable) : Nat → Json → Except String Term
  | 0, _ => throw "term view nesting exceeds 256 levels"
  | fuel + 1, j => do
    let kind ← (← field j "kind").getStr?
    let term ← match kind with
      | "constant" => pure (.constant (← fromJson? (← field j "value")))
      | "var" => pure (.var (← fromJson? (← field j "index")))
      | "not_bool" => pure (.notBool (← fromJson? (← field j "index")))
      | "neg" => pure (.neg (← decodeTerm vars fuel (← field j "term")))
      | "shift" => pure (.shift (← decodeTerm vars fuel (← field j "term"))
          (← fromJson? (← field j "offset")))
      | "scale" => pure (.scale (← decodeTerm vars fuel (← field j "term"))
          (← fromJson? (← field j "factor")))
      | _ => throw s!"unknown term kind: {kind}"
    unless term.wellFormed vars do throw "out-of-scope variable or non-Boolean negation"
    return term

private def decodeArgs (vars : List Variable) (j : Json) : Except String (List (List Term)) := do
  let args : List (List Json) ← fromJson? j
  args.mapM fun arg => arg.mapM (decodeTerm vars 256)

private def decodeConstraint (vars : List Variable) : Nat → Json → Except String Constraint
  | 0, _ => throw "constraint nesting exceeds 256 levels"
  | fuel + 1, j => do
    let name ← (← field j "name").getStr?
    let args ← decodeArgs vars (← field j "args")
    let childJson : List Json ← fromJson? (← field j "children")
    let children ← childJson.mapM (decodeConstraint vars fuel)
    let result ← match name, args, children with
      | "true", [], [] => pure (.truth true)
      | "false", [], [] => pure (.truth false)
      | "()()collectevents()()", [_], [] => pure (.truth true)
      | "__reify_eq", [[x], [y], [guard]], [] => pure (.reify (.compare .eq x y) guard)
      | "__reify_diseq", [[x], [y], [guard]], [] => pure (.reify (.compare .ne x y) guard)
      | "__reify_minuseq", [[x], [y], [guard]], [] => pure (.reify (.minusEq x y) guard)
      | "eq", [[x], [y]], []
      | "gaceq", [[x], [y]], []
      | "w-literal", [[x], [y]], [] => pure (.compare .eq x y)
      | "diseq", [[x], [y]], []
      | "watchneq", [[x], [y]], []
      | "w-notliteral", [[x], [y]], [] => pure (.compare .ne x y)
      | "watchless", [[x], [y]], [] => pure (.compare .lt x y)
      | "abs", [[x], [y]], [] => pure (.absolute x y)
      | "difference", [[x], [y], [z]], [] => pure (.difference x y z)
      | "minuseq", [[x], [y]], [] => pure (.minusEq x y)
      | "div", [[x], [y], [z]], [] => pure (.divide x y z false)
      | "div_undefzero", [[x], [y], [z]], [] => pure (.divide x y z true)
      | "modulo", [[x], [y], [z]], [] => pure (.modulo x y z false)
      | "modulo_undefzero", [[x], [y], [z]], [] => pure (.modulo x y z true)
      | "pow", [[x], [y], [z]], [] => pure (.power x y z)
      | "product", [[x], [y], [z]], [] => pure (.product x y z)
      | "ineq", [[x], [y], [k]], [] => pure (.inequality x y k)
      | "alldiff", [xs], []
      | "gacalldiff", [xs], [] => pure (.allDifferent xs)
      | "element", [xs, [i], [r]], []
      | "watchelement", [xs, [i], [r]], [] => pure (.element xs i r 0 false)
      | "element_one", [xs, [i], [r]], []
      | "watchelement_one", [xs, [i], [r]], [] => pure (.element xs i r 1 false)
      | "element_undefzero", [xs, [i], [r]], []
      | "watchelement_undefzero", [xs, [i], [r]], [] =>
          pure (.element xs i r 0 true)
      | "watchelement_one_undefzero", [xs, [i], [r]], [] => pure (.element xs i r 1 true)
      | "lexleq", [xs, ys], []
      | "lexleq[rv]", [xs, ys], []
      | "lexleq[quick]", [xs, ys], [] => pure (.lex xs ys false)
      | "lexless", [xs, ys], []
      | "lexless[quick]", [xs, ys], [] => pure (.lex xs ys true)
      | "sumgeq", [xs, [r]], []
      | "watchsumgeq", [xs, [r]], [] => pure (.sum xs r .ge)
      | "sumleq", [xs, [r]], []
      | "watchsumleq", [xs, [r]], [] => pure (.sum xs r .le)
      | "weightedsumgeq", [ws, xs, [r]], [] => pure (.weightedSum ws xs r .ge)
      | "weightedsumleq", [ws, xs, [r]], [] => pure (.weightedSum ws xs r .le)
      | "occurrence", [xs, [v], [n]], [] => pure (.occurrence xs v n .eq)
      | "occurrencegeq", [xs, [v], [n]], [] => pure (.occurrence xs v n .ge)
      | "occurrenceleq", [xs, [v], [n]], [] => pure (.occurrence xs v n .le)
      | "nvaluegeq", [xs, [n]], [] => pure (.nvalue xs n .ge)
      | "nvalueleq", [xs, [n]], [] => pure (.nvalue xs n .le)
      | "max", [xs, [r]], [] => pure (.extremum xs r true)
      | "min", [xs, [r]], [] => pure (.extremum xs r false)
      | "hamming", [xs, ys, [n]], [] => pure (.hamming xs ys n .ge)
      | "not-hamming", [xs, ys, [n]], [] => pure (.hamming xs ys n .lt)
      | "litsumgeq", [xs, ys, [n]], [] => pure (.literalSum xs ys n)
      | "w-inset", [[x], vs], [] => pure (.inSet x vs true)
      | "w-notinset", [[x], vs], [] => pure (.inSet x vs false)
      | "w-inrange", [[x], vs], []
      | "w-inintervalset", [[x], vs], [] =>
          if vs.length % 2 == 0 then pure (.inIntervals x vs true)
          else throw "interval endpoint list has odd length"
      | "w-notinrange", [[x], [lo, hi]], [] => pure (.inIntervals x [lo, hi] false)
      | "watchvecexists_less", [xs, ys], [] => pure (.vectorExists xs ys .lt)
      | "watchvecneq", [xs, ys], [] => pure (.vectorExists xs ys .ne)
      | "gcc", [xs, vs, ns], []
      | "gccweak", [xs, vs, ns], [] => pure (.gcc xs vs ns)
      | "alldiffmatrix", [xs, [v]], [] => pure (.allDifferentMatrix xs v)
      | "frameupdate", [src, tgt, rs, rt, [bs]], [] => pure (.frameUpdate src tgt rs rt bs)
      | "table", [xs], []
      | "str2plus", [xs], []
      | "gacschema", [xs], []
      | "lighttable", [xs], []
      | "mddc", [xs], []
      | "negativetable", [xs], []
      | "negativemddc", [xs], [] => do
          let tuples ← JsonTables.fullRows xs.length (← field j "tuples")
          pure (.table xs tuples (name != "negativetable" && name != "negativemddc"))
      | "shortstr2", [xs], []
      | "shortctuplestr2", [xs], []
      | "haggisgac", [xs], []
      | "haggisgac-stable", [xs], [] => do
          let tuples ← JsonTables.shortRows xs.length (← field j "short_tuples")
          pure (.shortTable xs tuples (name == "shortctuplestr2"))
      | "reify", [[guard]], [child] => pure (.reify child guard)
      | "reifyimply", [[guard]], [child]
      | "reifyimply-quick", [[guard]], [child] => pure (.imply child guard)
      | "forwardchecking", [], [child]
      | "check[gsa]", [], [child]
      | "check[assign]", [], [child] => pure child
      | "watched-and", [], cs => pure (.conjunction cs)
      | "watched-or", [], cs => pure (.disjunction cs)
      | _, _, _ => throw s!"unknown constraint or invalid argument shape: {name}"
    return result

structure SolutionFile where
  model : Model
  solutions : List Assignment
  deriving Repr

/-- Decoding is deliberately separate from the semantic core. Unknown constraints fail closed. -/
def decodeSolutionFile (j : Json) : Except String SolutionFile := do
  unless (← (← field j "format").getStr?) == "minion-solutions-v1" do
    throw "unsupported solution file format"
  let rawVars : List Json ← fromJson? (← field j "variables")
  let variables ← rawVars.mapM fun v => do
    let name ← (← field v "name").getStr?
    let domain : List Int ← match v.getObjVal? "domain" with
      | .ok value => fromJson? value
      | .error _ => do
        let bounds : List Int ← fromJson? (← field v "interval")
        match bounds with
        | [lo, hi] =>
          let size := (hi - lo + 1).toNat
          unless size ≤ 100000 do throw "interval exceeds replay domain budget (100000 values)"
          pure ((List.range size).map (fun (i : Nat) => lo + Int.ofNat i))
        | _ => throw "interval must have two endpoints"
    unless (Std.HashSet.ofList domain).size == domain.length do throw s!"duplicate domain value in {name}"
    pure ({ name, domain } : Variable)
  let names := variables.map Variable.name
  unless (Std.HashSet.ofList names).size == names.length do throw "duplicate variable name"
  let rawConstraints : List Json ← fromJson? (← field j "constraints")
  let constraints ← rawConstraints.mapM (decodeConstraint variables 256)
  let solutions : List Assignment ← fromJson? (← field j "solutions")
  return { model := { variables, constraints }, solutions }

end Minion
