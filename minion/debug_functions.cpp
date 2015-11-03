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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
* USA.
*/

#include "system/system.h"

using namespace std;

bool debug_crash = false;

bool in_cspcomp_for_failexit = false;

void output_fatal_error(string s) {
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
  output_fatal_error(oss.str());
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
  output_fatal_error(oss.str());
}

void DOM_NORETURN FAIL_EXIT(string s) {
  if(in_cspcomp_for_failexit) {
    if(s != "")
      cout << "c " << s << endl;
    cout << "s UNKNOWN" << endl;
    exit(1);
  }

  cerr << "Unrecoverable error. Exiting." << endl;
  cerr << s << endl;
  cerr.flush();
  if(debug_crash) {
    SysInt* i = NULL;
    *i = 0;
  }
  throw 9;
}

void error_printing_function(std::string a, std::string f, SysInt line) {
  cerr << "Assert Error!" << endl;
  cerr << "Test '" << a << "' failed." << endl;
  cerr << "In file " << f << ", line " << line << endl;
  cerr << "\n";
  cout << "\n";
  cout.flush();
  cerr.flush();
  FAIL_EXIT();
}

void user_error_printing_function(std::string a, std::string f, SysInt line) {
  cerr << "An error occurred while solving your instance!" << endl;
  cerr << a << endl;
  cout.flush();
  cerr.flush();
  FAIL_EXIT();
}
