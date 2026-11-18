# Constraint inventory

All 83 registered names have assignment semantics and a mapped proof family.
Names sharing semantics reuse a Lean constructor and fixed rule library.
This inventory does not claim every possible propagation is accepted or GAC is proved.
The rule used for each family is described in [COVERAGE.md](COVERAGE.md).

The internal event observer has truth semantics and performs no pruning.
Logical wrappers compose both polarities of their supported children; transparent
checking wrappers retain their child's semantics. `BoundsProof` uses interval shortcuts
where available and otherwise delegates to `SupportProof`.

Regenerate with `python3 lean/tests/constraint_inventory.py`; use `--check` to audit it.
The script compares the C++ JSON registrations, Lean decoder names, and explicit family map.

| Registered name | Family | Fixed proof library | C++ registration |
| --- | --- | --- | --- |
| `()()collectevents()()` | Truth / observer | [BoundsProof](Minion/BoundsProof.lean) | [constraint_collect_events.h](../minion/constraints/constraint_collect_events.h) |
| `__reify_diseq` | Logical composition / wrappers | [LogicProof](Minion/LogicProof.lean) | [constraint_equal.h](../minion/constraints/constraint_equal.h) |
| `__reify_eq` | Logical composition / wrappers | [LogicProof](Minion/LogicProof.lean) | [constraint_equal.h](../minion/constraints/constraint_equal.h) |
| `__reify_minuseq` | Logical composition / wrappers | [LogicProof](Minion/LogicProof.lean) | [constraint_equal.h](../minion/constraints/constraint_equal.h) |
| `abs` | Local arithmetic and membership | [BoundsProof](Minion/BoundsProof.lean) | [constraint_abs.h](../minion/constraints/constraint_abs.h) |
| `alldiff` | All-different | [AllDiffProof](Minion/AllDiffProof.lean) | [constraint_neq.h](../minion/constraints/constraint_neq.h) |
| `alldiffmatrix` | Matrix | [MatrixProof](Minion/MatrixProof.lean) | [constraint_alldiffmatrix.h](../minion/constraints/constraint_alldiffmatrix.h) |
| `check[assign]` | Logical composition / wrappers | [LogicProof](Minion/LogicProof.lean) | [check_assign.h](../minion/constraints/check_assign.h) |
| `check[gsa]` | Logical composition / wrappers | [LogicProof](Minion/LogicProof.lean) | [check_gsa.h](../minion/constraints/check_gsa.h) |
| `difference` | Local arithmetic and membership | [BoundsProof](Minion/BoundsProof.lean) | [constraint_difference.h](../minion/constraints/constraint_difference.h) |
| `diseq` | Local arithmetic and membership | [BoundsProof](Minion/BoundsProof.lean) | [constraint_equal.h](../minion/constraints/constraint_equal.h) |
| `div` | Local arithmetic and membership | [BoundsProof](Minion/BoundsProof.lean) | [constraint_div.h](../minion/constraints/nonlinear_arithmetic/constraint_div.h) |
| `div_undefzero` | Local arithmetic and membership | [BoundsProof](Minion/BoundsProof.lean) | [constraint_div.h](../minion/constraints/nonlinear_arithmetic/constraint_div.h) |
| `element` | Element | [ElementProof](Minion/ElementProof.lean) | [constraint_element.h](../minion/constraints/constraint_element.h) |
| `element_one` | Element | [ElementProof](Minion/ElementProof.lean) | [constraint_element.h](../minion/constraints/constraint_element.h) |
| `element_undefzero` | Element | [ElementProof](Minion/ElementProof.lean) | [constraint_element.h](../minion/constraints/constraint_element.h) |
| `eq` | Local arithmetic and membership | [BoundsProof](Minion/BoundsProof.lean) | [constraint_equal.h](../minion/constraints/constraint_equal.h) |
| `false` | Truth / observer | [BoundsProof](Minion/BoundsProof.lean) | [constraint_constant.h](../minion/constraints/constraint_constant.h) |
| `forwardchecking` | Logical composition / wrappers | [LogicProof](Minion/LogicProof.lean) | [forward_checking.h](../minion/constraints/forward_checking.h) |
| `frameupdate` | Frame | [FrameProof](Minion/FrameProof.lean) | [constraint_frameupdate.h](../minion/constraints/constraint_frameupdate.h) |
| `gacalldiff` | All-different | [AllDiffProof](Minion/AllDiffProof.lean) | [constraint_gacalldiff.h](../minion/constraints/constraint_gacalldiff.h) |
| `gaceq` | Local arithmetic and membership | [BoundsProof](Minion/BoundsProof.lean) | [constraint_gacequal.h](../minion/constraints/constraint_gacequal.h) |
| `gacschema` | Full tables | [TableProof](Minion/TableProof.lean) | [constraint_gacschema.h](../minion/constraints/constraint_gacschema.h) |
| `gcc` | Global cardinality | [GccCutProof](Minion/GccCutProof.lean) | [constraint_gcc.h](../minion/constraints/constraint_gcc.h) |
| `gccweak` | Global cardinality | [GccCutProof](Minion/GccCutProof.lean) | [constraint_gccweak.h](../minion/constraints/constraint_gccweak.h) |
| `haggisgac` | Short tables | [ShortTableProof](Minion/ShortTableProof.lean) | [constraint_haggisgac_bt.h](../minion/constraints/constraint_haggisgac_bt.h) |
| `haggisgac-stable` | Short tables | [ShortTableProof](Minion/ShortTableProof.lean) | [constraint_haggisgac_stable.h](../minion/constraints/constraint_haggisgac_stable.h) |
| `hamming` | Predicate counts | [CountProof](Minion/CountProof.lean) | [dynamic_sumor.h](../minion/constraints/dynamic_sumor.h) |
| `ineq` | Local arithmetic and membership | [BoundsProof](Minion/BoundsProof.lean) | [constraint_less.h](../minion/constraints/constraint_less.h) |
| `lexleq` | Lexicographic | [LexProof](Minion/LexProof.lean) | [constraint_lex.h](../minion/constraints/constraint_lex.h) |
| `lexleq[quick]` | Lexicographic | [LexProof](Minion/LexProof.lean) | [dynamic_quicklex.h](../minion/constraints/dynamic_quicklex.h) |
| `lexleq[rv]` | Lexicographic | [LexProof](Minion/LexProof.lean) | [constraint_gaclex.h](../minion/constraints/constraint_gaclex.h) |
| `lexless` | Lexicographic | [LexProof](Minion/LexProof.lean) | [constraint_lex.h](../minion/constraints/constraint_lex.h) |
| `lexless[quick]` | Lexicographic | [LexProof](Minion/LexProof.lean) | [dynamic_quicklex.h](../minion/constraints/dynamic_quicklex.h) |
| `lighttable` | Full tables | [TableProof](Minion/TableProof.lean) | [constraint_lighttable.h](../minion/constraints/constraint_lighttable.h) |
| `litsumgeq` | Predicate counts | [CountProof](Minion/CountProof.lean) | [dynamic_literalwatch.h](../minion/constraints/dynamic_literalwatch.h) |
| `max` | Extrema | [ExtremumProof](Minion/ExtremumProof.lean) | [constraint_min.h](../minion/constraints/constraint_min.h) |
| `mddc` | Full tables | [TableProof](Minion/TableProof.lean) | [constraint_mddc.h](../minion/constraints/constraint_mddc.h) |
| `min` | Extrema | [ExtremumProof](Minion/ExtremumProof.lean) | [constraint_min.h](../minion/constraints/constraint_min.h) |
| `minuseq` | Local arithmetic and membership | [BoundsProof](Minion/BoundsProof.lean) | [constraint_equal.h](../minion/constraints/constraint_equal.h) |
| `modulo` | Local arithmetic and membership | [BoundsProof](Minion/BoundsProof.lean) | [constraint_modulo.h](../minion/constraints/nonlinear_arithmetic/constraint_modulo.h) |
| `modulo_undefzero` | Local arithmetic and membership | [BoundsProof](Minion/BoundsProof.lean) | [constraint_modulo.h](../minion/constraints/nonlinear_arithmetic/constraint_modulo.h) |
| `negativemddc` | Full tables | [TableProof](Minion/TableProof.lean) | [constraint_mddc.h](../minion/constraints/constraint_mddc.h) |
| `negativetable` | Full tables | [TableProof](Minion/TableProof.lean) | [constraint_GACtable_master.h](../minion/constraints/constraint_GACtable_master.h) |
| `not-hamming` | Predicate counts | [CountProof](Minion/CountProof.lean) | [dynamic_sumor.h](../minion/constraints/dynamic_sumor.h) |
| `nvaluegeq` | Distinct-value counts | [NvalueProof](Minion/NvalueProof.lean) | [constraint_nvalue.h](../minion/constraints/constraint_nvalue.h) |
| `nvalueleq` | Distinct-value counts | [NvalueProof](Minion/NvalueProof.lean) | [constraint_nvalue.h](../minion/constraints/constraint_nvalue.h) |
| `occurrence` | Predicate counts | [CountProof](Minion/CountProof.lean) | [constraint_occurrence.h](../minion/constraints/constraint_occurrence.h) |
| `occurrencegeq` | Predicate counts | [CountProof](Minion/CountProof.lean) | [constraint_occurrence.h](../minion/constraints/constraint_occurrence.h) |
| `occurrenceleq` | Predicate counts | [CountProof](Minion/CountProof.lean) | [constraint_occurrence.h](../minion/constraints/constraint_occurrence.h) |
| `pow` | Local arithmetic and membership | [BoundsProof](Minion/BoundsProof.lean) | [constraint_pow_check.h](../minion/constraints/nonlinear_arithmetic/constraint_pow_check.h) |
| `product` | Local arithmetic and membership | [BoundsProof](Minion/BoundsProof.lean) | [constraint_product.h](../minion/constraints/nonlinear_arithmetic/constraint_product.h) |
| `reify` | Logical composition / wrappers | [LogicProof](Minion/LogicProof.lean) | [reify.h](../minion/constraints/reify.h) |
| `reifyimply` | Logical composition / wrappers | [LogicProof](Minion/LogicProof.lean) | [reify_true.h](../minion/constraints/reify_true.h) |
| `reifyimply-quick` | Logical composition / wrappers | [LogicProof](Minion/LogicProof.lean) | [reify_true.h](../minion/constraints/reify_true.h) |
| `shortctuplestr2` | Short tables | [ShortTableProof](Minion/ShortTableProof.lean) | [constraint_shortstr2ctuple.h](../minion/constraints/constraint_shortstr2ctuple.h) |
| `shortstr2` | Short tables | [ShortTableProof](Minion/ShortTableProof.lean) | [constraint_shortstr2.h](../minion/constraints/constraint_shortstr2.h) |
| `str2plus` | Full tables | [TableProof](Minion/TableProof.lean) | [constraint_shortstr2.h](../minion/constraints/constraint_shortstr2.h) |
| `sumgeq` | Sums | [SumProof](Minion/SumProof.lean) | [constraint_sum.h](../minion/constraints/sum_constraints/constraint_sum.h) |
| `sumleq` | Sums | [SumProof](Minion/SumProof.lean) | [constraint_sum.h](../minion/constraints/sum_constraints/constraint_sum.h) |
| `table` | Full tables | [TableProof](Minion/TableProof.lean) | [constraint_GACtable_master.h](../minion/constraints/constraint_GACtable_master.h) |
| `true` | Truth / observer | [BoundsProof](Minion/BoundsProof.lean) | [constraint_constant.h](../minion/constraints/constraint_constant.h) |
| `w-inintervalset` | Local arithmetic and membership | [BoundsProof](Minion/BoundsProof.lean) | [dynamic_inintervalset.h](../minion/constraints/unary/dynamic_inintervalset.h) |
| `w-inrange` | Local arithmetic and membership | [BoundsProof](Minion/BoundsProof.lean) | [dynamic_inrange.h](../minion/constraints/unary/dynamic_inrange.h) |
| `w-inset` | Local arithmetic and membership | [BoundsProof](Minion/BoundsProof.lean) | [dynamic_inset.h](../minion/constraints/unary/dynamic_inset.h) |
| `w-literal` | Local arithmetic and membership | [BoundsProof](Minion/BoundsProof.lean) | [dynamic_literal.h](../minion/constraints/unary/dynamic_literal.h) |
| `w-notinrange` | Local arithmetic and membership | [BoundsProof](Minion/BoundsProof.lean) | [dynamic_notinrange.h](../minion/constraints/unary/dynamic_notinrange.h) |
| `w-notinset` | Local arithmetic and membership | [BoundsProof](Minion/BoundsProof.lean) | [dynamic_notinset.h](../minion/constraints/unary/dynamic_notinset.h) |
| `w-notliteral` | Local arithmetic and membership | [BoundsProof](Minion/BoundsProof.lean) | [dynamic_notliteral.h](../minion/constraints/unary/dynamic_notliteral.h) |
| `watched-and` | Logical composition / wrappers | [LogicProof](Minion/LogicProof.lean) | [dynamic_new_and.h](../minion/constraints/dynamic_new_and.h) |
| `watched-or` | Logical composition / wrappers | [LogicProof](Minion/LogicProof.lean) | [dynamic_new_or.h](../minion/constraints/dynamic_new_or.h) |
| `watchelement` | Element | [ElementProof](Minion/ElementProof.lean) | [dynamic_element.h](../minion/constraints/dynamic_element.h) |
| `watchelement_one` | Element | [ElementProof](Minion/ElementProof.lean) | [dynamic_element.h](../minion/constraints/dynamic_element.h) |
| `watchelement_one_undefzero` | Element | [ElementProof](Minion/ElementProof.lean) | [dynamic_element.h](../minion/constraints/dynamic_element.h) |
| `watchelement_undefzero` | Element | [ElementProof](Minion/ElementProof.lean) | [dynamic_element.h](../minion/constraints/dynamic_element.h) |
| `watchless` | Local arithmetic and membership | [BoundsProof](Minion/BoundsProof.lean) | [dynamic_less.h](../minion/constraints/dynamic_less.h) |
| `watchneq` | Local arithmetic and membership | [BoundsProof](Minion/BoundsProof.lean) | [dynamic_neq.h](../minion/constraints/dynamic_neq.h) |
| `watchsumgeq` | Sums | [SumProof](Minion/SumProof.lean) | [dynamic_sum.h](../minion/constraints/dynamic_sum.h) |
| `watchsumleq` | Sums | [SumProof](Minion/SumProof.lean) | [dynamic_sum.h](../minion/constraints/dynamic_sum.h) |
| `watchvecexists_less` | Predicate counts | [CountProof](Minion/CountProof.lean) | [dynamic_vecneq.h](../minion/constraints/dynamic_vecneq.h) |
| `watchvecneq` | Predicate counts | [CountProof](Minion/CountProof.lean) | [dynamic_vecneq.h](../minion/constraints/dynamic_vecneq.h) |
| `weightedsumgeq` | Weighted sums | [WeightedProof](Minion/WeightedProof.lean) | [constraint_sum.h](../minion/constraints/sum_constraints/constraint_sum.h) |
| `weightedsumleq` | Weighted sums | [WeightedProof](Minion/WeightedProof.lean) | [constraint_sum.h](../minion/constraints/sum_constraints/constraint_sum.h) |
