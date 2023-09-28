// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#include "system/system.h"

using namespace std;

void outputFatalError(string s) {
  std::cerr << s << "\n";
  abort();
}

void FATAL_REPORTABLE_ERROR() {
  ostringstream oss;
  oss << "Minion has had an internal error, due to the instance you are using." << endl;
  oss << "This is (probably) not your fault, but instead a bug in Minion." << endl;
  oss << "We would appreciate it if you could report this output, and the "
         "instance which"
      << endl;
  oss << "caused the problem to us. Thank you." << endl;
  outputFatalError(oss.str());
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
  cout << "\n";
  cout.flush();
  cerr.flush();
  FAIL_EXIT();
}

void userErrorPrintingFunction(std::string a, std::string f, SysInt line) {
  cerr << "An error occurred while solving your instance!" << endl;
  cerr << a << endl;
  cout.flush();
  cerr.flush();
  FAIL_EXIT();
}
