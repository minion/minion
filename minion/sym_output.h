


struct GraphBuilder
{
  CSPInstance& csp;
  set<pair<string, string> > graph;
  map<string, string> vertex_colour;
  
  int free_vertices;
  
  string new_vertex()
  {
    free_vertices++;
    return "free" + to_string(free_vertices);
  }
  
  string new_vertex(string name)
  {
    string s = new_vertex();
    vertex_colour[s] = name;
  }
  
  void output_graph()
  {
    for(map<string, string>::iterator it = vertex_colour.begin();
    it != vertex_colour.end();
    ++it)
    {
      cout << it->first << ":" << it->second << endl;
    }
    
    cout << endl;
    
    for(set<pair<string, string> >::iterator it = graph.begin();
    it != graph.end();
    ++it)
    {
      cout << it->first << ", " << it->second << endl;
    }
  }
  
  GraphBuilder(CSPInstance& _csp) : csp(_csp), free_vertices(0)
  { 
    csp.add_variable_names();
    build_graph(); 
  }
  
  
  void build_graph()
  {
    colour_vertices();
    for(list<ConstraintBlob>::iterator it = csp.constraints.begin(); 
        it != csp.constraints.end(); ++it)
      colour_constraint(*it);
  }
  
  void colour_vertices()
  {
    vector<Var> vars = csp.vars.get_all_vars();
    for(int i = 0; i < vars.size(); ++i)
    {
      vertex_colour[ csp.vars.getName(vars[i]) ] =
        to_string(csp.vars.get_domain_for_graph(vars[i]));
    }
  }
  
  string name(Var v)
  { return csp.vars.getName(v); }
  
  void add_edge(string s1, string s2)
  { graph.insert(make_pair(s1, s2)); }
  
  void add_edge(Var v1, string s2)
  { graph.insert(make_pair(name(v1), s2)); }
  
  void add_edge(string s1, Var v2)
  { graph.insert(make_pair(s1, name(v2))); }
  
  void colour_element(const ConstraintBlob& b, string name)
  {
     string v = new_vertex(name + "_MASTER");
     for(int i = 0; i < b.vars[0].size(); ++i)
     {
       string t = new_vertex(name + "_CHILD_" + to_string(i));
       add_edge(v,t);
       add_edge(t, b.vars[0][i]);
     }
     
     string v_index = new_vertex(name + "_INDEX");
     add_edge(v, v_index);
     add_edge(v_index, b.vars[1][0]);
     
     string v_result = new_vertex(name + "_RESULT");
     add_edge(v, v_result);
     add_edge(v_result, b.vars[2][0]);
  }
  
  // A constraint where each array is independantly symmetric
  void colour_symmetric_constraint(const ConstraintBlob& b, string name)
  {
    string v = new_vertex(name + "_MASTER");  
    
    // Connect these directly to root, because there is likely to be most of them.
    for(int i = 0; i < b.vars[0].size(); ++i)
      add_edge(v, b.vars[0][i]);
      
    for(int i = 1; i < b.vars.size(); ++i)
    {
      string nv = new_vertex(name + "_CHILD" + to_string(i));
      add_edge(v, nv);
      for(int j = 0; j < b.vars[i].size(); ++j)
        add_edge(nv, b.vars[i][j]);
    }
    
  }
  
  void colour_eq(const ConstraintBlob& b, string name)
  {
    string v = new_vertex(name + "_MASTER");
    add_edge(v, b.vars[0][0]);
    add_edge(v, b.vars[1][0]);
  }
  
  void colour_no_symmetry(const ConstraintBlob& b, string name)
  {
    string v = new_vertex(name + "_MASTER");
    
    for(int i = 0; i < b.vars.size(); ++i)
      for(int j = 0; j < b.vars[i].size(); ++j)
      {
        string vij = new_vertex(name + "_CHILD_" + to_string(i) + ":" + to_string(j));
        add_edge(v, vij);
        add_edge(vij, b.vars[i][j]);
      }
  }
  
  // Symmetries where the first array can be permuted, if the same permutation
  // is applied to the second array.
  void colour_array_swap_each_index(const ConstraintBlob& b, string name)
  {
    string v = new_vertex(name + "_MASTER");
    
    D_ASSERT(b.vars[0].size() == b.vars[1].size());
    
    // Force each array to stay together.
    for(int i = 0; i < 2; ++i)
    {
      string vi = new_vertex(name + "_ARRAY_STAY_TOGETHER");
      add_edge(v, vi);
      for(int j = 0; j < b.vars[i].size(); ++j)
        add_edge(vi, b.vars[i][j]);
    }
    
    for(int i = 0; i < b.vars[0].size(); ++i)
    {
      string vi = new_vertex(name + "_INDEX");
      add_edge(v, vi);
      add_edge(vi, b.vars[0][i]);
      add_edge(vi, b.vars[1][i]);
    }
    
    for(int i = 2; i < b.vars.size(); ++i)
    {
      D_ASSERT(b.vars[i].size() == 1);
      string vi = new_vertex(name + "_POS_" + to_string(i));
      add_edge(v, vi);
    }
  }
  
  void colour_symmetric_indexes(const ConstraintBlob& b, string name)
  { }
  
  void colour_constraint(const ConstraintBlob& b)
  {
    switch(b.constraint->type)
    {
      case CT_ELEMENT:
      case CT_WATCHED_ELEMENT:
      case CT_GACELEMENT:
        colour_element(b, "ELEMENT");
        return;
      case CT_ELEMENT_ONE:
      case CT_WATCHED_ELEMENT_ONE:
        colour_element(b, "ELEMENT_ONE");
        return;
      case CT_ALLDIFF:
      case CT_GACALLDIFF:
        colour_symmetric_constraint(b, "ALLDIFF");
        return;
        
      case CT_WATCHED_NEQ:
      case CT_DISEQ: colour_eq(b, "DISEQ"); return;
      case CT_EQ:    colour_eq(b, "EQ"); return;
      
      case CT_MINUSEQ: colour_no_symmetry(b, "MINUSEQ"); return;
      case CT_ABS: colour_no_symmetry(b, "ABS"); return;
      case CT_INEQ: colour_no_symmetry(b, "INEQ"); return;
      case CT_WATCHED_LESS: colour_no_symmetry(b, "LESS"); return;
      
      case CT_LEXLEQ: colour_no_symmetry(b, "LEXLEQ"); return;
      case CT_LEXLESS: colour_no_symmetry(b, "LEXLESS"); return;
      
      case CT_MAX: colour_symmetric_constraint(b, "MAX"); return;
      case CT_MIN: colour_symmetric_constraint(b, "MIN"); return;
      
      case CT_OCCURRENCE: colour_symmetric_constraint(b, "OCCURRENCE"); return;
      case CT_LEQ_OCCURRENCE: colour_symmetric_constraint(b, "OCC_LEQ"); return;
      case CT_GEQ_OCCURRENCE: colour_symmetric_constraint(b, "OCC_GEQ"); return;
      
      case CT_PRODUCT2: colour_symmetric_constraint(b, "PRODUCT"); return;
      
      case CT_DIFFERENCE: colour_symmetric_constraint(b, "DIFFERENCE"); return;
      
      case CT_WEIGHTGEQSUM: colour_symmetric_indexes(b, "WEIGHT_GEQSUM"); return;
      case CT_WEIGHTLEQSUM: colour_symmetric_indexes(b, "WEIGHT_LEQSUM"); return;
      
      case CT_WATCHED_TABLE: colour_no_symmetry(b, "TABLE"); return;
      case CT_WATCHED_NEGATIVE_TABLE: colour_no_symmetry(b, "NEG_TABLE"); return;
      
      case CT_WATCHED_VECNEQ: colour_array_swap_each_index(b, "VECNEQ"); return;
      case CT_WATCHED_LITSUM: colour_no_symmetry(b, "LITSUM"); return;
      
      case CT_POW: colour_no_symmetry(b, "POW"); return;
      case CT_DIV: colour_no_symmetry(b, "DIV"); return;
      case CT_MODULO: colour_no_symmetry(b, "MOD"); return;
      
      case CT_WATCHED_VEC_OR_AND: colour_array_swap_each_index(b, "VEC_OR_AND"); return;
      
      case CT_WATCHED_VEC_OR_LESS: colour_symmetric_indexes(b, "VEC_OR_LESS"); return;
      case CT_WATCHED_HAMMING: colour_array_swap_each_index(b, "HAMMING"); return;
      
      default:
        abort();
    }
    
  }
  
};