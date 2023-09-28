// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#include "inputfile_parse.h"

#include "MinionThreeInputReader.hpp"
#include <fstream>
#include <iostream>

template <typename Reader, typename Stream>
void ReadCSP(Reader& reader, ConcreteFileReader<Stream>* infile) {
  reader.read(infile);
  getTableOut().set(string("Filename"), infile->filename);
}

void readInputFromFiles(ProbSpec::CSPInstance& instance, vector<string> fnames, bool parserVerbose,
                        MapLongTuplesToShort mltts, bool ensureBranchOnAllVars) {
  MinionThreeInputReader<ConcreteFileReader<CheapStream>> readerThree(parserVerbose, mltts,
                                                                      ensureBranchOnAllVars);

  bool needsFinaliseThree = false;
  for(vector<string>::const_iterator fname = fnames.begin(); fname != fnames.end(); fname++) {
    const char* filename = fname->c_str();
    string extension;
    if(fname->find_last_of(".") < fname->size())
      extension = fname->substr(fname->find_last_of("."), fname->size());

    // We need to use a pointer here, as we want to declare the object inside a
    // for loop,
    // and we need it to live until after we've finished parsing.
    // We use an auto_ptr because it will auto-delete on exit.
    istream* file;

    if(*fname != "--") {
      file = new ifstream(filename, ios_base::in | ios_base::binary);
      if(!(*file)) {
        INPUT_ERROR("Can't open given input file '" + *fname + "'.");
      }
    } else
      file = &cin;

    CheapStream cs(*file);

    ConcreteFileReader<CheapStream> infile(cs, filename);

    if(infile.eof()) {
      INPUT_ERROR("Can't open given input file '" + *fname + "'.");
    }

    try {
      {
        string testName = infile.getString();
        if(testName != "MINION")
          INPUT_ERROR("All Minion input files must begin 'MINION'");

        SysInt inputFileVersionNumber = infile.read_int();

        if(inputFileVersionNumber != 3)
          INPUT_ERROR("This version of Minion only supports format 3");

        readerThree.instance = &instance;
        ReadCSP(readerThree, &infile);
        // instance = std::move(readerThree.instance);
        needsFinaliseThree = true;
      }
    } catch(const parse_exception& s) {
      cerr << "Error in input!" << endl;
      cerr << s.what() << endl;

      SysInt pos = cs.getRawPos();
      cs.resetStream();

      string currentLine;
      SysInt startOfLine = 0;
      SysInt lineCount = -1;

      do {
        lineCount++;
        startOfLine = cs.getRawPos();
        currentLine = cs.getline();
      } while(cs.getRawPos() < pos);

      cerr << "Error occurred on line " << lineCount << endl;
      cerr << "Parser gave up around:" << endl;
      cerr << currentLine << endl;
      for(SysInt i = 0; i < pos - startOfLine - 1; ++i)
        cerr << "-";
      cerr << "^" << endl;
      exit(1);
    }
  }
  if(needsFinaliseThree) {
    readerThree.finalise();
    // instance = std::move(readerThree.instance);
  }
}
