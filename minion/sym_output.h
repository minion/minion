


struct GraphBuilder
{
  CSPInstance& csp;
  set<pair<string, string> > graph;
  // Matches colour to the vertices with that colour
  map<string, set<string> > vertex_colour;
  
  int free_vertices;
  
  string new_vertex()
  {
    free_vertices++;
    return "F." + to_string(free_vertices);
  }
  
  string new_vertex(string colour)
  {
    free_vertices++;
    string s =  "F." + colour + "." + to_string(free_vertices);
    vertex_colour[colour].insert(s);
    return s;
  }
  
  void output_graph()
  {
    for(map<string, set<string> >::iterator it = vertex_colour.begin();
    it != vertex_colour.end();
    ++it)
    {
      cout << it->first << " : ";
      for(set<string>::iterator it2 = it->second.begin(); it2 != it->second.end(); it2++)
        cout << *it2 << " ";
      cout << endl;
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
      vertex_colour[to_string(csp.vars.get_domain_for_graph(vars[i]))].insert(csp.vars.getName(vars[i]));
    }
  }
  
  string name(Var v)
  { 
    if(v.type() == VAR_CONSTANT)
      return "CONSTANT_" + to_string(v.pos());
    else
      return csp.vars.getName(v); 
  }
  
  void add_edge(string s1, string s2)
  { graph.insert(make_pair(s1, s2)); }
  
  void add_edge(Var v1, string s2)
  { graph.insert(make_pair(name(v1), s2)); }
  
  void add_edge(string s1, Var v2)
  { graph.insert(make_pair(s1, name(v2))); }
  
  string colour_element(const ConstraintBlob& b, string name)
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
     
     return v;
  }
  
  // A constraint where each array is independantly symmetric
  string colour_symmetric_constraint(const ConstraintBlob& b, string name)
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
    
    return v;
  }
  
  string colour_eq(const ConstraintBlob& b, string name)
  {
    string v = new_vertex(name + "_MASTER");
    add_edge(v, b.vars[0][0]);
    add_edge(v, b.vars[1][0]);
    return v;
  }
  
  string colour_no_symmetry(const ConstraintBlob& b, string name)
  {
    string v = new_vertex(name + "_MASTER");
    
    for(int i = 0; i < b.vars.size(); ++i)
      for(int j = 0; j < b.vars[i].size(); ++j)
      {
        string vij = new_vertex(name + "_CHILD_" + to_string(i) + ";" + to_string(j));
        add_edge(v, vij);
        add_edge(vij, b.vars[i][j]);
      }
      
    return v;
  }
  
  // Symmetries where the first array can be permuted, if the same permutation
  // is applied to the second array.
  string colour_array_swap_each_index(const ConstraintBlob& b, string name)
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
    
    return v;
  }
  
  string colour_symmetric_indexes(const ConstraintBlob& b, string name)
    { return "X"; }
  
  string colour_constraint(const ConstraintBlob& b)
  {
    switch(b.constraint->type)
    {
      case CT_ELEMENT:
      case CT_WATCHED_ELEMENT:
      case CT_GACELEMENT:
        return colour_element(b, "ELEMENT");
      case CT_ELEMENT_ONE:
      case CT_WATCHED_ELEMENT_ONE:
        return colour_element(b, "ELEMENT_ONE");
      case CT_ALLDIFF:
      case CT_GACALLDIFF:
        return colour_symmetric_constraint(b, "ALLDIFF");
        
      case CT_WATCHED_NEQ:
      case CT_DISEQ: return colour_eq(b, "DISEQ");
      case CT_EQ:    return colour_eq(b, "EQ");
      
      case CT_MINUSEQ: return colour_no_symmetry(b, "MINUSEQ");
      case CT_ABS:     return colour_no_symmetry(b, "ABS");
      case CT_INEQ:    return colour_no_symmetry(b, "INEQ");
      case CT_WATCHED_LESS: return colour_no_symmetry(b, "LESS");
      
      case CT_LEXLEQ: return colour_no_symmetry(b, "LEXLEQ");
      case CT_LEXLESS: return colour_no_symmetry(b, "LEXLESS");
      
      case CT_MAX: return colour_symmetric_constraint(b, "MAX");
      case CT_MIN: return colour_symmetric_constraint(b, "MIN");
      
      case CT_OCCURRENCE: return colour_symmetric_constraint(b, "OCCURRENCE");
      case CT_LEQ_OCCURRENCE: return colour_symmetric_constraint(b, "OCC_LEQ");
      case CT_GEQ_OCCURRENCE: return colour_symmetric_constraint(b, "OCC_GEQ");
      
      case CT_PRODUCT2: return colour_symmetric_constraint(b, "PRODUCT");
      
      case CT_DIFFERENCE: return colour_symmetric_constraint(b, "DIFFERENCE");
      
      case CT_WEIGHTGEQSUM: return colour_symmetric_indexes(b, "WEIGHT_GEQSUM");
      case CT_WEIGHTLEQSUM: return colour_symmetric_indexes(b, "WEIGHT_LEQSUM");
      
      case CT_GEQSUM:
      case CT_WATCHED_GEQSUM: return colour_symmetric_constraint(b, "GEQSUM");
      
      case CT_LEQSUM:
      case CT_WATCHED_LEQSUM: return colour_symmetric_constraint(b, "LEQSUM");
      
      case CT_WATCHED_TABLE: return colour_no_symmetry(b, "TABLE");
      case CT_WATCHED_NEGATIVE_TABLE: return colour_no_symmetry(b, "NEG_TABLE");
      
      case CT_WATCHED_VECNEQ: return colour_array_swap_each_index(b, "VECNEQ");
      case CT_WATCHED_LITSUM: return colour_no_symmetry(b, "LITSUM");
      
      case CT_POW: return colour_no_symmetry(b, "POW");
      case CT_DIV: return colour_no_symmetry(b, "DIV");
      case CT_MODULO: return colour_no_symmetry(b, "MOD");
      
      case CT_WATCHED_VEC_OR_AND: return colour_array_swap_each_index(b, "VEC_OR_AND");
      
      case CT_WATCHED_VEC_OR_LESS: return colour_symmetric_indexes(b, "VEC_OR_LESS");
      case CT_WATCHED_HAMMING: return colour_array_swap_each_index(b, "HAMMING");
      
      default:
        abort();
    }
    
  }
  
};


