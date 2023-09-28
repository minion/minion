// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <stdlib.h>
#include <string>
#include <utility>
#include <vector>

using namespace std;

// This is a v.simple logging component.
// Call tableout.set("PropertyName", 127)

class TableOut {
private:
  // All the data for this run is kept in the map
  map<string, string> data;
  string tablefilename;
  string jsonfilename;

  // Don't allow copying!
  TableOut(const TableOut& t);

public:
  TableOut() {}

  template <class Valtype>
  void set(string propname, Valtype value) {
    // create a new, or overwrite the, entry with name propname
    data[propname] = tostring(value);
  }

  void print_line() {
    if(tablefilename != "") {
      print_table_line();
    }
    if(jsonfilename != "") {
      print_table_json();
    }
  }

  void print_table_json() {
    ofstream f;
    if(jsonfilename == "-") {
      f.copyfmt(std::cout);
      f.clear(std::cout.rdstate());
      f.basic_ios<char>::rdbuf(std::cout.rdbuf());
    } else {
      f.open(jsonfilename.c_str(), ios::app | ios::out); // Open with append mode.
    }

    JSONStreamer json(&f);

    map<string, string>::iterator it;
    for(it = data.begin(); it != data.end(); it++) {
      json.mapElement(it->first, it->second);
    }
  }

  void print_table_line() {
    // First version: this checks if we are at the top of a file. If so, prints
    // column headers.
    // Then
    // outputs a line.

    // If the column headers are unexpected, should print something to cerr.

    // Second version should be able to cope with different column headers? At
    // least different orders?

    ofstream f;
    if(tablefilename == "-") {
      f.copyfmt(std::cout);
      f.clear(std::cout.rdstate());
      f.basic_ios<char>::rdbuf(std::cout.rdbuf());
    } else {
      f.open(tablefilename.c_str(), ios::app | ios::out); // Open with append mode.
    }

    if(!f) {
      cerr << "tableout.cpp: failed to open file to output table." << endl;
    }

    // if file position is the beginning of the file, then output the column
    // headers.
    if(tablefilename == "-" || f.tellp() == streampos(0)) {
      f << "#";

      map<string, string>::iterator it;
      for(it = data.begin(); it != data.end(); it++) {
        f << "\"" << (*it).first << "\" ";
      }
      f << endl;
    }

    // This doesn't work with strings that have spaces in them.

    map<string, string>::iterator it;
    for(it = data.begin(); it != data.end(); it++) {
      f << (*it).second << " ";
    }
    f << endl;

    f.close();
  }

  void set_table_filename(string file) {
    if(tablefilename != "") {
      outputFatalError("Can only set table output filename once");
    }
    tablefilename = file;
  }

  void set_json_filename(string file) {
    if(jsonfilename != "") {
      outputFatalError("Can only set JSON output filename once");
    }
    jsonfilename = file;
  }
};

// Provide a global singleton of the above class. Not threadsafe!
inline TableOut& getTableOut() {
  static TableOut t;
  return t;
}

// Design assumption: Column headings will always be sorted in alphabetical
// order. ??
