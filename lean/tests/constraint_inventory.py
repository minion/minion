"""Audit registered constraint names against decoding and the proof-family map."""
import argparse
import json
from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]

# Family membership is deliberately explicit: a new registered name must be
# reviewed rather than silently inheriting coverage from a naming convention.
FAMILIES = [
    ("Truth / observer", "BoundsProof", "true false ()()collectevents()()"),
    ("Local arithmetic and membership", "BoundsProof",
     "eq gaceq diseq watchneq watchless abs minuseq ineq difference div div_undefzero "
     "modulo modulo_undefzero pow product w-inset w-notinset w-inrange "
     "w-notinrange w-inintervalset w-literal w-notliteral"),
    ("Sums", "SumProof", "sumleq sumgeq watchsumleq watchsumgeq"),
    ("Weighted sums", "WeightedProof", "weightedsumleq weightedsumgeq"),
    ("Element", "ElementProof",
     "element element_one element_undefzero watchelement watchelement_one "
     "watchelement_undefzero watchelement_one_undefzero"),
    ("Lexicographic", "LexProof", "lexleq lexleq[quick] lexleq[rv] lexless lexless[quick]"),
    ("Predicate counts", "CountProof",
     "occurrence occurrencegeq occurrenceleq litsumgeq hamming not-hamming "
     "watchvecneq watchvecexists_less"),
    ("Extrema", "ExtremumProof", "min max"),
    ("Distinct-value counts", "NvalueProof", "nvalueleq nvaluegeq"),
    ("All-different", "AllDiffProof", "alldiff gacalldiff"),
    ("Global cardinality", "GccCutProof", "gcc gccweak"),
    ("Matrix", "MatrixProof", "alldiffmatrix"),
    ("Frame", "FrameProof", "frameupdate"),
    ("Full tables", "TableProof",
     "table str2plus gacschema lighttable mddc negativetable negativemddc"),
    ("Short tables", "ShortTableProof", "shortstr2 shortctuplestr2 haggisgac haggisgac-stable"),
    ("Logical composition / wrappers", "LogicProof",
     "reify reifyimply reifyimply-quick watched-and watched-or forwardchecking "
     "check[assign] check[gsa] __reify_eq __reify_diseq __reify_minuseq"),
]


def render():
    registry = {}
    for path in sorted((ROOT.parent / "minion").rglob("*")):
        if path.suffix not in (".h", ".hpp", ".cpp"):
            continue
        for block in re.findall(r"/\* JSON(.*?)\*/", path.read_text(), re.S):
            record = json.loads(block)
            if record.get("type") == "constraint":
                name = record["name"]
                if name in registry:
                    raise ValueError(f"duplicate registry name: {name}")
                registry[name] = path.relative_to(ROOT.parent)

    decoder = (ROOT / "Minion/Json.lean").read_text().split("match name, args, children with", 1)[1]
    decoder = decoder.split("return result", 1)[0]
    decoded = set(re.findall(r'\|\s*"([^"]+)"', decoder))
    mapped = {}
    for family, proof, names in FAMILIES:
        if not (ROOT / "Minion" / f"{proof}.lean").is_file():
            raise ValueError(f"missing proof module: {proof}")
        for name in names.split():
            if name in mapped:
                raise ValueError(f"duplicate family mapping: {name}")
            mapped[name] = family, proof
    for label, names in (("JSON decoder", decoded), ("family map", set(mapped))):
        if names != set(registry):
            raise ValueError(f"{label}: missing {set(registry) - names}; extra {names - set(registry)}")

    lines = [
        "# Constraint inventory", "",
        f"All {len(registry)} registered names have assignment semantics and a mapped proof family.",
        "Names sharing semantics reuse a Lean constructor and fixed rule library.",
        "This inventory does not claim every possible propagation is accepted or GAC is proved.",
        "The rule used for each family is described in [COVERAGE.md](COVERAGE.md).", "",
        "The internal event observer has truth semantics and performs no pruning.",
        "Logical wrappers compose both polarities of their supported children; transparent",
        "checking wrappers retain their child's semantics. `BoundsProof` uses interval shortcuts",
        "where available and otherwise delegates to `SupportProof`.", "",
        "Regenerate with `python3 lean/tests/constraint_inventory.py`; use `--check` to audit it.",
        "The script compares the C++ JSON registrations, Lean decoder names, and explicit family map.", "",
        "| Registered name | Family | Fixed proof library | C++ registration |",
        "| --- | --- | --- | --- |",
    ]
    for name, source in sorted(registry.items()):
        family, proof = mapped[name]
        lines.append(f"| `{name}` | {family} | [{proof}](Minion/{proof}.lean) | [{source.name}](../{source}) |")
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    content = render()
    path = ROOT / "CONSTRAINTS.md"
    if args.check:
        if not path.is_file() or path.read_text() != content:
            raise SystemExit("constraint inventory needs regeneration")
        print("Constraint inventory matches registry, decoder and proof-family map.")
    else:
        path.write_text(content)
        print(path)


if __name__ == "__main__":
    main()
