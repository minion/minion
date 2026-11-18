import Minion.CertificateShape
import Minion.Json
import Minion.CertificateData
import Minion.Trim
import Minion.BoundsEvidence

/-! Checking and exporting a proof program read from a trace. Reading and
    the optional rewrites are untrusted: acceptance requires the proved
    checker to accept the program from the original model domains, and the
    trace's reported solutions to be exactly its solution leaves. -/
namespace Minion.TraceCertificate
open Lean DomainProof SearchProof

/-- The untrusted trimmed form of a program, with the certificate shape before
    and after. Acceptance still checks the result. -/
def trim (m : Model) (program : BlockSearchProof.Program) :
    Except String (BlockSearchProof.Program × Json) := do
  let trimmed ← Trim.trim m program
  return (trimmed, Json.mkObj [
    ("before", toJson (CertificateShape.program program)),
    ("after", toJson (CertificateShape.program trimmed))])

inductive Layout where
  | automatic | tree | blocksArray | blocksIndex | blocksIndexBuild
  deriving BEq, Repr

def parseLayout : String → Except String Layout
  | "auto" => pure .automatic
  | "tree" => pure .tree
  | "blocks-array" => pure .blocksArray
  | "blocks-index" => pure .blocksIndex
  | "blocks-index-build" => pure .blocksIndexBuild
  | _ => throw "certificate layout must be auto, tree, blocks-array, blocks-index or blocks-index-build"

def verify (m : Model) (reported : List Assignment) (program : BlockSearchProof.Program)
    (layout : Layout := .blocksArray) : Except String Unit := do
  let accepted := match layout with
    | .tree => FastSearchProof.verify m (BlockSearchProof.expand program) reported
    | .automatic | .blocksArray => BlockSearchProof.verify m program reported false
    | .blocksIndex | .blocksIndexBuild => BlockSearchProof.verify m program reported true
  unless accepted do
    if !BlockSearchProof.check m (m.variables.map Variable.domain) program false then
      throw "certificate: pure search-tree check failed"
    else throw "certificate: reported solutions are not exactly the solution leaves"

-- Deep search trees otherwise accumulate quadratic indentation. Generated
-- data stays on compact lines; the propositions below remain easy to inspect.
private def dataRepr [Repr α] (value : α) : String := (repr value).pretty 1000000000

private structure Declarations where
  next : Nat := 0
  reversed : List String := []

private def declareData (type value : String) : StateM Declarations String := do
  let s ← getThe Declarations
  let name := "data_" ++ toString s.next
  set (Declarations.mk (s.next + 1)
    (("def " ++ name ++ " : " ++ type ++ " :=\n" ++ value) :: s.reversed))
  return name

/-- Bound the size of individual list literals. Large literals otherwise spend
    excessive time in syntax elaboration, before the acceptance theorem starts.
    Balanced named appends also avoid creating a long chain of definitions. -/
private def listData [Repr α] (type : String) (values : List α)
    (render : List α → String) : StateM Declarations String := do
  if values.length ≤ 128 then return dataRepr values
  let mut rest := values
  let mut names : Array String := #[]
  while !rest.isEmpty do
    names := names.push (← declareData type (render (rest.take 128)))
    rest := rest.drop 128
  while names.size > 1 do
    let mut next := #[]
    for i in [: (names.size + 1) / 2] do
      let j := 2 * i
      if j + 1 < names.size then
        next := next.push (← declareData type (names[j]! ++ " ++ " ++ names[j + 1]!))
      else next := next.push names[j]!
    names := next
  return names[0]!

private def constraintData (c : Constraint) : StateM Declarations String := do
  match c with
  | .table xs tuples positive =>
    let rows ← listData "List (List Int)" tuples fun rows =>
      "minion_int_rows% " ++ dataRepr (toJson rows).compress
    return "(Minion.Constraint.table (" ++ dataRepr xs ++ ") (" ++ rows ++ ") " ++ dataRepr positive ++ ")"
  | .shortTable xs tuples compressed =>
    let rows ← listData "List (List (Nat × Int))" tuples fun rows =>
      "minion_short_rows% " ++ dataRepr (toJson rows).compress
    return "(Minion.Constraint.shortTable (" ++ dataRepr xs ++ ") (" ++ rows ++ ") " ++ dataRepr compressed ++ ")"
  | .reify child guard =>
    return "(Minion.Constraint.reify " ++ (← constraintData child) ++ " (" ++ dataRepr guard ++ "))"
  | .imply child guard =>
    return "(Minion.Constraint.imply " ++ (← constraintData child) ++ " (" ++ dataRepr guard ++ "))"
  | .conjunction children =>
    let cs ← children.attach.mapM fun child => constraintData child.val
    return "(Minion.Constraint.conjunction [" ++ String.intercalate ", " cs ++ "])"
  | .disjunction children =>
    let cs ← children.attach.mapM fun child => constraintData child.val
    return "(Minion.Constraint.disjunction [" ++ String.intercalate ", " cs ++ "])"
  | c => return "(" ++ dataRepr c ++ ")"
termination_by sizeOf c
decreasing_by
  all_goals simp only [Constraint.reify.sizeOf_spec, Constraint.imply.sizeOf_spec,
    Constraint.conjunction.sizeOf_spec, Constraint.disjunction.sizeOf_spec]
  all_goals first | omega | exact Nat.lt_trans (List.sizeOf_lt_of_mem child.property) (by omega)

private def modelData (m : Model) (named : Bool) : String × String := Id.run do
  let (constraints, declarations) := (m.constraints.mapM constraintData).run {}
  let definitions := String.intercalate "\n\n" declarations.reversed.reverse
  let definitions := if named then definitions ++ "\n\n" ++
    String.intercalate "\n\n" (constraints.zipIdx.map fun (c, i) =>
      "def c" ++ toString i ++ " : Minion.Constraint :=\n" ++ c) else definitions
  let constraints := if named then (List.range constraints.length).map (fun i => "c" ++ toString i)
    else constraints
  return (definitions, "\n\ndef model : Minion.Model :=\n{ variables := " ++ dataRepr m.variables ++
    ", constraints := [" ++ String.intercalate ", " constraints ++ "] }\n\n")

/-- Export ordinary Lean data and a kernel-computed acceptance proof. No trace
    parsing or native decision tactic is required to check the generated file. -/
private def exportTreeLean (m : Model) (program : BlockSearchProof.Program) : Except String String := do
  let tree := BlockSearchProof.expand program
  let reported := (FastSearchProof.solutionsReverse tree).reverse
  let header := "import Minion.FastSearchProof\nimport Minion.CertificateData\n\nset_option maxRecDepth 100000\n" ++
    "set_option maxHeartbeats 0\n\nnamespace MinionCertificate\n\n"
  let (definitions, model) := modelData m false
  let data := "def tree : Minion.SearchProof.Tree :=\n" ++ dataRepr tree ++
    "\n\ndef reported : List Minion.Assignment :=\n" ++ dataRepr reported ++ "\n\n"
  let checked := "set_option maxRecDepth 100000 in\nset_option maxHeartbeats 0 in\n" ++
    "theorem fast_checked : Minion.FastSearchProof.check model (model.variables.map Minion.Variable.domain) tree = true := by decide +kernel\n\n" ++
    "theorem checked : Minion.SearchProof.check model (model.variables.map Minion.Variable.domain) tree = true := by\n" ++
    "  simpa only [Minion.FastSearchProof.check_eq] using fast_checked\n\n" ++
    "theorem reported_eq : reported = Minion.SearchProof.solutions tree := by decide +kernel\n\n"
  let proofText := "theorem solutions_correct :\n" ++
    "    ∀ a, a ∈ reported ↔ model.IsSolution a := by\n" ++
    "  rw [reported_eq]\n" ++
    "  exact Minion.SearchProof.check_complete model tree checked\n\n"
    ++ "theorem solutions_nodup : reported.Nodup := by\n" ++
      "  rw [reported_eq]\n" ++
      "  exact Minion.SearchProof.check_nodup model (model.variables.map Minion.Variable.domain) tree checked\n\n"
  let unsat := if reported.isEmpty then
    "theorem unsat : ∀ a, ¬model.IsSolution a :=\n" ++
      "  fun a hs => List.not_mem_nil ((solutions_correct a).mpr hs)\n\n" else ""
  return header ++ definitions ++ model ++ data ++ checked ++ proofText ++ unsat ++
    "#print axioms solutions_correct\n#print axioms solutions_nodup\n" ++
    (if reported.isEmpty then "#print axioms unsat\n" else "") ++
    "\nend MinionCertificate\n"

private def constraintDepth : Nat → Constraint → Nat
  | 0, _ => 0
  | fuel + 1, .reify c _ | fuel + 1, .imply c _ => 1 + constraintDepth fuel c
  | fuel + 1, .conjunction cs | fuel + 1, .disjunction cs =>
    1 + (cs.map (constraintDepth fuel)).foldl max 0
  | _ + 1, _ => 1

def shape (m : Model) (program : BlockSearchProof.Program) : Except String Json := do
  return Json.mkObj [
    ("tree", toJson (CertificateShape.expandedProgram program)),
    ("blocks", toJson (CertificateShape.program program)),
    ("constraintIndexHeight", toJson (ConstraintIndex.height
      (ConstraintIndex.build (List.range m.constraints.length)))),
    ("constraintDepth", toJson ((m.constraints.map (constraintDepth 256)).foldl max 0))]

private def indexData : ConstraintIndex.Index Nat → String
  | .empty => ".empty"
  | .node value left right =>
    let slot := match value with
      | none => "none"
      | some i => "(some c" ++ toString i ++ ")"
    "(.node " ++ slot ++ " " ++ indexData left ++ " " ++ indexData right ++ ")"

private def exportBlockLean (m : Model) (program : BlockSearchProof.Program) (layout : Layout) : Except String String := do
  let c := BlockSearchProof.cost program
  let layout := if layout == .automatic then
      if m.constraints.length ≥ 64 && c.references ≥ 16 * (c.modelChanges + 1) then
        Layout.blocksIndexBuild else Layout.blocksArray
    else layout
  let reported := BlockSearchProof.solutions program
  let header := "import Minion.BlockSearchProof\nimport Minion.CertificateData\n\nset_option maxRecDepth 100000\n" ++
    "set_option maxHeartbeats 0\n\nnamespace MinionCertificate\n\n"
  -- Constraints, including table payloads, have one named definition each.
  let named := layout == .blocksIndex
  let (definitions, model) := modelData m named
  let data := "def program : Minion.BlockSearchProof.Program :=\n" ++ dataRepr program ++
    "\n\ndef reported : List Minion.Assignment :=\n" ++ dataRepr reported ++ "\n\n"
  let prebuilt := layout == .blocksIndex
  let index := if prebuilt then
    "def constraintIndex : Minion.ConstraintIndex.Index Minion.Constraint :=\n" ++
      indexData (ConstraintIndex.build (List.range m.constraints.length)) ++
      "\n\ntheorem index_eq : constraintIndex = Minion.ConstraintIndex.build model.constraints := by with_unfolding_all rfl\n\n"
    else ""
  let predicate := if prebuilt then
    "Minion.BlockSearchProof.checkWithIndex model (model.variables.map Minion.Variable.domain) program constraintIndex"
    else "Minion.BlockSearchProof.check model (model.variables.map Minion.Variable.domain) program " ++
      (if layout == .blocksArray then "false" else "true")
  let checked := "theorem checked : " ++ predicate ++ " = true := by decide +kernel\n\n" ++
    "theorem reported_eq : reported = Minion.BlockSearchProof.solutions program := by decide +kernel\n\n"
  let mode := if layout == .blocksArray then "false" else "true"
  let acceptance := if prebuilt then
    "  exact Minion.BlockSearchProof.checkWithIndex_complete model program constraintIndex index_eq checked\n\n"
    else "  exact Minion.BlockSearchProof.check_complete_using model program " ++ mode ++ " checked\n\n"
  let proofText := "theorem solutions_correct :\n" ++
    "    ∀ a, a ∈ reported ↔ model.IsSolution a := by\n  rw [reported_eq]\n" ++ acceptance ++
    "theorem solutions_nodup : reported.Nodup := by\n  rw [reported_eq]\n" ++
    (if prebuilt then
      "  exact Minion.BlockSearchProof.checkWithIndex_nodup model (model.variables.map Minion.Variable.domain) program constraintIndex index_eq checked\n\n"
    else "  exact Minion.BlockSearchProof.check_nodup_using model (model.variables.map Minion.Variable.domain) program " ++
      mode ++ " checked\n\n")
  let unsat := if reported.isEmpty then
    "theorem unsat : ∀ a, ¬model.IsSolution a :=\n" ++
      "  fun a hs => List.not_mem_nil ((solutions_correct a).mpr hs)\n\n" else ""
  return header ++ definitions ++ model ++ index ++ data ++ checked ++ proofText ++ unsat ++
    "#print axioms solutions_correct\n#print axioms solutions_nodup\n" ++
    (if reported.isEmpty then "#print axioms unsat\n" else "") ++ "\nend MinionCertificate\n"

/-- Default exports use propagation blocks, choosing binary indexing when
    enough references amortize its construction. Diagnostic layouts stay explicit. -/
def exportLean (m : Model) (program : BlockSearchProof.Program) (layout : Layout := .automatic) : Except String String :=
  if layout == .tree then exportTreeLean m program else exportBlockLean m program layout

end Minion.TraceCertificate
