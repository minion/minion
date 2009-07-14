/*
* Minion http://minion.sourceforge.net
* Copyright (C) 2006-09
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "inputfile_parse.h"

#include "MinionInputReader.hpp"
#include "MinionThreeInputReader.hpp"

#include <fstream>
#include <iostream>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filter/bzip2.hpp>

using boost::iostreams::filtering_istream;
using boost::iostreams::gzip_decompressor;
using boost::iostreams::bzip2_decompressor;

template<typename Reader, typename Stream>
    void ReadCSP(Reader& reader, ConcreteFileReader<Stream>* infile)
{
    reader.read(infile) ;
    getTableOut().set(string("Filename"), infile->filename);  
}

void readInputFromFiles(CSPInstance& instance, vector<string> fnames, bool parser_verbose)
{
  MinionThreeInputReader<ConcreteFileReader<CheapStream> > readerThree(parser_verbose);
  MinionInputReader<ConcreteFileReader<CheapStream> > reader(parser_verbose);
  bool needs_finalise_three = false;
  for(vector<string>::const_iterator fname = fnames.begin(); fname != fnames.end(); fname++) {
    const char* filename = fname->c_str();
    string extension;
    if(fname->find_last_of(".") < fname->size())
      extension = fname->substr(fname->find_last_of("."), fname->size());
  
    filtering_istream in;
        
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
    
    // We need to use a pointer here, as we want to declare the object inside a for loop,
    // and we need it to live until after we've finished parsing.
    // We use an auto_ptr because it will auto-delete on exit.
    auto_ptr<ifstream> file;
    
    if(*fname != "--")
    {
      file = auto_ptr<ifstream>(new ifstream(filename, ios_base::in | ios_base::binary));
      if (!(*file)) {
        INPUT_ERROR("Can't open given input file '" + *fname + "'.");
      }
      in.push(*file);
    }
    else
      in.push(cin);

    CheapStream cs(in);
    ConcreteFileReader<CheapStream> infile(cs, filename);

    if (infile.eof()) {
      INPUT_ERROR("Can't open given input file '" + *fname + "'.");
    }   
    
    try
    {
      string test_name = infile.get_string();
      if(test_name != "MINION")
        INPUT_ERROR("All Minion input files must begin 'MINION'");
  
      int inputFileVersionNumber = infile.read_num();
  
      if(inputFileVersionNumber > 3)
        INPUT_ERROR("This version of Minion only supports formats up to 3");

      // C++0x comment : Need MOVE (which is std::move) here to activate r-value references.
      // Normally we wouldn't, but here the compiler can't figure out it can "steal" instance.
      if(inputFileVersionNumber == 3)
      {
        readerThree.instance = MOVE(instance);
        ReadCSP(readerThree, &infile);
        instance = MOVE(readerThree.instance);
        needs_finalise_three = true;
      } 
      else
      {
        reader.instance = MOVE(instance);
        ReadCSP(reader, &infile);
        instance = MOVE(reader.instance);
      }
    }
    catch(parse_exception s)
    {
      cerr << "Error in input!" << endl;
      cerr << s.what() << endl;
      
      int pos = cs.get_raw_pos();
      cs.reset_stream();
      
      string current_line;
      int start_of_line = 0;
      int line_count = -1;
      
      do
      {
          line_count++;
          start_of_line = cs.get_raw_pos();
          char buf[10000];
          cs.getline(buf, 10000);
          current_line = buf;
      }
      while(cs.get_raw_pos() < pos);
      
      cerr << "Error occurred on line " << line_count << endl;
      cerr << "Parser gave up around:" << endl;
      cerr << current_line << endl;
      for(int i = 0; i < pos - start_of_line - 1; ++i)
          cerr << "-";
      cerr << "^" << endl;
        exit(1);
    }
  }
  if(needs_finalise_three)
  {
      readerThree.finalise();
      instance = MOVE(readerThree.instance);
  }
}
