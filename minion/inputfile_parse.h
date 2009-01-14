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


template<typename Reader, typename Stream>
    void ReadCSP(Reader& reader, ConcreteFileReader<Stream>* infile)
{
    reader.read(infile) ;
    oldtableout.set(string("Filename"), infile->filename);  
}

template<typename InputReader>
CSPInstance readInput(InputReader* infile, bool parser_verbose)
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

CSPInstance readInputFromFile(string fname, bool parser_verbose);

#endif
