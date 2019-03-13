#ifndef MINIREADER_H
#define MINIREADER_H

#include "minlib/minlib.hpp"
#include <fstream>  // reading file
#include <iostream> // std::cerr
#include <stdlib.h> // exit(1)
#include <vector>

using std::cerr;
using std::endl;
using std::string;
using std::vector;

typedef SimpleMap<string, MultiDimCon<int, int>> simmap;

class MiniStream {
  string s; // text to parse
  const char* streamStart;
  const char* streamEnd;
  const char* streamPos;

  int linenum; // for errors
  int charnum;

  bool skipline;

  simmap* conmap;

  bool eof() {
    return streamPos == streamEnd;
  }
  char peek() {
    return *streamPos;
  }
  void incr() {
    charnum++;
    streamPos++;
  }

  void skip_space() {
    if(skipline) { // if triggered, skip comments
      while(!eof() && peek() != '\n') {
        incr();
      }
      skipline = false;
    }
    while(!eof() && isspace(peek())) { // skip all whitespace
      if(peek() == '\n') {             // track line numbers
        linenum++;
        charnum = 0;
      }
      incr();
    }
    if(peek() == '$') { // trigger a comment
      skipline = true;
      skip_space();
    }
  }

  bool getString(string& s) {
    skip_space();
    char buffer[1000];
    int pos = 0;
    while(isalnum(peek()) || peek() == '_') { // copy valid characters
      buffer[pos] = peek();
      pos++;
      incr();
      if(pos >= 999) {
        pError("Identifier too long:");
      }
    }

    buffer[pos] = '\0';
    s = string(buffer);
    if(s.empty())
      return false;
    return true;
  }

  bool is_sign(char c) { // if next non-space character maches
    skip_space();        // return true and move the iterator
    if(peek() == c) {
      incr();
      return true;
    }
    return false;
  }

  bool get_int(int& i) { //'stolen' int parser
    skip_space();
    bool neg = 1;
    i = 1;

    if(peek() == '-') {
      neg = -1;
      incr();
    }
    if(peek() >= '0' && peek() <= '9') {
      i *= (peek() - '0');
      incr();
    } else {
      cerr << peek() << endl;
      return false;
    }

    while(peek() >= '0' && peek() <= '9') {
      i = i * 10 + peek() - '0';
      incr();
    }
    i *= neg;

    return true;
  }

  // methods for printing errors
  void pError(const string error) {
    pError(error, charnum);
  }

  void pError(const string error, int cNum) {
    cerr << "Error occurred on line " << linenum << ":" << endl;
    cerr << error << endl;

    size_t p = 0;
    for(int i = 1; i < linenum; i++) { // find the right line
      p = s.find_first_of('\n', p);
      p++;
    }
    cerr << s.substr(p, s.find_first_of('\n', p + 1) - p) << endl;

    for(int i = 1; i < cNum; i++) // draw the fancy arrow
      cerr << '-';
    cerr << '^' << endl;

    exit(1);
  }

  // methods for storing the values
  void add_var(string name, int value) {
    conmap->add(name, MultiDimCon<int, int>(value));
  }

  void add_var(string name, vector<int> bounds, std::map<vector<int>, int> storage) {
    MultiDimCon<int, int> temp(bounds);
    for(auto i : storage) {
      temp.add(i.first, i.second);
    }
    conmap->add(name, temp);
  }

public:
  MiniStream(string fname, simmap& cm) {
    conmap = &cm;

    std::ifstream file(fname.c_str(), std::ios_base::in);
    s = string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    file.close();

    streamStart = &*s.begin();
    streamEnd = &*s.begin() + s.length();
    streamPos = streamStart;

    if(eof()) {
      throw std::runtime_error("Parameter file '" + fname + "' does not exist or is empty.");
    }

    skipline = false;
    linenum = charnum = 1;

    string var_name;
    int varValue;

    while(!eof()) {
      if(!getString(var_name)) { // fails if file contains only
        linenum--;                // spaces +/ comments
        pError("No 'letting's found.");
      }
      if(var_name != "letting") { // letting
        pError("Expected 'letting' here:", charnum - var_name.length());
      }

      if(!getString(var_name)) { // var name
        pError("Expected a variable name here:");
      }

      if(!is_sign('=')) { // equals
        pError("Expected '=' here:");
      }

      if(is_sign('[')) { // array
        int depth = 0;
        int c_lvl = 0;
        vector<int> array_pointer;
        array_pointer.push_back(0);
        vector<int> arrayMax;
        arrayMax.push_back(1);

        std::map<vector<int>, int> storage;
        bool needcomma = false;

        while(true) {
          if(is_sign(']')) { // closing array
            c_lvl--;
            if(c_lvl < 0) {
              add_var(var_name, arrayMax, storage);
              break;
            }
            needcomma = true;
          }

          else if(is_sign(',')) { // next value
            array_pointer[c_lvl]++;
            if(array_pointer[c_lvl] >= arrayMax[c_lvl]) {
              arrayMax[c_lvl] = array_pointer[c_lvl] + 1;
            }
            needcomma = false;
          }

          else if(!needcomma && is_sign('[')) { // go deeper
            c_lvl++;
            if(depth != 0 && c_lvl > depth) {
              pError("Array dimensions do not match:");
            }
            if((int)array_pointer.size() < c_lvl + 1) {
              array_pointer.push_back(0);
              arrayMax.push_back(1);
            } else
              array_pointer[c_lvl] = 0;
          }

          else if(!needcomma && get_int(varValue)) { // add value
            if(depth == 0) {
              depth = (int)array_pointer.size() - 1;
            }
            if(c_lvl != depth) {
              pError("Array dimensions do not match:", charnum - 1);
            }
            storage[array_pointer] = varValue;
            needcomma = true;
          }

          else { // array error
            string error = "Expected ";
            if(depth == 0)
              error += "number or ";
            if(depth != c_lvl && !needcomma)
              error += "'[' or ";
            error += "']' or ',' here:";
            pError(error);
          }
        }
      }

      else { // simple value
        if(!get_int(varValue)) {
          pError("Expected integer value or '[' here:");
        } else {

          add_var(var_name, varValue);
        }
      }

      skip_space();
    }
  }
};

#endif
