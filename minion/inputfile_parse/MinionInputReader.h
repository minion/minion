// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef _MINIONINPUTREADER_H
#define _MINIONINPUTREADER_H

#include "../system/system.h"
#include "cheap_stream.h"

template <typename StreamType>
struct ConcreteFileReader {
  StreamType& infile;
  string filename;

  /// Removes all comments after the current place in the file.
  // Returns peeked char.
  void checkFor_comments() {
    char peek = simplepeekChar();
    while(peek == '#') {
      simplegetline();
      peek = simplepeekChar();
    }
  }

  ConcreteFileReader(StreamType& name, string _filename) : infile(name), filename(_filename) {}

  BOOL failed_open() {
    return !infile;
  }

  string getString() {
    char buffer[1000];
    SysInt pos = 0;
    char nextChar = getChar();
    while(isalnum(nextChar) || nextChar == '_') {
      buffer[pos] = nextChar;
      pos++;
      if(pos >= 999) {
        D_FATAL_ERROR("Identifer too long!");
      }
      nextChar = infile.get();
    }

    putback(nextChar);
    buffer[pos] = '\0';
    return string(buffer);
  }

  void checkString(const string& string_in) {
    string s = getString();
    if(s != string_in) {
      throw parse_exception("Expected " + string_in + ", recieved '" + s + "'");
    }
  }

  string getAsciiString() {
    string s;
    char nextChar = getChar();
    while(!isspace(nextChar) && !infile.eof()) {
      s += nextChar;
      nextChar = infile.get();
    }

    // Fix for nasty windows issue -- long term we should clean this up.
    if(infile.eof() && !isspace(nextChar))
      s += nextChar;

    putback(nextChar);
    return s;
  }

  SysInt read_int() {
    SysInt i = 0;
    infile >> i;
    if(infile.fail())
      throw parse_exception("Problem parsing number");
    return i;
  }

  DomainInt readNum() {
    return this->read_int();
  }

  char simplepeekChar() {
    char peek = infile.peek();
    while(isspace(peek)) {
      infile.get();
      peek = infile.peek();
    }
    return peek;
  }

  char peekChar() {
    char peek = simplepeekChar();
    while(peek == '#') {
      simplegetline();
      peek = simplepeekChar();
    }
    return peek;
  }

  /// Check if the next character from infile is sym.
  void checkSym(char sym) {
    char idChar = getChar();
    if(idChar != sym) {
      throw parse_exception(string("Expected '") + sym + "'. Received '" + idChar + "'.");
    }
  }

  string getline() {
    checkFor_comments();
    return simplegetline();
  }

  string simplegetline() {
    return infile.getline();
  }

  /// Cleans rubbish off start of string.
  void clean_string(string& s) {
    while(!s.empty() && isspace(s[0]))
      s.erase(s.begin());
  }

  string getline(char deliminator) {
    checkFor_comments();
    std::string s = infile.getline(deliminator);
    // avoid copy for no reason
    if(s.empty() || (!isspace(s[0])))
      return s;

    int pos = 1;
    while(pos < (int)s.size() && isspace(s[pos]))
      pos++;
    return string(s.begin() + pos, s.end());
  }

  char getChar() {
    char peek = simplegetChar();
    while(peek == '#') {
      simplegetline();
      peek = simplegetChar();
    }
    return peek;
  }

  char simplegetChar() {
    char k;
    infile >> k;
    return k;
  }

  BOOL eof() {
    return infile.eof();
  }

  void putback(char c) {
    infile.putback(c);
  }

  ~ConcreteFileReader() {}
};

template <typename FileReader>
class MinionThreeInputReader {
  void parser_info(string);
  vector<vector<Var>> Vectors;
  vector<vector<vector<Var>>> Matrices;
  vector<vector<vector<vector<Var>>>> Tensors;
  vector<Var> flatten(char type, SysInt index);
  vector<Var> getColOfMatrix(vector<vector<Var>>& m, SysInt c);
  vector<Var> getRowThroughTensor(vector<vector<vector<Var>>>& t, SysInt r, SysInt c);
  ConstraintBlob readConstraint(FileReader* infile, BOOL reified = false);
  ConstraintBlob readConstraintTable(FileReader* infile, ConstraintDef* def);
  void readGadget(FileReader* infile);
  std::shared_ptr<TupleList> readConstraintTupleList(FileReader* infile);
  void readMutexDetect(FileReader* infile);
  Var readIdentifier(FileReader* infile);
  vector<Var> readPossibleMatrixIdentifier(FileReader* infile, bool mustBeMatrix = false);
  vector<Var> readLiteralVector(FileReader* infile, bool mustBeVector = true);
  vector<DomainInt> readConstantVector(FileReader* infile, char start = '[', char end = ']',
                                       bool = false);
  vector<DomainInt> readRange(FileReader* infile);
  void readObjective(FileReader* infile);
  void readShortTuples(FileReader* infile);
  void readTuples(FileReader* infile);
  void readMatrices(FileReader* infile);
  void readValOrder(FileReader* infile);
  void readVarOrder(FileReader* infile);
  void readPrint(FileReader* infile);
  void readVars(FileReader* infile);
  void readSearch(FileReader* infile);
  vector<pair<SysInt, DomainInt>> readShortTuple(FileReader*);
  std::shared_ptr<ShortTupleList> readConstraintShortTupleList(FileReader*);
  vector<vector<Var>> read2DMatrix(FileReader* infile);
  vector<vector<Var>> read2DMatrixVariable(FileReader* infile);
  void readAliasMatrix(FileReader* infile, const vector<DomainInt>& max_indices,
                       vector<DomainInt> indices, string name);
  vector<Var> readVectorExpression(FileReader* infile);
  ConstraintBlob readGeneralConstraint(FileReader*, ConstraintDef*);
  vector<ConstraintBlob> readConstraintList(FileReader* infile);

public:
  void read(FileReader* infile);

  void finalise();

  ProbSpec::CSPInstance* instance;
  bool parserVerbose;
  bool print_allVars;
  MapLongTuplesToShort map_long_short_mode;

  bool ensureBranchOnAllVars;

  bool isGadgetReader_m;

  void setGadgetReader() {
    isGadgetReader_m = true;
  }
  bool isGadgetReader() {
    return isGadgetReader_m;
  }

  MinionThreeInputReader(bool _parserVerbose, MapLongTuplesToShort mls, bool _e)
      : parserVerbose(_parserVerbose),
        print_allVars(true),
        map_long_short_mode(mls),
        ensureBranchOnAllVars(_e),
        isGadgetReader_m(false) {}
};

#endif
