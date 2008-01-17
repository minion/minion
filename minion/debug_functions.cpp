#include "system/system.h"

using namespace std;

void D_FATAL_ERROR2(string s, string file, string line)
{ 
  cerr << "Sorry, there has been some kind of error." << endl;
  cerr << "This could be caused by a misformed input file, or by an internal bug." << endl;
  cerr << "If you can't figure out what is causing the problem, please report it at http://www.sourceforge.net/projects/minion." << endl;
  cerr << "Either on the bug tracker, or on the mailing list." << endl;
  cerr << endl;
  cerr << "The generated error message was: " << s << endl;
  cerr << "The error was in the file " << file << " on line " << line << endl;
}

void _NORETURN FAIL_EXIT() 
{ 
  cerr << "Unrecoverable error. Exiting" << endl;
  cerr.flush();
  throw 9;
}

void assert_function(BOOL x, const char* a, const char* f, int line)
{ 
  if(!x) 
  {
    cerr << "Assert Error!" << endl;
	cerr << "Test '" << a << "' failed." << endl;
	cerr << "In file " << f << ", line " << line << endl;
    cerr << "\n";
	cout << "\n";
    cout.flush();
	cerr.flush();
    FAIL_EXIT();
  }
}
