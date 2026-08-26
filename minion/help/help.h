// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef MINION_HELP_H
#define MINION_HELP_H

#include <string>

/// Prints the top-level help, or the help for one topic.  Returns false if the
/// topic is not recognised, having listed the valid topics on stderr.
bool help(const std::string& topic);

/// Prints the version and the git revision it was built from.
void printVersion();

/// Prints the one-line "run me with --help" banner used when Minion is given
/// no arguments at all.
void printUsageBanner(const char* argv0);

/// The known flag closest to `flag`, or "" if none is close enough.  Used to
/// say "did you mean" when the command line has a typo in it.
std::string suggestFlag(const std::string& flag);

#endif
