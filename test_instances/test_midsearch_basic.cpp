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
        return (int)ref.assignedValue();
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
    // w-literal has no dynamic triggers and relies on its initial
    // fullPropagate. After backtrack unassigns z, the constraint never
    // fires again at intermediate depths (constraintsToPropagate only
    // re-runs fullPropagate on pops past its add-depth minus one), so z
    // gets freely branched to 0 in the aux block even though z must be 1.
    // Needs either a generalised re-propagation mechanism or a trigger in
    // w-literal for fresh-var support.
    {"midsearch_add_bool_with_eq_constant", test_midsearch_add_bool_with_eq_constant, true},
    {"midsearch_add_var_eq_existing", test_midsearch_add_var_eq_existing, false},
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
