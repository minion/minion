#ifndef INPUTFILE_PARSE_H
#define INPUTFILE_PARSE_H

#include "minion.h"

#include "MinionInputReader.hpp"
#include "MinionThreeInputReader.hpp"

/// Reads the CSP given by infile into reader.
/** Most of the code in this function related to trying to provide nice
  * output in the case when a parsing error occurs
  */
template<typename Reader, typename FileReader>
void ReadCSP(Reader& reader, FileReader* infile)
{
#ifndef NOCATCH  
  try
{
#endif
  
  reader.read(infile) ;
  tableout.set(string("Filename"), infile->filename);
#ifndef NOCATCH
}
catch(parse_exception& s)
{
  cerr << "Error in input." << endl;
  cerr << s.what() << endl;
  
  ConcreteFileReader<ifstream>* stream_cast = reinterpret_cast<ConcreteFileReader<ifstream>*>(infile);
  if(stream_cast)
  {
    // This nasty line will tell us the current position in the file 
    // even if a parse fail has occurred.
    int error_pos = stream_cast->infile.rdbuf()->pubseekoff(0, ios_base::cur, ios_base::in);
    int line_num = 0;
    int end_prev_line = 0;
    char* buf = new char[1000000];
    stream_cast->infile.close();
    // Open a new stream, because we don't know what kind of a mess
    // the old one might be in.
    ifstream error_file(stream_cast->filename.c_str());
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
  }
  else
  {
    // Had an input stream.
    cerr << "At the moment we can't show where a problem occured in the input stream." << endl;
    char* buf = new char[100000];
    buf[0]='\0';
    cin.getline(buf, 100000);
    cerr << "The rest of the line that went wrong is:" << endl;
    cerr << buf << endl;
    cerr << "Try saving your output to a temporary file." << endl;
  }
  
  cerr << "Sorry it didn't work out." << endl;
  exit(1);
}
#endif
}

template<typename InputReader>
CSPInstance readInput(InputReader* infile, bool parser_verbose)
{  
  string test_name = infile->get_string();
  if(test_name != "MINION")
    D_FATAL_ERROR("All Minion input files must begin 'MINION'");
  
  int inputFileVersionNumber = infile->read_num();
  
  if(inputFileVersionNumber > 3)
    D_FATAL_ERROR("This version of Minion only supports formats up to 3");
  

  
  if(inputFileVersionNumber == 3)
  {
    MinionThreeInputReader<InputReader> reader(parser_verbose);
    ReadCSP(reader, infile);
    return reader.instance;
  } 
  else
  {
    MinionInputReader<InputReader> reader(parser_verbose);
    ReadCSP(reader, infile);
    return reader.instance;
  }  

}

#endif