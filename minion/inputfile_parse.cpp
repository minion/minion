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

#include "svn_header.h"



CSPInstance readInputFromFile(string fname, bool parser_verbose)
{
  if( fname != "--" )
  {  
    const char* filename = fname.c_str();
    string extension;
    if(fname.find_last_of(".") < fname.size())
      extension = fname.substr(fname.find_last_of("."), fname.size());
  
    if(extension == ".gz" || extension == ".gzip" || extension == ".z" || extension == ".gzp" ||
        extension == ".bz2" || extension == ".bz" || extension == ".bzip2" || extension == ".bzip")
    {
  #ifdef USE_BOOST
  
    ifstream file(filename, ios_base::in | ios_base::binary);
  
     if (!file) {
        INPUT_ERROR("Can't open given input file '" + fname + "'.");
      }
    filtering_istream in;
  
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
      
    in.push(file);
  
    ConcreteFileReader<filtering_istream> infile(in, filename);
    
    if (infile.failed_open() || infile.eof()) {
      INPUT_ERROR("Can't open given input file '" + fname + "'.");
    }   
    return readInput(&infile, parser_verbose);
  #else 
     INPUT_ERROR("This copy of Minion was built without gzip and bzip2 support!");
  #endif
    }
    else
    {
      ifstream ifm(filename);
      ConcreteFileReader<ifstream> infile(ifm, filename);
      if (infile.failed_open() || infile.eof()) {
        INPUT_ERROR("Can't open given input file '" + fname + "'.");
      }   
      return readInput(&infile, parser_verbose);
    }

  }
  else
  {
    ConcreteFileReader<std::basic_istream<char, std::char_traits<char> > > infile(cin, "Standard Input");
    return readInput(&infile, parser_verbose);
  }
  
}
