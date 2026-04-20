// Reproducer for mid-search reify-on-fresh-bool-var assertion.
// Translated from the Rust test
// ffi::tests::midsearch_reify_newvar_issue_reproducer
// in conjure-oxide/crates/minion-sys.

#include "libwrapper.h"
#include "inputfile_parse/CSPSpec.h"
#include "minion.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>

using namespace ProbSpec;

struct IssueState {
    CSPInstance* instance;
    int callback_count = 0;
    std::vector<MinionResult> add_var_results;
    std::vector<MinionResult> add_constraint_results;
};

static void build_issue_instance(CSPInstance& instance) {
    MinionResult r1 = minion_newVar(instance, (char*)"x", VAR_BOUND, 0, 3);
    assert(r1 == MINION_OK);
    MinionResult r2 = minion_newVar(instance, (char*)"y", VAR_BOUND, 0, 3);
    assert(r2 == MINION_OK);

    VarResult x = minion_getVarByName(instance, (char*)"x");
    VarResult y = minion_getVarByName(instance, (char*)"y");
    assert(x.result == MINION_OK);
    assert(y.result == MINION_OK);

    instance.print_matrix.push_back({x.var});
    instance.print_matrix.push_back({y.var});

    std::vector<Var> order = {x.var, y.var};
    instance.searchOrder.push_back(SearchOrder(order, ORDER_STATIC, false));

    // sumgeq([x, y], 2)
    ConstraintBlob geq(lib_getConstraint(CT_GEQSUM));
    std::vector<Var> lhs = {x.var, y.var};
    constraint_addList(geq, lhs);
    std::vector<Var> rhs = {constantAsVar(2)};
    constraint_addList(geq, rhs);
    instance.constraints.push_back(geq);
}

// Build and register (mid-search) a reify over a freshly-added bool var aux:
//   reify(AND(AND(ineq(0,x,0), ineq(y_value,y,0)),
//             OR(sumgeq([-1,x], 0), sumgeq([-1,y], y_value))),
//         aux)
// plus a w-literal(aux, 0).
static std::pair<MinionResult, MinionResult>
add_issue_dominance_like_reify(MinionContext* ctx, CSPInstance& instance,
                               int y_value, const char* aux_name) {
    MinionResult add_var =
        minion_newVarMidsearch(ctx, instance, (char*)aux_name, VAR_BOOL, 0, 1);
    if(add_var != MINION_OK) {
        return {add_var, add_var};
    }

    VarResult x_res = minion_getVarByName(instance, (char*)"x");
    VarResult y_res = minion_getVarByName(instance, (char*)"y");
    VarResult aux_res = minion_getVarByName(instance, (char*)aux_name);
    if(x_res.result != MINION_OK || y_res.result != MINION_OK ||
       aux_res.result != MINION_OK) {
        return {add_var, MINION_INVALID_INSTANCE};
    }
    Var x = x_res.var;
    Var y = y_res.var;
    Var aux = aux_res.var;

    // w-literal(aux, 0)
    ConstraintBlob wlit(lib_getConstraint(CT_WATCHED_LIT));
    constraint_addVar(wlit, aux);
    constraint_addConstant(wlit, 0);
    MinionResult add_wlit = minion_addConstraintMidsearch(ctx, instance, wlit);
    if(add_wlit != MINION_OK) {
        return {add_var, add_wlit};
    }

    // ineq(0, x, 0)
    ConstraintBlob ineq_x(lib_getConstraint(CT_INEQ));
    Var c0 = constantAsVar(0);
    constraint_addVar(ineq_x, c0);
    constraint_addVar(ineq_x, x);
    constraint_addConstant(ineq_x, 0);

    // ineq(y_value, y, 0)
    ConstraintBlob ineq_y(lib_getConstraint(CT_INEQ));
    Var cy = constantAsVar(y_value);
    constraint_addVar(ineq_y, cy);
    constraint_addVar(ineq_y, y);
    constraint_addConstant(ineq_y, 0);

    ConstraintBlob and_inner(lib_getConstraint(CT_WATCHED_NEW_AND));
    constraint_addConstraint(and_inner, ineq_x);
    constraint_addConstraint(and_inner, ineq_y);

    // sumgeq([-1, x], 0)
    ConstraintBlob sum_x(lib_getConstraint(CT_GEQSUM));
    std::vector<Var> sx_lhs = {constantAsVar(-1), x};
    constraint_addList(sum_x, sx_lhs);
    std::vector<Var> sx_rhs = {constantAsVar(0)};
    constraint_addList(sum_x, sx_rhs);

    // sumgeq([-1, y], y_value)
    ConstraintBlob sum_y(lib_getConstraint(CT_GEQSUM));
    std::vector<Var> sy_lhs = {constantAsVar(-1), y};
    constraint_addList(sum_y, sy_lhs);
    std::vector<Var> sy_rhs = {constantAsVar(y_value)};
    constraint_addList(sum_y, sy_rhs);

    ConstraintBlob or_inner(lib_getConstraint(CT_WATCHED_NEW_OR));
    constraint_addConstraint(or_inner, sum_x);
    constraint_addConstraint(or_inner, sum_y);

    ConstraintBlob and_outer(lib_getConstraint(CT_WATCHED_NEW_AND));
    constraint_addConstraint(and_outer, and_inner);
    constraint_addConstraint(and_outer, or_inner);

    // reify(and_outer, aux)
    ConstraintBlob rfy(lib_getConstraint(CT_REIFY));
    constraint_addConstraint(rfy, and_outer);
    constraint_addVar(rfy, aux);

    MinionResult add_reify = minion_addConstraintMidsearch(ctx, instance, rfy);
    return {add_var, add_reify};
}

static bool issue_midsearch_callback(MinionContext* ctx, void* userdata) {
    IssueState* state = reinterpret_cast<IssueState*>(userdata);
    state->callback_count += 1;

    if(state->callback_count == 1) {
        auto r = add_issue_dominance_like_reify(ctx, *state->instance, 2, "dyn_aux_0");
        state->add_var_results.push_back(r.first);
        state->add_constraint_results.push_back(r.second);
        return true;
    }

    if(state->callback_count == 2) {
        MinionResult add_unused = minion_newVarMidsearch(
            ctx, *state->instance, (char*)"dyn_aux_1_unused", VAR_BOOL, 0, 1);
        state->add_var_results.push_back(add_unused);

        auto r = add_issue_dominance_like_reify(ctx, *state->instance, 3, "dyn_aux_2");
        state->add_var_results.push_back(r.first);
        state->add_constraint_results.push_back(r.second);
        return true;
    }

    return false;
}

int main() {
    MinionContext* ctx = minion_newContext();
    CSPInstance instance;
    build_issue_instance(instance);

    SearchOptions options;
    options.silent = false;
    options.print_solution = true;
    SearchMethod method;
    method.preprocess = PropLevel_GAC;
    method.propMethod = PropLevel_GAC;

    IssueState state;
    state.instance = &instance;

    MinionResult rc = runMinion(ctx, options, method, instance,
                                issue_midsearch_callback, &state);

    std::cerr << "runMinion returned " << (int)rc << std::endl;
    std::cerr << "callback_count=" << state.callback_count << std::endl;
    for(size_t i = 0; i < state.add_var_results.size(); ++i) {
        std::cerr << "add_var[" << i << "]=" << (int)state.add_var_results[i]
                  << std::endl;
    }
    for(size_t i = 0; i < state.add_constraint_results.size(); ++i) {
        std::cerr << "add_constraint[" << i
                  << "]=" << (int)state.add_constraint_results[i] << std::endl;
    }

    minion_freeContext(ctx);
    return rc == MINION_OK ? 0 : 1;
}
