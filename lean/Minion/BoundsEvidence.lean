import Minion.BlockSearchProof

/-! Untrusted selection of the interval rule. Where interval reasoning alone
    justifies a step, the step names that rule, so that checking it reads only
    domain bounds, without also enumerating supports. Acceptance still depends
    solely on the fixed checker. -/
namespace Minion.BoundsEvidence
open DomainProof BlockSearchProof

private def domains (d : Array (List Int)) : Domains := fun i => d[i]?.getD [0]

private def restrict (d : Array (List Int)) (t : Nat) (op : Operation) (b : Int) : Array (List Int) :=
  d.modify t (List.filter (op.holds b))

private def prunes (cs : Array Constraint) (d : Array (List Int)) (c t : Nat)
    (op : Operation) (b : Int) : Bool :=
  (cs[c]?).any fun k => (InferenceWitness.rule .bounds (some t)).prune (domains d) k t op b

private def conflicts (cs : Array Constraint) (d : Array (List Int)) (c : Nat) : Bool :=
  (cs[c]?).any fun k => (InferenceWitness.rule .bounds).conflict (domains d) k

private partial def go (cs : Array Constraint) (d : Array (List Int)) : Program → Program
  | .conflict c => if conflicts cs d c then .explainedConflict c .bounds else .conflict c
  | .block steps next => Id.run do
    let mut d := d
    let mut out := Array.mkEmpty steps.length
    for step in steps do
      match step with
      | .prune (some c) t op b =>
        out := out.push (if prunes cs d c t op b then .explainedPrune c t op b .bounds else step)
        d := restrict d t op b
      | .prune none t op b | .explainedPrune _ t op b _ =>
        out := out.push step
        d := restrict d t op b
      | .tableReduce .. | .filterTables _ => out := out.push step
    return .block out.toList (go cs d next)
  | .split t v left right =>
    .split t v (go cs (restrict d t .assign v) left) (go cs (restrict d t .remove v) right)
  | .refute t op b proof next =>
    let c := SearchProof.complement op b
    .refute t op b (go cs (restrict d t c.1 c.2) proof) (go cs (restrict d t op b) next)
  | leaf => leaf

def annotate (m : Model) (p : Program) : Program :=
  go m.constraints.toArray (m.variables.map Variable.domain).toArray p

end Minion.BoundsEvidence
