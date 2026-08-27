// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#include "system/system.h"

using namespace std;

// A problem with what Minion was asked to do -- a switch it does not accept,
// a model it cannot handle -- so report it and stop.  Not abort(): that gives
// the shell a signal death and, on some systems, a core dump, for what is
// usually a typo.  Genuine internal errors go through
// FATAL_REPORTABLE_ERROR, which still aborts.
void outputFatalError(string s) {
  throw minion_user_error(s);
}

void FATAL_REPORTABLE_ERROR() {
  ostringstream oss;
  oss << "Minion has had an internal error, due to the instance you are using." << endl;
  oss << "This is (probably) not your fault, but instead a bug in Minion." << endl;
  oss << "We would appreciate it if you could report this output, and the "
         "instance which"
      << endl;
  oss << "caused the problem to us. Thank you." << endl;
  // abort() rather than exit: this one is a bug in Minion, and a core dump
  // is worth having.
  std::cerr << oss.str() << "\n";
  std::cerr.flush();
  abort();
}

void D_FATAL_ERROR2(string s, string file, string line) {
  ostringstream oss;
  oss << "Sorry, there has been some kind of error." << endl;
  oss << "This could be caused by a misformed input file, or by an internal "
         "bug."
      << endl;
  oss << "If you can't figure out what is causing the problem, please report "
         "it at http://www.sourceforge.net/projects/minion."
      << endl;
  oss << "Either on the bug tracker, or on the mailing list." << endl;
  oss << endl;
  oss << "The generated error message was: " << s << endl;
  oss << "The error was in the file " << file << " on line " << line << endl;
  outputFatalError(oss.str());
}

void DOM_NORETURN FAIL_EXIT(string s) {
  cerr << "Unrecoverable error. Exiting." << endl;
  cerr << s << endl;
  cerr.flush();
  throw 9;
}

void errorPrintingFunction(std::string a, std::string f, SysInt line) {
  cerr << "Assert Error!" << endl;
  cerr << "Test '" << a << "' failed." << endl;
  cerr << "In file " << f << ", line " << line << endl;
  cerr << "\n";
  getOutput() << "\n";
  getOutput().flush();
  cerr.flush();
  FAIL_EXIT();
}

void userErrorPrintingFunction(std::string a, std::string f, SysInt line) {
  cerr << "An error occurred while solving your instance!" << endl;
  cerr << a << endl;
  getOutput().flush();
  cerr.flush();
  FAIL_EXIT();
}
