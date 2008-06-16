#include "system/system.h"

using namespace std;

bool debug_crash = false;

bool in_cspcomp_for_failexit = false;

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

void _NORETURN FAIL_EXIT(string s) 
{ 
  if(in_cspcomp_for_failexit)
  {
    if(s != "")
      cout << "c " << s << endl;
    cout << "s UNKNOWN" << endl;  
    exit(1);
  }
  
  cerr << "Unrecoverable error. Exiting." << endl;
  cerr << s << endl;
  cerr.flush();
  if(debug_crash)
  { 
    int* i = NULL;
    *i = 0;
  }
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
