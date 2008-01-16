// MinionInputReader.cpp
//
// Subversion Identity $Id$
//
// Plan here is to generate an instance of a problem (or whatever you have)
// and return that.

/// TODO: We need somewhere better to put these things.

#define NO_MAIN

#include "minion.h"
#include "CSPSpec.h"
using namespace ProbSpec;

#include "MinionInputReader.h"

int inputFileVersionNumber;

ConstraintDef constraint_list[] =
{
{ "element",  CT_ELEMENT, 2, { read_list, read_var }, STATIC_CT },
{ "watchelement", CT_WATCHED_ELEMENT, 2, { read_list, read_var }, DYNAMIC_CT },
{ "gacelement", CT_GACELEMENT, 2, { read_list, read_var }, STATIC_CT },
{ "alldiff", CT_ALLDIFF, 1, { read_list }, STATIC_CT },
{ "diseq",   CT_DISEQ,   2, { read_var, read_var }, STATIC_CT },
{ "eq",      CT_EQ,      2, { read_var, read_var }, STATIC_CT },
{ "ineq",    CT_INEQ,    3, { read_var, read_var, read_constant }, STATIC_CT },
{ "lexleq",  CT_LEXLEQ,  2, { read_list, read_list }, STATIC_CT },
{ "lexless", CT_LEXLESS, 2, { read_list, read_list }, STATIC_CT },
{ "max",     CT_MAX,     2, { read_list, read_var }, STATIC_CT },
{ "min",     CT_MIN,     2, { read_list, read_var }, STATIC_CT },
{ "occurrence", CT_OCCURRENCE, 3, { read_list, read_constant, read_constant }, STATIC_CT },
{ "product", CT_PRODUCT2, 2, {read_2_vars, read_var }, STATIC_CT },
{ "weightedsumleq", CT_WEIGHTLEQSUM, 3, { read_constant_list, read_list, read_var }, STATIC_CT },
{ "weightedsumgeq", CT_WEIGHTGEQSUM, 3, { read_constant_list, read_list, read_var }, STATIC_CT },
{ "sumgeq", CT_GEQSUM, 2, {read_list, read_var}, STATIC_CT },
{ "sumleq", CT_LEQSUM, 2, {read_list, read_var}, STATIC_CT },
{ "watchsumgeq", CT_WATCHED_GEQSUM, 2, {read_list, read_var}, DYNAMIC_CT },
{ "watchsumleq", CT_WATCHED_LEQSUM, 2, {read_list, read_var}, DYNAMIC_CT },
{ "table", CT_WATCHED_TABLE, 2, {read_list, read_tuples}, DYNAMIC_CT },
{ "watchvecneq", CT_WATCHED_VECNEQ, 2, {read_list, read_list}, DYNAMIC_CT },
{ "minuseq", CT_MINUSEQ, 2, { read_var, read_var }, STATIC_CT },
{ "reify", CT_REIFY, 0, {}, STATIC_CT },
{ "reifyimply", CT_REIFYIMPLY, 0, {}, STATIC_CT }
};

const int num_of_constraints = sizeof(constraint_list) / sizeof(ConstraintDef);

ConstraintDef& get_constraint(ConstraintType t)
{
  for(int i = 0; i < num_of_constraints; ++i)
  {
    if(constraint_list[i].type == t)
	  return constraint_list[i];
  }
  
  D_FATAL_ERROR( "Constraint not found");
}

template<typename T>
vector<T> make_vec(const T& t)
{
  vector<T> vec;
  vec.push_back(t);
  return vec;
}

template<typename T>
typename T::value_type& index(T& container, int index_pos)
{
  if(index_pos < 0 || index_pos >= (int)container.size())
    throw new parse_exception("Index position " + to_string(index_pos) + 
							  " out of range");
  return container[index_pos];
}

/// Check if the next character from @infile is @sym.
void check_sym(ifstream& infile, char sym)
{
  char idChar;
  infile >> idChar ;
  if(idChar != sym)
  {
    throw new parse_exception(string("Expected '") + sym + "'. Recieved '" + idChar + "'.");
  }
}

/// A 'better' peek, which ignores whitespace.
char peek_char(ifstream& infile)
{
  while(infile.peek() == ' ' || infile.peek() == '\n')
    infile.get();

  return infile.peek();
}

int read_num(ifstream& infile)
{
  // This function should just be "infile >> i;", however that is parsed differently in windows and linux
  // So we'll have to do it manually.
  
 /* int i;
  infile >> i;
  if(infile.fail())
    throw new parse_exception("Problem parsing number");
  return i;
  */
  string s;
  char next_char = infile.get();
  while(isspace(next_char))
	 next_char = infile.get();

  if(next_char == '-')
  {
	 s+= next_char;
	 next_char = infile.get();
  }

  while( (next_char >= '0' && next_char <= '9'))
  {
	 s += next_char;
	 next_char = infile.get();
  }

  infile.putback(next_char);
  if(infile.fail() || s == "" || s == "-")
    throw new parse_exception("Problem parsing number");
  return atoi(s.c_str());
}



void MinionInputReader::parser_info(string s)
{
  if(parser_verbose)
    cout << s << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// flatten
// type: m (2d matrix), t (3d matrix)
// Flattening is row-wise (2d), plane-wise row-wise (3d).
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
vector<Var> MinionInputReader::flatten(char type, int index) {
  unsigned int rowIndex, colIndex, planeIndex ;
  vector<Var> flattened ;
  // flatten row-wise
  if (type == 'm') {
    vector< vector<Var> > matrix = Matrices.at(index) ;
    for (rowIndex = 0; rowIndex < matrix.size() ; rowIndex++) {
      vector<Var> row = matrix.at(rowIndex) ;
      for (colIndex = 0; colIndex < row.size(); colIndex++)
        flattened.push_back(row.at(colIndex)) ;
    }
  }
  // flatten plane-wise then row-wise
  else {
    vector< vector <vector <Var> > > tensor = Tensors.at(index) ;
    for (planeIndex = 0; planeIndex < tensor.size(); planeIndex++) {
      vector< vector <Var> > plane = tensor.at(planeIndex) ;
      for (rowIndex = 0; rowIndex < plane.size(); rowIndex++) {
        vector<Var> row = plane.at(rowIndex) ;
        for (colIndex = 0; colIndex < row.size(); colIndex++)
          flattened.push_back(row.at(colIndex)) ;
      }
    }
  }
  return flattened ;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// getColOfMatrix
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
vector<Var> MinionInputReader::getColOfMatrix(vector<vector<Var> >& matrix, int colNo) {
  vector<Var> result ;
  for (unsigned int rowIndex = 0; rowIndex < matrix.size(); rowIndex++) {
	result.push_back(matrix.at(rowIndex).at(colNo)) ;
  }
  return result ;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// getRowThroughTensor
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
vector<Var> MinionInputReader::getRowThroughTensor(
												   vector< vector< vector<Var> > >& tensor, int rowNo, int colNo) {
  vector<Var> result ;
  for (unsigned int planeIndex = 0; planeIndex < tensor.size() ; planeIndex ++) {
    vector< vector<Var> >& plane = tensor.at(planeIndex) ;
    vector<Var>& row = plane.at(rowNo) ;
    result.push_back(row.at(colNo)) ;
  }
  return result ;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// read
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void MinionInputReader::read(char* fn) {
  // general purpose buffer
  char* buf = new char[10000];
  ifstream infile(fn) ;
  if (!infile) {
    D_FATAL_ERROR("Can't open given input file");
  }
  
  string test_name;
  infile >> test_name;
  if(test_name != "MINION")
  {
    D_FATAL_ERROR("Error! All Minion input files must begin 'MINION <input version num>");
  }
  
  inputFileVersionNumber = read_num(infile);
  
  if(parser_verbose)
    cout << "Input file has version num " << inputFileVersionNumber << endl;
  
  // Just swallow the rest of this line, in particular the return. Extra stuff could be added on the line
  // later without breaking this version of the parser..
  infile.getline(buf, 10000);
  
  if(inputFileVersionNumber != 1 && inputFileVersionNumber != 2)
  {
   	D_FATAL_ERROR("This version of Minion only reads files with input format 1 or 2.");
  }
  
  while(peek_char(infile) == '#')
  {
	infile.getline(buf, 10000);
	parser_info(string("Read comment line:") + buf);
  }
  
  // After this, we don't want the buffer any more.
  delete[] buf;
  
  try
  {
	readVars(infile) ;
	readVarOrder(infile) ;
	readValOrder(infile) ;
	readMatrices(infile) ;
	
	// At this point, may or may not have a tuples entry
	{
	  string s;
	  infile >> s;
	  if(s == "tuplelists")
	  {
	    readTuples(infile);
		infile >> s;
	  }
	  if(s == "objective")
	    readObjective(infile);
	  else
	  {
	    throw new parse_exception("I don't understand: " + s + ". Did you mean "
								  " tuplelists or objective?");
	  }
	}
	readPrint(infile);
	
	while(readConstraint(infile, false)) ;
  }
  catch(parse_exception* s)
  {
    cerr << "Error in input." << endl;
	cerr << s->what() << endl;
	// This nasty line will tell us the current position in the file even if a parse fail has occurred.
	int error_pos =  infile.rdbuf()->pubseekoff(0, ios_base::cur, ios_base::in);
	
	//cerr << "It was at character number:" << infile.tellg() << endl;
	int line_num = 0;
	int end_prev_line = 0;
	buf = new char[1000000];
    infile.close();
	// Open a new stream, because we don't know what kind of a mess the old one might be in.
	ifstream error_file(fn);
	while(error_pos > error_file.tellg())
	{ 
	  end_prev_line = error_file.tellg();
	  error_file.getline(buf,1000000);
	  line_num++;
	}
	cerr << "Error on line:" << line_num << ". Gave up parsing somewhere around here:" << endl;
	cerr << string(buf) << endl;
	for(int i = 0; i < (error_pos - end_prev_line); ++i)
	  cerr << "-";
	cerr << "^" << endl;
	cerr << "Sorry it didn't work out." << endl;
	// This is so we can catch things in the debugger.
    exit(1);
  }

}

/// Cleans rubbish off start of string.
void clean_string(string& s)
{
  while(!s.empty() && isspace(s[0]))
    s.erase(s.begin());
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// readConstraint
// Recognise constraint by its name, read past name and leading '('
// Return false if eof or unknown ct. Else true.
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool MinionInputReader::readConstraint(ifstream& infile, bool reified) {
  char char_id[1000];
  infile.getline(char_id, 1000, '(');
  string id(char_id);
  clean_string(id);
  
  int constraint_num = -1;
  for(int i = 0; i < num_of_constraints; ++i)
  {
	if(constraint_list[i].name == id)
	{
	  constraint_num = i;
	  break;
	}
  }
  
  if(constraint_num == -1) 
  {
	if (infile.eof()) 
	{
	  parser_info("Done.") ;
	  return false;
	}
	else
	{ throw new parse_exception(string("Unknown Constraint:") + id); }
  }
  ConstraintDef constraint = constraint_list[constraint_num];
 
  if( constraint.trig_type == DYNAMIC_CT )
  {
#ifndef WATCHEDLITERALS
	cerr << "This version of Minion was not complied with -WATCHEDLITERALS" << endl;
	cerr << "So there is not support for the " << constraint.name << "." << endl;
	exit(1);
#else
	dynamic_triggers_used = true;
	if(reified)
	{
	  cerr << "Cannot reify a watched constraint!" << endl;
	  exit(1);
	}
#endif
  }

  switch(constraint.type)
  {
	case CT_ELEMENT:
	case CT_WATCHED_ELEMENT:
	case CT_GACELEMENT:
	  readConstraintElement(infile, constraint) ;
	  break;
	case CT_REIFY:
	case CT_REIFYIMPLY:
	  { 
	  if(reified == true)
		throw new parse_exception("Can't reify a reified constraint!");
	  readConstraint(infile, true);
	  
	  check_sym(infile, ',');
	  Var reifyVar = readIdentifier(infile);
	  check_sym(infile, ')');
	  if(constraint.type == CT_REIFY)
	    instance.last_constraint_reify(reifyVar);
	  else
	    instance.last_constraint_reifyimply(reifyVar);
	  }
	  break;

	case CT_WATCHED_TABLE:
	  readConstraintTable(infile, get_constraint(CT_WATCHED_TABLE));
	  break;

	default:
	  readGeneralConstraint(infile, constraint);
  }
  return true ;
}


void MinionInputReader::readGeneralConstraint(ifstream& infile, const ConstraintDef& def)
{
  vector<vector<Var> > varsblob;
  for(int i = 0; i < def.number_of_params; ++i)
  {
    switch(def.read_types[i])
	{
	  case read_list:
	    varsblob.push_back(readVectorExpression(infile));
		break;
	  case read_var:
	    varsblob.push_back(make_vec(readIdentifier(infile)));
		break;
	  case read_2_vars:
	  {
	    vector<Var> vars;
	    vars.push_back(readIdentifier(infile));
	    check_sym(infile,',');
	    vars.push_back(readIdentifier(infile));
            varsblob.push_back(vars);
	  }
		break;
	  case read_constant:
	    varsblob.push_back(make_vec(readIdentifier(infile)));
		if(varsblob.back().back().type != VAR_CONSTANT)
		  throw new parse_exception("Expected constant but got variable.");
		break;
	  case read_constant_list:
	  {
		vector<Var> vectorOfConst ;
		vectorOfConst = readVectorExpression(infile) ;
		for(unsigned int loop = 0; loop < vectorOfConst.size(); ++loop)
		{
		  if(vectorOfConst[loop].type != VAR_CONSTANT)
			throw new parse_exception("Vector must only contain constants.");
		}
		varsblob.push_back(vectorOfConst);
	  }
		break;  
	  default:
	    D_FATAL_ERROR("Internal Error!");
	}
	if(i != def.number_of_params - 1)
	  check_sym(infile, ',');
  }
  check_sym(infile, ')');
  
  instance.add_constraint(ConstraintBlob(def, varsblob));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// readConstraintElement
// element(vectorofvars, indexvar, var)
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void MinionInputReader::readConstraintElement(ifstream& infile, const ConstraintDef& ctype) {
  parser_info("reading an element ct. " ) ;
  vector<vector<Var> > vars;
  // vectorofvars
  vars.push_back(readVectorExpression(infile));
  check_sym(infile, ',');
  // indexvar
  vars.push_back(make_vec(readIdentifier(infile)));
  check_sym(infile, ',');
  // The final var is shoved on the end of the vector of vars as it should
  // be of a similar type.
  // final var
  vars[0].push_back(readIdentifier(infile));
  check_sym(infile, ')');
  instance.add_constraint(ConstraintBlob(ctype, vars));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// readConstraintTable
// table(<vectorOfVars>, {<tuple> [, <tuple>]})
// Tuples represented as a vector of int arrays.
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void MinionInputReader::readConstraintTable(ifstream& infile, const ConstraintDef& def) 
{
  parser_info( "reading a table ct (unreifiable)" ) ;
  
  char delim = ' ';
  int count, elem ;
  vector<Var> vectorOfVars = readVectorExpression(infile) ;
  int tupleSize = vectorOfVars.size() ;
  
  check_sym(infile,',');
  
  char next_char = peek_char(infile);
  if(next_char != 't' && next_char != '{')
	throw new parse_exception("Expected either 't' or a tuple list");
  
  TupleList* tuplelist;
  
  if(next_char == 't')
  {
	check_sym(infile,'t');
	int tuple_num = read_num(infile);
	if(tuple_num >= getNumOfTupleLists())
	{
	  throw new parse_exception("There are only " + to_string(getNumOfTupleLists) +
								" tuplelists, requested " + to_string(tuple_num) + ".");
	}
	tuplelist = getTupleList(tuple_num);
  }
  else
  {
	vector<vector<int> > tuples ;
	check_sym(infile,'{');
	while (delim != '}') 
	{
	  check_sym(infile,'<');
	  vector<int> tuple(tupleSize);
	  elem = read_num(infile) ;
	  tuple[0] = elem ;
	  for (count = 1; count < tupleSize; count++) 
	  {
		check_sym(infile, ',');
		elem = read_num(infile) ;
		tuple[count] = elem ;
	  }
	  check_sym(infile, '>');
	  tuples.push_back(tuple) ;
	  infile >> delim ;                                          // ',' or '}'
	  if(delim != ',' && delim!= '}')
		throw new parse_exception("Expected ',' or '}'");
	  tuplelist = getNewTupleList(tuples);
	}
  }
	
	check_sym(infile,')');
	ConstraintBlob tableCon(def, vectorOfVars);
	tableCon.tuples = tuplelist;
	instance.add_constraint(tableCon);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// readIdentifier
// Expects "<idChar><index>", where <idChar> is 'x', 'v', 'm', 't'.
// Assumes caller knows what idChar should be.
// Returns an object of type Var.
// NB peek() does not ignore whitespace, >> does. Hence use of putBack()
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
Var MinionInputReader::readIdentifier(ifstream& infile) {
  char idChar ;
  infile >> idChar ;
  if ((('0' <= idChar) && ('9' >= idChar)) || idChar == '-') {
    infile.putback(idChar) ;
    int i = read_num(infile);
	return Var(VAR_CONSTANT, i);
  }
  int index = -1 ;
  
  if(idChar != 'x' && idChar != 'n')
  {
    string s("Found 'X', expected 'x' or 'n' at start of a variable");
	s[7] = idChar;
    throw new parse_exception(s);
  }
  if(idChar == 'x')
  {
    index = read_num(infile);
    return instance.vars.get_var(idChar, index);
  }
  check_sym(infile, 'x');
  index = read_num(infile);
  Var var = instance.vars.get_var(idChar, index);
  if(var.type != VAR_BOOL)
    throw new parse_exception("Can only 'not' a Boolean variable!");
  var.type = VAR_NOTBOOL;
  return var;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// readLiteralMatrix
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
vector< vector<Var> > MinionInputReader::readLiteralMatrix(ifstream& infile) {

  check_sym(infile, '[');
  
  // Delim here might end up being "x" or something similar. The reason
  // that we peek it is in case whis is an empty vector.
  char delim = peek_char(infile);
  vector< vector<Var> > newMatrix ;

  if(delim == ']')
  {
    // Eat the ']'
    infile.get();
    parser_info("Read empty matrix");
  }
  else
  {
    while(delim != ']') {
      newMatrix.push_back(readLiteralVector(infile)) ;
      infile >> delim ;                                        // , or ]
    }
  }
  return newMatrix ;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// readLiteralVector
// of vars or consts. Checks 1st elem of vect (empty vects not expected)
//  to see which.
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
vector<Var> MinionInputReader::readLiteralVector(ifstream& infile) {
  vector<Var> newVector ;
  check_sym(infile, '[');
 
  // Delim here might end up being "x" or something similar. The reason
  // that we peek it is in case whis is an empty vector.
  
  char delim = peek_char(infile);
    	
  if(delim == ']')
  {
    // Eat the ']'
    infile.get();
    parser_info("Read empty vector.");
  }
  else
  {
    while (delim != ']') {
	  newVector.push_back(readIdentifier(infile)) ;
	  infile >> delim ;
	     if(delim != ',' && delim != ']')
	     {
		   // replace X with the character we got.
		   string s = "Expected ',' or ']'. Got 'X'.";
		   s[s.size() - 3] = delim;
		   throw new parse_exception(s);
	     }
      }
  }
  return newVector ;
  
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// readMatrices
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void MinionInputReader::readMatrices(ifstream& infile) {
  char delim ;
  int count1 ;
  // Read Vectors%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  int noOfMatrixType = read_num(infile);
  if(parser_verbose)
    cout << "Number of 1d vectors: " << noOfMatrixType << endl ;
  for (count1 = 0; count1 < noOfMatrixType; count1++)
    Vectors.push_back(readLiteralVector(infile)) ;
  // Read 2dMatrices%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  noOfMatrixType = read_num(infile);
  if(parser_verbose)
    cout << "Number of 2d matrices: " << noOfMatrixType << endl ;
  for (count1 = 0; count1 < noOfMatrixType; count1++)
    Matrices.push_back(readLiteralMatrix(infile)) ;
  // Read 3dMatrices%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  noOfMatrixType = read_num(infile);
  if(parser_verbose)
    cout << "Number of 3d tensors: " << noOfMatrixType << endl ;
  for (count1 = 0; count1 < noOfMatrixType; count1++) {
    vector< vector< vector <Var> > > newTensor ;
    infile >> delim ;                                               // [
    while (delim != ']') {
      newTensor.push_back(readLiteralMatrix(infile)) ;
      infile >> delim ;                                        // , or ]
    }
    Tensors.push_back(newTensor) ;
  }
}

//%%%%
// readTuples
// 'tuplelists' <val>  ( <num_tuples> <tuple_length> <vals> ...
void MinionInputReader::readTuples(ifstream& infile) {
  int tuple_count = read_num(infile);
  for(int counter = 0; counter < tuple_count; counter++)
  {
    int num_of_tuples = read_num(infile);
	int tuple_length = read_num(infile);
	TupleList* tuplelist = getNewTupleList(num_of_tuples, tuple_length);
	int* tuple_ptr = tuplelist->getPointer();
	for(int i = 0; i < num_of_tuples; ++i)
	  for(int j = 0; j < tuple_length; ++j)
	  {
	    tuple_ptr[i * tuple_length + j] = read_num(infile);
	  }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// readObjective
// 'objective' 'none' | 'minimising' <var> | 'maximising' <var>
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void MinionInputReader::readObjective(ifstream& infile) {
  // Note that we will have read "objective" before entering this function.
  string s;
  infile >> s;
  if(s == "none")
  {
    parser_info( "objective none" );
	return;
  }
  
  if(s != "minimising" && s != "maximising")
  {
    throw new parse_exception(string("Expected 'none', 'maximising'") +
							  string("or 'minimising'. Got '") + s + "'");
  }
  
  bool minimising = (s == "minimising");
  Var var = readIdentifier(infile) ;
  if(parser_verbose)
    cout << ((minimising) ? "minimising" : "maximising") << string(var) << endl ;
  instance.set_optimise(minimising, var);
}

void MinionInputReader::readPrint(ifstream& infile) {
  string s;
  infile >> s;
  if(s != "print")
    throw new parse_exception(string("Expected 'print', recieved '")+s+"'");
  
  char letter;
  infile >> letter;
  if(letter == 'n')
  {
    infile >> s;
	if(s != "one")
	  throw new parse_exception(string("I don't understand '")+s+"'");
	parser_info( "print none" );
	return;
  }
  else if(letter == 'm')
  {
    int matrix_num = read_num(infile);
	instance.print_matrix = index(Matrices, matrix_num);
	if(parser_verbose)
	  cout << "print m" << matrix_num << endl;
	return;
  }
  else if(letter == 'v')
  {
    int vec_num = read_num(infile);
	instance.print_matrix = make_vec(index(Vectors, vec_num));
	if(parser_verbose)
	  cout << "print v" << vec_num << endl;
    return;
  }
  
  throw new parse_exception(string("I don't understand this print statement"));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// readValOrder
// '[' <valOrderIdentifier> [, <valOrderIdentifier>]* ']'
// <valOrderIdentifier> := 'a' | 'd' --- for ascending/descending
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void MinionInputReader::readValOrder(ifstream& infile) {
  parser_info( "Reading val order" ) ;
  
  check_sym(infile, '[');
  
  // Delim here might end up being "x" or something similar. The reason
  // that we peek it is in case whis is an empty vector.
  
  char delim = peek_char(infile);
  
  if(delim == ']')
  {
    // Eat the ']'
    infile.get();
    parser_info("No val order");
  }
  else
  {
    vector<char> valOrder ;
  
    while (delim != ']') {
      char valOrderIdentifier;
	  infile >> valOrderIdentifier ;
	  if(valOrderIdentifier != 'a' && valOrderIdentifier != 'd')
	    throw new parse_exception("Expected 'a' or 'd'");
	  valOrder.push_back(valOrderIdentifier == 'a');
      infile >> delim ;                                          // , or ]
    }
    instance.val_order = valOrder;
    ostringstream s;
    s << "Read val order. Length: " << valOrder.size();
    parser_info(s.str());
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// readVarOrder
// '[' <var> [, <var>]* ']'
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void MinionInputReader::readVarOrder(ifstream& infile) {
  parser_info( "Reading var order" ) ;
  vector<Var> varOrder = readLiteralVector(infile);

  instance.var_order = varOrder;
  
  ostringstream s;
  s << "Read var order. Length: " << varOrder.size();
  parser_info(s.str());
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void MinionInputReader::readVars(ifstream& infile) {
  int lb, ub, count ;
  int total_var_count = 0;
  char delim ;
  ProbSpec::VarContainer var_obj;
  // Read 01Vars%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  int noOfVarType = read_num(infile);
  total_var_count += noOfVarType;
  if(parser_verbose)
    cout << "Number of 01 Vars: " << noOfVarType << endl ;
  var_obj.bools = noOfVarType;
  
  
  // **** Construct this many 01Vars
  // Read Bounds Vars%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  noOfVarType = read_num(infile);
  total_var_count += noOfVarType;
  if(parser_verbose)
    cout << "Number of Bounds Vars: " << noOfVarType << endl ;
  while (noOfVarType > 0) {
    lb = read_num(infile);
	ub = read_num(infile);
	if(lb > ub)
	  throw new parse_exception("Lower bound must be less than upper bound!");
	count = read_num(infile);
	if(parser_verbose)
      cout << count << " of " << lb << ", " << ub << endl ;
    var_obj.bound.push_back(make_pair(count, ProbSpec::Bounds(lb, ub)));
    noOfVarType -= count ;
  }
  
  // Read Sparse Bounds Vars%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  noOfVarType = read_num(infile);
  total_var_count += noOfVarType;
  if(parser_verbose)
    cout << "Number of Sparse Bounds Vars: " << noOfVarType << endl ;
  int domainElem ;
  while (noOfVarType > 0) {
    vector<int> domainElements ;
    infile >> delim ;                                               // {
    while (delim != '}') {
      domainElem = read_num(infile);
      domainElements.push_back(domainElem) ;
      infile >> delim ;                                        // , or }
    }
    count = read_num(infile);
	if(parser_verbose)
      cout << count << " of these " << endl ;
    // **** Construct this many discrete vars.
    var_obj.sparse_bound.push_back(make_pair(count, domainElements));
    noOfVarType -= count ;
  }
  
  // Read Discrete Bounds Vars%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  noOfVarType = read_num(infile);
  total_var_count += noOfVarType;
  if(parser_verbose)
    cout << "Number of Discrete Vars: " << noOfVarType << endl ;
  while (noOfVarType > 0) {
    lb = read_num(infile);
	ub = read_num(infile);
	count = read_num(infile);
	if(parser_verbose)
      cout << count << " of " << lb << ", " << ub << endl ;
    var_obj.discrete.push_back(make_pair(count, ProbSpec::Bounds(lb, ub)));
    // **** Construct this many discrete bounds vars.
    noOfVarType -= count ;
  }
  // Read Discrete Vars%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  noOfVarType = read_num(infile);
  total_var_count += noOfVarType;
  if(parser_verbose)
    cout << "Number of Sparse Discrete Vars: " << noOfVarType << endl ;
  while (noOfVarType > 0) {
    vector<int> domainElements ;
    infile >> delim ;                                               // {
    while (delim != '}') {
      domainElem = read_num(infile);
      domainElements.push_back(domainElem) ;
      infile >> delim ;                                        // , or }
    }
    count = read_num(infile);
    if(parser_verbose)
      cout << count << " of these " << endl ;
    // **** Construct this many discrete vars.
    var_obj.sparse_discrete.push_back(make_pair(count, domainElements));
    noOfVarType -= count ;
  }
  
  var_obj.total_var_count = total_var_count;
  instance.vars = var_obj;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// readVectorExpression
// literal vector (of vars or consts), vi, mi(flattened), ti(flattened),
// row(mi, r), col(mi, c), col(ti, p, c), rowx(ti, p, r), rowz(ti, r, c)
// NB Expects caller knows whether vars or consts expected for lit vect.
// NB peek does not ignore wspace, >> does. Hence use of putback
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
vector<Var> MinionInputReader::readVectorExpression(ifstream& infile) {
  char idChar, delim ;
  int row, col, plane ;
  int input_val;
  infile >> idChar ;
  switch (idChar) {
    case '[':
      parser_info( "Reading Literal Vector of vars or consts" ) ;
      infile.putback(idChar) ;
      return readLiteralVector(infile) ;      
    case 'v':                                        // vector identifier
      parser_info( "Reading vector identifier" ) ;
      //infile.putback(idChar) ;
	  input_val = read_num(infile);
	  return Vectors.at(input_val) ;
    case 'm':                                       // matrix identifier
      parser_info( "Reading matrix identifier (will flatten)" ) ;
      //infile.putback(idChar) ;
	  input_val = read_num(infile);
	  return flatten('m', input_val) ;
    case 't':                                        // matrix identifier
      parser_info( "Reading tensor identifier (will flatten)" ) ;
      //infile.putback(idChar) ;
	  input_val = read_num(infile);
	  return flatten('t', input_val) ;
    case 'r':                                       // row of a mx/tensor
	  check_sym(infile,'o');
	  check_sym(infile,'w');
      infile >> idChar ;            // o w [( x or z]
      switch(idChar) {
		case '(':                                        // row of a matrix
		{parser_info( "Reading row of a matrix" ) ;
		  check_sym(infile,'m');
		  input_val = read_num(infile);
		  vector< vector<Var> > matrix = Matrices.at(input_val) ;
		  infile >> delim;
		  row = read_num(infile);
		  infile >> delim ;
		  return matrix.at(row) ;}
		case 'x':                             // row of a plane of a tensor
		{parser_info( "Reading row of a plane of a tensor" ) ;
		  check_sym(infile,'(');
		  check_sym(infile,'t');
		  input_val = read_num(infile);
		  vector< vector< vector<Var> > >& tensor = Tensors.at(input_val) ;
		  check_sym(infile, ',');
		  input_val = read_num(infile);
		  vector< vector <Var> >& tensorPlane = tensor.at(input_val) ;
		  check_sym(infile, ',');
		  input_val = read_num(infile);
		  check_sym(infile, ')');
		  return tensorPlane.at(input_val);
		}
		case 'z':                         // Row through planes of a tensor
		{parser_info( "Reading row through planes of a tensor" ) ;
		  check_sym(infile, '(');
		  input_val = read_num(infile);
		  vector< vector< vector<Var> > >& tensor = Tensors.at(input_val) ;
		  check_sym(infile, ',');
		  row = read_num(infile);
		  check_sym(infile, ',');
		  col = read_num(infile);
		  check_sym(infile, ')');
		  return getRowThroughTensor(tensor, row, col) ;}
		default:
		  throw new parse_exception("Malformed Row Expression");
		  break ;
	  }
		break ;
      //col(mi, c), col(ti, p, c)
    case 'c':                                        // col of a mx/tensor
	  check_sym(infile, 'o');
	  check_sym(infile, 'l');
	  check_sym(infile, '(');
      if(peek_char(infile) == 'm') {
		parser_info( "Reading col of matrix" ) ;
		check_sym(infile, 'm');
		input_val = read_num(infile);
		vector< vector<Var> >& matrix = Matrices.at(input_val) ;
		check_sym(infile, ',');
		col = read_num(infile);
		check_sym(infile, ')');
		return getColOfMatrix(matrix, col) ;
      }
		else {
		  parser_info( "Reading col of tensor" ) ;
		  check_sym(infile, 't');
		  input_val = read_num(infile);
		  vector< vector< vector<Var> > >& tensor = Tensors.at(input_val);
		  check_sym(infile, ',');
		  plane = read_num(infile);
		  check_sym(infile, ',');
		  col = read_num(infile);
		  check_sym(infile, ')');
		  return getColOfMatrix(tensor.at(plane), col) ;
		}
      default:
		throw new parse_exception("Malformed Vector Expression") ;
		break ;
  }
  exit(1);
}


