/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id$
*/

#ifndef _MINIONINPUTREADER_H
#define _MINIONINPUTREADER_H

#include "system/system.h"

#include "CSPSpec.h"


template<typename StreamType>
struct ConcreteFileReader
{
  StreamType& infile;
  string filename;
  
  /// Removes all comments after the current place in the file.
  // Returns peeked char.
  void check_for_comments()
  {
    char peek = simplepeek_char();
    while(peek == '#')
    {
      simplegetline();
      peek = simplepeek_char();
    }
  }
   
  ConcreteFileReader(StreamType& name, string _filename) : infile(name), filename(_filename)
  {}
  

  
   BOOL failed_open()
  { return !infile; }
  
  
   string get_string()
  { 
	  string s;
    char next_char = get_char();
    while(isalnum(next_char) || next_char == '_')
    {
      s += next_char;
      next_char = infile.get();
    }
    
    putback(next_char);
    return s;
  }
  
   void check_string(const string& string_in)
  {
    string s = get_string();
    if(s != string_in)
    { throw parse_exception("Expected " + string_in + ", recieved '" + s + "'"); }
  }
  
   string get_asciistring()
  {
    string s;
    char next_char = get_char();
    while(!isspace(next_char) && !infile.eof())
    {
      s +=  next_char;
      next_char = infile.get();
    }
    
    putback(next_char);
    return s;
  }
  
   int read_num()
  {	
	  int i;
	  infile >> i;
	  if(infile.fail())
      throw parse_exception("Problem parsing number");
	  return i;
	
/*
	// This function should just be "infile >> i;", however that is parsed differently in windows and linux
	// So we'll have to do it manually.

	char next_char = get_char();
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
	*/
  }
  
   char simplepeek_char()
  {
    char peek = infile.peek();
    while(isspace(peek))
    {
      infile.get();
      peek = infile.peek();
    }
    return peek;
  }
  
   char peek_char()
  {
    char peek = simplepeek_char();
    while(peek == '#')
    {
      simplegetline();
      peek = simplepeek_char();
    }
    return peek;
  }

  /// Check if the next character from infile is sym.
  void check_sym(char sym)
  {
    char idChar = get_char();
    if(idChar != sym)
    {
      throw parse_exception(string("Expected '") + sym + "'. Recieved '" + idChar + "'.");
    }
  }
  
   string getline()
  {
    check_for_comments();
    return simplegetline();
  }
  
   string simplegetline()
  {
	char buf[10000];
	infile.getline(buf,10000);
	char* buf_start = buf;
	while(buf_start < buf + 10000 && isspace(*buf_start))
	  buf_start++;
	
	return string(buf_start);
  }
  
  /// Cleans rubbish off start of string.
   void clean_string(string& s)
  {
	while(!s.empty() && isspace(s[0]))
	  s.erase(s.begin());
  }
  
   string getline(char deliminator)
  {
    check_for_comments();
	char buf[10000];
	infile.getline(buf,10000, deliminator);

	char* buf_start = buf;
	while(buf_start < buf + 10000 && isspace(*buf_start))
	  buf_start++;
	
	return string(buf_start);
  }
  
   char get_char()
  { 
    char peek = simpleget_char();
    while(peek == '#')
    {
      simplegetline();
      peek = simpleget_char();
    }
    return peek;
  }
  
   char simpleget_char()
  {
    char k;
    infile >> k;
    return k;
  }
  
  
   BOOL eof()
  { return infile.eof(); }
  
   void putback(char c)
  { infile.putback(c); }
  
   ~ConcreteFileReader() {}
};

template<typename FileReader>
class MinionInputReader {
  MinionInputReader(MinionInputReader&);
  void parser_info(string);
  vector< vector<Var> > Vectors ;
  vector< vector<vector<Var> > > Matrices ;
  vector< vector<vector<vector<Var> > > > Tensors ;
  vector<Var> flatten(char type, int index) ;
  vector<Var> getColOfMatrix(vector<vector<Var> >& m, int c) ;
  vector<Var> getRowThroughTensor(vector<vector<vector <Var> > >& t,int r,int c) ;
  BOOL readConstraint(FileReader* infile, BOOL reified) ;
  void readConstraintElement(FileReader* infile, ConstraintDef*) ;
  void readConstraintTable(FileReader* infile, ConstraintDef*) ;
  Var readIdentifier(FileReader* infile) ;
  vector<Var> readPossibleMatrixIdentifier(FileReader* infile);
  vector< vector<Var> > readLiteralMatrix(FileReader* infile) ;
  vector<Var> readLiteralVector(FileReader* infile) ;
  vector<int> readConstantVector(FileReader* infile, char start, char end, bool = false);
  vector<int> readRange(FileReader* infile);
  void readObjective(FileReader* infile) ;
  void readTuples(FileReader* infile) ;
  void readMatrices(FileReader* infile) ;
  void readValOrder(FileReader* infile) ;
  void readVarOrder(FileReader* infile) ;
  void readPrint(FileReader* infile) ;
  void readVars(FileReader* infile) ;
  void readSearch(FileReader* infile) ;
  vector<Var> readVectorExpression(FileReader* infile) ;
  
  void readGeneralConstraint(FileReader*, ConstraintDef*) ;
  //FileReader* infile ;
 public:
  void read(FileReader* infile) ;
  ProbSpec::CSPInstance instance ;
  
  BOOL parser_verbose ;
  
  MinionInputReader(bool _parser_verbose) : parser_verbose(_parser_verbose)
  {}
};

template<typename FileReader>
class MinionThreeInputReader {
  void parser_info(string);
  vector< vector<Var> > Vectors ;
  vector< vector<vector<Var> > > Matrices ;
  vector< vector<vector<vector<Var> > > > Tensors ;
  vector<Var> flatten(char type, int index) ;
  vector<Var> getColOfMatrix(vector<vector<Var> >& m, int c) ;
  vector<Var> getRowThroughTensor(vector<vector<vector <Var> > >& t,int r,int c) ;
  ConstraintBlob readConstraint(FileReader* infile, BOOL reified = false) ;
  void readGadget(FileReader* infile) ;
  //void readConstraintElement(FileReader* infile, ConstraintDef*) ;
//  ConstraintBlob readConstraintTable(FileReader* infile, ConstraintDef*) ;
  TupleList* readConstraintTupleList(FileReader* infile);
  ConstraintBlob readConstraintGadget(FileReader* infile);
  ConstraintBlob readConstraintOr(FileReader* infile, ConstraintDef*);
  Var readIdentifier(FileReader* infile) ;
  vector<Var> readPossibleMatrixIdentifier(FileReader* infile, bool mustBeMatrix = false);
  vector< vector<Var> > readLiteralMatrix(FileReader* infile) ;
  vector<Var> readLiteralVector(FileReader* infile) ;
  vector<int> readConstantVector(FileReader* infile, char start = '[', char end = ']', bool = false);
  vector<int> readRange(FileReader* infile);
  void readObjective(FileReader* infile) ;
  void readTuples(FileReader* infile) ;
  void readMatrices(FileReader* infile) ;
  void readValOrder(FileReader* infile) ;
  void readVarOrder(FileReader* infile) ;
  void readPrint(FileReader* infile) ;
  void readVars(FileReader* infile) ;
  void readSearch(FileReader* infile) ;
  vector<vector<Var> > read2DMatrix(FileReader* infile); 
  vector<vector<Var> > read2DMatrixVariable(FileReader* infile);
  void readAliasMatrix(FileReader* infile, const vector<int>& max_indices, vector<int> indices, string name);
  vector<Var> readVectorExpression(FileReader* infile) ;
  ConstraintBlob readGeneralConstraint(FileReader*, ConstraintDef*) ;
  vector<ConstraintBlob> readConstraintList(FileReader* infile);
public:
  void read(FileReader* infile) ;

  ProbSpec::CSPInstance instance ;
  bool parser_verbose;
  bool print_all_vars;
  
  bool isGadgetReader_m;
  
  void setGadgetReader()
  { isGadgetReader_m = true; }
  bool isGadgetReader()
  { return isGadgetReader_m; }
  
  MinionThreeInputReader(bool _parser_verbose) : parser_verbose(_parser_verbose), print_all_vars(true),
    isGadgetReader_m(false)
  {}
};

#endif
