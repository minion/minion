import Lean

/-! Compact certificate data notation. The elaborators construct ordinary list,
    pair and integer expressions; they do not assert propositions or discharge
    acceptance checks. The resulting data is checked by Lean's kernel. -/
namespace Minion.CertificateData
open Lean Elab

private def intExpr : Int → Expr
  | .ofNat n => mkApp (mkConst ``Int.ofNat) (mkNatLit n)
  | .negSucc n => mkApp (mkConst ``Int.negSucc) (mkNatLit n)

private def listExpr (type : Expr) (values : List Expr) : Expr :=
  let nil := mkApp (mkConst ``List.nil [.zero]) type
  let cons := mkApp (mkConst ``List.cons [.zero]) type
  values.foldr (fun value tail => mkApp2 cons value tail) nil

private def parseRows [FromJson α] (text : TSyntax `str) : Term.TermElabM α := do
  match Json.parse text.getString >>= fromJson? with
  | .ok rows => return rows
  | .error message => throwErrorAt text "invalid certificate data: {message}"

syntax "minion_int_rows% " str : term
elab_rules : term
  | `(minion_int_rows% $text:str) => do
    let rows : List (List Int) ← parseRows text
    let int := mkConst ``Int
    return listExpr (mkApp (mkConst ``List [.zero]) int)
      (rows.map fun row => listExpr int (row.map intExpr))

syntax "minion_short_rows% " str : term
elab_rules : term
  | `(minion_short_rows% $text:str) => do
    let rows : List (List (Nat × Int)) ← parseRows text
    let nat := mkConst ``Nat
    let int := mkConst ``Int
    let pair := mkApp2 (mkConst ``Prod [.zero, .zero]) nat int
    let entry := mkApp2 (mkConst ``Prod.mk [.zero, .zero]) nat int
    return listExpr (mkApp (mkConst ``List [.zero]) pair)
      (rows.map fun row => listExpr pair
        (row.map fun (i, v) => mkApp2 entry (mkNatLit i) (intExpr v)))

end Minion.CertificateData
