#ifndef INPUTFILE_PARSE_H
#define INPUTFILE_PARSE_H

#include "minion.h"

#include "MinionInputReader.hpp"
#include "MinionThreeInputReader.hpp"

#include <fstream>
#include <iostream>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filter/bzip2.hpp>

/// Reads the CSP given by infile into reader.
/** Most of the code in this function related to trying to provide nice
  * output in the case when a parsing error occurs
  */
template<typename Reader>
    void ReadCSP(Reader& reader, ConcreteFileReader<ifstream>* infile)
{
  try
  {
    reader.read(infile) ;
    oldtableout.set(string("Filename"), infile->filename);
  }
  catch(parse_exception s)
  {
    cerr << "Error in input." << endl;
    cerr << s.what() << endl;

    // This nasty line will tell us the current position in the file 
    // even if a parse fail has occurred.
    int error_pos = infile->infile.rdbuf()->pubseekoff(0, ios_base::cur, ios_base::in);
    int line_num = 0;
    int end_prev_line = 0;
    char* buf = new char[1000000];
    infile->infile.close();
    // Open a new stream, because we don't know what kind of a mess
    // the old one might be in.
    ifstream error_file(infile->filename.c_str());
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
    exit(1);
  }
}

template<typename Reader, typename Stream>
    void ReadCSP(Reader& reader, ConcreteFileReader<Stream>* infile)
{
  try
  {
    reader.read(infile) ;
    oldtableout.set(string("Filename"), infile->filename);
  }
  catch(parse_exception s)
  {
    cerr << "Error in input." << endl;
    cerr << s.what() << endl;

    // Had an input or compressed stream.
    cerr << "Error occurred in standard input or a compressed file. Cannot show where the error occurred." << endl;
    cerr << "Re-execute Minion with an uncompressed input file to see the error location." << endl;
 
    cerr << "Sorry it didn't work out." << endl;
    exit(1);
  }
}

template<typename InputReader>
CSPInstance readInput(InputReader* infile, bool parser_verbose)
{  
  try
  {
    string test_name = infile->get_string();
    if(test_name != "MINION")
      INPUT_ERROR("All Minion input files must begin 'MINION'");
  
    int inputFileVersionNumber = infile->read_num();
  
    if(inputFileVersionNumber > 3)
      INPUT_ERROR("This version of Minion only supports formats up to 3");
  

    // C++0x comment : Need MOVE (which is std::move) here to activate r-value references.
    // Normally we wouldn't, but here the compiler can't figure out it can "steal" instance.
    if(inputFileVersionNumber == 3)
    {
      MinionThreeInputReader<InputReader> reader(parser_verbose);
      ReadCSP(reader, infile);
      return MOVE(reader.instance);
    } 
    else
    {
      MinionInputReader<InputReader> reader(parser_verbose);
      ReadCSP(reader, infile);
      return MOVE(reader.instance);
    }  
  }
  catch(parse_exception s) // This catch should only trigger on the very top-most level issues, like test_name and inputFileVersionNumber above.
  {
    cerr << "Not a valid Minion instance - should start with 'MINION x' where x is 1,2 or 3" << endl;
    exit(1);
  }
}

CSPInstance readInputFromFile(string fname, bool parser_verbose);

#endif
