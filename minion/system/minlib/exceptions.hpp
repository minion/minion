#ifndef MINLIB_EXCEPTION_SCKQLL
#define MINLIB_EXCEPTION_SCKQLL

#include <exception>
#include <string>

struct parse_exception : public std::exception {
  std::string error;
  parse_exception(std::string s) : error(s) {}

  virtual const char* what() const throw() {
    return error.c_str();
  }

  virtual ~parse_exception() throw() {}
};

#endif
