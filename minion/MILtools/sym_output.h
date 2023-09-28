// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#include <algorithm>
#include <cmath>
#include <set>
#include <limits>

std::vector<std::vector<DomainInt>> build_graph(std::vector<std::set<SysInt>> graph,
                                                const std::vector<std::set<SysInt>>& partition);

SysInt repartition(const std::vector<std::set<SysInt>>& graph, std::vector<SysInt> partitionNum) {
  std::vector<std::multiset<SysInt>> partition_loop(graph.size());
  for(SysInt i = 0; i < (SysInt)graph.size(); ++i)
    for(set<SysInt>::const_iterator it = graph[i].begin(); it != graph[i].end(); ++it) {
      partition_loop[i].insert(partitionNum[*it]);
      partition_loop[*it].insert(partitionNum[i]);
    }

  std::set<std::multiset<SysInt>> partition_set(partition_loop.begin(), partition_loop.end());
  std::vector<std::multiset<SysInt>> partitionVec(partition_set.begin(), partition_set.end());

  for(SysInt i = 0; i < (SysInt)graph.size(); ++i) {
    partitionNum[i] =
        find(partitionVec.begin(), partitionVec.end(), partition_loop[i]) - partitionVec.begin();
  }

  return partition_set.size();
}

double
partition_graph(const std::tuple<SysInt, vector<set<SysInt>>, vector<set<SysInt>>>& graph_tuple) {
  std::vector<std::set<SysInt>> graph;
  std::vector<std::set<SysInt>> partition;
  SysInt blank;
  std::tie(blank, graph, partition) = graph_tuple;

  std::vector<SysInt> partitionNum(graph.size());

  for(SysInt i = 0; i < (SysInt)partition.size(); ++i) {
    for(std::set<SysInt>::iterator it = partition[i].begin(); it != partition[i].end(); ++it)
      partitionNum[*it] = i;
  }

  SysInt partitionCount = 0;
  bool done = false;
  while(!done) {
    SysInt new_partitionCount = repartition(graph, partitionNum);
    if(partitionCount == new_partitionCount)
      done = true;
    partitionCount = new_partitionCount;
  }

  SysInt diffCount = 0;
  for(SysInt i = 0; i < (SysInt)partitionNum.size(); ++i)
    for(SysInt j = 0; j < (SysInt)partitionNum.size(); ++j)
      if(partitionNum[i] != partitionNum[j])
        diffCount++;

  return (double)(diffCount) / (double)(partitionNum.size() * partitionNum.size());
}

template <typename Name = string, typename Colour = string>
struct Graph {
  set<pair<Name, Name>> graph;
  map<Name, set<Colour>> aux_vertex_colour;
  map<Name, set<Colour>> var_vertex_colour;
  SysInt free_vertices;

  Graph() : free_vertices(0) {}

  Name new_vertex() {
    free_vertices++;
    return "F." + tostring(free_vertices);
  }

  Name new_vertex(string colour) {
    free_vertices++;
    string s = "F." + colour + "." + tostring(free_vertices);
    aux_vertex_colour[colour].insert(s);
    return s;
  }

  void output_graph() {
    for(map<string, set<string>>::iterator it = var_vertex_colour.begin();
        it != var_vertex_colour.end(); ++it) {
      cout << it->first << " : ";
      for(set<string>::iterator it2 = it->second.begin(); it2 != it->second.end(); it2++)
        cout << *it2 << " ";
      cout << endl;
    }

    for(map<string, set<string>>::iterator it = aux_vertex_colour.begin();
        it != aux_vertex_colour.end(); ++it) {
      cout << it->first << " : ";
      for(set<string>::iterator it2 = it->second.begin(); it2 != it->second.end(); it2++)
        cout << *it2 << " ";
      cout << endl;
    }

    cout << endl;

    for(set<pair<string, string>>::iterator it = graph.begin(); it != graph.end(); ++it) {
      cout << it->first << ", " << it->second << endl;
    }
  }

  std::tuple<SysInt, vector<set<SysInt>>, vector<set<SysInt>>>
  build_graph_info(CSPInstance& csp, bool print_names = true) {

    map<string, SysInt> vNum;

    SysInt var_vertexCount = 0, aux_vertexCount = 0;
    for(map<string, set<string>>::iterator it = var_vertex_colour.begin();
        it != var_vertex_colour.end(); ++it)
      var_vertexCount += it->second.size();

    for(map<string, set<string>>::iterator it = aux_vertex_colour.begin();
        it != aux_vertex_colour.end(); ++it)
      aux_vertexCount += it->second.size();

    if(print_names)
      cout << "varnames := [";
    for(SysInt i = 0; i < (SysInt)csp.symOrder.size(); ++i) {
      if(print_names)
        cout << "\"" << name(csp.symOrder[i], csp) << "\", ";
      vNum[name(csp.symOrder[i], csp)] = i + 1;
    }
    if(print_names)
      cout << "];" << endl;
    SysInt vertexCounter = vNum.size() + 1;

    // Now output partitions

    vector<set<SysInt>> partitions;

    set<SysInt> zero;
    zero.insert(0);
    partitions.push_back(zero);

    for(map<string, set<string>>::iterator it = var_vertex_colour.begin();
        it != var_vertex_colour.end(); ++it) {
      D_ASSERT(it->second.size() > 0);
      set<SysInt> partition;
      for(set<string>::iterator it2 = it->second.begin(); it2 != it->second.end(); it2++)
        partition.insert(vNum[*it2]);

      partitions.push_back(partition);
    }

    for(map<string, set<string>>::iterator it = aux_vertex_colour.begin();
        it != aux_vertex_colour.end(); ++it) {
      set<SysInt> partition;
      D_ASSERT(it->second.size() > 0);
      for(set<string>::iterator it2 = it->second.begin(); it2 != it->second.end(); it2++) {
        vNum[*it2] = vertexCounter;
        partition.insert(vertexCounter);
        vertexCounter++;
      }

      partitions.push_back(partition);
    }

    vector<set<SysInt>> edges(var_vertexCount + aux_vertexCount + 1);

    for(set<pair<string, string>>::iterator it = graph.begin(); it != graph.end(); ++it) {
      // cout << it->first << ":" << it->second << endl;
      D_ASSERT(vNum.count(it->first) == 1);
      D_ASSERT(vNum.count(it->second) == 1);
      SysInt first_v = vNum[it->first];
      SysInt second_v = vNum[it->second];
      D_ASSERT(first_v != 0 && second_v != 0 && first_v != second_v);
      edges[first_v].insert(second_v);
    }

    return std::make_tuple(var_vertexCount, edges, partitions);
  }

  void output_nauty_graph(CSPInstance& csp) {
    SysInt var_vertexCount;
    vector<set<SysInt>> edges;
    vector<set<SysInt>> partitions;

    std::tie(var_vertexCount, edges, partitions) = build_graph_info(csp);
#ifdef USE_NAUTY
    vector<vector<DomainInt>> perms = build_graph(edges, partitions);
    cout << "generators := [()" << endl;
    for(SysInt i = 0; i < (SysInt)perms.size(); ++i) {
      cout << ", PermList([";
      bool first_pass = true;
      D_ASSERT(perms[i][0] == 0);
      for(SysInt j = 1; j <= var_vertexCount; ++j) {
        D_ASSERT(perms[i][j] <= var_vertexCount);
        if(first_pass) {
          first_pass = false;
          cout << perms[i][j];
        } else
          cout << ", " << perms[i][j];
      }
      cout << "])" << endl;
    }
    cout << "];" << endl;
#else
    cerr << "Need to compile Minion with nauty included!" << endl;
    exit(1);
#endif
  }

  string name(Var v, CSPInstance& csp) {
    if(v.type() == VAR_CONSTANT) {
      string const_name = "CONSTANT_" + tostring(v.pos());
      aux_vertex_colour[const_name].insert(const_name);
      return const_name;
    } else
      return csp.vars.getName(v);
  }
};

struct GraphBuilder {
  CSPInstance& csp;
  Graph<> g;

  GraphBuilder(CSPInstance& _csp) : csp(_csp) {
    csp.add_variable_names();
    if(csp.symOrder.empty())
      D_FATAL_ERROR("Symmetry detection doesn't work with input formats 1 and 2. Upgrade!");

    build_graph();
  }

  void build_graph() {
    colour_vertices();
    for(list<ConstraintBlob>::iterator it = csp.constraints.begin(); it != csp.constraints.end();
        ++it)
      colour_constraint(*it);
  }

  void colour_vertices() {
    vector<Var> vars = csp.vars.getAllVars();
    for(SysInt i = 0; i < (SysInt)vars.size(); ++i) {
      g.var_vertex_colour[tostring(csp.vars.getDomain(vars[i]))].insert(csp.vars.getName(vars[i]));
    }
  }

  string name(Var v) {
    if(v.type() == VAR_CONSTANT) {
      string const_name = "CONSTANT_" + tostring(v.pos());
      g.aux_vertex_colour[const_name].insert(const_name);
      return const_name;
    } else if(v.type() == VAR_NOTBOOL) {
      string aux_v = g.new_vertex("NOTBOOL");
      add_edge(aux_v, csp.vars.getName(Var(VAR_BOOL, v.pos())));
      return aux_v;
    } else
      return csp.vars.getName(v);
  }

  void add_edge(string s1, string s2) {
    g.graph.insert(make_pair(s1, s2));
  }

  void add_edge(Var v1, string s2) {
    g.graph.insert(make_pair(name(v1), s2));
  }

  void add_edge(string s1, Var v2) {
    g.graph.insert(make_pair(s1, name(v2)));
  }

  string colour_element(const ConstraintBlob& b, string name) {
    string v = g.new_vertex(name + "_MASTER");
    for(SysInt i = 0; i < (SysInt)b.vars[0].size(); ++i) {
      string t = g.new_vertex(name + "_CHILD_" + tostring(i));
      add_edge(v, t);
      add_edge(t, b.vars[0][i]);
    }

    string vIndex = g.new_vertex(name + "_INDEX");
    add_edge(v, vIndex);
    add_edge(vIndex, b.vars[1][0]);

    string vResult = g.new_vertex(name + "_RESULT");
    add_edge(v, vResult);
    add_edge(vResult, b.vars[2][0]);

    return v;
  }

  // A constraint where each array is independantly symmetric
  string colour_symmetric_constraint(const ConstraintBlob& b, string name) {
    string v = g.new_vertex(name + "_MASTER");

    for(SysInt i = 0; i < (SysInt)b.vars.size(); ++i) {
      string nv = g.new_vertex(name + "_CHILD" + tostring(i));
      add_edge(v, nv);
      for(SysInt j = 0; j < (SysInt)b.vars[i].size(); ++j)
        add_edge(nv, b.vars[i][j]);
    }

    for(SysInt i = 0; i < (SysInt)b.constants.size(); ++i) {
      string nv = g.new_vertex(name + "_CHILD_CONST" + tostring(i));
      add_edge(v, nv);
      for(SysInt j = 0; j < (SysInt)b.constants[i].size(); ++j)
        add_edge(nv, Var(VAR_CONSTANT, b.constants[i][j]));
    }

    return v;
  }

  string colour_eq(const ConstraintBlob& b, string name) {
    string v = g.new_vertex(name + "_MASTER");
    add_edge(v, b.vars[0][0]);
    add_edge(v, b.vars[1][0]);
    return v;
  }

  string colour_no_symmetry(const ConstraintBlob& b, string name) {
    string v = g.new_vertex(name + "_MASTER");

    for(SysInt i = 0; i < (SysInt)b.vars.size(); ++i)
      for(SysInt j = 0; j < (SysInt)b.vars[i].size(); ++j) {
        string vij = g.new_vertex(name + "_CHILD_" + tostring(i) + ";" + tostring(j));
        add_edge(v, vij);
        add_edge(vij, b.vars[i][j]);
      }

    return v;
  }

  // Symmetries where the first array can be permuted, if the same permutation
  // is applied to the second array, and also pairs X[i] and Y[i] can be
  // independantly swapped.
  // Other variables are assumed not symmetric.
  // Example: Hamming(M1, M2, C) - The hamming distance of arrays M1 and M2 is C
  string colour_array_swap_eachIndex(const ConstraintBlob& b, string name) {
    D_ASSERT(b.vars[0].size() == b.vars[1].size());

    string v = g.new_vertex(name + "_MASTER");

    // Force each array to stay together.
    for(SysInt i = 0; i < 2; ++i) {
      string vi = g.new_vertex(name + "_ARRAY_STAY_TOGETHER");
      add_edge(v, vi);
      for(SysInt j = 0; j < (SysInt)b.vars[i].size(); ++j)
        add_edge(vi, b.vars[i][j]);
    }

    for(SysInt i = 0; i < (SysInt)b.vars[0].size(); ++i) {
      string vi = g.new_vertex(name + "_INDEX");
      add_edge(v, vi);
      add_edge(vi, b.vars[0][i]);
      add_edge(vi, b.vars[1][i]);
    }

    for(SysInt i = 2; i < (SysInt)b.vars.size(); ++i) {
      D_ASSERT(b.vars[i].size() == 1);
      string vi = g.new_vertex(name + "_POS_" + tostring(i));
      add_edge(v, vi);
    }

    return v;
  }

  // The first array can be permuted, if the same permutation is applied to the
  // second array.
  string colour_symmetricIndexes(const ConstraintBlob& b, string name) {

    string v = g.new_vertex(name + "_MASTER");

    for(SysInt i = 0; i < (SysInt)b.vars[0].size(); ++i) {
      string vm = g.new_vertex(name + "_INDEX");
      string v1 = g.new_vertex(name + "_ARRAY1");
      string v2 = g.new_vertex(name + "_ARRAY2");

      add_edge(v, vm);
      add_edge(vm, v1);
      add_edge(vm, v2);
      add_edge(v1, b.vars[0][i]);
      add_edge(v2, b.vars[1][i]);
    }

    for(SysInt i = 2; i < (SysInt)b.vars.size(); ++i) {
      D_ASSERT(b.vars[i].size() == 1);
      string vi = g.new_vertex(name + "_POS_" + tostring(i));
      add_edge(v, vi);
    }

    return v;
  }

  // The first array can be permuted, if the same permutation is applied to the
  // second array.
  string colour_weighted_sum(const ConstraintBlob& b, string name) {
    D_ASSERT(b.vars[0].size() == b.constants[0].size());

    string v = g.new_vertex(name + "_MASTER");

    for(SysInt i = 0; i < (SysInt)b.vars[0].size(); ++i) {
      string vm = g.new_vertex(name + "_INDEX");
      string v1 = g.new_vertex(name + "_ARRAY1");
      string v2 = g.new_vertex(name + "_ARRAY2");

      add_edge(v, vm);
      add_edge(vm, v1);
      add_edge(vm, v2);
      add_edge(v1, b.vars[0][i]);
      add_edge(v2, Var(VAR_CONSTANT, b.constants[0][i]));
    }

    for(SysInt i = 1; i < (SysInt)b.vars.size(); ++i) {
      D_ASSERT(b.vars[i].size() == 1);
      string vi = g.new_vertex(name + "_POS_" + tostring(i));
      add_edge(v, vi);
    }
    return v;
  }

  string colour_lit(const ConstraintBlob& b, string name) {
    D_ASSERT(b.vars[0].size() == 1 && b.constants[0].size() == 1);
    string v = g.new_vertex(name + "_MASTER");

    string vm = g.new_vertex(name + "_CHILD_1");
    add_edge(v, Var(VAR_CONSTANT, b.constants[0][0]));
    add_edge(v, vm);
    add_edge(vm, b.vars[0][0]);

    return v;
  }

  string colour_litlist(const ConstraintBlob& b, string name) {
    D_ASSERT(b.vars[0].size() == 1 && b.constants.size() == 1);
    string v = g.new_vertex(name + "_MASTER");

    string vm = g.new_vertex(name + "_CHILD_1");
    for(SysInt i = 0; i < (SysInt)b.constants[0].size(); ++i)
      add_edge(v, Var(VAR_CONSTANT, b.constants[0][i]));
    add_edge(v, vm);
    add_edge(vm, b.vars[0][0]);

    return v;
  }

  string colour_gcc(const ConstraintBlob& b, string name) {
    D_ASSERT(b.vars.size() == 2 && b.constants.size() == 1);
    D_ASSERT(b.vars[1].size() == b.constants[0].size());

    string v = g.new_vertex(name + "_SECOND_MASTER");

    for(SysInt i = 0; i < (SysInt)b.vars[1].size(); ++i) {
      string vm = g.new_vertex(name + "_INDEX");
      string v1 = g.new_vertex(name + "_ARRAY1");
      string v2 = g.new_vertex(name + "_ARRAY2");

      add_edge(v, vm);
      add_edge(vm, v1);
      add_edge(vm, v2);
      add_edge(v1, b.vars[0][i]);
      add_edge(v2, Var(VAR_CONSTANT, b.constants[0][i]));
    }

    string w = g.new_vertex(name + "_FIRST_MASTER");

    for(SysInt j = 0; j < (SysInt)b.vars[0].size(); ++j)
      add_edge(w, b.vars[0][j]);

    string x = g.new_vertex(name + "_MASTER");

    add_edge(x, w);
    add_edge(x, v);

    return x;
  }

  string colour_reify(const ConstraintBlob& b, string name) {
    D_ASSERT(b.vars.size() == 1 && b.vars[0].size() == 1);
    D_ASSERT(b.internal_constraints.size() == 1);

    string v = g.new_vertex(name + "_HEAD");

    string reify_var = g.new_vertex(name + "_REIFYVAR");

    add_edge(v, reify_var);
    add_edge(reify_var, b.vars[0][0]);

    string child_con = colour_constraint(b.internal_constraints[0]);

    add_edge(v, child_con);

    return v;
  }

  string colour_symmetric_parent_constraint(const ConstraintBlob& b, string name) {
    D_ASSERT(b.vars.size() == 0 && b.constants.size() == 0);
    string v = g.new_vertex(name + "_HEAD");

    for(SysInt i = 0; i < (SysInt)b.internal_constraints.size(); ++i) {
      string child_con = colour_constraint(b.internal_constraints[i]);
      add_edge(v, child_con);
    }
    return v;
  }

  string colour_constraint(const ConstraintBlob& b) {
    switch(b.constraint->type) {

    case CT_REIFY: return colour_reify(b, "REIFY");

    case CT_CHECK_GSA: return colour_constraint(b.internal_constraints[0]);

    case CT_CHECK_ASSIGN: return colour_constraint(b.internal_constraints[0]);

    case CT_REIFYIMPLY: return colour_reify(b, "REIFYIMPLY");

    case CT_REIFYIMPLY_QUICK: return colour_reify(b, "REIFYIMPLY_QUICK");

    case CT_ELEMENT: return colour_element(b, "ELEMENT");

    case CT_WATCHED_ELEMENT: return colour_element(b, "ELEMENT");

    case CT_ELEMENT_UNDEFZERO: return colour_element(b, "ELEMENT_UNDEFZERO");

    case CT_WATCHED_ELEMENT_UNDEFZERO: return colour_element(b, "ELEMENT_UNDEFZERO");

    case CT_ELEMENT_ONE: return colour_element(b, "ELEMENT_ONE");

    case CT_WATCHED_ELEMENT_ONE: return colour_element(b, "ELEMENT_ONE");

    case CT_WATCHED_ELEMENT_ONE_UNDEFZERO: return colour_element(b, "ELEMENT_ONE_UNDEFZERO");

    case CT_ALLDIFF: return colour_symmetric_constraint(b, "ALLDIFF");

    case CT_GACALLDIFF: return colour_symmetric_constraint(b, "ALLDIFF");

    case CT_WATCHED_NEQ: return colour_eq(b, "DISEQ");

    case CT_DISEQ: return colour_eq(b, "DISEQ");

    case CT_EQ:
    case CT_GACEQ: return colour_eq(b, "EQ");

    case CT_MINUSEQ: return colour_no_symmetry(b, "MINUSEQ");

    case CT_ABS: return colour_no_symmetry(b, "ABS");

    case CT_INEQ: return colour_no_symmetry(b, "INEQ");

    case CT_WATCHED_LESS: return colour_no_symmetry(b, "LESS");

    case CT_LEXLEQ: return colour_no_symmetry(b, "LEXLEQ");

    case CT_LEXLESS: return colour_no_symmetry(b, "LEXLESS");

    case CT_GACLEXLEQ: return colour_no_symmetry(b, "LEXLEQ");

    case CT_QUICK_LEXLEQ: return colour_no_symmetry(b, "LEXLEQ");

    case CT_QUICK_LEXLESS: return colour_no_symmetry(b, "LEXLESS");

    case CT_MAX: return colour_symmetric_constraint(b, "MAX");

    case CT_MIN: return colour_symmetric_constraint(b, "MIN");

    case CT_OCCURRENCE: return colour_symmetric_constraint(b, "OCCURRENCE");

    case CT_LEQ_OCCURRENCE: return colour_symmetric_constraint(b, "OCC_LEQ");

    case CT_GEQ_OCCURRENCE: return colour_symmetric_constraint(b, "OCC_GEQ");

    case CT_PRODUCT2: return colour_symmetric_constraint(b, "PRODUCT");

    case CT_DIFFERENCE: return colour_symmetric_constraint(b, "DIFFERENCE");

    case CT_WEIGHTGEQSUM: return colour_weighted_sum(b, "WEIGHT_GEQSUM");

    case CT_WEIGHTLEQSUM: return colour_weighted_sum(b, "WEIGHT_LEQSUM");

    case CT_GEQSUM: return colour_symmetric_constraint(b, "GEQSUM");

    case CT_WATCHED_GEQSUM: return colour_symmetric_constraint(b, "GEQSUM");

    case CT_LEQSUM: return colour_symmetric_constraint(b, "LEQSUM");

    case CT_WATCHED_LEQSUM: return colour_symmetric_constraint(b, "LEQSUM");

    case CT_WATCHED_TABLE:
    case CT_MDDC:
    case CT_HAGGISGAC:
    case CT_HAGGISGAC_STABLE:
    case CT_LIGHTTABLE:
    case CT_STR: return colour_no_symmetry(b, "TABLE");

    case CT_WATCHED_NEGATIVE_TABLE:
    case CT_NEGATIVEMDDC: return colour_no_symmetry(b, "NEG_TABLE");

    case CT_WATCHED_VECNEQ: return colour_array_swap_eachIndex(b, "VECNEQ");

    case CT_WATCHED_LITSUM: return colour_no_symmetry(b, "LITSUM");

    case CT_POW: return colour_no_symmetry(b, "POW");

    case CT_DIV: return colour_no_symmetry(b, "DIV");

    case CT_DIV_UNDEFZERO: return colour_no_symmetry(b, "DIV_UNDEFZERO");

    case CT_MODULO: return colour_no_symmetry(b, "MOD");

    case CT_MODULO_UNDEFZERO: return colour_no_symmetry(b, "MOD_UNDEFZERO");

    case CT_WATCHED_VEC_OR_LESS: return colour_symmetricIndexes(b, "VEC_OR_LESS");

    case CT_WATCHED_HAMMING: return colour_array_swap_eachIndex(b, "HAMMING");

    case CT_WATCHED_LIT: return colour_lit(b, "WATCHED_LIT");

    case CT_WATCHED_NOTLIT: return colour_lit(b, "WATCHED_NOTLIT");

    case CT_WATCHED_INSET: return colour_litlist(b, "WATCHED_INSET");

    case CT_WATCHED_ININTERVALSET: return colour_litlist(b, "WATCHED_ININTERVAL_SET");

    case CT_WATCHED_NOT_INSET: return colour_litlist(b, "WATCHED_NOT_INSET");

    case CT_WATCHED_INRANGE: return colour_litlist(b, "WATCHED_INRANGE");

    case CT_WATCHED_NOT_INRANGE: return colour_litlist(b, "WATCHED_NOT_INRANGE");

    case CT_GCC: return colour_gcc(b, "GCC");

    case CT_GCCWEAK: return colour_gcc(b, "GCC");

    case CT_FALSE: return g.new_vertex("FALSE");

    case CT_TRUE: return g.new_vertex("TRUE");

    case CT_WATCHED_NEW_OR: return colour_symmetric_parent_constraint(b, "OR");

    case CT_WATCHED_NEW_AND: return colour_symmetric_parent_constraint(b, "AND");

    default: cerr << "No colouring defined for " << b.constraint->name << endl; abort();
    }
  }
};

struct InstanceStats {
  CSPInstance& csp;

  InstanceStats(CSPInstance& _csp) : csp(_csp) {}

  void classifyConstraint(ConstraintBlob i, SysInt* alldiff, SysInt* sums, SysInt* or_atleastk,
                          SysInt* ternary, SysInt* binary, SysInt* table, SysInt* reify,
                          SysInt* lex, SysInt* unary, SysInt* nullary, SysInt* element,
                          SysInt* minmax, SysInt* occurrence, vector<double>* alldiffdomovervars,
                          VarContainer& v) {
    ConstraintType ct = i.constraint->type;
    switch(ct) {
    case CT_WATCHED_LIT:
    case CT_WATCHED_NOTLIT:
    case CT_WATCHED_INSET:
    case CT_WATCHED_NOT_INSET:
    case CT_WATCHED_INRANGE:
    case CT_WATCHED_ININTERVALSET:
    case CT_WATCHED_NOT_INRANGE: (*unary)++; break;
    case CT_ALLDIFF:
    case CT_GACALLDIFF: {
      (*alldiff)++;
      SysInt num = 0;
      DomainInt upper = std::numeric_limits<int>::min();
      DomainInt lower = std::numeric_limits<int>::max();
      for(SysInt j = 0; j < (SysInt)i.vars.size(); j++) {
        for(SysInt k = 0; k < (SysInt)i.vars[j].size(); k++) {
          num++;
          Bounds bounds = v.getBounds(i.vars[j][k]);
          lower = lower > bounds.lowerBound ? bounds.lowerBound : lower;
          upper = upper < bounds.upperBound ? bounds.upperBound : upper;
        }
      }
      alldiffdomovervars->push_back((double)(checked_cast<SysInt>(upper - lower + 1)) /
                                    (double)num);
    } break;
    case CT_GEQSUM:
    case CT_LEQSUM:
    case CT_WEIGHTGEQSUM:
    case CT_WEIGHTLEQSUM:
    case CT_WATCHED_GEQSUM:
    case CT_WATCHED_LEQSUM: (*sums)++; break;
    case CT_WATCHED_NEW_OR:
    case CT_WATCHED_NEW_AND:
    case CT_WATCHED_LITSUM:
    case CT_WATCHED_VECNEQ:
    case CT_WATCHED_HAMMING:
    case CT_WATCHED_NOT_HAMMING:
    case CT_WATCHED_VEC_OR_LESS: (*or_atleastk)++; break;
    case CT_PRODUCT2:
    case CT_DIFFERENCE:
    case CT_MODULO:
    case CT_MODULO_UNDEFZERO:
    case CT_DIV:
    case CT_DIV_UNDEFZERO:
    case CT_POW: (*ternary)++; break;
    case CT_ABS:
    case CT_INEQ:
    case CT_EQ:
    case CT_GACEQ:
    case CT_MINUSEQ:
    case CT_DISEQ:
    case CT_WATCHED_NEQ:
    case CT_WATCHED_LESS: (*binary)++; break;
    case CT_REIFY:
    case CT_DISEQ_REIFY:
    case CT_EQ_REIFY:
    case CT_MINUSEQ_REIFY:
    case CT_REIFYIMPLY_QUICK:
    case CT_REIFYIMPLY:
      (*reify)++;
      for(vector<ConstraintBlob>::iterator j = i.internal_constraints.begin();
          j != i.internal_constraints.end(); ++j) {
        classifyConstraint(*j, alldiff, sums, or_atleastk, ternary, binary, table, reify, lex,
                           unary, nullary, element, minmax, occurrence, alldiffdomovervars, v);
      }
      break;
    case CT_WATCHED_TABLE:
    case CT_WATCHED_NEGATIVE_TABLE:
    case CT_GACSCHEMA:
    case CT_HAGGISGAC:
    case CT_HAGGISGAC_STABLE:
    case CT_MDDC:
    case CT_NEGATIVEMDDC:
    case CT_SHORTSTR:
    case CT_STR:
    case CT_SHORTSTR_CTUPLE:
    case CT_LIGHTTABLE: (*table)++; break;
    case CT_GACLEXLEQ:
    case CT_QUICK_LEXLEQ:
    case CT_LEXLEQ:
    case CT_LEXLESS:
    case CT_QUICK_LEXLESS: (*lex)++; break;
    case CT_TRUE:
    case CT_FALSE: (*nullary)++; break;
    case CT_ELEMENT:
    case CT_ELEMENT_ONE:
    case CT_WATCHED_ELEMENT:
    case CT_ELEMENT_UNDEFZERO:
    case CT_WATCHED_ELEMENT_ONE:
    case CT_WATCHED_ELEMENT_ONE_UNDEFZERO:
    case CT_WATCHED_ELEMENT_UNDEFZERO: (*element)++; break;
    case CT_MIN:
    case CT_MAX: (*minmax)++; break;
    case CT_OCCURRENCE:
    case CT_LEQ_OCCURRENCE:
    case CT_GEQ_OCCURRENCE:
    case CT_GCC:
    case CT_LEQNVALUE:
    case CT_GEQNVALUE:
    case CT_GCCWEAK: (*occurrence)++; break;

    case CT_CHECK_GSA:
    case CT_ALLDIFFMATRIX:
    case CT_FRAMEUPDATE:
    case CT_FORWARD_CHECKING:
    case CT_CHECK_ASSIGN:
    case CT_COLLECTEVENTS:
      cerr << "Stats: Uncategorised constraint:" << i.constraint->name << endl;
    }
  }

#define START_CLOCK() startTime = get_cpuTime()
#define END_CLOCK()                                                                                \
  measuredTime = get_cpuTime() - startTime;                                                     \
  cout << "TIME: " << measuredTime << endl;
#define output_stat cout << measuredTime << " " << s

  void output_stats() {
    string s("stats_"); // common prefix
    // Variables statistics
    double startTime, measuredTime;
    START_CLOCK();
    VarContainer& v = csp.vars;
    SysInt varcount = v.BOOLs + v.bound.size() + v.sparseBound.size() + v.discrete.size();
    END_CLOCK();
    output_stat << "varcount:" << varcount << endl;
    output_stat << "var_bool:" << v.BOOLs << endl;
    output_stat << "varDiscrete:" << v.discrete.size() << endl;
    output_stat << "varBound:" << v.bound.size() << endl;
    output_stat << "var_sparsebound:" << v.sparseBound.size() << endl;

    // collect all domain sizes into an array
    START_CLOCK();
    vector<DomainInt> domsizes;
    long long int varMemoryUsage = 0;
    double domain_product = 1;
    for(SysInt i = 0; i < v.BOOLs; i++) {
      domsizes.push_back(2);
      varMemoryUsage += 1;
      domain_product += log((double)2);
    }
    for(SysInt i = 0; i < (SysInt)v.bound.size(); i++) {
      SysInt domSize = checked_cast<SysInt>(v.bound[i].upperBound - v.bound[i].lowerBound + 1);
      domsizes.push_back(domSize);
      varMemoryUsage += 64;
      domain_product += log((double)domSize);
    }
    for(SysInt i = 0; i < (SysInt)v.discrete.size(); i++) {
      SysInt domSize =
          checked_cast<SysInt>(v.discrete[i].upperBound - v.discrete[i].lowerBound + 1);
      domsizes.push_back(domSize);
      varMemoryUsage += domSize;
      domain_product += log((double)domSize);
    }
    for(SysInt i = 0; i < (SysInt)v.sparseBound.size(); i++) {
      domsizes.push_back(v.sparseBound[i].size());
      varMemoryUsage += 64;
      domain_product += log((double)v.sparseBound[i].size());
    }
    END_CLOCK();
    output_stat << "VarMemory: " << varMemoryUsage << endl;
    output_stat << "DomainProductLog: " << domain_product << endl;
    std::sort(domsizes.begin(), domsizes.end());
    // Some rubbish which does not give you the real medians, quartiles
    output_stat << "dom_0:" << domsizes[0] << endl;
    output_stat << "dom_25:" << domsizes[domsizes.size() / 4] << endl;
    output_stat << "dom_50:" << domsizes[domsizes.size() / 2] << endl;
    output_stat << "dom_75:" << domsizes[(domsizes.size() * 3) / 4] << endl;
    output_stat << "dom_100:" << domsizes.back() << endl;

    SysInt totaldom =
        checked_cast<SysInt>(std::accumulate(domsizes.begin(), domsizes.end(), (DomainInt)0));
    output_stat << "dom_mean:" << ((double)totaldom) / (double)domsizes.size() << endl;

    SysInt num2s = std::count(domsizes.begin(), domsizes.end(), (DomainInt)2);
    output_stat << "dom_not2_2_ratio:" << ((double)(varcount - num2s)) / (double)num2s << endl;

    output_stat << "discrete_bool_ratio:" << ((double)v.discrete.size()) / (double)v.BOOLs << endl;

    START_CLOCK();
    SysInt branchingvars = 0;
    SysInt auxvars = 0;
    for(SysInt i = 0; i < (SysInt)csp.searchOrder.size(); i++) {
      if(csp.searchOrder[i].findOneAssignment) {
        auxvars = auxvars + csp.searchOrder[i].varOrder.size();
      } else {
        branchingvars = branchingvars + csp.searchOrder[i].varOrder.size();
      }
    }
    END_CLOCK();
    output_stat << "branchingvars:" << branchingvars << endl;
    output_stat << "auxvars:" << auxvars << endl;
    output_stat << "auxvar_branching_ratio:" << ((double)auxvars) / (double)branchingvars << endl;

    //////////////////////////////////////////////////////////////////////////
    // Constraint stats
    START_CLOCK();
    list<ConstraintBlob>& c = csp.constraints;
    END_CLOCK();
    output_stat << "conscount:" << c.size() << endl;
    START_CLOCK();
    vector<DomainInt> arities;
    for(list<ConstraintBlob>::iterator i = c.begin(); i != c.end(); ++i) {
      arities.push_back(arity(*i));
    }
    std::sort(arities.begin(), arities.end());
    END_CLOCK();
    output_stat << "arity_0:" << arities[0] << endl;
    output_stat << "arity_25:" << arities[arities.size() / 4] << endl;
    output_stat << "arity_50:" << arities[arities.size() / 2] << endl;
    output_stat << "arity_75:" << arities[(arities.size() * 3) / 4] << endl;
    output_stat << "arity_100:" << arities.back() << endl;

    const SysInt totalarity =
        checked_cast<SysInt>(std::accumulate(arities.begin(), arities.end(), (DomainInt)0));

    output_stat << "TotalArity: " << totalarity << endl;
    output_stat << "arity_mean:" << ((double)totalarity) / (double)arities.size() << endl;
    output_stat << "arity_mean_normalised:"
                << (((double)totalarity) / (double)arities.size()) / ((double)varcount) << endl;
    output_stat << "cts_per_var_mean:" << ((double)totalarity) / (double)varcount << endl;
    output_stat << "cts_per_var_mean_normalised:"
                << (((double)totalarity) / ((double)varcount)) / ((double)c.size()) << endl;

    START_CLOCK();
    // alldiff stats
    vector<double> alldiffdomovervars;

    // six categories of constraint, output their proportion and count
    SysInt alldiff = 0, sums = 0, or_atleastk = 0, ternary = 0, binary = 0, table = 0;
    SysInt reify = 0, lex = 0, unary = 0, nullary = 0, element = 0, minmax = 0, occurrence = 0;
    for(list<ConstraintBlob>::iterator i = c.begin(); i != c.end(); ++i) {
      classifyConstraint(*i, &alldiff, &sums, &or_atleastk, &ternary, &binary, &table, &reify, &lex,
                         &unary, &nullary, &element, &minmax, &occurrence, &alldiffdomovervars, v);
    }

    if(alldiffdomovervars.size() == 0) {
      alldiffdomovervars.push_back(0.0);
    }

    std::sort(alldiffdomovervars.begin(), alldiffdomovervars.end());
    END_CLOCK();

    output_stat << "alldiffdomovervars_0:" << alldiffdomovervars[0] << endl;
    output_stat << "alldiffdomovervars_25:" << alldiffdomovervars[alldiffdomovervars.size() / 4]
                << endl;
    output_stat << "alldiffdomovervars_50:" << alldiffdomovervars[alldiffdomovervars.size() / 2]
                << endl;
    output_stat << "alldiffdomovervars_75:"
                << alldiffdomovervars[(alldiffdomovervars.size() * 3) / 4] << endl;
    output_stat << "alldiffdomovervars_100:" << alldiffdomovervars.back() << endl;

    double alldiffdomovervarstotal =
        std::accumulate(alldiffdomovervars.begin(), alldiffdomovervars.end(), 0.0);
    output_stat << "alldiffdomovervars_mean:"
                << (alldiffdomovervarstotal) / (double)alldiffdomovervars.size() << endl;

    output_stat << "alldiffCount:" << alldiff << endl;
    output_stat << "alldiff_proportion:" << ((double)alldiff) / (double)c.size() << endl;
    output_stat << "sumsCount:" << sums << endl;
    output_stat << "sums_proportion:" << ((double)sums) / (double)c.size() << endl;
    output_stat << "or_atleastkCount:" << or_atleastk << endl;
    output_stat << "or_atleastk_proportion:" << ((double)or_atleastk) / (double)c.size() << endl;
    output_stat << "ternaryCount:" << ternary << endl;
    output_stat << "ternary_proportion:" << ((double)ternary) / (double)c.size() << endl;
    output_stat << "binaryCount:" << binary << endl;
    output_stat << "binary_proportion:" << ((double)binary) / (double)c.size() << endl;
    output_stat << "reifyCount:" << reify << endl;
    output_stat << "reify_proportion:" << ((double)reify) / (double)c.size() << endl;
    output_stat << "tableCount:" << table << endl;
    output_stat << "table_proportion:" << ((double)table) / (double)c.size() << endl;
    output_stat << "lexCount:" << lex << endl;
    output_stat << "lex_proportion:" << ((double)lex) / (double)c.size() << endl;
    output_stat << "unaryCount:" << unary << endl;
    output_stat << "unary_proportion:" << ((double)unary) / (double)c.size() << endl;
    output_stat << "nullaryCount:" << nullary << endl;
    output_stat << "nullary_proportion:" << ((double)nullary) / (double)c.size() << endl;
    output_stat << "elementCount:" << element << endl;
    output_stat << "element_proportion:" << ((double)element) / (double)c.size() << endl;
    output_stat << "minmaxCount:" << minmax << endl;
    output_stat << "minmax_proportion:" << ((double)minmax) / (double)c.size() << endl;
    output_stat << "occurrenceCount:" << occurrence << endl;
    output_stat << "occurrence_proportion:" << ((double)occurrence) / (double)c.size() << endl;

    // Count the number of pairs of constraints that overlap by two or more
    // variables.
    START_CLOCK();
    SysInt count_2_overlaps = 0;

    vector<vector<Var>> var_sets;

    for(list<ConstraintBlob>::iterator i = c.begin(); i != c.end(); ++i) {
      set<Var> v = find_allVars(*i);
      var_sets.push_back(vector<Var>(v.begin(), v.end()));
    }

    vector<Var> inter;
    for(SysInt i = 0; i < (SysInt)var_sets.size(); ++i) {
      for(SysInt j = i + 1; j < (SysInt)var_sets.size(); ++j) {
        inter.clear();

        if(!(!var_sets[i].empty() && !var_sets[j].empty() &&
             ((var_sets[i].back() < var_sets[j].front()) ||
              (var_sets[j].back() < var_sets[i].front()))))
          set_intersection(var_sets[i].begin(), var_sets[i].end(), var_sets[j].begin(),
                           var_sets[j].end(), back_inserter(inter));
        if(inter.size() >= 2) {
          count_2_overlaps++;
        }
      }
    }

    SysInt conspairs = ((double)(c.size() * (c.size() - 1))) / 2.0;
    END_CLOCK();
    double proportion = 0;
    if(conspairs > 0)
      proportion = ((double)count_2_overlaps) / conspairs;
    // proportion of pairs of constraints that share two or more variables.
    output_stat << "multi_sharedVars:" << proportion << endl;

    START_CLOCK();

    // Edge density of primal graph
    std::set<pair<Var, Var>> seen_pairs;
    SysInt count_pairs = 0;
    for(SysInt i = 0; i < (SysInt)var_sets.size(); ++i) {
      SysInt size = var_sets[i].size();
      for(SysInt j = 0; j < (SysInt)size; ++j) {
        for(SysInt k = j + 1; k < size; ++k) {
          Var t1 = var_sets[i][j];
          Var t2 = var_sets[i][k];
          std::set<pair<Var, Var>>::const_iterator it = seen_pairs.find(make_pair(t1, t2));
          if(it == seen_pairs.end()) {
            seen_pairs.insert(make_pair(t1, t2));
            count_pairs++;
          }
        }
      }
    }
    END_CLOCK();
    output_stat << "edge_density:"
                << ((double)count_pairs) / (((double)(varcount * (varcount - 1))) / 2.0) << endl;

    START_CLOCK();
    GraphBuilder graph(csp);
    double p = partition_graph(graph.g.build_graph_info(csp, false));
    END_CLOCK();
    output_stat << "Local_Variance:" << p << endl;
  }

  void output_stats_tightness(vector<AbstractConstraint*> cons) {
    double startTime, measuredTime;

    string s("stats_");
    {
      START_CLOCK();
      vector<DomainInt> tightness;
      for(SysInt i = 0; i < (SysInt)cons.size(); i++) {
        tightness.push_back(cons[i]->getTightnessEstimate());
      }
      std::sort(tightness.begin(), tightness.end());
      END_CLOCK();
      output_stat << "tightness_0:" << tightness[0] << endl;
      output_stat << "tightness_25:" << tightness[tightness.size() / 4] << endl;
      output_stat << "tightness_50:" << tightness[tightness.size() / 2] << endl;
      output_stat << "tightness_75:" << tightness[(tightness.size() * 3) / 4] << endl;
      output_stat << "tightness_100:" << tightness.back() << endl;

      const SysInt totaltightness =
          checked_cast<SysInt>(std::accumulate(tightness.begin(), tightness.end(), (DomainInt)0));
      output_stat << "tightness_mean:" << ((double)totaltightness) / (double)tightness.size()
                  << endl;
    }
    START_CLOCK();
    // now literal tightness
    map<pair<Var, DomainInt>, vector<DomainInt>> scoresForVarVal;
    // all available tightnesses for varvals
    // iterate over constraints, collecting all available tightnesses for
    // varvals involved in con
    for(SysInt con = 0; con < (SysInt)cons.size(); con++) {
      vector<AnyVarRef>& allVars = *cons[con]->getVarsSingleton();
      for(size_t var = 0; var < allVars.size(); var++) {
        Var vv = allVars[var].getBaseVar();
        for(DomainInt val = allVars[var].initialMin(); val <= allVars[var].initialMax();
            val++) {
          scoresForVarVal[make_pair(vv, val)].push_back(
              cons[con]->getTightnessEstimateVarVal(var, val));
        }
      }
    }
    // now average the tightnesses
    vector<double> lit_tightness;
    for(map<pair<Var, DomainInt>, vector<DomainInt>>::iterator curr = scoresForVarVal.begin();
        curr != scoresForVarVal.end(); curr++) {
      vector<DomainInt> nums = curr->second;
      lit_tightness.push_back(
          (double)checked_cast<SysInt>(std::accumulate(nums.begin(), nums.end(), (DomainInt)0)) /
          (double)nums.size()); // mean literal tightness
    }

    std::sort(lit_tightness.begin(), lit_tightness.end());
    END_CLOCK();
    output_stat << "literal_tightness_0:" << lit_tightness[0] << endl;
    output_stat << "literal_tightness_25:" << lit_tightness[lit_tightness.size() / 4] << endl;
    output_stat << "literal_tightness_50:" << lit_tightness[lit_tightness.size() / 2] << endl;
    output_stat << "literal_tightness_75:" << lit_tightness[(lit_tightness.size() * 3) / 4] << endl;
    output_stat << "literal_tightness_100:" << lit_tightness.back() << endl;

    double newTotaltightness = std::accumulate(lit_tightness.begin(), lit_tightness.end(), 0.0);
    double lt_mean = (double)newTotaltightness / (double)lit_tightness.size();
    output_stat << "literal_tightness_mean:" << lt_mean << endl;
    // coefficient of variation
    double st_dev = 0;
    for(size_t i = 0; i < lit_tightness.size(); i++) {
      st_dev += pow((double)lit_tightness[i] - lt_mean, 2);
    }
    st_dev = sqrt(st_dev / lit_tightness.size());
    output_stat << "literal_coeffOf_variation:" << st_dev / lt_mean << endl;
  }

  SysInt arity(ConstraintBlob& ct) {
    return find_allVars(ct).size();
  }

  set<Var> find_allVars(ConstraintBlob& ct) {
    set<Var> t2;
    for(SysInt i = 0; i < (SysInt)ct.vars.size(); ++i) {
      for(SysInt j = 0; j < (SysInt)ct.vars[i].size(); j++) {
        t2.insert(ct.vars[i][j]);
      }
    }

    for(SysInt i = 0; i < (SysInt)ct.internal_constraints.size(); i++) {
      set<Var> t3 = find_allVars(ct.internal_constraints[i]);
      for(set<Var>::iterator j = t3.begin(); j != t3.end(); ++j) {
        t2.insert(*j);
      }
    }

    // filter out constants here.
    for(set<Var>::iterator i = t2.begin(); i != t2.end();) {
      if((*i).type() == VAR_CONSTANT) {
        set<Var>::iterator prev = i;
        i++;
        t2.erase(prev);
      } else {
        i++;
      }
    }

    return t2;
  }
};
