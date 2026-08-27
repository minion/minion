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

// Minion was asked for something it will not do: an unrecognised switch or
// switch value, or a model using a feature it does not have.  Thrown rather
// than exited, so that a library caller gets an error back instead of having
// its process killed.  The command line catches it, prints it and exits 1.
struct minion_user_error : public std::exception {
  std::string error;
  minion_user_error(std::string s) : error(s) {}

  virtual const char* what() const throw() {
    return error.c_str();
  }

  virtual ~minion_user_error() throw() {}
};

#endif
