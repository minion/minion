/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id$
*/



struct InputFileReader
{  
  virtual BOOL failed_open() = 0;
  virtual string get_string() = 0;
    virtual string get_asciistring() = 0;
  virtual int read_num() = 0;
  virtual char peek_char() = 0;
  virtual char simplepeek_char() = 0;
  
  /// Check if the next character from @infile is @sym.
  virtual void check_sym(char sym) = 0;

  void check_for_comments()
  {
    char peek = simplepeek_char();
	  while(peek == '#')
      {
	    simplegetline();
        peek = simplepeek_char();
      }
  }
  
  /// Cleans rubbish off start of string.
  virtual void clean_string(string& s) = 0;
  virtual string getline() = 0;  
  virtual string simplegetline() = 0;
  virtual string getline(char deliminator) = 0;
  virtual char get_char() = 0;
  virtual char simpleget_char() = 0;
  virtual BOOL eof() = 0;
  virtual void putback(char c) = 0;
  
  virtual ~InputFileReader() {}
};

template<typename StreamType>
struct ConcreteFileReader : public InputFileReader
{
  StreamType infile;
  
  template<typename InputType>
  ConcreteFileReader(InputType& name) : infile(name)
  {}
  
  virtual BOOL failed_open()
  { return !infile; }
  
  
  virtual string get_string()
  { 
	check_for_comments();
	string s;
    char next_char = simpleget_char();
    while(isalnum(next_char))
    {
      s += next_char;
      next_char = infile.get();
    }
    
    putback(next_char);
    return s;
  }
  
  virtual string get_asciistring()
  {
	check_for_comments();
	string s;
    char next_char = simpleget_char();
    while(!isspace(next_char) && !infile.eof())
    {
      s +=  next_char;
      next_char = infile.get();
    }
    
    putback(next_char);
    return s;
    
    
  }
  
  virtual int read_num()
  {
	check_for_comments();
	// This function should just be "infile >> i;", however that is parsed differently in windows and linux
	// So we'll have to do it manually.
	
	/* int i;
	infile >> i;
	if(infile.fail())
    throw parse_exception("Problem parsing number");
	return i;
	*/
	char next_char = infile.get();
	while(isspace(next_char))
	  next_char = infile.get();
	
	bool negative = false;
	
	if(next_char == '-')
	{
	  negative = true;
	  next_char = infile.get();
	}
	
	if(!(next_char >= '0' && next_char <= '9') )
	  throw parse_exception("Problem parsing number");  
	
	BigInt num=0;
	
	while( (next_char >= '0' && next_char <= '9'))
	{
	  num *= 10;
	  num += (next_char - '0');
	  if(num > DomainInt_Max)
		parse_exception("Number out of bounds");
	  next_char = infile.get();
	}
	
	infile.putback(next_char);
    if(negative)
	  num*= -1;
	
	return num;
  }
  
  virtual char simplepeek_char()
  {
	while(infile.peek() == ' ' || infile.peek() == '\n' || 
		  infile.peek() == '\r')
	  infile.get();
	
	return infile.peek();
  }
  
  virtual char peek_char()
  {
    check_for_comments();
    return simplepeek_char();
  }
  
  /// Check if the next character from @infile is @sym.
  virtual void check_sym(char sym)
  {
    check_for_comments();
	char idChar;
	infile >> idChar ;
	if(idChar != sym)
	{
	  throw parse_exception(string("Expected '") + sym + "'. Recieved '" + idChar + "'.");
	}
  }
  
  virtual string getline()
  {
    check_for_comments();
    return simplegetline();
  }
  
  virtual string simplegetline()
  {
	char buf[10000];
	infile.getline(buf,10000);
	char* buf_start = buf;
	while(buf_start < buf + 10000 && isspace(*buf_start))
	  buf_start++;
	
	return string(buf_start);
  }
  
  /// Cleans rubbish off start of string.
  virtual void clean_string(string& s)
  {
	while(!s.empty() && isspace(s[0]))
	  s.erase(s.begin());
  }
  
  virtual string getline(char deliminator)
  {
    check_for_comments();
	char buf[10000];
	infile.getline(buf,10000, deliminator);

	char* buf_start = buf;
	while(buf_start < buf + 10000 && isspace(*buf_start))
	  buf_start++;
	
	return string(buf_start);
  }
  
  virtual char get_char()
  { 
    check_for_comments();
	return simpleget_char();
  }
  
  virtual char simpleget_char()
  {
    char k;
    infile >> k;
    return k;
  }
  
  
  virtual BOOL eof()
  { return infile.eof(); }
  
  virtual void putback(char c)
  { infile.putback(c); }
  
  virtual ~ConcreteFileReader() {}
};

class MinionInputReader {
  void parser_info(string);
  vector< vector<Var> > Vectors ;
  vector< vector<vector<Var> > > Matrices ;
  vector< vector<vector<vector<Var> > > > Tensors ;
  vector<Var> flatten(char type, int index) ;
  vector<Var> getColOfMatrix(vector<vector<Var> >& m, int c) ;
  vector<Var> getRowThroughTensor(vector<vector<vector <Var> > >& t,int r,int c) ;
  BOOL readConstraint(InputFileReader* infile, BOOL reified) ;
  void readConstraintElement(InputFileReader* infile, const ConstraintDef&) ;
  void readConstraintTable(InputFileReader* infile, const ConstraintDef&) ;
  Var readIdentifier(InputFileReader* infile) ;
  vector<Var> readPossibleMatrixIdentifier(InputFileReader* infile);
  vector< vector<Var> > readLiteralMatrix(InputFileReader* infile) ;
  vector<Var> readLiteralVector(InputFileReader* infile) ;
  vector<int> readConstantVector(InputFileReader* infile, char start, char end, bool = false);
  vector<int> readRange(InputFileReader* infile);
  void readObjective(InputFileReader* infile) ;
  void readTuples(InputFileReader* infile) ;
  void readMatrices(InputFileReader* infile) ;
  void readValOrder(InputFileReader* infile) ;
  void readVarOrder(InputFileReader* infile) ;
  void readPrint(InputFileReader* infile) ;
  void readVars(InputFileReader* infile) ;
  void readSearch(InputFileReader* infile) ;
  vector<Var> readVectorExpression(InputFileReader* infile) ;
  
  void readGeneralConstraint(InputFileReader*, const ConstraintDef&) ;
  //InputFileReader* infile ;
 public:
  void read(InputFileReader* infile) ;
  ProbSpec::CSPInstance instance ;
  BOOL parser_verbose ;
  
  MinionInputReader() : parser_verbose(false)
  {}
};

class MinionThreeInputReader {
  void parser_info(string);
  vector< vector<Var> > Vectors ;
  vector< vector<vector<Var> > > Matrices ;
  vector< vector<vector<vector<Var> > > > Tensors ;
  vector<Var> flatten(char type, int index) ;
  vector<Var> getColOfMatrix(vector<vector<Var> >& m, int c) ;
  vector<Var> getRowThroughTensor(vector<vector<vector <Var> > >& t,int r,int c) ;
  BOOL readConstraint(InputFileReader* infile, BOOL reified) ;
  void readConstraintElement(InputFileReader* infile, const ConstraintDef&) ;
  void readConstraintTable(InputFileReader* infile, const ConstraintDef&) ;
  Var readIdentifier(InputFileReader* infile) ;
  vector<Var> readPossibleMatrixIdentifier(InputFileReader* infile);
  vector< vector<Var> > readLiteralMatrix(InputFileReader* infile) ;
  vector<Var> readLiteralVector(InputFileReader* infile) ;
  vector<int> readConstantVector(InputFileReader* infile, char start, char end, bool = false);
  vector<int> readRange(InputFileReader* infile);
  void readObjective(InputFileReader* infile) ;
  void readTuples(InputFileReader* infile) ;
  void readMatrices(InputFileReader* infile) ;
  void readValOrder(InputFileReader* infile) ;
  void readVarOrder(InputFileReader* infile) ;
  void readPrint(InputFileReader* infile) ;
  void readVars(InputFileReader* infile) ;
  void readSearch(InputFileReader* infile) ;
  vector<Var> readVectorExpression(InputFileReader* infile) ;
  
  void readGeneralConstraint(InputFileReader*, const ConstraintDef&) ;
  //InputFileReader* infile ;
public:
  void read(InputFileReader* infile) ;
  ProbSpec::CSPInstance instance ;
  BOOL parser_verbose ;
  
  bool print_all_vars;
  
  MinionThreeInputReader() : parser_verbose(false), print_all_vars(true)
  {}
};

