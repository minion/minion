  
class MinionInputReader {
  void parser_info(string);
  vector< vector<Var> > Vectors ;
  vector< vector<vector<Var> > > Matrices ;
  vector< vector<vector<vector<Var> > > > Tensors ;
  vector<Var> flatten(char type, int index) ;
  vector<Var> getColOfMatrix(vector<vector<Var> >& m, int c) ;
  vector<Var> getRowThroughTensor(vector<vector<vector <Var> > >& t,int r,int c) ;
  bool readConstraint(ifstream& infile, bool reified) ;
  void readConstraintElement(ifstream& infile, const ConstraintDef&) ;
  void readConstraintTable(ifstream& infile, const ConstraintDef&) ;
  Var readIdentifier(ifstream& infile) ;
  vector< vector<Var> > readLiteralMatrix(ifstream& infile) ;
  vector<Var> readLiteralVector(ifstream& infile) ;
  void readObjective(ifstream& infile);
  void readTuples(ifstream& infile); 
  void readMatrices(ifstream& infile) ;
  void readValOrder(ifstream& infile) ;
  void readVarOrder(ifstream& infile) ;
  void readPrint(ifstream& infile) ;
  void readVars(ifstream& infile) ;
  vector<Var> readVectorExpression(ifstream& infile) ;
  
  void readGeneralConstraint(ifstream&, const ConstraintDef&);
 public:
  void read(char* fn) ;
  ProbSpec::CSPInstance instance;
  bool parser_verbose;
  
  MinionInputReader() : parser_verbose(false)
  {}
};
