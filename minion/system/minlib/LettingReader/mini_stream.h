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
  const char* stream_start;
  const char* stream_end;
  const char* stream_pos;

  int linenum; // for errors
  int charnum;

  bool skipline;

  simmap* conmap;

  bool eof() {
    return stream_pos == stream_end;
  }
  char peek() {
    return *stream_pos;
  }
  void incr() {
    charnum++;
    stream_pos++;
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

  bool get_string(string& s) {
    skip_space();
    char buffer[1000];
    int pos = 0;
    while(isalnum(peek()) || peek() == '_') { // copy valid characters
      buffer[pos] = peek();
      pos++;
      incr();
      if(pos >= 999) {
        p_error("Identifier too long:");
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
  void p_error(const string error) {
    p_error(error, charnum);
  }

  void p_error(const string error, int c_num) {
    cerr << "Error occurred on line " << linenum << ":" << endl;
    cerr << error << endl;

    size_t p = 0;
    for(int i = 1; i < linenum; i++) { // find the right line
      p = s.find_first_of('\n', p);
      p++;
    }
    cerr << s.substr(p, s.find_first_of('\n', p + 1) - p) << endl;

    for(int i = 1; i < c_num; i++) // draw the fancy arrow
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

    stream_start = &*s.begin();
    stream_end = &*s.begin() + s.length();
    stream_pos = stream_start;

    if(eof()) {
      throw std::runtime_error("Parameter file '" + fname + "' does not exist or is empty.");
    }

    skipline = false;
    linenum = charnum = 1;

    string var_name;
    int var_value;

    while(!eof()) {
      if(!get_string(var_name)) { // fails if file contains only
        linenum--;                // spaces +/ comments
        p_error("No 'letting's found.");
      }
      if(var_name != "letting") { // letting
        p_error("Expected 'letting' here:", charnum - var_name.length());
      }

      if(!get_string(var_name)) { // var name
        p_error("Expected a variable name here:");
      }

      if(!is_sign('=')) { // equals
        p_error("Expected '=' here:");
      }

      if(is_sign('[')) { // array
        int depth = 0;
        int c_lvl = 0;
        vector<int> array_pointer;
        array_pointer.push_back(0);
        vector<int> array_max;
        array_max.push_back(1);

        std::map<vector<int>, int> storage;
        bool needcomma = false;

        while(true) {
          if(is_sign(']')) { // closing array
            c_lvl--;
            if(c_lvl < 0) {
              add_var(var_name, array_max, storage);
              break;
            }
            needcomma = true;
          }

          else if(is_sign(',')) { // next value
            array_pointer[c_lvl]++;
            if(array_pointer[c_lvl] >= array_max[c_lvl]) {
              array_max[c_lvl] = array_pointer[c_lvl] + 1;
            }
            needcomma = false;
          }

          else if(!needcomma && is_sign('[')) { // go deeper
            c_lvl++;
            if(depth != 0 && c_lvl > depth) {
              p_error("Array dimensions do not match:");
            }
            if((int)array_pointer.size() < c_lvl + 1) {
              array_pointer.push_back(0);
              array_max.push_back(1);
            } else
              array_pointer[c_lvl] = 0;
          }

          else if(!needcomma && get_int(var_value)) { // add value
            if(depth == 0) {
              depth = (int)array_pointer.size() - 1;
            }
            if(c_lvl != depth) {
              p_error("Array dimensions do not match:", charnum - 1);
            }
            storage[array_pointer] = var_value;
            needcomma = true;
          }

          else { // array error
            string error = "Expected ";
            if(depth == 0)
              error += "number or ";
            if(depth != c_lvl && !needcomma)
              error += "'[' or ";
            error += "']' or ',' here:";
            p_error(error);
          }
        }
      }

      else { // simple value
        if(!get_int(var_value)) {
          p_error("Expected integer value or '[' here:");
        } else {

          add_var(var_name, var_value);
        }
      }

      skip_space();
    }
  }
};

#endif
