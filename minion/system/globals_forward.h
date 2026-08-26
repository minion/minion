#ifndef _GLOBALS_FORWARD_H
#define _GLOBALS_FORWARD_H
#include <iosfwd>
struct Globals;

/// This context's output stream; use instead of std::cout inside the
/// solver so a run can be silenced or redirected per context rather
/// than by mutating the process-global std::cout. Declared here, the
/// earliest header, so it is visible to the low-level utilities
/// (tostring, tableout) as well.
///
/// Not inline: the definition needs Globals, which is not available this
/// early, so the low-level users would see a declaration with no
/// definition. Defined in globals.cpp.
std::ostream& getOutput();
#endif
