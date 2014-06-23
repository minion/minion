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
#include "MinionJSONInputReader.hpp"
#include <fstream>
#include <iostream>


template<typename Reader, typename Stream>
    void ReadCSP(Reader& reader, ConcreteFileReader<Stream>* infile)
{
    reader.read(infile) ;
    getTableOut().set(string("Filename"), infile->filename);
}

void readInputFromFiles(ProbSpec::CSPInstance& instance, vector<string> fnames, bool parser_verbose, MapLongTuplesToShort mltts)
{
  MinionThreeInputReader<ConcreteFileReader<CheapStream> > readerThree(parser_verbose, mltts);
  MinionInputReader<ConcreteFileReader<CheapStream> > reader(parser_verbose);
  MinionJSONInputReader readerJSON(parser_verbose, mltts);

  bool needs_finalise_three = false;
  bool needs_finalise_json = false;
  for(vector<string>::const_iterator fname = fnames.begin(); fname != fnames.end(); fname++) {
    const char* filename = fname->c_str();
    string extension;
    if(fname->find_last_of(".") < fname->size())
      extension = fname->substr(fname->find_last_of("."), fname->size());

    // We need to use a pointer here, as we want to declare the object inside a for loop,
    // and we need it to live until after we've finished parsing.
    // We use an auto_ptr because it will auto-delete on exit.
    istream* file;

    if(*fname != "--")
    {
      file = new ifstream(filename, ios_base::in | ios_base::binary);
      if (!(*file)) {
        INPUT_ERROR("Can't open given input file '" + *fname + "'.");
      }
    }
    else
      file = &cin;

    CheapStream cs(*file);

    ConcreteFileReader<CheapStream> infile(cs, filename);

    if (infile.eof()) {
      INPUT_ERROR("Can't open given input file '" + *fname + "'.");
    }

    try
    {
#if 0
      if(cs.peek() == '{' || cs.peek() == '/')
      {
          std::string stripped = removeComments(cs.get_raw_string());
          JsonValue value;
          JsonAllocator allocator;
          char* endptr = 0;
          JsonParseStatus status = jsonParse(&(stripped[0]), &endptr, &value, allocator);
          if(status != JSON_PARSE_OK)
          { gason_print_error(filename, status, endptr, &(stripped[0]), stripped.size()); }
          readerJSON.instance = &instance;
          readerJSON.read(value);
          getTableOut().set(string("Filename"), filename);
          needs_finalise_json = true;
      }
      else
#endif
      {
        string test_name = infile.get_string();
        if(test_name != "MINION")
          INPUT_ERROR("All Minion input files must begin 'MINION'");

        SysInt inputFileVersionNumber = infile.read_int();

        if(inputFileVersionNumber > 3)
          INPUT_ERROR("This version of Minion only supports formats up to 3");

        if(inputFileVersionNumber == 3)
        {
          readerThree.instance = &instance;
          ReadCSP(readerThree, &infile);
          //instance = std::move(readerThree.instance);
          needs_finalise_three = true;
        }
        else
        {
          reader.instance = &instance;
          ReadCSP(reader, &infile);
          // fix variable names in case we want to write a resume file (which is
          // in Minion 3 format)
          instance.add_variable_names();
        }
      }
    }
    catch(parse_exception s)
    {
      cerr << "Error in input!" << endl;
      cerr << s.what() << endl;

      SysInt pos = cs.get_raw_pos();
      cs.reset_stream();

      string current_line;
      SysInt start_of_line = 0;
      SysInt line_count = -1;

      do
      {
          line_count++;
          start_of_line = cs.get_raw_pos();
          current_line = cs.getline();
      }
      while(cs.get_raw_pos() < pos);

      cerr << "Error occurred on line " << line_count << endl;
      cerr << "Parser gave up around:" << endl;
      cerr << current_line << endl;
      for(SysInt i = 0; i < pos - start_of_line - 1; ++i)
          cerr << "-";
      cerr << "^" << endl;
        exit(1);
    }
  }
  if(needs_finalise_three)
  {
      readerThree.finalise();
     // instance = std::move(readerThree.instance);
  }
  if(needs_finalise_json)
  {
    readerJSON.finalise();
  }
}
