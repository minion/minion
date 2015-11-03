/** \weakgroup MinLib
 * @{
 */

#ifndef STRING_OPS_CDS
#define STRING_OPS_CDS

#include <string>
#include <fstream>
#include <exception>
#include "tostring.hpp"

/// Read file into a std::string
inline std::string readFile(std::string fileName) {
  std::ifstream t(fileName);
  if(!t)
    throw std::runtime_error("Invalid filename:" + tostring(fileName));

  return std::string((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
}

inline std::string removeComments(const std::string& s) {
  std::ostringstream oss;

  // state: 0 - normal text, 1 - found 1 '/', 2 - found 2nd '/'
  int state = 0;

  for(auto it = s.begin(); it != s.end(); ++it) {
    switch(state) {
    case 0: {
      if(*it == '/')
        state++;
      else
        oss << *it;
    } break;
    case 1: {
      if(*it == '/')
        state++;
      else {
        state = 0;
        oss << *(it - 1) << *it;
      }
    } break;
    case 2: {
      if(*it == '\n') {
        state = 0;
        oss << *it;
      }
    } break;
    default: abort();
    }
  }
  if(state == 1)
    oss << s.back();

  return oss.str();
}

// Write std::string to file
inline void writeFile(std::string fileName, std::string contents) {
  std::ofstream t(fileName);
  if(!t)
    throw std::runtime_error("Could not write to file:" + tostring(fileName));
  t << contents;
}

#ifndef _WIN32
/// Execute a program and return a pair 'retval, output'
inline std::pair<int, std::string> executeProgram(std::string cmd) {
  std::string data;
  FILE* stream;
  int MAX_BUFFER = 256;
  char buffer[MAX_BUFFER];
  cmd.append(" 2>&1");
  stream = popen(cmd.c_str(), "r");
  if(!stream) {
    exit(1);
  }
  while(!feof(stream)) {
    if(fgets(buffer, MAX_BUFFER, stream) != NULL) {
      data.append(buffer);
    }
  }
  int retval = pclose(stream);
  return make_pair(retval, data);
}
#endif
/** @}
 */

#endif
