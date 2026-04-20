// Basic mid-search test harness. Each test builds a small CSP, runs it via
// runMinion+callback, collects the assignment of the tracked vars at every
// solution, and compares against an expected set. Mid-search modifications
// happen in per-test `on_callback` hooks.
//
// Tests are ordered by increasing complexity. Simpler constraints first —
// landmines in reify are most easily diagnosed once add-var and add-redundant
// are known good.

#include "BuildVariables.h"
#include "inputfile_parse/CSPSpec.h"
#include "libwrapper.h"
#include "minion.h"
#include "variables/AnyVarRef.h"

#include <climits>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

using namespace ProbSpec;

constexpr int MISSING = INT_MIN;
using Snapshot = std::map<std::string, int>;
using SolutionSet = std::multiset<Snapshot>;

static int peek_var_value(CSPInstance& instance, const std::string& name) {
    try {
        Var v = instance.vars.getSymbol(name);
        AnyVarRef ref = BuildCon::getAnyVarRefFromVar(v);
        if(!ref.isAssigned())
            return MISSING;
        return checked_cast<int>(ref.assignedValue());
    } catch(const parse_exception&) {
        return MISSING;
    }
}

static Snapshot snapshot(CSPInstance& instance,
                         const std::vector<std::string>& var_names) {
    Snapshot s;
    for(const auto& n : var_names)
        s[n] = peek_var_value(instance, n);
    return s;
}

struct TestCtx {
    CSPInstance instance;
    // Names we want to compare at each solution. Midsearch-added vars should
    // be added here by the on_callback hook so their values are captured on
    // subsequent solutions.
    std::vector<std::string> tracked_vars;
    std::vector<Snapshot> solutions_seen;
    int callback_count = 0;
    // Return true to continue search, false to stop.
    std::function<bool(MinionContext*, TestCtx&)> on_callback;
};

static bool collect_callback(MinionContext* ctx, void* userdata) {
    auto* tc = reinterpret_cast<TestCtx*>(userdata);
    tc->callback_count++;
    tc->solutions_seen.push_back(snapshot(tc->instance, tc->tracked_vars));
    if(tc->on_callback)
        return tc->on_callback(ctx, *tc);
    return true;
}

static MinionResult run(TestCtx& tc) {
    MinionContext* ctx = minion_newContext();
    SearchOptions options;
    options.silent = true;
    options.print_solution = false;
    SearchMethod method;
    MinionResult rc = runMinion(ctx, options, method, tc.instance,
                                collect_callback, &tc);
    minion_freeContext(ctx);
    return rc;
}

static std::string snap_str(const Snapshot& s) {
    std::string r = "{";
    bool first = true;
    for(const auto& kv : s) {
        if(!first) r += " ";
        first = false;
        r += kv.first + "=";
        if(kv.second == MISSING) r += "_";
        else r += std::to_string(kv.second);
    }
    r += "}";
    return r;
}

static bool check_solutions(const char* test_name,
                            const std::vector<Snapshot>& got_vec,
                            const std::vector<Snapshot>& expected_vec) {
    SolutionSet got(got_vec.begin(), got_vec.end());
    SolutionSet expected(expected_vec.begin(), expected_vec.end());
    if(got == expected)
        return true;
    std::cerr << "  [" << test_name << "] solution-set mismatch\n";
    std::cerr << "    got " << got.size() << " solutions:\n";
    for(const auto& s : got)
        std::cerr << "      " << snap_str(s) << "\n";
    std::cerr << "    expected " << expected.size() << " solutions:\n";
    for(const auto& s : expected)
        std::cerr << "      " << snap_str(s) << "\n";
    return false;
}

// Helper: push a var onto searchOrder. Not used for midsearch-added vars
// (those the harness puts into the aux block automatically).
static void add_search_order(CSPInstance& instance,
                             const std::vector<Var>& vars) {
    instance.searchOrder.push_back(SearchOrder(vars, ORDER_STATIC, false));
}

// ============================================================================
//  Tests
// ============================================================================

// Baseline: x, y in [0..2], no constraints. Nine solutions.
static bool test_baseline_unconstrained() {
    TestCtx tc;
    newVar(tc.instance, "x", VAR_DISCRETE, {0, 2});
    newVar(tc.instance, "y", VAR_DISCRETE, {0, 2});
    add_search_order(tc.instance, {tc.instance.vars.getSymbol("x"),
                                   tc.instance.vars.getSymbol("y")});
    tc.tracked_vars = {"x", "y"};

    if(run(tc) != MINION_OK) return false;

    std::vector<Snapshot> expected;
    for(int xv = 0; xv <= 2; ++xv)
        for(int yv = 0; yv <= 2; ++yv)
            expected.push_back({{"x", xv}, {"y", yv}});

    return check_solutions("baseline_unconstrained", tc.solutions_seen, expected);
}

// Baseline with constraint: x, y in [0..2], eq(x, y). Three solutions.
static bool test_baseline_eq() {
    TestCtx tc;
    newVar(tc.instance, "x", VAR_DISCRETE, {0, 2});
    newVar(tc.instance, "y", VAR_DISCRETE, {0, 2});
    Var x = tc.instance.vars.getSymbol("x");
    Var y = tc.instance.vars.getSymbol("y");
    add_search_order(tc.instance, {x, y});
    tc.tracked_vars = {"x", "y"};

    ConstraintBlob eq(lib_getConstraint(CT_EQ));
    eq.vars.push_back({x});
    eq.vars.push_back({y});
    tc.instance.constraints.push_back(eq);

    if(run(tc) != MINION_OK) return false;

    std::vector<Snapshot> expected = {
        {{"x", 0}, {"y", 0}},
        {{"x", 1}, {"y", 1}},
        {{"x", 2}, {"y", 2}},
    };
    return check_solutions("baseline_eq", tc.solutions_seen, expected);
}

// At the first solution, add an unconstrained bool var `z`. Subsequent
// solutions should include z pinned to its lower bound (aux + ascending
// valorder picks 0). First solution was emitted before z existed, so z
// reads as MISSING there.
static bool test_midsearch_add_bool_unconstrained() {
    TestCtx tc;
    newVar(tc.instance, "x", VAR_DISCRETE, {0, 2});
    newVar(tc.instance, "y", VAR_DISCRETE, {0, 2});
    add_search_order(tc.instance, {tc.instance.vars.getSymbol("x"),
                                   tc.instance.vars.getSymbol("y")});
    tc.tracked_vars = {"x", "y", "z"};

    tc.on_callback = [](MinionContext* ctx, TestCtx& tcc) {
        if(tcc.callback_count == 1) {
            MinionResult rc = minion_newVarMidsearch(ctx, tcc.instance, (char*)"z",
                                                    VAR_BOOL, 0, 1);
            if(rc != MINION_OK) {
                std::cerr << "  newVarMidsearch failed: " << (int)rc << "\n";
                return false;
            }
        }
        return true;
    };

    if(run(tc) != MINION_OK) return false;

    std::vector<Snapshot> expected;
    // First solution emitted with only x, y.
    expected.push_back({{"x", 0}, {"y", 0}, {"z", MISSING}});
    // All subsequent (x, y) enumerations with z fixed to its lower bound.
    for(int xv = 0; xv <= 2; ++xv)
        for(int yv = 0; yv <= 2; ++yv) {
            if(xv == 0 && yv == 0) continue;
            expected.push_back({{"x", xv}, {"y", yv}, {"z", 0}});
        }

    return check_solutions("midsearch_add_bool_unconstrained",
                           tc.solutions_seen, expected);
}

// Same shape for each of the four var types.
static bool test_midsearch_add_bound_unconstrained() {
    TestCtx tc;
    newVar(tc.instance, "x", VAR_DISCRETE, {0, 2});
    newVar(tc.instance, "y", VAR_DISCRETE, {0, 2});
    add_search_order(tc.instance, {tc.instance.vars.getSymbol("x"),
                                   tc.instance.vars.getSymbol("y")});
    tc.tracked_vars = {"x", "y", "z"};

    tc.on_callback = [](MinionContext* ctx, TestCtx& tcc) {
        if(tcc.callback_count == 1) {
            MinionResult rc = minion_newVarMidsearch(ctx, tcc.instance, (char*)"z",
                                                    VAR_BOUND, 3, 5);
            if(rc != MINION_OK) return false;
        }
        return true;
    };

    if(run(tc) != MINION_OK) return false;

    std::vector<Snapshot> expected;
    expected.push_back({{"x", 0}, {"y", 0}, {"z", MISSING}});
    for(int xv = 0; xv <= 2; ++xv)
        for(int yv = 0; yv <= 2; ++yv) {
            if(xv == 0 && yv == 0) continue;
            // VAR_BOUND z with domain [3,5], VALORDER_ASCEND → z = 3.
            expected.push_back({{"x", xv}, {"y", yv}, {"z", 3}});
        }

    return check_solutions("midsearch_add_bound_unconstrained",
                           tc.solutions_seen, expected);
}

static bool test_midsearch_add_discrete_unconstrained() {
    TestCtx tc;
    newVar(tc.instance, "x", VAR_DISCRETE, {0, 2});
    newVar(tc.instance, "y", VAR_DISCRETE, {0, 2});
    add_search_order(tc.instance, {tc.instance.vars.getSymbol("x"),
                                   tc.instance.vars.getSymbol("y")});
    tc.tracked_vars = {"x", "y", "z"};

    tc.on_callback = [](MinionContext* ctx, TestCtx& tcc) {
        if(tcc.callback_count == 1) {
            MinionResult rc = minion_newVarMidsearch(ctx, tcc.instance, (char*)"z",
                                                    VAR_DISCRETE, 7, 9);
            if(rc != MINION_OK) return false;
        }
        return true;
    };

    if(run(tc) != MINION_OK) return false;

    std::vector<Snapshot> expected;
    expected.push_back({{"x", 0}, {"y", 0}, {"z", MISSING}});
    for(int xv = 0; xv <= 2; ++xv)
        for(int yv = 0; yv <= 2; ++yv) {
            if(xv == 0 && yv == 0) continue;
            expected.push_back({{"x", xv}, {"y", yv}, {"z", 7}});
        }

    return check_solutions("midsearch_add_discrete_unconstrained",
                           tc.solutions_seen, expected);
}

// Midsearch: add a var + a constraint pinning it to a specific value.
static bool test_midsearch_add_bool_with_eq_constant() {
    TestCtx tc;
    newVar(tc.instance, "x", VAR_DISCRETE, {0, 2});
    newVar(tc.instance, "y", VAR_DISCRETE, {0, 2});
    add_search_order(tc.instance, {tc.instance.vars.getSymbol("x"),
                                   tc.instance.vars.getSymbol("y")});
    tc.tracked_vars = {"x", "y", "z"};

    tc.on_callback = [](MinionContext* ctx, TestCtx& tcc) {
        if(tcc.callback_count == 1) {
            MinionResult rc = minion_newVarMidsearch(ctx, tcc.instance, (char*)"z",
                                                    VAR_BOOL, 0, 1);
            if(rc != MINION_OK) return false;
            // w-literal(z, 1): pins z to 1.
            Var z = tcc.instance.vars.getSymbol("z");
            ConstraintBlob wlit(lib_getConstraint(CT_WATCHED_LIT));
            wlit.vars.push_back({z});
            wlit.constants.push_back({1});
            rc = minion_addConstraintMidsearch(ctx, tcc.instance, wlit);
            if(rc != MINION_OK) return false;
        }
        return true;
    };

    if(run(tc) != MINION_OK) return false;

    std::vector<Snapshot> expected;
    expected.push_back({{"x", 0}, {"y", 0}, {"z", MISSING}});
    for(int xv = 0; xv <= 2; ++xv)
        for(int yv = 0; yv <= 2; ++yv) {
            if(xv == 0 && yv == 0) continue;
            expected.push_back({{"x", xv}, {"y", yv}, {"z", 1}});
        }

    return check_solutions("midsearch_add_bool_with_eq_constant",
                           tc.solutions_seen, expected);
}

// Midsearch: add a var, then a constraint involving both it and a
// pre-existing var. z = x on all remaining-search solutions.
static bool test_midsearch_add_var_eq_existing() {
    TestCtx tc;
    newVar(tc.instance, "x", VAR_DISCRETE, {0, 2});
    newVar(tc.instance, "y", VAR_DISCRETE, {0, 2});
    add_search_order(tc.instance, {tc.instance.vars.getSymbol("x"),
                                   tc.instance.vars.getSymbol("y")});
    tc.tracked_vars = {"x", "y", "z"};

    tc.on_callback = [](MinionContext* ctx, TestCtx& tcc) {
        if(tcc.callback_count == 1) {
            MinionResult rc = minion_newVarMidsearch(ctx, tcc.instance, (char*)"z",
                                                    VAR_DISCRETE, 0, 2);
            if(rc != MINION_OK) return false;
            Var x = tcc.instance.vars.getSymbol("x");
            Var z = tcc.instance.vars.getSymbol("z");
            ConstraintBlob eq(lib_getConstraint(CT_EQ));
            eq.vars.push_back({z});
            eq.vars.push_back({x});
            rc = minion_addConstraintMidsearch(ctx, tcc.instance, eq);
            if(rc != MINION_OK) return false;
        }
        return true;
    };

    if(run(tc) != MINION_OK) return false;

    std::vector<Snapshot> expected;
    expected.push_back({{"x", 0}, {"y", 0}, {"z", MISSING}});
    for(int xv = 0; xv <= 2; ++xv)
        for(int yv = 0; yv <= 2; ++yv) {
            if(xv == 0 && yv == 0) continue;
            expected.push_back({{"x", xv}, {"y", yv}, {"z", xv}});
        }

    return check_solutions("midsearch_add_var_eq_existing",
                           tc.solutions_seen, expected);
}

// Midsearch: add a watched-or of two w-literals, neither with a dynamic
// trigger. Semantic: x=0 OR z=1. Aux valorder-ascending picks z=0; for
// x!=0 the left child fails so the or must assign z=1.
static bool test_midsearch_add_watched_or_two_wlit() {
    TestCtx tc;
    newVar(tc.instance, "x", VAR_DISCRETE, {0, 2});
    newVar(tc.instance, "y", VAR_DISCRETE, {0, 2});
    add_search_order(tc.instance, {tc.instance.vars.getSymbol("x"),
                                   tc.instance.vars.getSymbol("y")});
    tc.tracked_vars = {"x", "y", "z"};

    tc.on_callback = [](MinionContext* ctx, TestCtx& tcc) {
        if(tcc.callback_count == 1) {
            if(minion_newVarMidsearch(ctx, tcc.instance, (char*)"z",
                                      VAR_BOOL, 0, 1) != MINION_OK)
                return false;
            Var x = tcc.instance.vars.getSymbol("x");
            Var z = tcc.instance.vars.getSymbol("z");

            ConstraintBlob wlit_x(lib_getConstraint(CT_WATCHED_LIT));
            wlit_x.vars.push_back({x});
            wlit_x.constants.push_back({0});

            ConstraintBlob wlit_z(lib_getConstraint(CT_WATCHED_LIT));
            wlit_z.vars.push_back({z});
            wlit_z.constants.push_back({1});

            ConstraintBlob wor(lib_getConstraint(CT_WATCHED_NEW_OR));
            wor.internal_constraints.push_back(wlit_x);
            wor.internal_constraints.push_back(wlit_z);

            if(minion_addConstraintMidsearch(ctx, tcc.instance, wor) != MINION_OK)
                return false;
        }
        return true;
    };

    if(run(tc) != MINION_OK) return false;

    std::vector<Snapshot> expected;
    expected.push_back({{"x", 0}, {"y", 0}, {"z", MISSING}});
    for(int xv = 0; xv <= 2; ++xv)
        for(int yv = 0; yv <= 2; ++yv) {
            if(xv == 0 && yv == 0) continue;
            int zv = (xv == 0) ? 0 : 1;
            expected.push_back({{"x", xv}, {"y", yv}, {"z", zv}});
        }

    return check_solutions("midsearch_add_watched_or_two_wlit",
                           tc.solutions_seen, expected);
}

// Same shape with eq children. Both eq constraints have dynamic triggers,
// so the or's internal trigger-routing is exercised differently.
static bool test_midsearch_add_watched_or_two_eq() {
    TestCtx tc;
    newVar(tc.instance, "x", VAR_DISCRETE, {0, 2});
    newVar(tc.instance, "y", VAR_DISCRETE, {0, 2});
    add_search_order(tc.instance, {tc.instance.vars.getSymbol("x"),
                                   tc.instance.vars.getSymbol("y")});
    tc.tracked_vars = {"x", "y", "z"};

    tc.on_callback = [](MinionContext* ctx, TestCtx& tcc) {
        if(tcc.callback_count == 1) {
            if(minion_newVarMidsearch(ctx, tcc.instance, (char*)"z",
                                      VAR_BOOL, 0, 1) != MINION_OK)
                return false;
            Var x = tcc.instance.vars.getSymbol("x");
            Var z = tcc.instance.vars.getSymbol("z");

            // eq(x, 0)
            ConstraintBlob eq_x(lib_getConstraint(CT_EQ));
            Var c0 = constantAsVar(0);
            eq_x.vars.push_back({x});
            eq_x.vars.push_back({c0});

            // eq(z, 1)
            ConstraintBlob eq_z(lib_getConstraint(CT_EQ));
            Var c1 = constantAsVar(1);
            eq_z.vars.push_back({z});
            eq_z.vars.push_back({c1});

            ConstraintBlob wor(lib_getConstraint(CT_WATCHED_NEW_OR));
            wor.internal_constraints.push_back(eq_x);
            wor.internal_constraints.push_back(eq_z);

            if(minion_addConstraintMidsearch(ctx, tcc.instance, wor) != MINION_OK)
                return false;
        }
        return true;
    };

    if(run(tc) != MINION_OK) return false;

    std::vector<Snapshot> expected;
    expected.push_back({{"x", 0}, {"y", 0}, {"z", MISSING}});
    for(int xv = 0; xv <= 2; ++xv)
        for(int yv = 0; yv <= 2; ++yv) {
            if(xv == 0 && yv == 0) continue;
            int zv = (xv == 0) ? 0 : 1;
            expected.push_back({{"x", xv}, {"y", yv}, {"z", zv}});
        }

    return check_solutions("midsearch_add_watched_or_two_eq",
                           tc.solutions_seen, expected);
}

// reifyimply(eq(x, 0), z): z=1 → x=0; z=0 → no constraint. With z aux
// and valorder-ascending, z is picked 0 first, so eq(x, 0) is never
// activated. Exercises reifyimply's idle path — fullPropagate_called
// should stay false, watches on r and the inner constraint's vars.
static bool test_midsearch_add_reifyimply_idle() {
    TestCtx tc;
    newVar(tc.instance, "x", VAR_DISCRETE, {0, 2});
    newVar(tc.instance, "y", VAR_DISCRETE, {0, 2});
    add_search_order(tc.instance, {tc.instance.vars.getSymbol("x"),
                                   tc.instance.vars.getSymbol("y")});
    tc.tracked_vars = {"x", "y", "z"};

    tc.on_callback = [](MinionContext* ctx, TestCtx& tcc) {
        if(tcc.callback_count == 1) {
            if(minion_newVarMidsearch(ctx, tcc.instance, (char*)"z",
                                      VAR_BOOL, 0, 1) != MINION_OK)
                return false;
            Var x = tcc.instance.vars.getSymbol("x");
            Var z = tcc.instance.vars.getSymbol("z");

            // eq(x, 0)
            ConstraintBlob eq_x(lib_getConstraint(CT_EQ));
            Var c0 = constantAsVar(0);
            eq_x.vars.push_back({x});
            eq_x.vars.push_back({c0});

            // reifyimply(eq_x, z)
            ConstraintBlob rimp(lib_getConstraint(CT_REIFYIMPLY));
            rimp.internal_constraints.push_back(eq_x);
            rimp.vars.push_back({z});

            if(minion_addConstraintMidsearch(ctx, tcc.instance, rimp) != MINION_OK)
                return false;
        }
        return true;
    };

    if(run(tc) != MINION_OK) return false;

    std::vector<Snapshot> expected;
    expected.push_back({{"x", 0}, {"y", 0}, {"z", MISSING}});
    for(int xv = 0; xv <= 2; ++xv)
        for(int yv = 0; yv <= 2; ++yv) {
            if(xv == 0 && yv == 0) continue;
            // z aux picks 0; eq(x, 0) not required.
            expected.push_back({{"x", xv}, {"y", yv}, {"z", 0}});
        }

    return check_solutions("midsearch_add_reifyimply_idle",
                           tc.solutions_seen, expected);
}

// reifyimply(eq(x, 0), z) plus w-literal(z, 1) forcing z=1. Active path:
// the w-literal pins z=1, reifyimply then has to enforce eq(x, 0), so
// x=0 in all post-add solutions.
static bool test_midsearch_add_reifyimply_forced() {
    TestCtx tc;
    newVar(tc.instance, "x", VAR_DISCRETE, {0, 2});
    newVar(tc.instance, "y", VAR_DISCRETE, {0, 2});
    add_search_order(tc.instance, {tc.instance.vars.getSymbol("x"),
                                   tc.instance.vars.getSymbol("y")});
    tc.tracked_vars = {"x", "y", "z"};

    tc.on_callback = [](MinionContext* ctx, TestCtx& tcc) {
        if(tcc.callback_count == 1) {
            if(minion_newVarMidsearch(ctx, tcc.instance, (char*)"z",
                                      VAR_BOOL, 0, 1) != MINION_OK)
                return false;
            Var x = tcc.instance.vars.getSymbol("x");
            Var z = tcc.instance.vars.getSymbol("z");

            // w-literal(z, 1)
            ConstraintBlob wlit(lib_getConstraint(CT_WATCHED_LIT));
            wlit.vars.push_back({z});
            wlit.constants.push_back({1});
            if(minion_addConstraintMidsearch(ctx, tcc.instance, wlit) != MINION_OK)
                return false;

            // eq(x, 0)
            ConstraintBlob eq_x(lib_getConstraint(CT_EQ));
            Var c0 = constantAsVar(0);
            eq_x.vars.push_back({x});
            eq_x.vars.push_back({c0});

            // reifyimply(eq_x, z)
            ConstraintBlob rimp(lib_getConstraint(CT_REIFYIMPLY));
            rimp.internal_constraints.push_back(eq_x);
            rimp.vars.push_back({z});
            if(minion_addConstraintMidsearch(ctx, tcc.instance, rimp) != MINION_OK)
                return false;
        }
        return true;
    };

    if(run(tc) != MINION_OK) return false;

    std::vector<Snapshot> expected;
    expected.push_back({{"x", 0}, {"y", 0}, {"z", MISSING}});
    // z=1 forced; eq(x, 0) forced; only x=0 survives.
    for(int yv = 0; yv <= 2; ++yv) {
        if(yv == 0) continue; // (0,0) is the pre-add solution
        expected.push_back({{"x", 0}, {"y", yv}, {"z", 1}});
    }

    return check_solutions("midsearch_add_reifyimply_forced",
                           tc.solutions_seen, expected);
}

// reify(eq(x, 0), z): z iff x==0. Adds z mid-search and lets the
// reification drive z in both directions: x=0 forces z=1, x!=0 forces z=0.
static bool test_midsearch_add_reify() {
    TestCtx tc;
    newVar(tc.instance, "x", VAR_DISCRETE, {0, 2});
    newVar(tc.instance, "y", VAR_DISCRETE, {0, 2});
    add_search_order(tc.instance, {tc.instance.vars.getSymbol("x"),
                                   tc.instance.vars.getSymbol("y")});
    tc.tracked_vars = {"x", "y", "z"};

    tc.on_callback = [](MinionContext* ctx, TestCtx& tcc) {
        if(tcc.callback_count == 1) {
            if(minion_newVarMidsearch(ctx, tcc.instance, (char*)"z",
                                      VAR_BOOL, 0, 1) != MINION_OK)
                return false;
            Var x = tcc.instance.vars.getSymbol("x");
            Var z = tcc.instance.vars.getSymbol("z");

            ConstraintBlob eq_x(lib_getConstraint(CT_EQ));
            Var c0 = constantAsVar(0);
            eq_x.vars.push_back({x});
            eq_x.vars.push_back({c0});

            ConstraintBlob rfy(lib_getConstraint(CT_REIFY));
            rfy.internal_constraints.push_back(eq_x);
            rfy.vars.push_back({z});

            if(minion_addConstraintMidsearch(ctx, tcc.instance, rfy) != MINION_OK)
                return false;
        }
        return true;
    };

    if(run(tc) != MINION_OK) return false;

    std::vector<Snapshot> expected;
    expected.push_back({{"x", 0}, {"y", 0}, {"z", MISSING}});
    for(int xv = 0; xv <= 2; ++xv)
        for(int yv = 0; yv <= 2; ++yv) {
            if(xv == 0 && yv == 0) continue;
            expected.push_back({{"x", xv}, {"y", yv}, {"z", xv == 0 ? 1 : 0}});
        }

    return check_solutions("midsearch_add_reify", tc.solutions_seen, expected);
}

// Build the dominance-like reify that conjure-oxide injects mid-search:
//   w-literal(aux, 0)
//   reify(and(and(ineq(0, x, 0), ineq(y_value, y, 0)),
//             or(sumgeq([-1, x], 0), sumgeq([-1, y], y_value))),
//         aux)
// Translated from the Rust add_issue_dominance_like_reify helper in
// conjure-oxide's minion-sys ffi tests.
static std::pair<MinionResult, MinionResult>
add_issue_dominance_like_reify(MinionContext* ctx, CSPInstance& instance,
                               int y_value, const char* aux_name) {
    MinionResult add_var =
        minion_newVarMidsearch(ctx, instance, (char*)aux_name, VAR_BOOL, 0, 1);
    if(add_var != MINION_OK) return {add_var, add_var};

    Var x = instance.vars.getSymbol("x");
    Var y = instance.vars.getSymbol("y");
    Var aux = instance.vars.getSymbol(aux_name);

    ConstraintBlob wlit(lib_getConstraint(CT_WATCHED_LIT));
    wlit.vars.push_back({aux});
    wlit.constants.push_back({0});
    MinionResult add_wlit = minion_addConstraintMidsearch(ctx, instance, wlit);
    if(add_wlit != MINION_OK) return {add_var, add_wlit};

    ConstraintBlob ineq_x(lib_getConstraint(CT_INEQ));
    Var c0 = constantAsVar(0);
    ineq_x.vars.push_back({c0});
    ineq_x.vars.push_back({x});
    ineq_x.constants.push_back({0});

    ConstraintBlob ineq_y(lib_getConstraint(CT_INEQ));
    Var cy = constantAsVar(y_value);
    ineq_y.vars.push_back({cy});
    ineq_y.vars.push_back({y});
    ineq_y.constants.push_back({0});

    ConstraintBlob and_inner(lib_getConstraint(CT_WATCHED_NEW_AND));
    and_inner.internal_constraints.push_back(ineq_x);
    and_inner.internal_constraints.push_back(ineq_y);

    ConstraintBlob sum_x(lib_getConstraint(CT_GEQSUM));
    std::vector<Var> sx_lhs = {constantAsVar(-1), x};
    sum_x.vars.push_back(sx_lhs);
    std::vector<Var> sx_rhs = {constantAsVar(0)};
    sum_x.vars.push_back(sx_rhs);

    ConstraintBlob sum_y(lib_getConstraint(CT_GEQSUM));
    std::vector<Var> sy_lhs = {constantAsVar(-1), y};
    sum_y.vars.push_back(sy_lhs);
    std::vector<Var> sy_rhs = {constantAsVar(y_value)};
    sum_y.vars.push_back(sy_rhs);

    ConstraintBlob or_inner(lib_getConstraint(CT_WATCHED_NEW_OR));
    or_inner.internal_constraints.push_back(sum_x);
    or_inner.internal_constraints.push_back(sum_y);

    ConstraintBlob and_outer(lib_getConstraint(CT_WATCHED_NEW_AND));
    and_outer.internal_constraints.push_back(and_inner);
    and_outer.internal_constraints.push_back(or_inner);

    ConstraintBlob rfy(lib_getConstraint(CT_REIFY));
    rfy.internal_constraints.push_back(and_outer);
    rfy.vars.push_back({aux});
    MinionResult add_reify = minion_addConstraintMidsearch(ctx, instance, rfy);
    return {add_var, add_reify};
}

// The conjure-oxide reproducer scenario: multiple callbacks each inject a
// dominance-like reify over freshly-added bool vars. Originally segfaulted
// (missing aux-block plumbing), then tripped the
// reify-fullPropagate stale-trigger assert; should now run clean.
//
// Solution-set verification here is deliberately absent: the instance +
// mid-search mutations are complex enough that hand-deriving the expected
// set is error-prone. The check is "runs to MINION_OK, fires ≥2 callbacks,
// every mid-search add/addConstraint returns MINION_OK." That's what the
// Rust issue_child_runner test verifies.
static bool test_midsearch_reify_dominance_reproducer() {
    TestCtx tc;
    MinionResult r1 = minion_newVar(tc.instance, (char*)"x", VAR_BOUND, 0, 3);
    MinionResult r2 = minion_newVar(tc.instance, (char*)"y", VAR_BOUND, 0, 3);
    if(r1 != MINION_OK || r2 != MINION_OK) return false;
    Var x = tc.instance.vars.getSymbol("x");
    Var y = tc.instance.vars.getSymbol("y");
    add_search_order(tc.instance, {x, y});
    tc.tracked_vars = {"x", "y"};

    // sumgeq([x, y], 2)
    ConstraintBlob geq(lib_getConstraint(CT_GEQSUM));
    std::vector<Var> lhs = {x, y};
    geq.vars.push_back(lhs);
    std::vector<Var> rhs = {constantAsVar(2)};
    geq.vars.push_back(rhs);
    tc.instance.constraints.push_back(geq);

    struct State {
        int add_ok = 0;
        int add_fail = 0;
    } state;

    auto hook = [](MinionContext* ctx, TestCtx& tcc, State* st) -> bool {
        auto tally = [st](MinionResult r) {
            if(r == MINION_OK) st->add_ok++;
            else st->add_fail++;
        };
        if(tcc.callback_count == 1) {
            auto r = add_issue_dominance_like_reify(ctx, tcc.instance, 2, "dyn_aux_0");
            tally(r.first);
            tally(r.second);
            return true;
        }
        if(tcc.callback_count == 2) {
            tally(minion_newVarMidsearch(ctx, tcc.instance,
                                         (char*)"dyn_aux_1_unused",
                                         VAR_BOOL, 0, 1));
            auto r = add_issue_dominance_like_reify(ctx, tcc.instance, 3, "dyn_aux_2");
            tally(r.first);
            tally(r.second);
            return true;
        }
        return false;
    };

    tc.on_callback = [&state, hook](MinionContext* ctx, TestCtx& tcc) {
        return hook(ctx, tcc, &state);
    };

    if(run(tc) != MINION_OK) return false;
    if(tc.callback_count < 2) {
        std::cerr << "  reify_dominance_reproducer: callback_count=" << tc.callback_count
                  << " < 2\n";
        return false;
    }
    if(state.add_fail != 0) {
        std::cerr << "  reify_dominance_reproducer: " << state.add_fail
                  << " mid-search calls failed\n";
        return false;
    }
    return true;
}

// Midsearch: watched-and of two w-literals. Enforces both z=1 AND x=0.
// Only (0, *, 1) solutions for * in {1, 2}; (0, 0, _) was pre-add.
static bool test_midsearch_add_watched_and_two_wlit() {
    TestCtx tc;
    newVar(tc.instance, "x", VAR_DISCRETE, {0, 2});
    newVar(tc.instance, "y", VAR_DISCRETE, {0, 2});
    add_search_order(tc.instance, {tc.instance.vars.getSymbol("x"),
                                   tc.instance.vars.getSymbol("y")});
    tc.tracked_vars = {"x", "y", "z"};

    tc.on_callback = [](MinionContext* ctx, TestCtx& tcc) {
        if(tcc.callback_count == 1) {
            if(minion_newVarMidsearch(ctx, tcc.instance, (char*)"z",
                                      VAR_BOOL, 0, 1) != MINION_OK)
                return false;
            Var x = tcc.instance.vars.getSymbol("x");
            Var z = tcc.instance.vars.getSymbol("z");

            ConstraintBlob wlit_x(lib_getConstraint(CT_WATCHED_LIT));
            wlit_x.vars.push_back({x});
            wlit_x.constants.push_back({0});

            ConstraintBlob wlit_z(lib_getConstraint(CT_WATCHED_LIT));
            wlit_z.vars.push_back({z});
            wlit_z.constants.push_back({1});

            ConstraintBlob wand(lib_getConstraint(CT_WATCHED_NEW_AND));
            wand.internal_constraints.push_back(wlit_x);
            wand.internal_constraints.push_back(wlit_z);

            if(minion_addConstraintMidsearch(ctx, tcc.instance, wand) != MINION_OK)
                return false;
        }
        return true;
    };

    if(run(tc) != MINION_OK) return false;

    std::vector<Snapshot> expected;
    expected.push_back({{"x", 0}, {"y", 0}, {"z", MISSING}});
    for(int yv = 0; yv <= 2; ++yv) {
        if(yv == 0) continue;
        expected.push_back({{"x", 0}, {"y", yv}, {"z", 1}});
    }

    return check_solutions("midsearch_add_watched_and_two_wlit",
                           tc.solutions_seen, expected);
}

// Same shape with eq children (dynamic triggers exercised).
static bool test_midsearch_add_watched_and_two_eq() {
    TestCtx tc;
    newVar(tc.instance, "x", VAR_DISCRETE, {0, 2});
    newVar(tc.instance, "y", VAR_DISCRETE, {0, 2});
    add_search_order(tc.instance, {tc.instance.vars.getSymbol("x"),
                                   tc.instance.vars.getSymbol("y")});
    tc.tracked_vars = {"x", "y", "z"};

    tc.on_callback = [](MinionContext* ctx, TestCtx& tcc) {
        if(tcc.callback_count == 1) {
            if(minion_newVarMidsearch(ctx, tcc.instance, (char*)"z",
                                      VAR_BOOL, 0, 1) != MINION_OK)
                return false;
            Var x = tcc.instance.vars.getSymbol("x");
            Var z = tcc.instance.vars.getSymbol("z");

            ConstraintBlob eq_x(lib_getConstraint(CT_EQ));
            Var c0 = constantAsVar(0);
            eq_x.vars.push_back({x});
            eq_x.vars.push_back({c0});

            ConstraintBlob eq_z(lib_getConstraint(CT_EQ));
            Var c1 = constantAsVar(1);
            eq_z.vars.push_back({z});
            eq_z.vars.push_back({c1});

            ConstraintBlob wand(lib_getConstraint(CT_WATCHED_NEW_AND));
            wand.internal_constraints.push_back(eq_x);
            wand.internal_constraints.push_back(eq_z);

            if(minion_addConstraintMidsearch(ctx, tcc.instance, wand) != MINION_OK)
                return false;
        }
        return true;
    };

    if(run(tc) != MINION_OK) return false;

    std::vector<Snapshot> expected;
    expected.push_back({{"x", 0}, {"y", 0}, {"z", MISSING}});
    for(int yv = 0; yv <= 2; ++yv) {
        if(yv == 0) continue;
        expected.push_back({{"x", 0}, {"y", yv}, {"z", 1}});
    }

    return check_solutions("midsearch_add_watched_and_two_eq",
                           tc.solutions_seen, expected);
}

struct Test {
    const char* name;
    bool (*run)();
    bool expected_fail;  // true → failure is known, exit stays clean
};

static std::vector<Test> tests = {
    {"baseline_unconstrained", test_baseline_unconstrained, false},
    {"baseline_eq", test_baseline_eq, false},
    {"midsearch_add_bool_unconstrained", test_midsearch_add_bool_unconstrained, false},
    {"midsearch_add_bound_unconstrained", test_midsearch_add_bound_unconstrained, false},
    {"midsearch_add_discrete_unconstrained", test_midsearch_add_discrete_unconstrained, false},
    {"midsearch_add_bool_with_eq_constant", test_midsearch_add_bool_with_eq_constant, false},
    {"midsearch_add_var_eq_existing", test_midsearch_add_var_eq_existing, false},
    {"midsearch_add_watched_or_two_wlit", test_midsearch_add_watched_or_two_wlit, false},
    {"midsearch_add_watched_or_two_eq", test_midsearch_add_watched_or_two_eq, false},
    {"midsearch_add_reifyimply_idle", test_midsearch_add_reifyimply_idle, false},
    {"midsearch_add_reifyimply_forced", test_midsearch_add_reifyimply_forced, false},
    {"midsearch_add_reify", test_midsearch_add_reify, false},
    {"midsearch_add_watched_and_two_wlit", test_midsearch_add_watched_and_two_wlit, false},
    {"midsearch_add_watched_and_two_eq", test_midsearch_add_watched_and_two_eq, false},
    {"midsearch_reify_dominance_reproducer", test_midsearch_reify_dominance_reproducer, false},
};

int main() {
    int passed = 0, failed_expected = 0, failed_unexpected = 0, passed_unexpected = 0;
    for(auto& t : tests) {
        bool ok = false;
        try {
            ok = t.run();
        } catch(const std::exception& e) {
            std::cerr << "  [" << t.name << "] EXCEPTION: " << e.what() << "\n";
        }
        const char* tag = "[PASS]";
        if(ok && t.expected_fail) {
            tag = "[XPASS]";
            passed_unexpected++;
        } else if(ok) {
            passed++;
        } else if(t.expected_fail) {
            tag = "[XFAIL]";
            failed_expected++;
        } else {
            tag = "[FAIL]";
            failed_unexpected++;
        }
        std::cerr << tag << " " << t.name << "\n";
    }
    std::cerr << "\n" << passed << " passed";
    if(failed_expected) std::cerr << ", " << failed_expected << " expected-fail";
    if(passed_unexpected) std::cerr << ", " << passed_unexpected << " PASSED UNEXPECTEDLY";
    if(failed_unexpected) std::cerr << ", " << failed_unexpected << " FAILED";
    std::cerr << " (of " << tests.size() << ")\n";
    // Unexpected-pass is also a regression signal: the known-broken
    // behaviour changed, update expectations.
    return (failed_unexpected + passed_unexpected) == 0 ? 0 : 1;
}
