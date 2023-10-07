// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

/*
 * Functions for using minion as a library.
 */

#include "libwrapper.h"
#include "minion.h"

#ifdef LIBMINION
extern Globals* globals;

void resetMinion() {
  delete globals;
  globals = new Globals();
}
#endif
