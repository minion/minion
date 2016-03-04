// Stupid visual C++ needs a little hacking
#ifdef _MSC_VER
#define BOOST_ALL_NO_LIB
// We don't want no stupid safe library warnings
#define _SCL_SECURE_NO_DEPRECATE
#define DEFAULT_CALL __std_call
#pragma warning(disable : 4715)
// Supress 'size_t -> int' warnings.
#pragma warning(disable : 4267)
// I don't even get this warning.
#pragma warning(disable : 4244)
// I'll buy a pint for anyone who can figure how to fix this..
// 'unsigned long' : forcing value to BOOL 'true' or 'false'. Of course I am,
// that's what I want to test!
#pragma warning(disable : 4800)
// At some point I might fix these "signed/unsigned mismatch" warnings...
#pragma warning(disable : 4018)
// Why can't you realise that abort() means the function doesn't have to return?
#pragma warning(disable : 4716)
// Another annoying warning. I'm not sure why Microsoft want to warn about this,
// it's perfectly common
#pragma warning(disable : 4355)
#else
#define DEFAULT_CALL
#endif // _MSC_VER
