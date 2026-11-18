# Propagation rules by constraint family

Each family of constraints has a proved rule for checking its deductions.
A rule is sound: it accepts only deductions which remove no solution. It need
not be as strong as the propagator, so a correct deduction can be rejected.
[PROOFS.md](PROOFS.md) describes the modules, and [CONSTRAINTS.md](CONSTRAINTS.md)
lists every registered name.

| Family | Proved rule | Implementations covered |
| --- | --- | --- |
| `sumleq`, `sumgeq` | Bounds contradiction in each direction; pruning retains solutions | Ordinary top-level general, two-term, Boolean implementations |
| Arithmetic negation views | Evaluation and domain membership; constant folding and double-negation cancellation | Negated terms in any constraint; max is checked as min on negated views |
| Equality / disequality / order / minusEq / abs | Local argument-support rule; generic pruning preservation | Ordinary top-level implementations, including GAC equality and bound variants where supported |
| Unary membership / literals / truth | Local argument-support rule; constant set/interval predicates | Ordinary top-level implementations; discrete, bound, sparse-bound cases where supported |
| Reification / implication / Boolean combinations / transparent wrappers | Compositional truth/falsity feasibility; sum complements use canonical sumleq with negation/shift | Trees containing supported leaves, including generated negative children and specialised reified equality |
| All seven element variants | Index/result/selected-entry support, in both polarities | General and watched, 0/1 indexing, undefined-to-zero, supported reification/implication/nesting |
| Weighted and watched sums | Weighted normalization through scaling views; same canonical bounds rule | Weighted general/light and watched/Boolean-clause implementations; supported logical wrappers |
| Nonlinear arithmetic / absolute difference | Fixed local support rules in both polarities | Product, Boolean product, div/modulo with and without undefined-to-zero, power (including fallback wrappers), difference; supported logical wrappers |
| Occurrence / literal sums / Hamming | Shared must/may predicate count bounds, both polarities | Three occurrence names, literal sums and both Hamming forms; supported logical wrappers |
| Min/max | Min support and complement; proved max-to-min negation normalization | General implementations and supported logical wrappers |
| Nvalue | Forced-value lower bound, required fresh value, set-union and unassigned-occurrence upper bounds | Both directions and supported logical wrappers |
| All five lexicographic implementations | Prefix feasibility with candidate bindings; shared view correlations | General, quick and repeated-variable variants; supported logical wrappers |
| Vector-exists constraints | Shared predicate-count bound, threshold one | Both names and supported logical wrappers |
| Full tables / MDD | Tuple support and forbidden-prefix support, both polarities | Seven names, including STR2+; supported logical wrappers |
| Short tables | Row support and row failure; bounded one-variable forward checking, both polarities | All four names, including compressed rows and supported logical wrappers |
| All-different | Checked Hall cardinality cuts; pairwise forced collision and negative support | Both implementations, supported logical wrappers |
| GCC globals | Occurrence bounds; checked joint upper-capacity and lower-demand cuts; noninterval capacity support | Both implementations and supported logical wrappers |
| Matrix | Row/column counts and projected Hall cuts, both polarities | General implementation and supported logical wrappers |
| Frameupdate | Fixed-index block correspondence; bounded generic negation | General implementation and supported logical wrappers |
| SAC/SSAC probes | General refutation-preservation rule, including rejected intervals and nested refutations | Sequential SAC/SSAC, bounds and limited variants; node and preprocessing probes |
