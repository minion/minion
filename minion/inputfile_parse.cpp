/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id$
*/

#include "minion.h"
#include "CSPSpec.h"


#include "BuildConstraint.h"
#include "MinionInputReader.h"

#include "inputfile_parse.h"

#include "counter.hpp"

using boost::iostreams::filtering_istream;
using boost::iostreams::gzip_decompressor;
using boost::iostreams::bzip2_decompressor;

using boost::iostreams::error_counter;

CSPInstance readInputFromFile(string fname, bool parser_verbose)
{
    const char* filename = fname.c_str();
    string extension;
    if(fname.find_last_of(".") < fname.size())
      extension = fname.substr(fname.find_last_of("."), fname.size());
  
    filtering_istream in;
    
    error_counter e_count;
    
    in.push(boost::ref(e_count));
    
    if(extension == ".gz" || extension == ".gzip" || extension == ".z" || extension == ".gzp" ||
        extension == ".bz2" || extension == ".bz" || extension == ".bzip2" || extension == ".bzip")
    {  
      if(extension == ".gz" || extension == ".gzip" || extension == ".z" || extension == ".gzp")
      {
        if(parser_verbose)
          cout << "# Using gzip uncompression" << endl;
        in.push(gzip_decompressor());
      }    
  
      if(extension == ".bz2" || extension == ".bz" || extension == ".bzip2" || extension == ".bzip")
      {
        if(parser_verbose)
          cout << "# Using bzip2 uncompression" << endl;
        in.push(bzip2_decompressor());
      }
      
    }
    
    ifstream* file;
    if(fname != "--")
    {
      file = new ifstream(filename, ios_base::in | ios_base::binary);
      if (!(*file)) {
        INPUT_ERROR("Can't open given input file '" + fname + "'.");
      }
      in.push(*file);
    }
    else
      in.push(cin);

    ConcreteFileReader<filtering_istream> infile(in, filename);

    if (infile.failed_open() || infile.eof()) {
      INPUT_ERROR("Can't open given input file '" + fname + "'.");
    }   
    
    try
    {
      return readInput(&infile, parser_verbose);
      // delete file;
    }
    catch(parse_exception s)
    {
      cerr << "Error in input!" << endl;
      cerr << s.what() << endl;
      
      cerr << "Error occurred on line " << e_count.lines_prev << ", around character " << e_count.chars_prev << endl;
      cerr << "The parser gave up around: '" << e_count.current_line_prev << "'" << endl;
      exit(1);
    }
    
}
