import Minion.Semantics

namespace Minion

def assignments : List (List Int) → List Assignment
  | [] => [[]]
  | domain :: rest => domain.flatMap fun value => (assignments rest).map (value :: ·)

/-- Domain membership includes the assignment's exact length. -/
def InDomains : List (List Int) → Assignment → Prop
  | [], [] => True
  | domain :: rest, value :: values => value ∈ domain ∧ InDomains rest values
  | _, _ => False

theorem mem_assignments (domains : List (List Int)) (a : Assignment) :
    a ∈ assignments domains ↔ InDomains domains a := by
  induction domains generalizing a with
  | nil => cases a <;> simp [assignments, InDomains]
  | cons domain rest ih =>
    cases a with
    | nil => simp [assignments, InDomains]
    | cons value values => simp [assignments, InDomains, ih]

def Model.satisfies (m : Model) (a : Assignment) : Bool :=
  a.length == m.variables.length &&
  (m.variables.zip a).all (fun (v, x) => v.domain.contains x) &&
  m.constraints.all (Constraint.eval a)

/-- Declarative solution predicate: finite-domain membership and all constraints. -/
def Model.IsSolution (m : Model) (a : Assignment) : Prop :=
  InDomains (m.variables.map Variable.domain) a ∧
  m.constraints.all (Constraint.eval a) = true

def Model.solutions (m : Model) : List Assignment :=
  (assignments (m.variables.map Variable.domain)).filter fun a =>
    m.constraints.all (Constraint.eval a)

theorem Model.mem_solutions (m : Model) (a : Assignment) :
    a ∈ m.solutions ↔ m.IsSolution a := by
  simp [Model.solutions, Model.IsSolution, mem_assignments]

def sortAssignments (xs : List Assignment) : List Assignment :=
  xs.mergeSort (fun a b => compare a b != .gt)

/-- Exact multiset equality. The JSON reader rejects repeated domain values,
    so enumeration produces each assignment once and duplicates are rejected. -/
def verifySolutions (m : Model) (reported : List Assignment) : Bool :=
  sortAssignments reported == sortAssignments m.solutions

/-- Acceptance proves both soundness and completeness of the reported solution set.
    This theorem does not assume anything about Minion or the JSON producer. -/
theorem verifySolutions_correct (m : Model) (reported : List Assignment)
    (h : verifySolutions m reported = true) :
    ∀ a, a ∈ reported ↔ m.IsSolution a := by
  have heq : sortAssignments reported = sortAssignments m.solutions := by
    exact beq_iff_eq.mp h
  intro a
  have hmem := congrArg (fun xs => a ∈ xs) heq
  simpa only [sortAssignments, List.mem_mergeSort, Model.mem_solutions] using (Iff.of_eq hmem)

end Minion
