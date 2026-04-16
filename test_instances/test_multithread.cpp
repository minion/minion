// Stress test for multiple concurrent Minion contexts.
// Spins up many threads, each creating/solving/destroying contexts repeatedly.
//
// Build:
//   cd bin-lib
//   c++ -std=c++14 -I ../minion -I src/ -DLIBMINION -DWDEG -O2 \
//       ../test_instances/test_multithread.cpp -L. -lminion -lpthread -o test_multithread

#include "libwrapper.h"
#include "inputfile_parse/CSPSpec.h"
#include "minion.h"

#include <thread>
#include <atomic>
#include <vector>
#include <iostream>
#include <cassert>
#include <mutex>

using namespace ProbSpec;

std::mutex output_lock;

// ---- Problem builders ----

// Problem 1: x,y in [1..n], x < y
// Solutions: n*(n-1)/2
int solve_pairs(MinionContext* ctx, int n) {
    CSPInstance instance;
    newVar(instance, "x", VAR_DISCRETE, {1, n});
    newVar(instance, "y", VAR_DISCRETE, {1, n});
    Var x = instance.vars.getSymbol("x");
    Var y = instance.vars.getSymbol("y");

    instance.print_matrix.push_back({x});
    instance.print_matrix.push_back({y});
    instance.searchOrder.push_back(SearchOrder({x, y}, ORDER_STATIC, false));

    ConstraintBlob ineq(lib_getConstraint(CT_INEQ));
    ineq.vars.push_back({x});
    ineq.vars.push_back({y});
    ineq.constants.push_back({-1});
    instance.constraints.push_back(ineq);

    SearchOptions options;
    options.findAllSolutions();
    SearchMethod method;
    method.preprocess = PropLevel_GAC;
    method.propMethod = PropLevel_GAC;

    MinionResult rc = runMinion(ctx, options, method, instance, +[](MinionContext*, void*) -> bool { return true; }, nullptr);
    assert(rc == MINION_OK);

    // Read solution count from TableOut
    char* sols_str = TableOut_get(ctx, (char*)"SolutionsFound");
    assert(sols_str != nullptr);
    int sols = std::atoi(sols_str);
    std::free(sols_str);
    return sols;
}

// Problem 2: alldiff with k vars in [1..n]
// Solutions: n! / (n-k)! (permutations of k from n)
int solve_alldiff(MinionContext* ctx, int k, int n) {
    CSPInstance instance;

    std::vector<Var> vars;
    for(int i = 0; i < k; i++) {
        std::string name = "x" + std::to_string(i);
        newVar(instance, name, VAR_DISCRETE, {1, n});
        vars.push_back(instance.vars.getSymbol(name));
    }

    for(auto& v : vars)
        instance.print_matrix.push_back({v});

    instance.searchOrder.push_back(SearchOrder(vars, ORDER_STATIC, false));

    ConstraintBlob ad(lib_getConstraint(CT_ALLDIFF));
    ad.vars.push_back(vars);
    instance.constraints.push_back(ad);

    SearchOptions options;
    options.findAllSolutions();
    SearchMethod method;
    method.preprocess = PropLevel_GAC;
    method.propMethod = PropLevel_GAC;

    MinionResult rc = runMinion(ctx, options, method, instance, +[](MinionContext*, void*) -> bool { return true; }, nullptr);
    assert(rc == MINION_OK);

    char* sols_str = TableOut_get(ctx, (char*)"SolutionsFound");
    assert(sols_str != nullptr);
    int sols = std::atoi(sols_str);
    std::free(sols_str);
    return sols;
}

// Problem 3: sum constraint. x1..xk in [1..n], sum <= bound
// Count solutions by brute-force comparison
int solve_bounded_sum(MinionContext* ctx, int k, int n, int bound) {
    CSPInstance instance;

    std::vector<Var> vars;
    for(int i = 0; i < k; i++) {
        std::string name = "x" + std::to_string(i);
        newVar(instance, name, VAR_DISCRETE, {1, n});
        vars.push_back(instance.vars.getSymbol(name));
    }

    for(auto& v : vars)
        instance.print_matrix.push_back({v});

    instance.searchOrder.push_back(SearchOrder(vars, ORDER_STATIC, false));

    // sumleq(vars, bound)
    ConstraintBlob sumleq(lib_getConstraint(CT_LEQSUM));
    sumleq.vars.push_back(vars);
    sumleq.vars.push_back({constantAsVar(bound)});
    instance.constraints.push_back(sumleq);

    SearchOptions options;
    options.findAllSolutions();
    SearchMethod method;
    method.preprocess = PropLevel_GAC;
    method.propMethod = PropLevel_GAC;

    MinionResult rc = runMinion(ctx, options, method, instance, +[](MinionContext*, void*) -> bool { return true; }, nullptr);
    assert(rc == MINION_OK);

    char* sols_str = TableOut_get(ctx, (char*)"SolutionsFound");
    assert(sols_str != nullptr);
    int sols = std::atoi(sols_str);
    std::free(sols_str);
    return sols;
}

// ---- Brute-force expected values ----

int expected_pairs(int n) {
    return n * (n - 1) / 2;
}

// n! / (n-k)!
int expected_alldiff(int k, int n) {
    int result = 1;
    for(int i = 0; i < k; i++)
        result *= (n - i);
    return result;
}

int expected_bounded_sum(int k, int n, int bound) {
    // Recursively count tuples (x1..xk) in [1..n]^k with sum <= bound
    if(k == 0) return (bound >= 0) ? 1 : 0;
    int count = 0;
    for(int v = 1; v <= n; v++) {
        count += expected_bounded_sum(k - 1, n, bound - v);
    }
    return count;
}

// ---- Mid-search mutation tests ----

enum MidsearchScenario {
    MID_BULK_ADD_AND_CONSTRAIN,
    MID_MULTI_CALLBACK_ADD_AND_CONSTRAIN,
    MID_DUPLICATE_NAME_ACROSS_CALLBACKS
};

struct MidsearchState {
    MidsearchScenario scenario;
    CSPInstance* instance;
    int callback_count = 0;
    int added_count = 0;
    int constrained_count = 0;
    bool all_ok = true;
    MinionResult duplicate_second_add = MINION_OK;
};

bool midsearch_callback(MinionContext* ctx, void* userdata) {
    MidsearchState* state = reinterpret_cast<MidsearchState*>(userdata);
    state->callback_count++;

    VarResult x_res = minion_getVarByName(*state->instance, (char*)"x");
    if(x_res.result != MINION_OK) {
        state->all_ok = false;
        return false;
    }

    if(state->scenario == MID_BULK_ADD_AND_CONSTRAIN) {
        // Add and constrain many variables in one callback.
        for(int i = 0; i < 40; i++) {
            std::string name = "mid_bulk_" + std::to_string(i);
            MinionResult add_res = minion_newVarMidsearch(ctx, *state->instance, (char*)name.c_str(),
                                                          VAR_BOUND, 0, 3);
            if(add_res != MINION_OK) {
                state->all_ok = false;
                break;
            }
            state->added_count++;

            VarResult v_res = minion_getVarByName(*state->instance, (char*)name.c_str());
            if(v_res.result != MINION_OK) {
                state->all_ok = false;
                break;
            }

            ConstraintBlob eq(lib_getConstraint(CT_EQ));
            eq.vars.push_back({v_res.var});
            eq.vars.push_back({x_res.var});
            MinionResult con_res = minion_addConstraintMidsearch(ctx, *state->instance, eq);
            if(con_res != MINION_OK) {
                state->all_ok = false;
                break;
            }
            state->constrained_count++;
        }
        return false;
    }

    if(state->scenario == MID_MULTI_CALLBACK_ADD_AND_CONSTRAIN) {
        // Add one variable each callback and keep searching for a few callbacks.
        std::string name = "mid_step_" + std::to_string(state->callback_count);
        MinionResult add_res =
            minion_newVarMidsearch(ctx, *state->instance, (char*)name.c_str(), VAR_BOUND, 0, 2);
        if(add_res != MINION_OK) {
            state->all_ok = false;
            return false;
        }
        state->added_count++;

        VarResult v_res = minion_getVarByName(*state->instance, (char*)name.c_str());
        if(v_res.result != MINION_OK) {
            state->all_ok = false;
            return false;
        }
        ConstraintBlob eq(lib_getConstraint(CT_EQ));
        eq.vars.push_back({v_res.var});
        eq.vars.push_back({x_res.var});
        MinionResult con_res = minion_addConstraintMidsearch(ctx, *state->instance, eq);
        if(con_res != MINION_OK) {
            state->all_ok = false;
            return false;
        }
        state->constrained_count++;
        return state->callback_count < 3;
    }

    // MID_DUPLICATE_NAME_ACROSS_CALLBACKS
    const char* dup_name = "mid_dup";
    if(state->callback_count == 1) {
        MinionResult add_res =
            minion_newVarMidsearch(ctx, *state->instance, (char*)dup_name, VAR_BOUND, 0, 1);
        if(add_res != MINION_OK) {
            state->all_ok = false;
            return false;
        }
        state->added_count++;
        return true;
    }

    state->duplicate_second_add =
        minion_newVarMidsearch(ctx, *state->instance, (char*)dup_name, VAR_BOUND, 0, 1);
    return false;
}

std::pair<bool, std::string> solve_midsearch_mutation(MinionContext* ctx, MidsearchScenario scenario) {
    CSPInstance instance;
    newVar(instance, "x", VAR_DISCRETE, {0, 2});
    newVar(instance, "y", VAR_DISCRETE, {0, 2});
    Var x = instance.vars.getSymbol("x");
    Var y = instance.vars.getSymbol("y");

    instance.print_matrix.push_back({x});
    instance.print_matrix.push_back({y});
    instance.searchOrder.push_back(SearchOrder({x, y}, ORDER_STATIC, false));

    SearchOptions options;
    options.findAllSolutions();
    SearchMethod method;
    method.preprocess = PropLevel_GAC;
    method.propMethod = PropLevel_GAC;

    MidsearchState state;
    state.scenario = scenario;
    state.instance = &instance;

    MinionResult rc = runMinion(ctx, options, method, instance, midsearch_callback, &state);
    if(rc != MINION_OK) {
        return {false, "runMinion returned " + std::to_string((int)rc)};
    }
    if(!state.all_ok) {
        return {false, "callback operation failed"};
    }

    if(scenario == MID_BULK_ADD_AND_CONSTRAIN) {
        bool ok = (state.callback_count >= 1 && state.added_count == 40 && state.constrained_count == 40);
        return {ok, "callbacks=" + std::to_string(state.callback_count) + ", added=" +
                        std::to_string(state.added_count) + ", constrained=" +
                        std::to_string(state.constrained_count)};
    }

    if(scenario == MID_MULTI_CALLBACK_ADD_AND_CONSTRAIN) {
        bool ok = (state.callback_count >= 2 && state.added_count >= 2 && state.constrained_count >= 2);
        return {ok, "callbacks=" + std::to_string(state.callback_count) + ", added=" +
                        std::to_string(state.added_count) + ", constrained=" +
                        std::to_string(state.constrained_count)};
    }

    bool ok = (state.callback_count == 2 && state.duplicate_second_add == MINION_PARSE_ERROR);
    return {ok, "callbacks=" + std::to_string(state.callback_count) + ", second_add_result=" +
                    std::to_string((int)state.duplicate_second_add)};
}

// ---- Test runners ----

struct TestResult {
    std::string name;
    bool passed;
    std::string detail;
};

void check(std::vector<TestResult>& results, const std::string& name, int got, int expected) {
    bool ok = (got == expected);
    std::string detail = "got " + std::to_string(got) + ", expected " + std::to_string(expected);
    results.push_back({name, ok, detail});
}

void check_bool(std::vector<TestResult>& results, const std::string& name, bool ok,
                const std::string& detail) {
    results.push_back({name, ok, detail});
}

void worker_thread(int thread_id, int iterations, std::vector<TestResult>& results) {
    for(int iter = 0; iter < iterations; iter++) {
        // Each iteration creates a fresh context, solves a problem, destroys it.
        // Vary the problem type and parameters.

        int problem = (thread_id * 7 + iter * 3) % 8;

        MinionContext* ctx = minion_newContext();

        switch(problem) {
        case 0: {
            int n = 5 + (iter % 16);
            int got = solve_pairs(ctx, n);
            check(results, "T" + std::to_string(thread_id) + " iter" + std::to_string(iter)
                  + " pairs(n=" + std::to_string(n) + ")", got, expected_pairs(n));
            break;
        }
        case 1: {
            // alldiff: k distinct values from [1..n]
            int k = 3 + (iter % 3); // k=3,4,5
            int n = k + 1 + (iter % 3); // n=k+1..k+3
            int got = solve_alldiff(ctx, k, n);
            check(results, "T" + std::to_string(thread_id) + " iter" + std::to_string(iter)
                  + " alldiff(k=" + std::to_string(k) + ",n=" + std::to_string(n) + ")",
                  got, expected_alldiff(k, n));
            break;
        }
        case 2: {
            int k = 3, n = 4, bound = 7;
            int got = solve_bounded_sum(ctx, k, n, bound);
            check(results, "T" + std::to_string(thread_id) + " iter" + std::to_string(iter)
                  + " bounded_sum(3,4,7)", got, expected_bounded_sum(k, n, bound));
            break;
        }
        case 3: {
            int k = 4, n = 3, bound = 8;
            int got = solve_bounded_sum(ctx, k, n, bound);
            check(results, "T" + std::to_string(thread_id) + " iter" + std::to_string(iter)
                  + " bounded_sum(4,3,8)", got, expected_bounded_sum(k, n, bound));
            break;
        }
        case 4: {
            int n = 10 + (iter % 11);
            int got = solve_pairs(ctx, n);
            check(results, "T" + std::to_string(thread_id) + " iter" + std::to_string(iter)
                  + " pairs(n=" + std::to_string(n) + ")", got, expected_pairs(n));
            break;
        }
        case 5: {
            auto ret = solve_midsearch_mutation(ctx, MID_BULK_ADD_AND_CONSTRAIN);
            check_bool(results,
                       "T" + std::to_string(thread_id) + " iter" + std::to_string(iter)
                       + " mid_bulk_add_and_constrain",
                       ret.first, ret.second);
            break;
        }
        case 6: {
            auto ret = solve_midsearch_mutation(ctx, MID_MULTI_CALLBACK_ADD_AND_CONSTRAIN);
            check_bool(results,
                       "T" + std::to_string(thread_id) + " iter" + std::to_string(iter)
                       + " mid_multi_callback_add_and_constrain",
                       ret.first, ret.second);
            break;
        }
        case 7: {
            auto ret = solve_midsearch_mutation(ctx, MID_DUPLICATE_NAME_ACROSS_CALLBACKS);
            check_bool(results,
                       "T" + std::to_string(thread_id) + " iter" + std::to_string(iter)
                       + " mid_duplicate_name_across_callbacks",
                       ret.first, ret.second);
            break;
        }
        }

        minion_freeContext(ctx);
    }
}

int main() {
    const int NUM_THREADS = 8;
    const int ITERATIONS_PER_THREAD = 10;

    std::vector<std::thread> threads;
    std::vector<std::vector<TestResult>> all_results(NUM_THREADS);

    std::cerr << "Starting " << NUM_THREADS << " threads, "
              << ITERATIONS_PER_THREAD << " iterations each..." << std::endl;

    for(int t = 0; t < NUM_THREADS; t++) {
        threads.emplace_back(worker_thread, t, ITERATIONS_PER_THREAD,
                             std::ref(all_results[t]));
    }

    for(auto& t : threads)
        t.join();

    // Collate results
    int total = 0, passed = 0, failed = 0;
    for(int t = 0; t < NUM_THREADS; t++) {
        for(auto& r : all_results[t]) {
            total++;
            if(r.passed) {
                passed++;
            } else {
                failed++;
                std::cerr << "FAIL: " << r.name << " - " << r.detail << std::endl;
            }
        }
    }

    std::cerr << std::endl;
    std::cerr << passed << " / " << total << " tests passed." << std::endl;
    if(failed > 0) {
        std::cerr << failed << " FAILURES." << std::endl;
        return 1;
    }

    std::cerr << "PASS: All multi-threaded stress tests succeeded!" << std::endl;
    return 0;
}
